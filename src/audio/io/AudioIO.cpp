#include "AudioIO.h"
#include <QDebug>
#include <QThread>
#include <cmath>

#include <portaudio.h>

AudioIO::AudioIO(QObject* parent)
    : QObject(parent) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        m_lastError = QString("PortAudio initialization error: %1").arg(Pa_GetErrorText(err));
        qCritical() << m_lastError;
        emit error(m_lastError);
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

    if (Pa_GetDeviceCount() < 0) {
        m_lastError = "No audio devices found";
        emit error(m_lastError);
        return false;
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
        m_lastError = "AudioIO not initialized";
        emit error(m_lastError);
        return false;
    }
    if (m_running) {
        qWarning() << "AudioIO already running";
        return true;
    }

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice) {
        m_lastError = "No audio output device found";
        emit error(m_lastError);
        return false;
    }

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(outputParams.device);
    if (!deviceInfo) {
        m_lastError = "Failed to get device info";
        emit error(m_lastError);
        return false;
    }

    outputParams.channelCount = m_numChannels;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = deviceInfo->defaultHighOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters inputParams;
    PaDeviceIndex inputDevice = Pa_GetDefaultInputDevice();
    if (inputDevice != paNoDevice) {
        const PaDeviceInfo* inputInfo = Pa_GetDeviceInfo(inputDevice);
        if (inputInfo) {
            inputParams.device = inputDevice;
            inputParams.channelCount = m_numChannels;
            inputParams.sampleFormat = paFloat32;
            inputParams.suggestedLatency = inputInfo->defaultHighInputLatency;
            inputParams.hostApiSpecificStreamInfo = nullptr;
        } else {
            inputDevice = paNoDevice;
        }
    }

    PaError err = Pa_OpenStream(&m_audioStream,
                                (inputDevice != paNoDevice) ? &inputParams : nullptr,
                                &outputParams,
                                m_sampleRate,
                                m_bufferSize,
                                paClipOff,
                                AudioIO::paCallback,
                                this);

    if (err != paNoError) {
        m_lastError = QString("PortAudio open error: %1").arg(Pa_GetErrorText(err));
        qCritical() << m_lastError;
        emit error(m_lastError);
        return false;
    }

    err = Pa_StartStream(m_audioStream);
    if (err != paNoError) {
        m_lastError = QString("PortAudio start error: %1").arg(Pa_GetErrorText(err));
        qCritical() << m_lastError;
        emit error(m_lastError);
        Pa_CloseStream(m_audioStream);
        m_audioStream = nullptr;
        return false;
    }

    m_running = true;
    qDebug() << "AudioIO stream started";
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
    qDebug() << "AudioIO stream stopped";
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
        m_lastError = "Cannot change device while running";
        emit error(m_lastError);
        return false;
    }

    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && QString(info->name) == deviceName && info->maxInputChannels > 0) {
            m_currentInputDevice = deviceName;
            emit deviceChanged(deviceName);
            return true;
        }
    }
    m_lastError = "Device not found: " + deviceName;
    return false;
}

bool AudioIO::setOutputDevice(const QString& deviceName) {
    if (m_running) {
        m_lastError = "Cannot change device while running";
        emit error(m_lastError);
        return false;
    }

    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && QString(info->name) == deviceName && info->maxOutputChannels > 0) {
            m_currentOutputDevice = deviceName;
            emit deviceChanged(deviceName);
            return true;
        }
    }
    m_lastError = "Device not found: " + deviceName;
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

    // Apply input gain
    if (input) {
        for (unsigned long i = 0; i < framesPerBuffer * self->m_numChannels; i++) {
            input[i] *= self->m_inputGain;
        }
    }

    // Call user callback (routes to AudioEngine)
    if (self->m_callback) {
        self->m_callback(inputChannels, outputChannels, (int)framesPerBuffer);
    } else if (output && input && output != input) {
        memcpy(output, input, framesPerBuffer * self->m_numChannels * sizeof(float));
    }

    // Apply output gain
    if (output) {
        for (unsigned long i = 0; i < framesPerBuffer * self->m_numChannels; i++) {
            output[i] *= self->m_outputGain;
        }
    }

    // Detect underruns/overruns
    if (statusFlags & paOutputUnderflow) {
        self->emit bufferUnderrun();
    }
    if (statusFlags & paInputOverflow) {
        self->emit bufferOverrun();
    }

    return paContinue;
}
