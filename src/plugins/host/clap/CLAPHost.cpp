#include "CLAPHost.h"
#include <QWidget>
#include <QWindow>
#include <QDebug>
#include <QDir>
#include <QLibrary>

#ifdef _WIN32
    #include <windows.h>
#elif __APPLE__
    #include <dlfcn.h>
#else
    #include <dlfcn.h>
#endif

#include <clap/clap.h>

CLAPHost::CLAPHost(const PluginInfo& info)
    : m_info(info), m_handle(nullptr), m_plugin(nullptr), m_entry(nullptr), m_factory(nullptr), m_gui(nullptr), m_editorWidget(nullptr), m_initialized(false), m_sampleRate(44100.0), m_blockSize(256) {
    for (int i = 0; i < 2; i++) {
        m_inputBuffers[i] = nullptr;
        m_outputBuffers[i] = nullptr;
    }
}

CLAPHost::~CLAPHost() { shutdown(); }

bool CLAPHost::initialize(double sampleRate, int blockSize) {
    if (m_initialized) return true;
    m_sampleRate = sampleRate;
    m_blockSize = blockSize;
    if (!loadPlugin()) {
        qWarning() << "Failed to load CLAP plugin:" << m_info.path;
        return false;
    }
    // Allocate buffers
    for (int i = 0; i < 2; i++) {
        m_inputBuffers[i] = new float[m_blockSize];
        m_outputBuffers[i] = new float[m_blockSize];
        memset(m_inputBuffers[i], 0, m_blockSize * sizeof(float));
        memset(m_outputBuffers[i], 0, m_blockSize * sizeof(float));
    }
    // Activate the plugin
    if (m_plugin && m_plugin->activate) {
        m_plugin->activate(m_plugin, m_sampleRate, m_blockSize, m_blockSize);
    }
    m_initialized = true;
    return true;
}

void CLAPHost::shutdown() {
    if (!m_initialized) return;
    if (m_editorWidget) {
        destroyEditor(m_editorWidget);
        m_editorWidget = nullptr;
    }
    if (m_plugin) {
        if (m_plugin->deactivate) {
            m_plugin->deactivate(m_plugin);
        }
        if (m_plugin->destroy) {
            m_plugin->destroy(m_plugin);
        }
        m_plugin = nullptr;
    }
    if (m_entry && m_entry->deinit) {
        m_entry->deinit();
        m_entry = nullptr;
    }
    for (int i = 0; i < 2; i++) {
        delete[] m_inputBuffers[i];
        delete[] m_outputBuffers[i];
        m_inputBuffers[i] = nullptr;
        m_outputBuffers[i] = nullptr;
    }
#ifdef _WIN32
    if (m_handle) {
        FreeLibrary((HMODULE)m_handle);
        m_handle = nullptr;
    }
#else
    if (m_handle) {
        dlclose(m_handle);
        m_handle = nullptr;
    }
#endif
    m_factory = nullptr;
    m_gui = nullptr;
    m_initialized = false;
    m_parameterCache.clear();
}

void CLAPHost::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!m_initialized || !m_plugin || !m_plugin->process) {
        for (int c = 0; c < numChannels && c < 2; c++) {
            if (inputs[c] && outputs[c] && inputs[c] != outputs[c])
                memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }
        return;
    }

    // Copy inputs to internal buffers
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (inputs[c]) {
            memcpy(m_inputBuffers[c], inputs[c], numFrames * sizeof(float));
        } else {
            memset(m_inputBuffers[c], 0, numFrames * sizeof(float));
        }
    }

    // Prepare CLAP process structure
    clap_process_t process;
    memset(&process, 0, sizeof(process));
    process.steady_time = 0;
    process.frames_count = numFrames;
    process.transport = nullptr;

    // Audio buffers
    clap_audio_buffer_t inputBuffer, outputBuffer;
    inputBuffer.channel_count = numChannels;
    outputBuffer.channel_count = numChannels;
    inputBuffer.data32 = m_inputBuffers;
    outputBuffer.data32 = m_outputBuffers;
    inputBuffer.data64 = nullptr;
    outputBuffer.data64 = nullptr;

    process.inputs = &inputBuffer;
    process.inputs_count = 1;
    process.outputs = &outputBuffer;
    process.outputs_count = 1;

    // Process
    bool result = m_plugin->process(m_plugin, &process);
    if (!result) {
        qWarning() << "CLAP processing failed for:" << m_info.name;
        // Passthrough on failure
        for (int c = 0; c < numChannels && c < 2; c++) {
            if (inputs[c] && outputs[c] && inputs[c] != outputs[c])
                memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }
        return;
    }

    // Copy outputs back
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (outputs[c]) {
            memcpy(outputs[c], m_outputBuffers[c], numFrames * sizeof(float));
        }
    }
}

