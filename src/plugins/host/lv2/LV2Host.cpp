#include "LV2Host.h"
#include <QDebug>
#include <QFile>
#include <QDir>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
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
    if (!m_initialized || !m_plugin) {
        // Passthrough
        for (int c = 0; c < numChannels && c < 2; c++) {
            if (inputs[c] && outputs[c] && inputs[c] != outputs[c]) {
                memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
            }
        }
        return;
    }

    // Copy inputs to LV2 buffers
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (inputs[c]) {
            std::memcpy(m_inputBuffers[c], inputs[c], numFrames * sizeof(float));
        } else {
            std::memset(m_inputBuffers[c], 0, numFrames * sizeof(float));
        }
    }

    // Call LV2 run function
    if (m_run) {
        m_run(m_plugin, numFrames);
    }

    // Copy outputs back
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (outputs[c]) {
            std::memcpy(outputs[c], m_outputBuffers[c], numFrames * sizeof(float));
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
            // Try .so with the same name as the directory
            QString baseName = info.baseName();
#ifdef _WIN32
            path = dir.absoluteFilePath(baseName + ".dll");
#else
            path = dir.absoluteFilePath("lib" + baseName + ".so");
#endif
        }
    }

    // Load library
#ifdef _WIN32
    m_handle = LoadLibraryW(path.toStdWString().c_str());
    if (!m_handle) {
        qWarning() << "Failed to load LV2 DLL:" << path;
        return false;
    }

    // Get LV2 descriptor function
    using DescriptorFunc = void* (*)(uint32_t index);
    auto descriptor = (DescriptorFunc)GetProcAddress((HMODULE)m_handle, "lv2_descriptor");
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

    // Get instantiate function
    using InstantiateFunc = void* (*)(void* descriptor, double sampleRate, const char* bundlePath, const void* features);
    m_instantiate = (InstantiateFunc)GetProcAddress((HMODULE)m_handle, "instantiate");
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

    // Get run function
    using RunFunc = void (*)(void* instance, uint32_t sampleCount);
    m_run = (RunFunc)GetProcAddress((HMODULE)m_handle, "run");
    if (!m_run) {
        qWarning() << "Failed to get run function";
        unloadPlugin();
        return false;
    }

    return true;

#else
    m_handle = dlopen(path.toUtf8().constData(), RTLD_LAZY);
    if (!m_handle) {
        qWarning() << "Failed to load LV2 library:" << dlerror();
        return false;
    }

    // Get LV2 descriptor function
    using DescriptorFunc = void* (*)(uint32_t index);
    auto descriptor = (DescriptorFunc)dlsym(m_handle, "lv2_descriptor");
    if (!descriptor) {
        qWarning() << "Failed to get lv2_descriptor function:" << dlerror();
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

    // Get instantiate function
    using InstantiateFunc = void* (*)(void* descriptor, double sampleRate, const char* bundlePath, const void* features);
    m_instantiate = (InstantiateFunc)dlsym(m_handle, "instantiate");
    if (!m_instantiate) {
        qWarning() << "Failed to get instantiate function:" << dlerror();
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

    // Get run function
    using RunFunc = void (*)(void* instance, uint32_t sampleCount);
    m_run = (RunFunc)dlsym(m_handle, "run");
    if (!m_run) {
        qWarning() << "Failed to get run function:" << dlerror();
        unloadPlugin();
        return false;
    }

    return true;
#endif
}

void LV2Host::unloadPlugin() {
    if (m_plugin && m_cleanup) {
        m_cleanup(m_plugin);
        m_plugin = nullptr;
    }
    if (m_handle) {
#ifdef _WIN32
        FreeLibrary((HMODULE)m_handle);
#else
        dlclose(m_handle);
#endif
        m_handle = nullptr;
    }
    m_instantiate = nullptr;
    m_run = nullptr;
    m_cleanup = nullptr;
}
