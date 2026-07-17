#include "Track.h"
#include <QDebug>

Track::Track(TrackType type, int sampleRate, QObject* parent)
    : QObject(parent)
    , m_type(type)
    , m_name("Track")
    , m_color(QColor(100, 100, 120))
    , m_sampleRate(sampleRate) {
}

Track::~Track() {
    for (Clip* clip : m_clips) {
        delete clip;
    }
    m_clips.clear();
}

void Track::setVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 2.0f) volume = 2.0f;
    if (m_volume != volume) {
        m_volume = volume;
        emit volumeChanged(volume);
    }
}

void Track::setPan(float pan) {
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    if (m_pan != pan) {
        m_pan = pan;
        emit panChanged(pan);
    }
}

void Track::setMuted(bool muted) {
    if (m_muted != muted) {
        m_muted = muted;
        emit muteToggled(muted);
    }
}

void Track::setSoloed(bool soloed) {
    if (m_soloed != soloed) {
        m_soloed = soloed;
        emit soloToggled(soloed);
    }
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

void Track::process(float** output, int numFrames) {
    if (m_muted) return;
    if (numFrames <= 0) return;

    QMutexLocker locker(&m_mutex);

    // Allocate temporary buffer for this track
    float* buffer[2] = { nullptr, nullptr };
    buffer[0] = new float[numFrames];
    buffer[1] = new float[numFrames];
    memset(buffer[0], 0, numFrames * sizeof(float));
    memset(buffer[1], 0, numFrames * sizeof(float));

    // Process each clip
    for (Clip* clip : m_clips) {
        if (clip->isActive()) {
            clip->process(buffer, numFrames);
        }
    }

    // Copy to output
    if (output[0] && buffer[0]) {
        memcpy(output[0], buffer[0], numFrames * sizeof(float));
    }
    if (output[1] && buffer[1]) {
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

void Track::sendMIDIEvent(const MIDIEvent& event) {
    if (m_type != TrackType::MIDI) return;
    // Forward to MIDI clips or external synth
    Q_UNUSED(event);
}

QVector<MIDIEvent> Track::getMIDIEvents() const {
    QVector<MIDIEvent> events;
    if (m_type != TrackType::MIDI) return events;

    for (Clip* clip : m_clips) {
        if (clip->getType() == ClipType::MIDI) {
            MIDIClip* midiClip = static_cast<MIDIClip*>(clip);
            events.append(midiClip->getEvents());
        }
    }
    return events;
}