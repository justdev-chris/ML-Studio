#include "CLAPHost.h"
#include <QDebug>
#include <QDir>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

CLAPHost::CLAPHost(const PluginInfo& info) : m_info(info) {}
CLAPHost::~CLAPHost() { shutdown(); }

bool CLAPHost::initialize(double sampleRate, int blockSize) {
    if (m_initialized) return true;
    m_sampleRate = sampleRate; m_blockSize = blockSize;
    if (!loadPlugin()) return false;
    m_initialized = true;
    return true;
}

void CLAPHost::shutdown() {
    if (!m_initialized) return;
    if (m_plugin && m_destroy) m_destroy(m_plugin);
#ifdef _WIN32
    if (m_handle) FreeLibrary((HMODULE)m_handle);
#else
    if (m_handle) dlclose(m_handle);
#endif
    m_handle = nullptr;
    m_plugin = nullptr;
    m_initialized = false;
    m_parameterCache.clear();
}

void CLAPHost::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!m_initialized || !m_plugin) {
        for (int c = 0; c < numChannels && c < 2; c++) {
            if (inputs[c] && outputs[c] && inputs[c] != outputs[c])
                memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }
        return;
    }
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (inputs[c] && outputs[c] && inputs[c] != outputs[c])
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
    }
}

void CLAPHost::reset() {}
void CLAPHost::setParameter(int index, float value) {
    if (index < 0 || index >= getParameterCount()) return;
    m_parameterCache[index] = value;
    if (m_plugin && m_setParam) m_setParam(m_plugin, index, value);
}
float CLAPHost::getParameter(int index) const {
    if (m_plugin && m_getParam) return m_getParam(m_plugin, index);
    return m_parameterCache.value(index, 0.0f);
}
int CLAPHost::getParameterCount() const { return m_info.parameters.size(); }

bool CLAPHost::loadPlugin() {
    QString path = m_info.path;
    if (!QFile::exists(path)) return false;

#ifdef _WIN32
    m_handle = LoadLibraryW(path.toStdWString().c_str());
    if (!m_handle) return false;
    using EntryFunc = void* (*)();
    auto entry = (EntryFunc)GetProcAddress((HMODULE)m_handle, "clap_entry");
    if (!entry) { unloadPlugin(); return false; }
    m_entry = entry();
    using FactoryFunc = void* (*)(const char*);
    auto factory = (FactoryFunc)GetProcAddress((HMODULE)m_handle, "clap_factory");
    if (!factory) { unloadPlugin(); return false; }
    m_plugin = factory(m_info.id.toUtf8().constData());
#else
    m_handle = dlopen(path.toUtf8().constData(), RTLD_LAZY);
    if (!m_handle) return false;
    using EntryFunc = void* (*)();
    auto entry = (EntryFunc)dlsym(m_handle, "clap_entry");
    if (!entry) { unloadPlugin(); return false; }
    m_entry = entry();
    using FactoryFunc = void* (*)(const char*);
    auto factory = (FactoryFunc)dlsym(m_handle, "clap_factory");
    if (!factory) { unloadPlugin(); return false; }
    m_plugin = factory(m_info.id.toUtf8().constData());
#endif

    if (!m_plugin) { unloadPlugin(); return false; }

#ifdef _WIN32
    m_init = (InitFunc)GetProcAddress((HMODULE)m_handle, "clap_plugin_init");
    m_destroy = (DestroyFunc)GetProcAddress((HMODULE)m_handle, "clap_plugin_destroy");
    m_process = (ProcessFunc)GetProcAddress((HMODULE)m_handle, "clap_plugin_process");
    m_setParam = (SetParamFunc)GetProcAddress((HMODULE)m_handle, "clap_plugin_set_param");
    m_getParam = (GetParamFunc)GetProcAddress((HMODULE)m_handle, "clap_plugin_get_param");
#else
    m_init = (InitFunc)dlsym(m_handle, "clap_plugin_init");
    m_destroy = (DestroyFunc)dlsym(m_handle, "clap_plugin_destroy");
    m_process = (ProcessFunc)dlsym(m_handle, "clap_plugin_process");
    m_setParam = (SetParamFunc)dlsym(m_handle, "clap_plugin_set_param");
    m_getParam = (GetParamFunc)dlsym(m_handle, "clap_plugin_get_param");
#endif

    if (m_init) m_init(m_plugin);
    return true;
}

void CLAPHost::unloadPlugin() {
    if (m_plugin && m_destroy) m_destroy(m_plugin);
#ifdef _WIN32
    if (m_handle) FreeLibrary((HMODULE)m_handle);
#else
    if (m_handle) dlclose(m_handle);
#endif
    m_handle = nullptr;
    m_plugin = nullptr;
    m_entry = nullptr;
    m_init = m_destroy = m_process = m_setParam = m_getParam = nullptr;
}
