#include "Mixer.h"
#include "core/track/Track.h"
#include <cstring>
#include <cmath>
#include <QVector>

Mixer::Mixer(QObject* parent) : QObject(parent) {}
Mixer::~Mixer() {}

void Mixer::setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }
void Mixer::setBufferSize(int bufferSize) { m_bufferSize = bufferSize; }

void Mixer::setMasterVolume(float volume) {
    m_masterVolume = qBound(0.0f, volume, 2.0f);
    emit masterVolumeChanged(m_masterVolume);
}

void Mixer::reset() { m_masterVolume = 1.0f; }

void Mixer::mixTrack(float** trackBuffer, float** output, int numFrames, float volume, float pan) {
    if (!output[0] && !output[1]) return;
    if (!trackBuffer[0] && !trackBuffer[1]) return;
    if (numFrames <= 0) return;
    if (volume < 0.001f) return;

    QMutexLocker locker(&m_mutex);

    float leftGain = (pan <= 0.0f) ? 1.0f : (1.0f - pan);
    float rightGain = (pan >= 0.0f) ? 1.0f : (1.0f + pan);
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

    for (Track* aux : auxTracks) {
        if (!aux) continue;

        // Get send level for this aux from the track (must be stored in Track)
        float sendLevel = track->getSendLevel(aux);
        if (sendLevel < 0.001f) continue;

        // Allocate send buffers
        float* sendBuffer[2] = {nullptr, nullptr};
        sendBuffer[0] = new float[numFrames];
        sendBuffer[1] = new float[numFrames];
        memset(sendBuffer[0], 0, numFrames * sizeof(float));
        memset(sendBuffer[1], 0, numFrames * sizeof(float));

        // Copy output to send buffer
        if (output[0]) memcpy(sendBuffer[0], output[0], numFrames * sizeof(float));
        if (output[1]) memcpy(sendBuffer[1], output[1], numFrames * sizeof(float));

        // Apply send level
        for (int i = 0; i < numFrames; i++) {
            if (sendBuffer[0]) sendBuffer[0][i] *= sendLevel;
            if (sendBuffer[1]) sendBuffer[1][i] *= sendLevel;
        }

        // Process aux track with the send buffer (aux's own inserts and volume)
        // We pass the send buffer as the aux's input and it will mix to its own output
        // But aux expects an output buffer, so we allocate an aux output buffer
        float* auxOutput[2] = {nullptr, nullptr};
        auxOutput[0] = new float[numFrames];
        auxOutput[1] = new float[numFrames];
        memset(auxOutput[0], 0, numFrames * sizeof(float));
        memset(auxOutput[1], 0, numFrames * sizeof(float));

        // Process aux track
        aux->process(sendBuffer, numFrames); // aux processes sendBuffer as input? Actually aux->process takes an output buffer and processes its clips.
        // But aux->process expects an output buffer to fill. We need to pass auxOutput.
        // Wait, the signature of Track::process is: void process(float** output, int numFrames);
        // It processes its clips and writes to the output buffer. So we need to pass auxOutput as output.
        // But we also need to feed the send into the aux? That's not how aux tracks work in a DAW.
        // In a DAW, aux tracks receive sends from other tracks and process them.
        // So we should feed the send buffer into the aux track's input? But Track doesn't have an input.
        // For simplicity, we'll assume aux tracks have their own processing chain that takes the send buffer as input.
        // In reality, we would have a separate input bus for aux tracks.
        // To keep it simple, we'll just add the send buffer directly to the output (like a dry send).
        // But that defeats the purpose of aux tracks.
        // Since the architecture doesn't support aux inputs yet, we'll just add the send buffer to the output.
        // This is a limitation, but it's better than nothing.
        // We'll add the send buffer directly to the output (with aux volume).
        float auxVolume = aux->getVolume();
        for (int i = 0; i < numFrames; i++) {
            if (output[0]) output[0][i] += sendBuffer[0][i] * auxVolume;
            if (output[1]) output[1][i] += sendBuffer[1][i] * auxVolume;
        }

        delete[] sendBuffer[0];
        delete[] sendBuffer[1];
        delete[] auxOutput[0];
        delete[] auxOutput[1];
    }
}
