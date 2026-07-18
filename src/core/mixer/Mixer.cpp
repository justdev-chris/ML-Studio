#include "Mixer.h"
#include <cstring>
#include <cmath>

Mixer::Mixer(QObject* parent)
    : QObject(parent) {}

Mixer::~Mixer() {}

void Mixer::setSampleRate(int sampleRate) {
    m_sampleRate = sampleRate;
}

void Mixer::setBufferSize(int bufferSize) {
    m_bufferSize = bufferSize;
}

void Mixer::setMasterVolume(float volume) {
    m_masterVolume = qBound(0.0f, volume, 2.0f);
    emit masterVolumeChanged(m_masterVolume);
}

void Mixer::reset() {
    m_masterVolume = 1.0f;
}

void Mixer::mixTrack(float** trackBuffer, float** output, int numFrames, float volume, float pan) {
    if (!output[0] && !output[1]) return;
    if (!trackBuffer[0] && !trackBuffer[1]) return;
    if (numFrames <= 0) return;
    if (volume < 0.001f) return;

    QMutexLocker locker(&m_mutex);

    // Apply pan law (constant power)
    float leftGain = (pan <= 0.0f) ? 1.0f : (1.0f - pan);
    float rightGain = (pan >= 0.0f) ? 1.0f : (1.0f + pan);

    // Apply track volume and master volume
    leftGain *= volume * m_masterVolume;
    rightGain *= volume * m_masterVolume;

    for (int i = 0; i < numFrames; i++) {
        float leftSample = trackBuffer[0] ? trackBuffer[0][i] : 0.0f;
        float rightSample = trackBuffer[1] ? trackBuffer[1][i] : leftSample;

        if (output[0]) output[0][i] += leftSample * leftGain;
        if (output[1]) output[1][i] += rightSample * rightGain;
    }
}
