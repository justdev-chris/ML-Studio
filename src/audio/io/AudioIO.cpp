#include "AudioIO.h"
#include <QDebug>
#include <QThread>

// PortAudio headers
#include <portaudio.h>

AudioIO::AudioIO(QObject* parent)
    : QObject(parent) {
    Pa_Initialize();
}

AudioIO::~AudioIO() {
    shutdown();
    Pa_Terminate();
}

bool AudioIO::initialize(int sampleRate, int bufferSize, int numChannels) {
    if (m_initialized) {
        qWarning() << "AudioIO already initialized";
        return true;
    }

    m_sampleRate = sampleRate;
    m_bufferSize = bufferSize;
    m_numChannels = numChannels;

    qDebug() << "AudioIO: initializing with " << sampleRate << "Hz, " << bufferSize << " samples, " << numChannels << " channels";

    m_initialized = true;
    emit initialized();
    return true;
}

void AudioIO::shutdown() {
    if (!m_initialized) return;
    stop();
    m_initialized = false;
    emit shutdownComplete();
    qDebug() << "AudioIO: shutdown";
}

bool AudioIO::start() {
    if (!m_initialized) {
        qWarning() << "AudioIO not initialized";
        return false;
    }
    if (m_running) {
        qWarning() << "AudioIO already running";
        return true;
    }

    // PortAudio stream setup
    PaStreamParameters outputParams;
    PaStreamParameters inputParams;

    outputParams.device = Pa_GetDefaultOutputDevice();
    outputParams.channelCount = m_numChannels;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultHighOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenDefaultStream(&m_audioStream, 0, m_numChannels, paFloat32, m_sampleRate, m_bufferSize, nullptr, nullptr);
    if (err != paNoError) {
        qCritical() << "PortAudio error: " << Pa_GetErrorText(err);
        emit error(QString("PortAudio: %1").arg(Pa_GetErrorText(err)));
        return false;
    }

    err = Pa_StartStream(m_audioStream);
    if (err != paNoError) {
        qCritical() << "PortAudio start error: " << Pa_GetErrorText(err);
        emit error(QString("PortAudio start: %1").arg(Pa_GetErrorText(err)));
        return false;
    }

    m_running = true;
    qDebug() << "AudioIO: started";
    return true;
}

void AudioIO::stop() {
    if (!m_running) return;
    if (m_audioStream) {
        Pa_StopStream(m_audioStream);
        Pa_CloseStream(m_audioStream);
        m_audioStream = nullptr;
    }
    m_running = false;
    qDebug() << "AudioIO: stopped";
}

QList<QString> AudioIO::getInputDevices() const {
    QList<QString> devices;
    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info->maxInputChannels > 0) {
            devices.append(QString(info->name));
        }
    }
    return devices;
}

QList<QString> AudioIO::getOutputDevices() const {
    QList<QString> devices;
    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info->maxOutputChannels > 0) {
            devices.append(QString(info->name));
        }
    }
    return devices;
}

bool AudioIO::setInputDevice(const QString& deviceName) {
    // TODO: Implement device selection
    Q_UNUSED(deviceName);
    return false;
}

bool AudioIO::setOutputDevice(const QString& deviceName) {
    // TODO: Implement device selection
    Q_UNUSED(deviceName);
    return false;
}

void AudioIO::setLatency(int input, int output) {
    if (input < 0) input = 0;
    if (output < 0) output = 0;
    m_inputLatency = input;
    m_outputLatency = output;
    emit latencyChanged(input, output);
}

void AudioIO::setInputGain(float gain) {
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 4.0f) gain = 4.0f;
    m_inputGain = gain;
}

void AudioIO::setOutputGain(float gain) {
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 4.0f) gain = 4.0f;
    m_outputGain = gain;
}

void AudioIO::processAudio(float** input, float** output, int numFrames) {
    if (!m_callback) return;

    if (input) {
        for (int i = 0; i < m_numChannels && i < numFrames; i++) {
            if (input[i]) {
                input[i][i] *= m_inputGain;
            }
        }
    }

    m_callback(input, output, numFrames);

    if (output) {
        for (int i = 0; i < m_numChannels && i < numFrames; i++) {
            if (output[i]) {
                output[i][i] *= m_outputGain;
            }
        }
    }
}