void CLAPHost::reset() {
    if (m_plugin && m_plugin->reset) {
        // m_plugin->reset(m_plugin);
    }
}

void CLAPHost::setParameter(int index, float value) {
    if (index < 0 || index >= getParameterCount()) return;
    m_parameterCache[index] = value;
    if (m_plugin && m_plugin->params && m_plugin->params->set_value) {
        m_plugin->params->set_value(m_plugin, index, value);
    }
}

float CLAPHost::getParameter(int index) const {
    if (index < 0 || index >= getParameterCount()) return 0.0f;
    if (m_plugin && m_plugin->params && m_plugin->params->get_value) {
        return m_plugin->params->get_value(m_plugin, index);
    }
    return m_parameterCache.value(index, 0.0f);
}

int CLAPHost::getParameterCount() const {
    return m_info.parameters.size();
}

bool CLAPHost::hasEditor() const {
    return (m_gui != nullptr);
}

void* CLAPHost::createEditor(void* parent) {
    if (!m_gui || m_editorWidget) return nullptr;

    QWidget* parentWidget = static_cast<QWidget*>(parent);
    if (!parentWidget) return nullptr;

#ifdef _WIN32
    const char* api = "clap_win";
#elif __APPLE__
    const char* api = "clap_cocoa";
#else
    const char* api = "clap_x11";
#endif

    bool created = m_gui->create(m_plugin, api, false);
    if (!created) return nullptr;

    m_editorWidget = new QWidget(parentWidget);
    m_editorWidget->setAttribute(Qt::WA_NativeWindow);
    m_editorWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

#ifdef _WIN32
    HWND parentHwnd = (HWND)m_editorWidget->winId();
    m_gui->set_parent(m_plugin, (void*)parentHwnd);
#elif __APPLE__
    void* nsView = (void*)m_editorWidget->windowHandle()->winId();
    m_gui->set_parent(m_plugin, nsView);
#else
    WId xid = m_editorWidget->windowHandle()->winId();
    m_gui->set_parent(m_plugin, (void*)xid);
#endif

    uint32_t width, height;
    if (m_gui->get_size(m_plugin, &width, &height)) {
        m_editorWidget->resize(width, height);
        m_gui->resize(m_plugin, width, height);
    }

    m_gui->show(m_plugin);
    m_editorWidget->show();

    connect(m_editorWidget, &QWidget::resize, this, [this](const QSize& size) {
        if (m_gui) {
            m_gui->resize(m_plugin, size.width(), size.height());
        }
    });

    return m_editorWidget;
}

void CLAPHost::destroyEditor(void* editor) {
    if (!editor || !m_gui) return;
    m_gui->hide(m_plugin);
    m_gui->destroy(m_plugin);
    m_gui = nullptr;

    QWidget* widget = static_cast<QWidget*>(editor);
    widget->hide();
    widget->deleteLater();
    m_editorWidget = nullptr;
}

bool CLAPHost::loadPlugin() {
    QString path = m_info.path;
    if (!QFile::exists(path)) return false;

#ifdef _WIN32
    HMODULE hModule = LoadLibraryW(path.toStdWString().c_str());
    if (!hModule) return false;
    m_handle = hModule;
    auto entryFunc = (clap_plugin_entry_t*)GetProcAddress(hModule, "clap_entry");
    if (!entryFunc) { unloadPlugin(); return false; }
#else
    void* handle = dlopen(path.toUtf8().constData(), RTLD_LAZY);
    if (!handle) return false;
    m_handle = handle;
    auto entryFunc = (clap_plugin_entry_t*)dlsym(handle, "clap_entry");
    if (!entryFunc) { unloadPlugin(); return false; }
#endif

    m_entry = entryFunc;
    if (!m_entry->init(m_info.path.toUtf8().constData())) {
        unloadPlugin();
        return false;
    }

    m_factory = m_entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    if (!m_factory) { unloadPlugin(); return false; }

    const clap_plugin_descriptor_t* desc = m_factory->get_plugin_descriptor(m_factory, 0);
    if (!desc) { unloadPlugin(); return false; }

    m_plugin = m_factory->create_plugin(m_factory, desc->id);
    if (!m_plugin) { unloadPlugin(); return false; }

    if (m_plugin->init && !m_plugin->init(m_plugin)) {
        unloadPlugin();
        return false;
    }

    const clap_plugin_gui_t* gui = (const clap_plugin_gui_t*)m_plugin->get_extension(m_plugin, CLAP_EXT_GUI);
    if (gui) {
        m_gui = gui;
    }

    return true;
}
