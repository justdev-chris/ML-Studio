#include "Track.h"
#include <QDebug>

Track::Track(TrackType type, int sampleRate, QObject* parent)
    : QObject(parent), m_type(type), m_name("Track"), m_color(QColor(100, 100, 120)), m_sampleRate(sampleRate) {
    m_inserts.reserve(8);
}

Track::~Track() {
    for (Clip* clip : m_clips) delete clip;
    for (FX* insert : m_inserts) delete insert;
    m_clips.clear();
    m_inserts.clear();
}

void Track::setVolume(float volume) {
    m_volume = qBound(0.0f, volume, 2.0f);
    emit volumeChanged(m_volume);
}

void Track::setPan(float pan) {
    m_pan = qBound(-1.0f, pan, 1.0f);
    emit panChanged(m_pan);
}

void Track::setMuted(bool muted) {
    m_muted = muted;
    emit muteToggled(muted);
}

void Track::setSoloed(bool soloed) {
    m_soloed = soloed;
    emit soloToggled(soloed);
}

void Track::setRecordArmed(bool armed) {
    m_recordArm = armed;
}

void Track::addClip(Clip* clip) {
    if (!clip) return;
    m_clips.append(clip);
    emit clipAdded(clip);
}

void Track::removeClip(int index) {
    if (index < 0 || index >= m_clips.size()) return;
    Clip* clip = m_clips.at(index);
    m_clips.removeAt(index);
    delete clip;
    emit clipRemoved(index);
}

Clip* Track::getClip(int index) const {
    if (index < 0 || index >= m_clips.size()) return nullptr;
    return m_clips.at(index);
}

void Track::addInsert(FX* insert) {
    if (!insert) return;
    insert->setSampleRate(m_sampleRate);
    m_inserts.append(insert);
}

void Track::removeInsert(int index) {
    if (index < 0 || index >= m_inserts.size()) return;
    FX* insert = m_inserts.at(index);
    m_inserts.removeAt(index);
    delete insert;
}

void Track::process(float** output, int numFrames) {
    if (m_muted || numFrames <= 0) return;
    QMutexLocker locker(&m_mutex);

    float* buffer[2] = {nullptr, nullptr};
    buffer[0] = new float[numFrames];
    buffer[1] = new float[numFrames];
    memset(buffer[0], 0, numFrames * sizeof(float));
    memset(buffer[1], 0, numFrames * sizeof(float));

    for (Clip* clip : m_clips) {
        if (clip->isActive()) {
            clip->process(buffer, numFrames);
        }
    }

    // Apply inserts
    if (!m_inserts.isEmpty()) {
        float* processed[2] = {nullptr, nullptr};
        processed[0] = new float[numFrames];
        processed[1] = new float[numFrames];
        memcpy(processed[0], buffer[0], numFrames * sizeof(float));
        memcpy(processed[1], buffer[1], numFrames * sizeof(float));
        for (FX* insert : m_inserts) {
            insert->process(processed, processed, 2, numFrames);
        }
        memcpy(output[0], processed[0], numFrames * sizeof(float));
        memcpy(output[1], processed[1], numFrames * sizeof(float));
        delete[] processed[0];
        delete[] processed[1];
    } else {
        memcpy(output[0], buffer[0], numFrames * sizeof(float));
        memcpy(output[1], buffer[1], numFrames * sizeof(float));
    }

    delete[] buffer[0];
    delete[] buffer[1];
}

void Track::updateClips(double position) {
    QMutexLocker locker(&m_mutex);
    for (Clip* clip : m_clips) {
        clip->update(position);
    }
}
