#ifndef VST3HOST_H
#define VST3HOST_H

#include "plugins/host/PluginHost.h"
#include <QString>
#include <QMap>

class VST3Host : public PluginInstance {
public:
    explicit VST3Host(const PluginInfo& info);
    ~VST3Host() override;

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

    bool hasEditor() const override;
    void* createEditor(void* parent) override;
    void destroyEditor(void* editor) override;
    void editorResized(int width, int height) override;

private:
    bool loadPlugin();
    void unloadPlugin();

    PluginInfo m_info;
    void* m_handle = nullptr;
    void* m_processor = nullptr;
    void* m_controller = nullptr;
    void* m_component = nullptr;

    double m_sampleRate = 44100.0;
    int m_blockSize = 256;
    bool m_initialized = false;

    QMap<int, float> m_parameterCache;

    // VST3 function pointers
    using CreateInstanceFunc = void* (*)(const char* cid, const char* iid);
    CreateInstanceFunc m_createInstance = nullptr;

#ifdef _WIN32
    using HMODULE = void*;
#else
    using HMODULE = void*;
#endif

    HMODULE loadLibrary(const QString& path);
    void unloadLibrary(HMODULE handle);
    void* getFunction(HMODULE handle, const char* name);
};

#endif // VST3HOST_H
