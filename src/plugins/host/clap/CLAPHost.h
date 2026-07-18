#ifndef CLAPHOST_H
#define CLAPHOST_H

#include "plugins/host/PluginHost.h"
#include <QString>
#include <QMap>
#include <QVector>
#include <QByteArray>

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

    void sendMIDI(const QVector<MIDIEvent>& events) override { m_midiEvents.append(events); }

private:
    bool loadPlugin();
    void unloadPlugin();

    PluginInfo m_info;
    void* m_handle = nullptr;
    void* m_plugin = nullptr;
    void* m_entry = nullptr;
    void* m_factory = nullptr;
    void* m_gui = nullptr;
    void* m_editorWidget = nullptr;

    double m_sampleRate = 44100.0;
    int m_blockSize = 256;
    bool m_initialized = false;

    // Function pointers (CLAP)
    using InitFunc = bool (*)(void* plugin);
    using DestroyFunc = void (*)(void* plugin);
    using ProcessFunc = bool (*)(void* plugin, void* processData);
    using SetParamFunc = void (*)(void* plugin, int index, double value);
    using GetParamFunc = double (*)(void* plugin, int index);

    InitFunc m_init = nullptr;
    DestroyFunc m_destroy = nullptr;
    ProcessFunc m_process = nullptr;
    SetParamFunc m_setParam = nullptr;
    GetParamFunc m_getParam = nullptr;

    QMap<int, float> m_parameterCache;
    float* m_inputBuffers[2] = {nullptr, nullptr};
    float* m_outputBuffers[2] = {nullptr, nullptr};
    QVector<MIDIEvent> m_midiEvents;
    QByteArray m_eventBuffer;

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
