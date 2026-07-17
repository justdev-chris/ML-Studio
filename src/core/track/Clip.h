#ifndef CLIP_H
#define CLIP_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QColor>

enum class ClipType {
    Audio,
    MIDI
};

struct MIDIEvent {
    int note = 60;
    int velocity = 100;
    int start = 0;      // in ticks or samples
    int length = 480;   // in ticks or samples
    int channel = 0;
};

class Clip : public QObject {
    Q_OBJECT

public:
    explicit Clip(QObject* parent = nullptr);
    virtual ~Clip();

    ClipType getType() const { return m_type; }
    QString getName() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    int getStart() const { return m_start; }
    void setStart(int start) { m_start = start; }

    int getLength() const { return m_length; }
    void setLength(int length) { m_length = length; }

    QColor getColor() const { return m_color; }
    void setColor(const QColor& color) { m_color = color; }

    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    virtual void process(float** output, int numFrames) = 0;
    virtual void update(double position) = 0;

protected:
    ClipType m_type;
    QString m_name;
    int m_start = 0;
    int m_length = 44100; // 1 second at 44.1kHz
    QColor m_color;
    bool m_active = true;
};

class AudioClip : public Clip {
    Q_OBJECT

public:
    explicit AudioClip(QObject* parent = nullptr);
    ~AudioClip();

    bool loadFromFile(const QString& filePath);
    const QString& getFilePath() const { return m_filePath; }

    float getGain() const { return m_gain; }
    void setGain(float gain);

    float getPitch() const { return m_pitch; }
    void setPitch(float pitch);

    void process(float** output, int numFrames) override;
    void update(double position) override;

private:
    QString m_filePath;
    float* m_audioData = nullptr;
    int m_dataLength = 0;
    float m_gain = 1.0f;
    float m_pitch = 1.0f;
    int m_playhead = 0;
};

class MIDIClip : public Clip {
    Q_OBJECT

public:
    explicit MIDIClip(QObject* parent = nullptr);

    void addEvent(const MIDIEvent& event);
    void removeEvent(int index);
    void clearEvents();
    const QVector<MIDIEvent>& getEvents() const { return m_events; }

    void process(float** output, int numFrames) override;
    void update(double position) override;

    // MIDI output
    void setMIDIOutput(void* output) { m_midiOutput = output; }

signals:
    void eventAdded(const MIDIEvent& event);
    void eventRemoved(int index);

private:
    QVector<MIDIEvent> m_events;
    void* m_midiOutput = nullptr;
    int m_currentEventIndex = 0;
};

#endif // CLIP_H