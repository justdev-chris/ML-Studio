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

void Mixer::processSends(Track* track, float** output, int numFrames, const QVector<Track*>& auxTracks) {
    if (!track || auxTracks.isEmpty()) return;

    // Get send levels from track (would be stored in track)
    // For now, we'll use a default send level
    // In a full implementation, each track would have a send level per aux track
    float sendLevel = 0.3f; // Default send level

    // Process each aux track
    for (Track* aux : auxTracks) {
        if (!aux) continue;

        // Allocate temporary buffer for send
        float* sendBuffer[2] = {nullptr, nullptr};
        sendBuffer[0] = new float[numFrames];
        sendBuffer[1] = new float[numFrames];
        memset(sendBuffer[0], 0, numFrames * sizeof(float));
        memset(sendBuffer[1], 0, numFrames * sizeof(float));

        // Copy track output to send buffer
        if (output[0]) memcpy(sendBuffer[0], output[0], numFrames * sizeof(float));
        if (output[1]) memcpy(sendBuffer[1], output[1], numFrames * sizeof(float));

        // Apply send level
        for (int i = 0; i < numFrames; i++) {
            if (sendBuffer[0]) sendBuffer[0][i] *= sendLevel;
            if (sendBuffer[1]) sendBuffer[1][i] *= sendLevel;
        }

        // Process the aux track with the send buffer
        // In a real implementation, the aux track would process the send buffer
        // and mix it back into the output or its own output
        // For now, we'll just add it to the output
        if (output[0] && sendBuffer[0]) {
            for (int i = 0; i < numFrames; i++) {
                output[0][i] += sendBuffer[0][i];
            }
        }
        if (output[1] && sendBuffer[1]) {
            for (int i = 0; i < numFrames; i++) {
                output[1][i] += sendBuffer[1][i];
            }
        }

        delete[] sendBuffer[0];
        delete[] sendBuffer[1];
    }
}
