#ifndef AUDIOIO_H
#define AUDIOIO_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QList>
#include <QMutex>
#include <atomic>
#include <functional>

class AudioEngine;

class AudioIO : public QObject {
    Q_OBJECT

public:
    explicit AudioIO(QObject* parent = nullptr);
    ~AudioIO();

    bool initialize(int sampleRate = 44100, int bufferSize = 256, int numChannels = 2);
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    bool start();
    void stop();
    bool isRunning() const { return m_running; }

    // Device management
    QList<QString> getInputDevices() const;
    QList<QString> getOutputDevices() const;
    bool setInputDevice(const QString& deviceName);
    bool setOutputDevice(const QString& deviceName);
    QString getCurrentInputDevice() const { return m_currentInputDevice; }
    QString getCurrentOutputDevice() const { return m_currentOutputDevice; }

    // Latency
    int getInputLatency() const { return m_inputLatency; }
    int getOutputLatency() const { return m_outputLatency; }
    void setLatency(int input, int output);

    // Callback
    using AudioCallback = std::function<void(float** input, float** output, int numFrames)>;
    void setCallback(AudioCallback callback) { m_callback = callback; }

    // Volume
    void setInputGain(float gain);
    void setOutputGain(float gain);
    float getInputGain() const { return m_inputGain; }
    float getOutputGain() const { return m_outputGain; }

signals:
    void initialized();
    void shutdownComplete();
    void deviceChanged(const QString& device);
    void latencyChanged(int input, int output);
    void error(const QString& message);
    void bufferUnderrun();
    void bufferOverrun();

private:
    bool m_initialized = false;
    bool m_running = false;
    int m_sampleRate = 44100;
    int m_bufferSize = 256;
    int m_numChannels = 2;

    QString m_currentInputDevice;
    QString m_currentOutputDevice;
    int m_inputLatency = 0;
    int m_outputLatency = 0;

    float m_inputGain = 1.0f;
    float m_outputGain = 1.0f;

    AudioCallback m_callback;
    mutable QMutex m_mutex;

    // PortAudio specific
    void* m_audioStream = nullptr;
    void* m_audioApi = nullptr;

    void processAudio(float** input, float** output, int numFrames);
};

#endif // AUDIOIO_H