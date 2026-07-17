#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <QObject>
#include <QElapsedTimer>

class Transport : public QObject {
    Q_OBJECT

public:
    explicit Transport(QObject* parent = nullptr);
    ~Transport();

    // Transport control
    void play();
    void stop();
    void pause();
    void record();
    void togglePlay();

    // State getters
    bool isPlaying() const { return m_playing; }
    bool isRecording() const { return m_recording; }
    bool isPaused() const { return m_paused; }

    // Position
    double getPosition() const { return m_position; }
    void setPosition(double seconds);

    // Tempo
    double getTempo() const { return m_tempo; }
    void setTempo(double bpm);

    // Time signature
    void setTimeSignature(int beats, int noteValue);
    int getBeats() const { return m_beats; }
    int getNoteValue() const { return m_noteValue; }

    // Loop
    bool isLooping() const { return m_looping; }
    void setLooping(bool looping);
    void setLoopRange(double start, double end);
    double getLoopStart() const { return m_loopStart; }
    double getLoopEnd() const { return m_loopEnd; }

    // Tempo automation
    void setTempoAt(double time, double bpm);
    double getTempoAt(double time) const;

signals:
    void playStateChanged(bool playing);
    void recordStateChanged(bool recording);
    void positionChanged(double seconds);
    void tempoChanged(double bpm);
    void loopStateChanged(bool looping);
    void loopRangeChanged(double start, double end);
    void tick();  // Called every sample? No, every buffer

public slots:
    void updatePosition();

private:
    bool m_playing = false;
    bool m_recording = false;
    bool m_paused = false;
    bool m_looping = false;

    double m_position = 0.0;
    double m_tempo = 120.0;
    int m_beats = 4;
    int m_noteValue = 4;

    double m_loopStart = 0.0;
    double m_loopEnd = 4.0; // 4 beats

    double m_tickInterval = 1.0 / 96.0; // 96 ticks per beat
    int m_currentTick = 0;

    QElapsedTimer m_timer;
    double m_timerOffset = 0.0;

    void updateTimer();
};

#endif // TRANSPORT_H