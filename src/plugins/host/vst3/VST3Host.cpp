#include "VST3Host.h"
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

#include <pluginterfaces/vst3/ivstcomponent.h>
#include <pluginterfaces/vst3/ivstaudioprocessor.h>
#include <pluginterfaces/vst3/ivsteditcontroller.h>
#include <pluginterfaces/vst3/ivstplugview.h>
#include <pluginterfaces/base/fplatform.h>
#include <public.sdk/source/vst/vstfactory.h>

using namespace Steinberg::Vst;
using namespace Steinberg;

VST3Host::VST3Host(const PluginInfo& info)
    : m_info(info), m_factory(nullptr), m_component(nullptr), m_processor(nullptr), m_controller(nullptr), m_pluginView(nullptr), m_editorWidget(nullptr), m_initialized(false), m_sampleRate(44100.0), m_blockSize(256) {}

VST3Host::~VST3Host() { shutdown(); }

bool VST3Host::initialize(double sampleRate, int blockSize) {
    if (m_initialized) return true;
    m_sampleRate = sampleRate;
    m_blockSize = blockSize;
    if (!loadPlugin()) {
        qWarning() << "Failed to load VST3 plugin:" << m_info.path;
        return false;
    }
    // Setup processing
    if (m_processor) {
        ProcessSetup setup;
        setup.sampleRate = m_sampleRate;
        setup.maxSamplesPerBlock = m_blockSize;
        setup.processMode = kRealtime;
        setup.symbolicSampleSize = kSample32;
        m_processor->setupProcessing(setup);
    }
    m_initialized = true;
    return true;
}

void VST3Host::shutdown() {
    if (!m_initialized) return;
    if (m_editorWidget) {
        destroyEditor(m_editorWidget);
        m_editorWidget = nullptr;
    }
    unloadPlugin();
    m_initialized = false;
    m_parameterCache.clear();
}

void VST3Host::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!m_initialized || !m_processor || !m_component) {
        for (int c = 0; c < numChannels && c < 2; c++) {
            if (inputs[c] && outputs[c] && inputs[c] != outputs[c])
                memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }
        return;
    }

    // Prepare processing data
    ProcessData data;
    data.processMode = kRealtime;
    data.symbolicSampleSize = kSample32;
    data.numSamples = numFrames;
    data.numInputs = numChannels;
    data.numOutputs = numChannels;

    // Allocate and assign buffers
    AudioBusBuffers inputBuffers[2];
    AudioBusBuffers outputBuffers[2];
    for (int i = 0; i < numChannels && i < 2; i++) {
        inputBuffers[i].numChannels = 1;
        inputBuffers[i].channelBuffers32 = &inputs[i];
        outputBuffers[i].numChannels = 1;
        outputBuffers[i].channelBuffers32 = &outputs[i];
    }
    data.inputs = inputBuffers;
    data.outputs = outputBuffers;

    // Process
    m_processor->process(data);
}

void VST3Host::reset() {
    if (m_processor) {
        m_processor->reset();
    }
}

void VST3Host::setParameter(int index, float value) {
    if (index < 0 || index >= getParameterCount()) return;
    m_parameterCache[index] = value;
    if (m_controller) {
        m_controller->setParamNormalized(index, value);
    }
}

float VST3Host::getParameter(int index) const {
    if (index < 0 || index >= getParameterCount()) return 0.0f;
    if (m_controller) {
        return m_controller->getParamNormalized(index);
    }
    return m_parameterCache.value(index, 0.0f);
}

int VST3Host::getParameterCount() const {
    return m_info.parameters.size();
}

bool VST3Host::hasEditor() const {
    return (m_controller != nullptr);
}

void* VST3Host::createEditor(void* parent) {
    if (!m_controller || m_editorWidget) return nullptr;

    m_pluginView = m_controller->createView("editor");
    if (!m_pluginView) {
        m_pluginView = m_controller->createView(nullptr);
    }
    if (!m_pluginView) return nullptr;

    QWidget* parentWidget = static_cast<QWidget*>(parent);
    if (!parentWidget) {
        m_pluginView->release();
        m_pluginView = nullptr;
        return nullptr;
    }

    m_editorWidget = new QWidget(parentWidget);
    m_editorWidget->setAttribute(Qt::WA_NativeWindow);
    m_editorWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

#ifdef _WIN32
    HWND parentHwnd = (HWND)m_editorWidget->winId();
    if (!m_pluginView->attach((void*)parentHwnd)) {
        delete m_editorWidget;
        m_editorWidget = nullptr;
        m_pluginView->release();
        m_pluginView = nullptr;
        return nullptr;
    }
#elif __APPLE__
    QWindow* window = m_editorWidget->windowHandle();
    if (window) {
        void* nsView = (void*)window->winId();
        if (!m_pluginView->attach(nsView)) {
            delete m_editorWidget;
            m_editorWidget = nullptr;
            m_pluginView->release();
            m_pluginView = nullptr;
            return nullptr;
        }
    }
#else
    QWindow* window = m_editorWidget->windowHandle();
    if (window) {
        WId xid = window->winId();
        if (!m_pluginView->attach((void*)xid)) {
            delete m_editorWidget;
            m_editorWidget = nullptr;
            m_pluginView->release();
            m_pluginView = nullptr;
            return nullptr;
        }
    }
#endif

    ViewRect rect;
    if (m_pluginView->getSize(&rect) == kResultTrue) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        m_editorWidget->resize(width, height);
        m_pluginView->onSize(width, height);
    }

    m_editorWidget->show();

    connect(m_editorWidget, &QWidget::resize, this, [this](const QSize& size) {
        if (m_pluginView) {
            m_pluginView->onSize(size.width(), size.height());
        }
    });

    return m_editorWidget;
}

