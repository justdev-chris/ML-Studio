#ifndef CLAPHOST_H
#define CLAPHOST_H

#include "plugins/host/PluginHost.h"
#include <QString>
#include <QMap>

class CLAPHost : public PluginInstance {
public:
    explicit CLAPHost(const PluginInfo& info);
    ~CLAPHost() override;

    bool initialize(double sampleRate, int blockSize) override;
    void shutdown() override;
    bool isInitialized() const override { return m_initialized; }

    void process(float** inputs, float** outputs, int numChannels, int numFrames) override;
    void reset() override;

    void setParameter(int index, float value) override;
    float getParameter(int index) const override;
    int getParameterCount() const override;

    void setSampleRate(double sampleRate) override { m_sampleRate = sampleRate; }
    void setBlockSize(int blockSize) override { m_blockSize = blockSize; }

    PluginInfo getInfo() const override { return m_info; }
    void* getNativeHandle() const override { return m_handle; }

private:
    bool loadPlugin();
    void unloadPlugin();

    PluginInfo m_info;
    void* m_handle = nullptr;
    void* m_plugin = nullptr;

    double m_sampleRate = 44100.0;
    int m_blockSize = 256;
    bool m_initialized = false;

    QMap<int, float> m_parameterCache;

#ifdef _WIN32
    using HMODULE = void*;
#else
    using HMODULE = void*;
#endif

    HMODULE loadLibrary(const QString& path);
    void unloadLibrary(HMODULE handle);
    void* getFunction(HMODULE handle, const char* name);
};

#endif // CLAPHOST_H