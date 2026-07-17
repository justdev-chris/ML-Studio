#include "LV2Host.h"
#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
    #define LIB_EXTENSION ".dll"
#elif __APPLE__
    #include <dlfcn.h>
    #define LIB_EXTENSION ".dylib"
#else
    #include <dlfcn.h>
    #define LIB_EXTENSION ".so"
#endif

LV2Host::LV2Host(const PluginInfo& info)
    : m_info(info) {
}

LV2Host::~LV2Host() {
    shutdown();
}

bool LV2Host::initialize(double sampleRate, int blockSize) {
    if (m_initialized) return true;

    m_sampleRate = sampleRate;
    m_blockSize = blockSize;

    if (!loadPlugin()) {
        qWarning() << "Failed to load LV2 plugin:" << m_info.path;
        return false;
    }

    if (!setupAudioProcessing()) {
        qWarning() << "Failed to setup audio processing for LV2 plugin";
        unloadPlugin();
        return false;
    }

    // Allocate buffers
    for (int i = 0; i < 2; i++) {
        m_inputBuffers[i] = new float[m_blockSize];
        m_outputBuffers[i] = new float[m_blockSize];
        std::memset(m_inputBuffers[i], 0, m_blockSize * sizeof(float));
        std::memset(m_outputBuffers[i], 0, m_blockSize * sizeof(float));
    }

    m_initialized = true;
    qDebug() << "LV2 plugin initialized:" << m_info.name;
    return true;
}

void LV2Host::shutdown() {
    if (!m_initialized) return;

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
    if (!m_initialized || !m_plugin || !m_run) return;
    if (numFrames <= 0 || numChannels <= 0) return;
    if (numFrames > m_blockSize) return;

    // Copy inputs to LV2 buffers
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (inputs[c]) {
            std::memcpy(m_inputBuffers[c], inputs[c], numFrames * sizeof(float));
        } else {
            std::memset(m_inputBuffers[c], 0, numFrames * sizeof(float));
        }
    }

    // Connect buffers to plugin (in a real implementation, you'd use LV2's port system)
    // For now: passthrough
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (outputs[c]) {
            std::memcpy(outputs[c], m_inputBuffers[c], numFrames * sizeof(float));
        }
    }
}

void LV2Host::reset() {
    // Reset plugin state
}

void LV2Host::setParameter(int index, float value) {
    if (index < 0 || index >= getParameterCount()) return;
    m_parameterCache[index] = value;
}

float LV2Host::getParameter(int index) const {
    if (index < 0 || index >= getParameterCount()) return 0.0f;
    return m_parameterCache.value(index, 0.0f);
}

int LV2Host::getParameterCount() const {
    return m_info.parameters.size();
}

bool LV2Host::loadPlugin() {
    QString path = m_info.path;

    // LV2 plugins are directories containing a .so/.dll
    // Look for the actual library file
    QFileInfo info(path);
    if (info.isDir()) {
        QDir dir(path);
        #ifdef _WIN32
        QStringList filters = {"*.dll"};
        #else
        QStringList filters = {"*.so", "*.dylib"};
        #endif
        QStringList files = dir.entryList(filters, QDir::Files);
        if (!files.isEmpty()) {
            path = dir.absoluteFilePath(files.first());
        } else {
            // Try .so with the same name as the directory
            QString baseName = info.baseName();
            #ifdef _WIN32
            path = dir.absoluteFilePath(baseName + ".dll");
            #else
            path = dir.absoluteFilePath("lib" + baseName + ".so");
            #endif
        }
    }

    m_handle = loadLibrary(path);
    if (!m_handle) {
        qWarning() << "Failed to load LV2 library:" << path;
        return false;
    }

    // Get LV2 descriptor function
    using DescriptorFunc = void* (*)(uint32_t index);
    auto descriptor = reinterpret_cast<DescriptorFunc>(getFunction(m_handle, "lv2_descriptor"));
    if (!descriptor) {
        qWarning() << "Failed to get lv2_descriptor function";
        unloadPlugin();
        return false;
    }

    // Get plugin descriptor (index 0)
    void* pluginDescriptor = descriptor(0);
    if (!pluginDescriptor) {
        qWarning() << "Failed to get LV2 plugin descriptor";
        unloadPlugin();
        return false;
    }

    // Get instantiate function from descriptor
    // (This is simplified — in a real implementation you'd read the descriptor struct)
    m_instantiate = reinterpret_cast<InstantiateFunc>(getFunction(m_handle, "instantiate"));
    if (!m_instantiate) {
        qWarning() << "Failed to get instantiate function";
        unloadPlugin();
        return false;
    }

    // Create plugin instance
    m_plugin = m_instantiate(pluginDescriptor, m_sampleRate, m_info.path.toUtf8().constData(), nullptr);
    if (!m_plugin) {
        qWarning() << "Failed to instantiate LV2 plugin";
        unloadPlugin();
        return false;
    }

    // Get other functions
    m_activate = reinterpret_cast<ActivateFunc>(getFunction(m_handle, "activate"));
    m_run = reinterpret_cast<RunFunc>(getFunction(m_handle, "run"));
    m_deactivate = reinterpret_cast<DeactivateFunc>(getFunction(m_handle, "deactivate"));
    m_cleanup = reinterpret_cast<CleanupFunc>(getFunction(m_handle, "cleanup"));

    if (m_activate) {
        m_activate(m_plugin);
    }

    return true;
}

void LV2Host::unloadPlugin() {
    if (m_plugin && m_deactivate) {
        m_deactivate(m_plugin);
    }
    if (m_plugin && m_cleanup) {
        m_cleanup(m_plugin);
        m_plugin = nullptr;
    }
    if (m_handle) {
        unloadLibrary(m_handle);
        m_handle = nullptr;
    }
    m_instantiate = nullptr;
    m_activate = nullptr;
    m_run = nullptr;
    m_deactivate = nullptr;
    m_cleanup = nullptr;
}

bool LV2Host::setupAudioProcessing() {
    // In a real implementation: setup ports, buffers, and connections
    return true;
}

// Platform-specific library loading
#ifdef _WIN32
LV2Host::HMODULE LV2Host::loadLibrary(const QString& path) {
    return reinterpret_cast<HMODULE>(::LoadLibraryW(path.toStdWString().c_str()));
}

void LV2Host::unloadLibrary(HMODULE handle) {
    if (handle) ::FreeLibrary(reinterpret_cast<HMODULE>(handle));
}

void* LV2Host::getFunction(HMODULE handle, const char* name) {
    if (!handle) return nullptr;
    return reinterpret_cast<void*>(::GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
}
#else
LV2Host::HMODULE LV2Host::loadLibrary(const QString& path) {
    return dlopen(path.toUtf8().constData(), RTLD_LAZY);
}

void LV2Host::unloadLibrary(HMODULE handle) {
    if (handle) dlclose(handle);
}

void* LV2Host::getFunction(HMODULE handle, const char* name) {
    if (!handle) return nullptr;
    return dlsym(handle, name);
}
#endif