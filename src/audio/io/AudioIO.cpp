#include "AudioIO.h"
#include <QDebug>
#include <portaudio.h>

AudioIO::AudioIO(QObject* parent) : QObject(parent) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        m_lastError = QString("PortAudio init error: %1").arg(Pa_GetErrorText(err));
        emit error(m_lastError);
    }
}

AudioIO::~AudioIO() {
    shutdown();
    Pa_Terminate();
}

bool AudioIO::initialize(int sampleRate, int bufferSize, int numChannels) {
    if (m_initialized) return true;
    m_sampleRate = sampleRate;
    m_bufferSize = bufferSize;
    m_numChannels = numChannels;
    m_initialized = true;
    emit initialized();
    return true;
}

void AudioIO::shutdown() {
    if (!m_initialized) return;
    stop();
    m_initialized = false;
    emit shutdownComplete();
}

bool AudioIO::start() {
    if (!m_initialized) {
        m_lastError = "AudioIO not initialized";
        emit error(m_lastError);
        return false;
    }
    if (m_running) return true;

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice) {
        m_lastError = "No audio output device";
        emit error(m_lastError);
        return false;
    }
    outputParams.channelCount = m_numChannels;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultHighOutputLatency;

    PaStreamParameters inputParams;
    inputParams.device = Pa_GetDefaultInputDevice();
    inputParams.channelCount = m_numChannels;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultHighInputLatency;

    PaError err = Pa_OpenStream(&m_audioStream,
                                (inputParams.device != paNoDevice) ? &inputParams : nullptr,
                                &outputParams,
                                m_sampleRate,
                                m_bufferSize,
                                paClipOff,
                                AudioIO::paCallback,
                                this);
    if (err != paNoError) {
        m_lastError = QString("PortAudio open error: %1").arg(Pa_GetErrorText(err));
        emit error(m_lastError);
        return false;
    }

    err = Pa_StartStream(m_audioStream);
    if (err != paNoError) {
        m_lastError = QString("PortAudio start error: %1").arg(Pa_GetErrorText(err));
        emit error(m_lastError);
        Pa_CloseStream(m_audioStream);
        m_audioStream = nullptr;
        return false;
    }

    m_running = true;
    return true;
}

void AudioIO::stop() {
    if (!m_running || !m_audioStream) return;
    Pa_StopStream(m_audioStream);
    Pa_CloseStream(m_audioStream);
    m_audioStream = nullptr;
    m_running = false;
}

QList<QString> AudioIO::getInputDevices() const {
    QList<QString> devices;
    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0) devices.append(QString(info->name));
    }
    return devices;
}

QList<QString> AudioIO::getOutputDevices() const {
    QList<QString> devices;
    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxOutputChannels > 0) devices.append(QString(info->name));
    }
    return devices;
}

bool AudioIO::setInputDevice(const QString& deviceName) {
    if (m_running) stop();
    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && QString(info->name) == deviceName && info->maxInputChannels > 0) {
            m_currentInputDevice = deviceName;
            m_inputParams.device = i;
            if (m_running) start();
            emit deviceChanged(deviceName);
            return true;
        }
    }
    return false;
}

bool AudioIO::setOutputDevice(const QString& deviceName) {
    if (m_running) stop();
    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && QString(info->name) == deviceName && info->maxOutputChannels > 0) {
            m_currentOutputDevice = deviceName;
            m_outputParams.device = i;
            if (m_running) start();
            emit deviceChanged(deviceName);
            return true;
        }
    }
    return false;
}

void AudioIO::setLatency(int input, int output) {
    m_inputLatency = qMax(0, input);
    m_outputLatency = qMax(0, output);
    emit latencyChanged(m_inputLatency, m_outputLatency);
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
        if (self->m_numChannels > 1) inputChannels[1] = input + framesPerBuffer;
    }
    if (output) {
        outputChannels[0] = output;
        if (self->m_numChannels > 1) outputChannels[1] = output + framesPerBuffer;
    }

    if (input) {
        for (unsigned long i = 0; i < framesPerBuffer * self->m_numChannels; i++) {
            input[i] *= self->m_inputGain;
        }
    }

    if (self->m_callback) {
        self->m_callback(inputChannels, outputChannels, (int)framesPerBuffer);
    } else if (output && input && output != input) {
        memcpy(output, input, framesPerBuffer * self->m_numChannels * sizeof(float));
    }

    if (output) {
        for (unsigned long i = 0; i < framesPerBuffer * self->m_numChannels; i++) {
            output[i] *= self->m_outputGain;
        }
    }

    if (statusFlags & paOutputUnderflow) self->emit bufferUnderrun();
    if (statusFlags & paInputOverflow) self->emit bufferOverrun();

    return paContinue;
}
