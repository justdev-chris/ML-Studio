#include "Clip.h"
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <cmath>

// ----------------------------------------------------------------------------
// Clip base class
// ----------------------------------------------------------------------------

Clip::Clip(QObject* parent)
    : QObject(parent)
    , m_type(ClipType::Audio)
    , m_name("Clip")
    , m_color(QColor(100, 150, 200)) {
}

Clip::~Clip() {}

// ----------------------------------------------------------------------------
// AudioClip
// ----------------------------------------------------------------------------

AudioClip::AudioClip(QObject* parent)
    : Clip(parent) {
    m_type = ClipType::Audio;
    m_name = "Audio Clip";
}

AudioClip::~AudioClip() {
    if (m_audioData) {
        delete[] m_audioData;
        m_audioData = nullptr;
    }
}

bool AudioClip::loadFromFile(const QString& filePath) {
    // TODO: Implement audio file loading with libsndfile
    Q_UNUSED(filePath);
    return false;
}

void AudioClip::setGain(float gain) {
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 4.0f) gain = 4.0f;
    m_gain = gain;
}

void AudioClip::setPitch(float pitch) {
    if (pitch < 0.5f) pitch = 0.5f;
    if (pitch > 2.0f) pitch = 2.0f;
    m_pitch = pitch;
}

void AudioClip::process(float** output, int numFrames) {
    if (!m_active || !m_audioData || m_dataLength == 0) return;
    if (!output[0] && !output[1]) return;

    int samplesToCopy = numFrames;
    if (m_playhead + samplesToCopy > m_dataLength) {
        samplesToCopy = m_dataLength - m_playhead;
    }

    if (samplesToCopy <= 0) return;

    for (int i = 0; i < samplesToCopy; i++) {
        float sample = m_audioData[m_playhead + i] * m_gain;
        if (output[0]) output[0][i] += sample;
        if (output[1]) output[1][i] += sample;
    }

    m_playhead += samplesToCopy;
}

void AudioClip::update(double position) {
    // TODO: Update playhead based on transport position
    Q_UNUSED(position);
}

// ----------------------------------------------------------------------------
// MIDIClip
// ----------------------------------------------------------------------------

MIDIClip::MIDIClip(QObject* parent)
    : Clip(parent) {
    m_type = ClipType::MIDI;
    m_name = "MIDI Clip";
}

void MIDIClip::addEvent(const MIDIEvent& event) {
    m_events.append(event);
    emit eventAdded(event);
}

void MIDIClip::removeEvent(int index) {
    if (index < 0 || index >= m_events.size()) return;
    m_events.removeAt(index);
    emit eventRemoved(index);
}

void MIDIClip::clearEvents() {
    m_events.clear();
}

void MIDIClip::process(float** output, int numFrames) {
    // MIDI clips don't produce audio directly
    // They send MIDI events to the synthesizer
    Q_UNUSED(output);
    Q_UNUSED(numFrames);
}

void MIDIClip::update(double position) {
    // TODO: Trigger MIDI events based on playhead position
    Q_UNUSED(position);
}