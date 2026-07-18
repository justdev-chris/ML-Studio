#include "LV2Host.h"
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

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

LV2Host::LV2Host(const PluginInfo& info)
    : m_info(info), m_handle(nullptr), m_plugin(nullptr), m_descriptor(nullptr), m_uiDescriptor(nullptr), m_uiHandle(nullptr), m_editorWidget(nullptr), m_initialized(false), m_sampleRate(44100.0), m_blockSize(256) {
    for (int i = 0; i < 2; i++) {
        m_inputBuffers[i] = nullptr;
        m_outputBuffers[i] = nullptr;
    }
}

LV2Host::~LV2Host() { shutdown(); }

bool LV2Host::initialize(double sampleRate, int blockSize) {
    if (m_initialized) return true;
    m_sampleRate = sampleRate;
    m_blockSize = blockSize;
    if (!loadPlugin()) {
        qWarning() << "Failed to load LV2 plugin:" << m_info.path;
        return false;
    }
    for (int i = 0; i < 2; i++) {
        m_inputBuffers[i] = new float[m_blockSize];
        m_outputBuffers[i] = new float[m_blockSize];
        memset(m_inputBuffers[i], 0, m_blockSize * sizeof(float));
        memset(m_outputBuffers[i], 0, m_blockSize * sizeof(float));
    }
    m_initialized = true;
    return true;
}

void LV2Host::shutdown() {
    if (!m_initialized) return;
    if (m_editorWidget) {
        destroyEditor(m_editorWidget);
        m_editorWidget = nullptr;
    }
    for (int i = 0; i < 2; i++) {
        delete[] m_inputBuffers[i];
        delete[] m_outputBuffers[i];
        m_inputBuffers[i] = nullptr;
        m_outputBuffers[i] = nullptr;
    }
    unloadPlugin();
    m_initialized = false;
    m_parameterCache.clear();
}

void LV2Host::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!m_initialized || !m_plugin || !m_descriptor || !m_descriptor->run) {
        for (int c = 0; c < numChannels && c < 2; c++) {
            if (inputs[c] && outputs[c] && inputs[c] != outputs[c])
                memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }
        return;
    }

    // Copy inputs to LV2 buffers
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (inputs[c]) {
            memcpy(m_inputBuffers[c], inputs[c], numFrames * sizeof(float));
        } else {
            memset(m_inputBuffers[c], 0, numFrames * sizeof(float));
        }
    }

    // Run the plugin
    m_descriptor->run(m_plugin, numFrames);

    // Copy outputs back
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (outputs[c]) {
            memcpy(outputs[c], m_outputBuffers[c], numFrames * sizeof(float));
        }
    }
}

void LV2Host::reset() {
    if (m_descriptor && m_descriptor->deactivate) {
        m_descriptor->deactivate(m_plugin);
    }
    if (m_descriptor && m_descriptor->activate) {
        m_descriptor->activate(m_plugin);
    }
}

void LV2Host::setParameter(int index, float value) {
    if (index < 0 || index >= getParameterCount()) return;
    m_parameterCache[index] = value;
}

float LV2Host::getParameter(int index) const {
    return m_parameterCache.value(index, 0.0f);
}

int LV2Host::getParameterCount() const {
    return m_info.parameters.size();
}

bool LV2Host::hasEditor() const {
    return (m_uiDescriptor != nullptr);
}

void* LV2Host::createEditor(void* parent) {
    if (!m_uiDescriptor || m_editorWidget) return nullptr;

    QWidget* parentWidget = static_cast<QWidget*>(parent);
    if (!parentWidget) return nullptr;

#ifdef _WIN32
    void* parentHandle = (void*)parentWidget->winId();
#elif __APPLE__
    void* parentHandle = (void*)parentWidget->windowHandle()->winId();
#else
    void* parentHandle = (void*)parentWidget->windowHandle()->winId();
#endif

    m_uiHandle = m_uiDescriptor->instantiate(m_plugin, m_info.path.toUtf8().constData(), parentHandle, nullptr);
    if (!m_uiHandle) return nullptr;

    m_editorWidget = new QWidget(parentWidget);
    m_editorWidget->setAttribute(Qt::WA_NativeWindow);
    m_editorWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (m_uiDescriptor->resize) {
        connect(m_editorWidget, &QWidget::resize, this, [this](const QSize& size) {
            if (m_uiDescriptor && m_uiDescriptor->resize) {
                m_uiDescriptor->resize(m_uiHandle, size.width(), size.height());
            }
        });
        m_uiDescriptor->resize(m_uiHandle, 300, 200);
    }

    m_editorWidget->show();

    return m_editorWidget;
}

