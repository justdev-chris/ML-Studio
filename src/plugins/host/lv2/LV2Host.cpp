#include "LV2Host.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <cstring>

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
    if (!m_initialized || !m_plugin || !m_run) {
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

    // In a real LV2 implementation, we would:
    // 1. Connect ports to buffers
    // 2. Call run() on the plugin

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
    m_descriptor = descriptor(0);
    if (!m_descriptor) {
        qWarning() << "Failed to get LV2 plugin descriptor";
        unloadPlugin();
        return false;
    }

    // Get instantiate function
    m_instantiate = (InstantiateFunc)GetProcAddress((HMODULE)m_handle, "instantiate");
    if (!m_instantiate) {
        qWarning() << "Failed to get instantiate function";
        unloadPlugin();
        return false;
    }

    // Create plugin instance
    m_plugin = m_instantiate(m_descriptor, m_sampleRate, m_info.path.toUtf8().constData(), nullptr);
    if (!m_plugin) {
        qWarning() << "Failed to instantiate LV2 plugin";
        unloadPlugin();
        return false;
    }

    // Get activate function
    m_activate = (ActivateFunc)GetProcAddress((HMODULE)m_handle, "activate");
    if (m_activate) {
        m_activate(m_plugin);
    }

    // Get run function
    m_run = (RunFunc)GetProcAddress((HMODULE)m_handle, "run");
    if (!m_run) {
        qWarning() << "Failed to get run function";
        unloadPlugin();
        return false;
    }

    // Get deactivate function
    m_deactivate = (DeactivateFunc)GetProcAddress((HMODULE)m_handle, "deactivate");

    // Get cleanup function
    m_cleanup = (CleanupFunc)GetProcAddress((HMODULE)m_handle, "cleanup");

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
    m_descriptor = descriptor(0);
    if (!m_descriptor) {
        qWarning() << "Failed to
