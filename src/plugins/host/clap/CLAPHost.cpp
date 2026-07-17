#include "CLAPHost.h"
#include <QDebug>
#include <QFile>

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

CLAPHost::CLAPHost(const PluginInfo& info)
    : m_info(info) {
}

CLAPHost::~CLAPHost() {
    shutdown();
}

bool CLAPHost::initialize(double sampleRate, int blockSize) {
    if (m_initialized) return true;

    m_sampleRate = sampleRate;
    m_blockSize = blockSize;

    if (!loadPlugin()) {
        qWarning() << "Failed to load CLAP plugin:" << m_info.path;
        return false;
    }

    m_initialized = true;
    qDebug() << "CLAP plugin initialized:" << m_info.name;
    return true;
}

void CLAPHost::shutdown() {
    if (!m_initialized) return;
    unloadPlugin();
    m_initialized = false;
    m_parameterCache.clear();
}

void CLAPHost::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!m_initialized || !m_plugin) return;
    if (numFrames <= 0 || numChannels <= 0) return;

    // Placeholder: passthrough
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (inputs[c] && outputs[c] && inputs[c] != outputs[c]) {
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }
    }
}

void CLAPHost::reset() {
    // Reset plugin state
}

void CLAPHost::setParameter(int index, float value) {
    if (index < 0 || index >= getParameterCount()) return;
    m_parameterCache[index] = value;
}

float CLAPHost::getParameter(int index) const {
    if (index < 0 || index >= getParameterCount()) return 0.0f;
    return m_parameterCache.value(index, 0.0f);
}

int CLAPHost::getParameterCount() const {
    return m_info.parameters.size();
}

bool CLAPHost::loadPlugin() {
    QString path = m_info.path;

    // CLAP plugins are standalone .clap files (shared libraries)
    QFileInfo info(path);
    if (!info.exists()) {
        qWarning() << "CLAP plugin file does not exist:" << path;
        return false;
    }

    m_handle = loadLibrary(path);
    if (!m_handle) {
        qWarning() << "Failed to load CLAP plugin:" << path;
        return false;
    }

    // In a real implementation:
    // 1. Get clap_plugin_entry_t
    // 2. Call init on the entry
    // 3. Create plugin instance

    return true;
}

void CLAPHost::unloadPlugin() {
    if (m_handle) {
        unloadLibrary(m_handle);
        m_handle = nullptr;
    }
    m_plugin = nullptr;
}

// Platform-specific library loading
#ifdef _WIN32
CLAPHost::HMODULE CLAPHost::loadLibrary(const QString& path) {
    return reinterpret_cast<HMODULE>(::LoadLibraryW(path.toStdWString().c_str()));
}

void CLAPHost::unloadLibrary(HMODULE handle) {
    if (handle) ::FreeLibrary(reinterpret_cast<HMODULE>(handle));
}

void* CLAPHost::getFunction(HMODULE handle, const char* name) {
    if (!handle) return nullptr;
    return reinterpret_cast<void*>(::GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
}
#else
CLAPHost::HMODULE CLAPHost::loadLibrary(const QString& path) {
    return dlopen(path.toUtf8().constData(), RTLD_LAZY);
}

void CLAPHost::unloadLibrary(HMODULE handle) {
    if (handle) dlclose(handle);
}

void* CLAPHost::getFunction(HMODULE handle, const char* name) {
    if (!handle) return nullptr;
    return dlsym(handle, name);
}
#endif