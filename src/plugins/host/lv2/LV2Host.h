#ifndef LV2HOST_H
#define LV2HOST_H

#include "plugins/host/PluginHost.h"
#include <QString>
#include <QMap>
#include <QVector>

class LV2Host : public PluginInstance {
public:
    explicit LV2Host(const PluginInfo& info);
    ~LV2Host() override;

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
    void* m_descriptor = nullptr;
    void* m_uiDescriptor = nullptr;
    void* m_uiHandle = nullptr;
    void* m_editorWidget = nullptr;

    // MIDI port buffers (will be set during port connection)
    void* m_midiInputPort = nullptr;
    void* m_midiOutputPort = nullptr;

    double m_sampleRate = 44100.0;
    int m_blockSize = 256;
    bool m_initialized = false;

    // Function pointers
    using InstantiateFunc = void* (*)(void* descriptor, double sampleRate, const char* bundlePath, const void* features);
    using ActivateFunc = void (*)(void* instance);
    using RunFunc = void (*)(void* instance, uint32_t sampleCount);
    using DeactivateFunc = void (*)(void* instance);
    using CleanupFunc = void (*)(void* instance);

    InstantiateFunc m_instantiate = nullptr;
    ActivateFunc m_activate = nullptr;
    RunFunc m_run = nullptr;
    DeactivateFunc m_deactivate = nullptr;
    CleanupFunc m_cleanup = nullptr;

    QMap<int, float> m_parameterCache;
    float* m_inputBuffers[2] = {nullptr, nullptr};
    float* m_outputBuffers[2] = {nullptr, nullptr};
    QVector<MIDIEvent> m_midiEvents;

#ifdef _WIN32
    using HMODULE = void*;
#else
    using HMODULE = void*;
#endif

    HMODULE loadLibrary(const QString& path);
    void unloadLibrary(HMODULE handle);
    void* getFunction(HMODULE handle, const char* name);
};

#endif // LV2HOST_H
