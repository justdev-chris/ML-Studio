#include "CLAPHost.h"
#include <QDebug>
#include <QFile>
#include <QDir>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
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

    if (m_plugin && m_destroy) {
        m_destroy(m_plugin);
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

    m_initialized = false;
    m_parameterCache.clear();
}

void CLAPHost::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!m_initialized || !m_plugin || !m_process) {
        // Passthrough
        for (int c = 0; c < numChannels && c < 2; c++) {
            if (inputs[c] && outputs[c] && inputs[c] != outputs[c]) {
                memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
            }
        }
        return;
    }

    // In a real CLAP implementation:
    // 1. Prepare clap_process_t structure
    // 2. Call clap_plugin::process()

    // For now: passthrough
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

    if (m_plugin && m_setParam) {
        m_setParam(m_plugin, index, value);
    }
}

float CLAPHost::getParameter(int index) const {
    if (index < 0 || index >= getParameterCount()) return 0.0f;
    if (m_plugin && m_getParam) {
        return m_getParam(m_plugin, index);
    }
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

    // Load library
#ifdef _WIN32
    m_handle = LoadLibraryW(path.toStdWString().c_str());
    if (!m_handle) {
        qWarning() << "Failed to load CLAP plugin:" << path;
        return false;
    }

    // Get clap_plugin_entry_t
    using EntryFunc = void* (*)();
    auto entry = (EntryFunc)GetProcAddress((HMODULE)m_handle, "clap_entry");
    if (!entry) {
        qWarning() << "Failed to get clap_entry function";
        unloadPlugin();
        return false;
    }

    m_entry = entry();

    // Get plugin factory
    using FactoryFunc = void* (*)(const char* plugin_id);
    auto factory = (FactoryFunc)GetProcAddress((HMODULE)m_handle, "clap_factory");
    if (!factory) {
        qWarning() << "Failed to get clap_factory function";
        unloadPlugin();
        return false;
    }

    // Create plugin instance
    m_plugin = factory(m_info.id.toUtf8().constData());
    if (!m_plugin) {
        qWarning() << "Failed to create CLAP plugin instance";
        unloadPlugin();
        return false;
    }

    // Get function pointers
    m_init = (InitFunc)GetProcAddress((HMODULE)m_handle, "clap_plugin_init");
    m_destroy = (DestroyFunc)GetProcAddress((HMODULE)m_handle, "clap_plugin_destroy");
    m_process = (ProcessFunc)GetProcAddress((HMODULE)m_handle, "clap_plugin_process");
    m_setParam = (SetParamFunc)GetProcAddress((HMODULE)m_handle, "clap_plugin_set_param");
    m_getParam = (GetParamFunc)GetProcAddress((HMODULE)m_handle, "clap_plugin_get_param");

    if (m_init) {
        m_init(m_plugin);
    }

    return true;

#else
    m_handle = dlopen(path.toUtf8().constData(), RTLD_LAZY);
    if (!m_handle) {
        qWarning() << "Failed to load CLAP plugin:" << dlerror();
        return false;
    }

    // Get clap_plugin_entry_t
    using EntryFunc = void* (*)();
    auto entry = (EntryFunc)dlsym(m_handle, "clap_entry");
    if (!entry) {
        qWarning() << "Failed to get clap_entry function:" << dlerror();
        unloadPlugin();
        return false;
    }

    m_entry = entry();

    // Get plugin factory
    using FactoryFunc = void* (*)(const char* plugin_id);
    auto factory = (FactoryFunc)dlsym(m_handle, "clap_factory");
    if (!factory) {
        qWarning() << "Failed to get clap_factory function:" << dlerror();
        unloadPlugin();
        return false;
    }

    // Create plugin instance
    m_plugin = factory(m_info.id.toUtf8().constData());
    if (!m_plugin) {
        qWarning() << "Failed to create CLAP plugin instance";
        unloadPlugin();
        return false;
    }

    // Get function pointers
    m_init = (InitFunc)dlsym(m_handle, "clap_plugin_init");
    m_destroy = (DestroyFunc)dlsym(m_handle, "clap_plugin_destroy");
    m_process = (ProcessFunc)dlsym(m_handle, "clap_plugin_process");
    m_setParam = (SetParamFunc)dlsym(m_handle, "clap_plugin_set_param");
    m_getParam = (GetParamFunc)dlsym(m_handle, "clap_plugin_get_param");

    if (m_init) {
        m_init(m_plugin);
    }

    return true;
#endif
}

void CLAPHost::unloadPlugin() {
    if (m_plugin && m_destroy) {
        m_destroy(m_plugin);
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
    m_entry = nullptr;
    m_init = nullptr;
    m_destroy = nullptr;
    m_process = nullptr;
    m_setParam = nullptr;
    m_getParam = nullptr;
}
