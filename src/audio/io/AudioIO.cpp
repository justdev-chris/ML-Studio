#include "AudioIO.h"
#include <QDebug>
#include <QThread>
#include <cmath>

// PortAudio headers
#include <portaudio.h>

AudioIO::AudioIO(QObject* parent)
    : QObject(parent) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        qCritical() << "PortAudio initialization error:" << Pa_GetErrorText(err);
        emit error(QString("PortAudio: %1").arg(Pa_GetErrorText(err)));
    }
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

    // Set up PortAudio stream parameters
    m_outputParams.device = Pa_GetDefaultOutputDevice();
    if (m_outputParams.device == paNoDevice) {
        emit error("No audio output device found");
        return false;
    }

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(m_outputParams.device);
    if (!deviceInfo) {
        emit error("Failed to get device info");
        return false;
    }

    m_outputParams.channelCount = numChannels;
    m_outputParams.sampleFormat = paFloat32;
    m_outputParams.suggestedLatency = deviceInfo->defaultHighOutputLatency;
    m_outputParams.hostApiSpecificStreamInfo = nullptr;

    m_inputParams.device = Pa_GetDefaultInputDevice();
    if (m_inputParams.device != paNoDevice) {
        m_inputParams.channelCount = numChannels;
        m_inputParams.sampleFormat = paFloat32;
        m_inputParams.suggestedLatency = Pa_GetDeviceInfo(m_inputParams.device)->defaultHighInputLatency;
        m_inputParams.hostApiSpecificStreamInfo = nullptr;
    }

    m_initialized = true;
    qDebug() << "AudioIO initialized:" << sampleRate << "Hz," << bufferSize << "samples," << numChannels << "channels";
    emit initialized();
    return true;
}

void AudioIO::shutdown() {
    if (!m_initialized) return;
    stop();
    m_initialized = false;
    emit shutdownComplete();
    qDebug() << "AudioIO shutdown";
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

    PaError err = Pa_OpenStream(&m_audioStream,
                                (m_inputParams.device != paNoDevice) ? &m_inputParams : nullptr,
                                &m_outputParams,
                                m_sampleRate,
                                m_bufferSize,
                                paClipOff,
                                AudioIO::paCallback,
                                this);

    if (err != paNoError) {
        qCritical() << "PortAudio open error:" << Pa_GetErrorText(err);
        emit error(QString("PortAudio open: %1").arg(Pa_GetErrorText(err)));
        return false;
    }

    err = Pa_StartStream(m_audioStream);
    if (err != paNoError) {
        qCritical() << "PortAudio start error:" << Pa_GetErrorText(err);
        emit error(QString("PortAudio start: %1").arg(Pa_GetErrorText(err)));
        Pa_CloseStream(m_audioStream);
        m_audioStream = nullptr;
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
        if (info && info->maxInputChannels > 0) {
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
        if (info && info->maxOutputChannels > 0) {
            devices.append(QString(info->name));
        }
    }
    return devices;
}

bool AudioIO::setInputDevice(const QString& deviceName) {
    if (m_running) {
        emit error("Cannot change device while running");
        return false;
    }

    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && QString(info->name) == deviceName && info->maxInputChannels > 0) {
            m_inputParams.device = i;
            m_currentInputDevice = deviceName;
            emit deviceChanged(deviceName);
            return true;
        }
    }
    return false;
}

bool AudioIO::setOutputDevice(const QString& deviceName) {
    if (m_running) {
        emit error("Cannot change device while running");
        return false;
    }

    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && QString(info->name) == deviceName && info->maxOutputChannels > 0) {
            m_outputParams.device = i;
            m_currentOutputDevice = deviceName;
            emit deviceChanged(deviceName);
            return true;
        }
    }
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

int AudioIO::paCallback(const void* inputBuffer, void* outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void* userData) {
    AudioIO* self = static_cast<AudioIO*>(userData);
    if (!self) return paContinue;

    float* input = (float*)inputBuffer;
    float* output = (float*)outputBuffer;

    // Convert input/output to float arrays
    float* inputChannels[2] = {nullptr, nullptr};
    float* outputChannels[2] = {nullptr, nullptr};

    if (input) {
        inputChannels[0] = input;
        if (self->m_numChannels > 1) {
            inputChannels[1] = input + framesPerBuffer;
        }
    }

    if (output) {
        outputChannels[0] = output;
        if (self->m_numChannels > 1) {
            outputChannels[1] = output + framesPerBuffer;
        }
    }

    // Apply input gain if any
    if (input) {
        for (unsigned long i = 0; i < framesPerBuffer * self->m_numChannels; i++) {
            input[i] *= self->m_inputGain;
        }
    }

    // Call user callback
    if (self->m_callback) {
        self->m_callback(inputChannels, outputChannels, (int)framesPerBuffer);
    } else {
        // Passthrough if no callback
        if (output && input && output != input) {
            memcpy(output, input, framesPerBuffer * self->m_numChannels * sizeof(float));
        }
    }

    // Apply output gain if any
    if (output) {
        for (unsigned long i = 0; i < framesPerBuffer * self->m_numChannels; i++) {
            output[i] *= self->m_outputGain;
        }
    }

    // Check for underruns/overruns
    if (statusFlags & paOutputUnderflow) {
        self->emit bufferUnderrun();
    }
    if (statusFlags & paInputOverflow) {
        self->emit bufferOverrun();
    }

    return paContinue;
}
