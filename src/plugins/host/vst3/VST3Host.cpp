#include "VST3Host.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

// VST3 SDK forward declarations (minimal)
#define DECLARE_CLASS_IID(className, iid) \
    static const char* const IID_##className = iid;

DECLARE_CLASS_IID(IComponent, "00000000-0000-0000-0000-000000000001")
DECLARE_CLASS_IID(IAudioProcessor, "00000000-0000-0000-0000-000000000002")
DECLARE_CLASS_IID(IEditController, "00000000-0000-0000-0000-000000000003")

VST3Host::VST3Host(const PluginInfo& info)
    : m_info(info) {
}

VST3Host::~VST3Host() {
    shutdown();
}

bool VST3Host::initialize(double sampleRate, int blockSize) {
    if (m_initialized) return true;

    m_sampleRate = sampleRate;
    m_blockSize = blockSize;

    if (!loadPlugin()) {
        qWarning() << "Failed to load VST3 plugin:" << m_info.path;
        return false;
    }

    m_initialized = true;
    qDebug() << "VST3 plugin initialized:" << m_info.name;
    return true;
}

void VST3Host::shutdown() {
    if (!m_initialized) return;
    unloadPlugin();
    m_initialized = false;
    m_parameterCache.clear();
}

void VST3Host::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!m_initialized || !m_processor) {
        // Passthrough
        for (int c = 0; c < numChannels && c < 2; c++) {
            if (inputs[c] && outputs[c] && inputs[c] != outputs[c]) {
                memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
            }
        }
        return;
    }

    // In a real VST3 implementation, we would:
    // 1. Copy inputs to VST3 buffers
    // 2. Call IAudioProcessor::process()
    // 3. Copy outputs back

    // For now: passthrough
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (inputs[c] && outputs[c] && inputs[c] != outputs[c]) {
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }
    }
}

void VST3Host::reset() {
    // Reset plugin state
}

void VST3Host::setParameter(int index, float value) {
    if (index < 0 || index >= getParameterCount()) return;
    m_parameterCache[index] = value;

    // In a real VST3 implementation: call IEditController::setParamNormalized()
}

float VST3Host::getParameter(int index) const {
    if (index < 0 || index >= getParameterCount()) return 0.0f;
    return m_parameterCache.value(index, 0.0f);
}

int VST3Host::getParameterCount() const {
    return m_info.parameters.size();
}

bool VST3Host::hasEditor() const {
    // In a real VST3 implementation: check if IEditController::createView() exists
    return false;
}

void* VST3Host::createEditor(void* parent) {
    // In a real VST3 implementation: create plugin UI
    return nullptr;
}

void VST3Host::destroyEditor(void* editor) {
    // In a real VST3 implementation: destroy plugin UI
}

void VST3Host::editorResized(int width, int height) {
    // Notify plugin of resize
}

bool VST3Host::loadPlugin() {
    QString path = m_info.path;

    // Handle VST3 bundle structure
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
            if (!files.isEmpty()) {
                path = contentsDir.absoluteFilePath(files.first());
            }
        }
#else
        QDir linuxDir(dir.absoluteFilePath("x86_64-linux"));
        if (linuxDir.exists()) {
            QStringList files = linuxDir.entryList({"*.so"}, QDir::Files);
            if (!files.isEmpty()) {
                path = linuxDir.absoluteFilePath(files.first());
            }
        }
#endif
    }

    // Load library
#ifdef _WIN32
    m_handle = LoadLibraryW(path.toStdWString().c_str());
    if (!m_handle) {
        qWarning() << "Failed to load VST3 DLL:" << path;
        return false;
    }

    // Get factory function
    using GetFactoryFunc = void* (*)();
    auto getFactory = (GetFactoryFunc)GetProcAddress((HMODULE)m_handle, "GetPluginFactory");
    if (!getFactory) {
        qWarning() << "Failed to get GetPluginFactory";
        return false;
    }

    // Get create instance function
    m_createInstance = (CreateInstanceFunc)GetProcAddress((HMODULE)m_handle, "createInstance");
    if (!m_createInstance) {
        qWarning() << "Failed to get createInstance";
        return false;
    }

    // Create component
    m_component = m_createInstance(IID_IComponent, IID_IComponent);
    if (!m_component) {
        qWarning() << "Failed to create IComponent";
        return false;
    }

    // Create audio processor
    m_processor = m_createInstance(IID_IAudioProcessor, IID_IAudioProcessor);
    if (!m_processor) {
        qWarning() << "Failed to create IAudioProcessor";
        return false;
    }

    // Create controller
    m_controller = m_createInstance(IID_IEditController, IID_IEditController);
    if (m_controller) {
        // Initialize controller
        qDebug() << "VST3 controller created";
    }

    return true;

#else
    m_handle = dlopen(path.toUtf8().constData(), RTLD_LAZY);
    if (!m_handle) {
        qWarning() << "Failed to load VST3 library:" << dlerror();
        return false;
    }

    // Get factory function
    using GetFactoryFunc = void* (*)();
    auto getFactory = (GetFactoryFunc)dlsym(m_handle, "GetPluginFactory");
    if (!getFactory) {
        qWarning() << "Failed to get GetPluginFactory:" << dlerror();
        return false;
    }

    // Get create instance function
    m_createInstance = (CreateInstanceFunc)dlsym(m_handle, "createInstance");
    if (!m_createInstance) {
        qWarning() << "Failed to get createInstance:" << dlerror();
        return false;
    }

    // Create component
    m_component = m_createInstance(IID_IComponent, IID_IComponent);
    if (!m_component) {
        qWarning() << "Failed to create IComponent";
        return false;
    }

    // Create audio processor
    m_processor = m_createInstance(IID_IAudioProcessor, IID_IAudioProcessor);
    if (!m_processor) {
        qWarning() << "Failed to create IAudioProcessor";
        return false;
    }

    // Create controller
    m_controller = m_createInstance(IID_IEditController, IID_IEditController);
    if (m_controller) {
        qDebug() << "VST3 controller created";
    }

    return true;
#endif
}

void VST3Host::unloadPlugin() {
    if (m_handle) {
#ifdef _WIN32
        FreeLibrary((HMODULE)m_handle);
#else
        dlclose(m_handle);
#endif
        m_handle = nullptr;
    }
    m_processor = nullptr;
    m_controller = nullptr;
    m_component = nullptr;
    m_createInstance = nullptr;
}
