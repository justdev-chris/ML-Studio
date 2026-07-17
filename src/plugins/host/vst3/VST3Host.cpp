#include "VST3Host.h"
#include <QDebug>
#include <QFile>
#include <QCoreApplication>

#ifdef _WIN32
    #include <windows.h>
    #define LIB_EXTENSION ".dll"
    #define LIB_PREFIX ""
#elif __APPLE__
    #include <dlfcn.h>
    #define LIB_EXTENSION ".dylib"
    #define LIB_PREFIX ""
#else
    #include <dlfcn.h>
    #define LIB_EXTENSION ".so"
    #define LIB_PREFIX "lib"
#endif

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

    if (!setupAudioProcessing()) {
        qWarning() << "Failed to setup audio processing for VST3 plugin";
        unloadPlugin();
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
    if (!m_initialized || !m_processor) return;
    if (numFrames <= 0 || numChannels <= 0) return;

    // In a real implementation:
    // 1. Copy inputs to VST3 buffers
    // 2. Call process() on the VST3 processor
    // 3. Copy outputs back

    // Placeholder: passthrough
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (inputs[c] && outputs[c] && inputs[c] != outputs[c]) {
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }
    }
}

void VST3Host::reset() {
    // Reset plugin state
    // In a real implementation: call reset() on the VST3 processor
}

void VST3Host::setParameter(int index, float value) {
    if (index < 0 || index >= getParameterCount()) return;
    m_parameterCache[index] = value;

    // In a real implementation: call setParamNormalized() on the VST3 controller
}

float VST3Host::getParameter(int index) const {
    if (index < 0 || index >= getParameterCount()) return 0.0f;
    return m_parameterCache.value(index, 0.0f);
}

int VST3Host::getParameterCount() const {
    return m_info.parameters.size();
}

bool VST3Host::hasEditor() const {
    // In a real implementation: check if the plugin has a UI
    return false;
}

void* VST3Host::createEditor(void* parent) {
    // In a real implementation: create the plugin UI
    return nullptr;
}

void VST3Host::destroyEditor(void* editor) {
    // In a real implementation: destroy the plugin UI
}

void VST3Host::editorResized(int width, int height) {
    // Notify plugin of resize
}

bool VST3Host::loadPlugin() {
    QString path = m_info.path;

    // Check if path is a directory (.vst3 bundle)
    QFileInfo info(path);
    if (info.isDir()) {
        // For VST3 bundles on macOS/Windows, look for the actual DLL inside
        #ifdef _WIN32
        path = path + "/Contents/x86_64-win/";
        #elif __APPLE__
        path = path + "/Contents/MacOS/";
        #else
        path = path + "/Contents/x86_64-linux/";
        #endif
        // Find the first .dll/.so/.dylib in the directory
        QDir dir(path);
        if (dir.exists()) {
            QStringList filters;
            #ifdef _WIN32
            filters << "*.dll";
            #elif __APPLE__
            filters << "*.dylib";
            #else
            filters << "*.so";
            #endif
            QStringList files = dir.entryList(filters, QDir::Files);
            if (!files.isEmpty()) {
                path = dir.absoluteFilePath(files.first());
            }
        }
    }

    m_handle = loadLibrary(path);
    if (!m_handle) {
        qWarning() << "Failed to load VST3 library:" << path;
        return false;
    }

    // Get factory function
    using GetFactoryFunc = void* (*)();
    auto getFactory = reinterpret_cast<GetFactoryFunc>(getFunction(m_handle, "GetPluginFactory"));
    if (!getFactory) {
        qWarning() << "Failed to get plugin factory";
        unloadPlugin();
        return false;
    }

    // Get create function
    m_createFunc = reinterpret_cast<CreateFunc>(getFunction(m_handle, "createInstance"));
    if (!m_createFunc) {
        qWarning() << "Failed to get createInstance function";
        unloadPlugin();
        return false;
    }

    // Create plugin instance
    m_plugin = m_createFunc();
    if (!m_plugin) {
        qWarning() << "Failed to create plugin instance";
        unloadPlugin();
        return false;
    }

    return true;
}

void VST3Host::unloadPlugin() {
    if (m_plugin && m_deleteFunc) {
        m_deleteFunc(m_plugin);
        m_plugin = nullptr;
    }
    if (m_handle) {
        unloadLibrary(m_handle);
        m_handle = nullptr;
    }
    m_createFunc = nullptr;
    m_deleteFunc = nullptr;
    m_processor = nullptr;
    m_controller = nullptr;
}

bool VST3Host::setupAudioProcessing() {
    // In a real implementation:
    // 1. Query the plugin for its processor
    // 2. Initialize the processor with sample rate and block size
    // 3. Setup bus configurations

    return true;
}

// Platform-specific library loading
#ifdef _WIN32
VST3Host::HMODULE VST3Host::loadLibrary(const QString& path) {
    return reinterpret_cast<HMODULE>(::LoadLibraryW(path.toStdWString().c_str()));
}

void VST3Host::unloadLibrary(HMODULE handle) {
    if (handle) ::FreeLibrary(reinterpret_cast<HMODULE>(handle));
}

void* VST3Host::getFunction(HMODULE handle, const char* name) {
    if (!handle) return nullptr;
    return reinterpret_cast<void*>(::GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
}
#else
VST3Host::HMODULE VST3Host::loadLibrary(const QString& path) {
    return dlopen(path.toUtf8().constData(), RTLD_LAZY);
}

void VST3Host::unloadLibrary(HMODULE handle) {
    if (handle) dlclose(handle);
}

void* VST3Host::getFunction(HMODULE handle, const char* name) {
    if (!handle) return nullptr;
    return dlsym(handle, name);
}
#endif