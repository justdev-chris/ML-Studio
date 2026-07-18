#include "Clip.h"
#include "utils/FileUtils.h"
#include <QDebug>
#include <QFile>
#include <cstring>
#include <cmath>
#include <sndfile.h>

Clip::Clip(QObject* parent)
    : QObject(parent)
    , m_type(ClipType::Audio)
    , m_name("Clip")
    , m_color(QColor(100, 150, 200)) {}

Clip::~Clip() {}

AudioClip::AudioClip(QObject* parent) : Clip(parent) {
    m_type = ClipType::Audio;
    m_name = "Audio Clip";
}

AudioClip::~AudioClip() {
    if (m_audioData) delete[] m_audioData;
}

bool AudioClip::loadFromFile(const QString& filePath) {
    if (filePath.isEmpty()) return false;
    if (!QFile::exists(filePath)) {
        qWarning() << "Audio file not found:" << filePath;
        return false;
    }

    SF_INFO info;
    SNDFILE* file = sf_open(filePath.toUtf8().constData(), SFM_READ, &info);
    if (!file) {
        qWarning() << "Failed to open audio file:" << filePath;
        return false;
    }

    m_filePath = filePath;
    int numFrames = info.frames;
    int numChannels = info.channels;
    m_dataLength = numFrames * numChannels;
    m_audioData = new float[m_dataLength];

    sf_read_float(file, m_audioData, m_dataLength);
    sf_close(file);

    int sampleRate = info.samplerate;
    if (sampleRate > 0) m_length = numFrames;

    qDebug() << "Loaded audio clip:" << filePath << numFrames << "frames," << numChannels << "channels";
    return true;
}

void AudioClip::setGain(float gain) {
    m_gain = qBound(0.0f, gain, 4.0f);
}

void AudioClip::setPitch(float pitch) {
    m_pitch = qBound(0.5f, pitch, 2.0f);
}

void AudioClip::process(float** output, int numFrames) {
    if (!m_active || !m_audioData || m_dataLength == 0) return;

    int samplesToCopy = numFrames;
    if (m_playhead + samplesToCopy > m_dataLength) {
        samplesToCopy = m_dataLength - m_playhead;
    }
    if (samplesToCopy <= 0) return;

    // Pitch shift using linear interpolation
    if (m_pitch != 1.0f) {
        float pitch = m_pitch;
        for (int i = 0; i < samplesToCopy; i++) {
            float index = (m_playhead + i) * pitch;
            int idx0 = (int)index;
            int idx1 = idx0 + 1;
            float frac = index - idx0;
            if (idx1 >= m_dataLength) break;
            float sample = (m_audioData[idx0] * (1 - frac) + m_audioData[idx1] * frac) * m_gain;
            if (output[0]) output[0][i] += sample;
            if (output[1]) output[1][i] += sample;
        }
        m_playhead += samplesToCopy * pitch;
    } else {
        for (int i = 0; i < samplesToCopy; i++) {
            float sample = m_audioData[m_playhead + i] * m_gain;
            if (output[0]) output[0][i] += sample;
            if (output[1]) output[1][i] += sample;
        }
        m_playhead += samplesToCopy;
    }
}

void AudioClip::update(double position) {
    double sampleRate = 44100.0;
    double clipStartTime = m_start / sampleRate;
    double clipEndTime = (m_start + m_length) / sampleRate;

    if (position >= clipStartTime && position < clipEndTime) {
        double offset = position - clipStartTime;
        int newPlayhead = offset * sampleRate;
        if (newPlayhead < m_dataLength) m_playhead = newPlayhead;
        else m_playhead = m_dataLength - 1;
    } else if (position >= clipEndTime) {
        m_playhead = m_dataLength - 1;
    } else {
        m_playhead = 0;
    }
}

void AudioClip::setAudioData(const QVector<float>& data) {
    m_dataLength = data.size();
    if (m_audioData) delete[] m_audioData;
    m_audioData = new float[m_dataLength];
    memcpy(m_audioData, data.data(), m_dataLength * sizeof(float));
}

MIDIClip::MIDIClip(QObject* parent) : Clip(parent) {
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
    Q_UNUSED(output);
    Q_UNUSED(numFrames);
}

void MIDIClip::update(double position) {
    double sampleRate = 44100.0;
    double clipStartTime = m_start / sampleRate;
    double clipEndTime = (m_start + m_length) / sampleRate;

    if (position >= clipStartTime && position < clipEndTime) {
        m_active = true;
        double clipPosition = position - clipStartTime;

        for (const MIDIEvent& event : m_events) {
            double eventTime = event.start / 480.0;
            double eventEndTime = (event.start + event.length) / 480.0;

            if (clipPosition >= eventTime && clipPosition < eventEndTime) {
                if (!m_triggeredEvents.contains(event.note)) {
                    m_triggeredEvents.insert(event.note);
                    emit midiNoteOn(event.note, event.velocity);
                }
            } else if (clipPosition >= eventEndTime) {
                if (m_triggeredEvents.contains(event.note)) {
                    m_triggeredEvents.remove(event.note);
                    emit midiNoteOff(event.note);
                }
            }
        }
    } else if (position >= clipEndTime) {
        for (int note : m_triggeredEvents) {
            emit midiNoteOff(note);
        }
        m_triggeredEvents.clear();
        m_active = false;
    } else {
        m_active = false;
    }
}