void VST3Host::destroyEditor(void* editor) {
    if (!editor || !m_pluginView) return;
    m_pluginView->detach();
    m_pluginView->release();
    m_pluginView = nullptr;

    QWidget* widget = static_cast<QWidget*>(editor);
    widget->hide();
    widget->deleteLater();
    m_editorWidget = nullptr;
}

void VST3Host::editorResized(int width, int height) {
    if (m_pluginView) {
        m_pluginView->onSize(width, height);
    }
}

bool VST3Host::loadPlugin() {
    QString path = m_info.path;
    QFileInfo info(path);
    if (info.isDir()) {
        QDir dir(path);
#ifdef _WIN32
        QStringList subDirs = {"x86_64-win", "win64", "x64"};
        for (const QString& sub : subDirs) {
            QDir subDir(dir.absoluteFilePath(sub));
            if (subDir.exists()) {
                QStringList files = subDir.entryList({"*.dll"}, QDir::Files);
                if (!files.isEmpty()) {
                    path = subDir.absoluteFilePath(files.first());
                    break;
                }
            }
        }
#elif __APPLE__
        QDir contentsDir(dir.absoluteFilePath("Contents/MacOS"));
        if (contentsDir.exists()) {
            QStringList files = contentsDir.entryList({"*.dylib", "*.so"}, QDir::Files);
            if (!files.isEmpty())
                path = contentsDir.absoluteFilePath(files.first());
        }
#else
        QDir linuxDir(dir.absoluteFilePath("x86_64-linux"));
        if (linuxDir.exists()) {
            QStringList files = linuxDir.entryList({"*.so"}, QDir::Files);
            if (!files.isEmpty())
                path = linuxDir.absoluteFilePath(files.first());
        }
#endif
    }

#ifdef _WIN32
    HMODULE hModule = LoadLibraryW(path.toStdWString().c_str());
    if (!hModule) {
        qWarning() << "Failed to load VST3 DLL:" << path;
        return false;
    }
    m_handle = hModule;
    auto factoryProc = (IPluginFactory* (*)())GetProcAddress(hModule, "GetPluginFactory");
    if (!factoryProc) {
        qWarning() << "Failed to get GetPluginFactory from VST3";
        unloadPlugin();
        return false;
    }
    m_factory = factoryProc();
    if (!m_factory) {
        qWarning() << "GetPluginFactory returned null";
        unloadPlugin();
        return false;
    }
#else
    void* handle = dlopen(path.toUtf8().constData(), RTLD_LAZY);
    if (!handle) {
        qWarning() << "Failed to load VST3 library:" << dlerror();
        return false;
    }
    m_handle = handle;
    auto factoryProc = (IPluginFactory* (*)())dlsym(handle, "GetPluginFactory");
    if (!factoryProc) {
        qWarning() << "Failed to get GetPluginFactory from VST3:" << dlerror();
        unloadPlugin();
        return false;
    }
    m_factory = factoryProc();
    if (!m_factory) {
        qWarning() << "GetPluginFactory returned null";
        unloadPlugin();
        return false;
    }
#endif

    // Create the component
    const char* componentCID = "00000000-0000-0000-0000-000000000001";
    const char* iidComponent = "00000000-0000-0000-0000-000000000001";
    tresult result = m_factory->createInstance(componentCID, iidComponent, (void**)&m_component);
    if (result != kResultOk) {
        qWarning() << "Failed to create IComponent";
        unloadPlugin();
        return false;
    }

    // Create the audio processor
    const char* iidProcessor = "00000000-0000-0000-0000-000000000002";
    result = m_factory->createInstance(componentCID, iidProcessor, (void**)&m_processor);
    if (result != kResultOk) {
        qWarning() << "Failed to create IAudioProcessor";
        unloadPlugin();
        return false;
    }

    // Create the edit controller
    const char* iidController = "00000000-0000-0000-0000-000000000003";
    result = m_factory->createInstance(componentCID, iidController, (void**)&m_controller);
    if (result != kResultOk) {
        // Not all plugins have controllers; this is optional
        m_controller = nullptr;
    }

    // Initialize the component
    if (m_component) {
        // m_component->initialize(nullptr);
    }

    return true;
}

void VST3Host::unloadPlugin() {
    if (m_pluginView) {
        m_pluginView->release();
        m_pluginView = nullptr;
    }
    if (m_controller) {
        m_controller->release();
        m_controller = nullptr;
    }
    if (m_processor) {
        m_processor->release();
        m_processor = nullptr;
    }
    if (m_component) {
        m_component->release();
        m_component = nullptr;
    }
    if (m_factory) {
        m_factory->release();
        m_factory = nullptr;
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
}
