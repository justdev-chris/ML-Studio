#ifndef TRACK_H
#define TRACK_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QVector>
#include <QMutex>

#include "core/track/Clip.h"

enum class TrackType {
    Audio,
    MIDI,
    Aux
};

class Track : public QObject {
    Q_OBJECT

public:
    explicit Track(TrackType type, int sampleRate, QObject* parent = nullptr);
    ~Track();

    // Basic properties
    QString getName() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    TrackType getType() const { return m_type; }
    QColor getColor() const { return m_color; }
    void setColor(const QColor& color) { m_color = color; }

    // Volume and pan
    float getVolume() const { return m_volume; }
    void setVolume(float volume);
    float getPan() const { return m_pan; }
    void setPan(float pan);

    // Mute / Solo
    bool isMuted() const { return m_muted; }
    void setMuted(bool muted);
    bool isSoloed() const { return m_soloed; }
    void setSoloed(bool soloed);

    // Record arm
    bool isRecordArmed() const { return m_recordArm; }
    void setRecordArmed(bool armed);

    // Clips
    void addClip(Clip* clip);
    void removeClip(int index);
    Clip* getClip(int index) const;
    int getClipCount() const { return m_clips.size(); }
    QVector<Clip*>& getClips() { return m_clips; }

    // Processing
    void process(float** output, int numFrames);
    void updateClips(double position);

    // MIDI specific
    void sendMIDIEvent(const MIDIEvent& event);
    QVector<MIDIEvent> getMIDIEvents() const;

signals:
    void volumeChanged(float volume);
    void panChanged(float pan);
    void muteToggled(bool muted);
    void soloToggled(bool soloed);
    void clipAdded(Clip* clip);
    void clipRemoved(int index);
    void nameChanged(const QString& name);

private:
    TrackType m_type;
    QString m_name;
    QColor m_color;
    int m_sampleRate;

    float m_volume = 0.8f;
    float m_pan = 0.0f;
    bool m_muted = false;
    bool m_soloed = false;
    bool m_recordArm = false;

    QVector<Clip*> m_clips;
    mutable QMutex m_mutex;
};

#endif // TRACK_H