void LV2Host::destroyEditor(void* editor) {
    if (!editor || !m_uiHandle) return;
    if (m_uiDescriptor && m_uiDescriptor->cleanup) {
        m_uiDescriptor->cleanup(m_uiHandle);
    }
    m_uiHandle = nullptr;

    QWidget* widget = static_cast<QWidget*>(editor);
    widget->hide();
    widget->deleteLater();
    m_editorWidget = nullptr;
}

bool LV2Host::loadPlugin() {
    QString path = m_info.path;
    QFileInfo info(path);
    if (info.isDir()) {
        QDir dir(path);
#ifdef _WIN32
        QStringList files = dir.entryList({"*.dll"}, QDir::Files);
#else
        QStringList files = dir.entryList({"*.so", "*.dylib"}, QDir::Files);
#endif
        if (!files.isEmpty()) {
            path = dir.absoluteFilePath(files.first());
        } else {
            QString baseName = info.baseName();
#ifdef _WIN32
            path = dir.absoluteFilePath(baseName + ".dll");
#else
            path = dir.absoluteFilePath("lib" + baseName + ".so");
#endif
        }
    }

#ifdef _WIN32
    HMODULE hModule = LoadLibraryW(path.toStdWString().c_str());
    if (!hModule) {
        qWarning() << "Failed to load LV2 DLL:" << path;
        return false;
    }
    m_handle = hModule;
    auto descriptorFunc = (LV2_Descriptor* (*)(uint32_t))GetProcAddress(hModule, "lv2_descriptor");
    if (!descriptorFunc) {
        qWarning() << "Failed to get lv2_descriptor";
        unloadPlugin();
        return false;
    }
#else
    void* handle = dlopen(path.toUtf8().constData(), RTLD_LAZY);
    if (!handle) {
        qWarning() << "Failed to load LV2 library:" << dlerror();
        return false;
    }
    m_handle = handle;
    auto descriptorFunc = (LV2_Descriptor* (*)(uint32_t))dlsym(handle, "lv2_descriptor");
    if (!descriptorFunc) {
        qWarning() << "Failed to get lv2_descriptor:" << dlerror();
        unloadPlugin();
        return false;
    }
#endif

    m_descriptor = descriptorFunc(0);
    if (!m_descriptor) {
        qWarning() << "No LV2 descriptor at index 0";
        unloadPlugin();
        return false;
    }

    m_plugin = m_descriptor->instantiate(m_descriptor, m_sampleRate, m_info.path.toUtf8().constData(), nullptr);
    if (!m_plugin) {
        qWarning() << "Failed to instantiate LV2 plugin";
        unloadPlugin();
        return false;
    }

    if (m_descriptor->activate) {
        m_descriptor->activate(m_plugin);
    }

#ifdef _WIN32
    auto uiDescriptorFunc = (LV2UI_Descriptor* (*)(uint32_t))GetProcAddress((HMODULE)m_handle, "lv2ui_descriptor");
#else
    auto uiDescriptorFunc = (LV2UI_Descriptor* (*)(uint32_t))dlsym(m_handle, "lv2ui_descriptor");
#endif
    if (uiDescriptorFunc) {
        LV2UI_Descriptor* uiDesc = uiDescriptorFunc(0);
        if (uiDesc && strcmp(uiDesc->uri, m_descriptor->uri) == 0) {
            m_uiDescriptor = uiDesc;
        } else {
            for (uint32_t i = 1; ; i++) {
                uiDesc = uiDescriptorFunc(i);
                if (!uiDesc) break;
                if (strcmp(uiDesc->uri, m_descriptor->uri) == 0) {
                    m_uiDescriptor = uiDesc;
                    break;
                }
            }
        }
    }

    return true;
}

void LV2Host::unloadPlugin() {
    if (m_uiHandle) {
        if (m_uiDescriptor && m_uiDescriptor->cleanup) {
            m_uiDescriptor->cleanup(m_uiHandle);
        }
        m_uiHandle = nullptr;
    }
    if (m_plugin) {
        if (m_descriptor && m_descriptor->deactivate) {
            m_descriptor->deactivate(m_plugin);
        }
        if (m_descriptor && m_descriptor->cleanup) {
            m_descriptor->cleanup(m_plugin);
        }
        m_plugin = nullptr;
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
    m_descriptor = nullptr;
    m_uiDescriptor = nullptr;
}
