#include "LV2Host.h"
#include <QDebug>
#include <QDir>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

LV2Host::LV2Host(const PluginInfo& info) : m_info(info) {}
LV2Host::~LV2Host() { shutdown(); }

bool LV2Host::initialize(double sampleRate, int blockSize) {
    if (m_initialized) return true;
    m_sampleRate = sampleRate; m_blockSize = blockSize;
    if (!loadPlugin()) return false;
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
    for (int i = 0; i < 2; i++) { delete[] m_inputBuffers[i]; delete[] m_outputBuffers[i]; }
    unloadPlugin();
    m_initialized = false;
    m_parameterCache.clear();
}

void LV2Host::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!m_initialized || !m_plugin) {
        for (int c = 0; c < numChannels && c < 2; c++) {
            if (inputs[c] && outputs[c] && inputs[c] != outputs[c])
                memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }
        return;
    }
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (inputs[c]) memcpy(m_inputBuffers[c], inputs[c], numFrames * sizeof(float));
        else memset(m_inputBuffers[c], 0, numFrames * sizeof(float));
    }
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (outputs[c]) memcpy(outputs[c], m_inputBuffers[c], numFrames * sizeof(float));
    }
}

void LV2Host::reset() {}
void LV2Host::setParameter(int index, float value) {
    if (index < 0 || index >= getParameterCount()) return;
    m_parameterCache[index] = value;
}
float LV2Host::getParameter(int index) const { return m_parameterCache.value(index, 0.0f); }
int LV2Host::getParameterCount() const { return m_info.parameters.size(); }

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
        if (!files.isEmpty()) path = dir.absoluteFilePath(files.first());
        else {
            QString baseName = info.baseName();
#ifdef _WIN32
            path = dir.absoluteFilePath(baseName + ".dll");
#else
            path = dir.absoluteFilePath("lib" + baseName + ".so");
#endif
        }
    }

#ifdef _WIN32
    m_handle = LoadLibraryW(path.toStdWString().c_str());
    if (!m_handle) return false;
    using DescriptorFunc = void* (*)(uint32_t);
    auto descriptor = (DescriptorFunc)GetProcAddress((HMODULE)m_handle, "lv2_descriptor");
    if (!descriptor) { unloadPlugin(); return false; }
    m_descriptor = descriptor(0);
    if (!m_descriptor) { unloadPlugin(); return false; }
    m_instantiate = (InstantiateFunc)GetProcAddress((HMODULE)m_handle, "instantiate");
    m_activate = (ActivateFunc)GetProcAddress((HMODULE)m_handle, "activate");
    m_run = (RunFunc)GetProcAddress((HMODULE)m_handle, "run");
    m_deactivate = (DeactivateFunc)GetProcAddress((HMODULE)m_handle, "deactivate");
    m_cleanup = (CleanupFunc)GetProcAddress((HMODULE)m_handle, "cleanup");
#else
    m_handle = dlopen(path.toUtf8().constData(), RTLD_LAZY);
    if (!m_handle) return false;
    using DescriptorFunc = void* (*)(uint32_t);
    auto descriptor = (DescriptorFunc)dlsym(m_handle, "lv2_descriptor");
    if (!descriptor) { unloadPlugin(); return false; }
    m_descriptor = descriptor(0);
    if (!m_descriptor) { unloadPlugin(); return false; }
    m_instantiate = (InstantiateFunc)dlsym(m_handle, "instantiate");
    m_activate = (ActivateFunc)dlsym(m_handle, "activate");
    m_run = (RunFunc)dlsym(m_handle, "run");
    m_deactivate = (DeactivateFunc)dlsym(m_handle, "deactivate");
    m_cleanup = (CleanupFunc)dlsym(m_handle, "cleanup");
#endif

    if (!m_instantiate || !m_run) { unloadPlugin(); return false; }
    m_plugin = m_instantiate(m_descriptor, m_sampleRate, m_info.path.toUtf8().constData(), nullptr);
    if (!m_plugin) { unloadPlugin(); return false; }
    if (m_activate) m_activate(m_plugin);
    return true;
}

void LV2Host::unloadPlugin() {
    if (m_plugin) {
        if (m_deactivate) m_deactivate(m_plugin);
        if (m_cleanup) m_cleanup(m_plugin);
    }
#ifdef _WIN32
    if (m_handle) FreeLibrary((HMODULE)m_handle);
#else
    if (m_handle) dlclose(m_handle);
#endif
    m_handle = nullptr;
    m_plugin = nullptr;
    m_descriptor = nullptr;
    m_instantiate = m_activate = m_run = m_deactivate = m_cleanup = nullptr;
}
