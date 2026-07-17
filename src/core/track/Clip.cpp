#include "Clip.h"
#include "utils/FileUtils.h"
#include <QDebug>
#include <QFile>
#include <cstring>
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
    if (filePath.isEmpty()) return false;

    // Check if file exists
    if (!FileUtils::exists(filePath)) {
        qWarning() << "Audio file not found:" << filePath;
        return false;
    }

    m_filePath = filePath;

    // Use libsndfile to load audio
    SF_INFO info;
    SNDFILE* file = sf_open(filePath.toUtf8().constData(), SFM_READ, &info);
    if (!file) {
        qWarning() << "Failed to open audio file:" << filePath;
        return false;
    }

    // Allocate buffer
    int numFrames = info.frames;
    int numChannels = info.channels;
    m_dataLength = numFrames * numChannels;
    m_audioData = new float[m_dataLength];

    // Read audio data
    sf_read_float(file, m_audioData, m_dataLength);
    sf_close(file);

    qDebug() << "Loaded audio clip:" << filePath << numFrames << "frames," << numChannels << "channels";

    // Update clip length based on audio duration
    int sampleRate = info.samplerate;
    if (sampleRate > 0) {
        m_length = numFrames;
    }

    return true;
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

    // Calculate how many samples to process
    int samplesToCopy = numFrames;
    if (m_playhead + samplesToCopy > m_dataLength) {
        samplesToCopy = m_dataLength - m_playhead;
    }

    if (samplesToCopy <= 0) return;

    // Copy audio data to output buffer with gain applied
    for (int i = 0; i < samplesToCopy; i++) {
        float sample = m_audioData[m_playhead + i] * m_gain;

        // Apply pitch shift (simple resampling - not implemented yet)
        if (m_pitch != 1.0f) {
            // TODO: Implement pitch shifting
        }

        if (output[0]) output[0][i] += sample;
        if (output[1]) output[1][i] += sample;
    }

    m_playhead += samplesToCopy;
}

void AudioClip::update(double position) {
    // Update playhead based on transport position
    // The clip's start position determines where it plays from
    // position is in seconds, m_start is in samples
    if (m_active) {
        double sampleRate = 44100.0; // Should come from engine
        double clipStartTime = static_cast<double>(m_start) / sampleRate;
        double clipEndTime = static_cast<double>(m_start + m_length) / sampleRate;

        if (position >= clipStartTime && position < clipEndTime) {
            double offset = position - clipStartTime;
            int newPlayhead = static_cast<int>(offset * sampleRate);
            if (newPlayhead < m_dataLength) {
                m_playhead = newPlayhead;
            } else {
                m_playhead = m_dataLength - 1;
            }
        } else if (position >= clipEndTime) {
            m_playhead = m_dataLength - 1;
        } else {
            m_playhead = 0;
        }
    }
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
    // This is handled by the MIDI output system
    Q_UNUSED(output);
    Q_UNUSED(numFrames);
}

void MIDIClip::update(double position) {
    // Check if position is within clip range
    double sampleRate = 44100.0; // Should come from engine
    double clipStartTime = static_cast<double>(m_start) / sampleRate;
    double clipEndTime = static_cast<double>(m_start + m_length) / sampleRate;

    if (position >= clipStartTime && position < clipEndTime) {
        m_active = true;
        // Trigger MIDI events based on position
        // Events are stored in ticks, need to convert to time
        double clipPosition = position - clipStartTime;

        // Check each event
        for (const MIDIEvent& event : m_events) {
            double eventTime = static_cast<double>(event.start) / 480.0; // 480 ticks per quarter note
            double eventEndTime = static_cast<double>(event.start + event.length) / 480.0;

            if (clipPosition >= eventTime && clipPosition < eventEndTime) {
                // Note should be playing
                // Send MIDI note on if not already triggered
                if (!m_triggeredEvents.contains(event.note)) {
                    m_triggeredEvents.insert(event.note);
                    emit midiNoteOn(event.note, event.velocity);
                }
            } else if (clipPosition >= eventEndTime) {
                // Note should be off
                if (m_triggeredEvents.contains(event.note)) {
                    m_triggeredEvents.remove(event.note);
                    emit midiNoteOff(event.note);
                }
            }
        }
    } else if (position >= clipEndTime) {
        // Clip ended, turn off all notes
        for (int note : m_triggeredEvents) {
            emit midiNoteOff(note);
        }
        m_triggeredEvents.clear();
        m_active = false;
    } else {
        m_active = false;
    }
}
