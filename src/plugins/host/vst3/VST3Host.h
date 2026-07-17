#ifndef VST3HOST_H
#define VST3HOST_H

#include "plugins/host/PluginHost.h"
#include <QString>
#include <QMap>
#include <vector>

// Forward declarations (VST3 SDK types)
struct VST3Plugin;

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
    bool setupAudioProcessing();

    PluginInfo m_info;
    void* m_handle = nullptr;           // Shared library handle
    void* m_plugin = nullptr;           // VST3 component instance
    void* m_processor = nullptr;        // VST3 audio processor
    void* m_controller = nullptr;       // VST3 edit controller

    double m_sampleRate = 44100.0;
    int m_blockSize = 256;
    bool m_initialized = false;

    // Plugin function pointers
    using CreateFunc = void* (*)();
    using DeleteFunc = void (*)(void*);

    CreateFunc m_createFunc = nullptr;
    DeleteFunc m_deleteFunc = nullptr;

    // Parameter cache
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

#endif // VST3HOST_H