#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QVector>
#include <QMutex>
#include <QThread>
#include <atomic>

#include "core/track/Track.h"
#include "core/mixer/Mixer.h"
#include "core/transport/Transport.h"

class AudioEngine : public QObject {
    Q_OBJECT

public:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine();

    bool initialize(int sampleRate = 44100, int bufferSize = 256);
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    // Transport control
    void play();
    void stop();
    void pause();
    void togglePlay();
    void record();
    bool isPlaying() const { return m_transport->isPlaying(); }
    bool isRecording() const { return m_transport->isRecording(); }
    double getCurrentPosition() const { return m_transport->getPosition(); }
    void setPosition(double seconds);
    void setTempo(double bpm) { m_transport->setTempo(bpm); }
    double getTempo() const { return m_transport->getTempo(); }

    // Tracks
    Track* addTrack(TrackType type = TrackType::Audio);
    void removeTrack(int index);
    Track* getTrack(int index) const;
    int getTrackCount() const { return m_tracks.size(); }
    QVector<Track*>& getTracks() { return m_tracks; }

    // Mixer
    Mixer* getMixer() { return m_mixer; }
    void setMasterVolume(float volume);
    float getMasterVolume() const;

    // Audio I/O
    void setAudioDevice(const QString& device);
    QStringList getAudioDevices() const;

signals:
    void trackAdded(Track* track);
    void trackRemoved(int index);
    void trackMoved(int from, int to);
    void playheadMoved(double seconds);
    void transportStateChanged(bool playing);
    void recordingStateChanged(bool recording);
    void positionChanged(double seconds);
    void tempoChanged(double bpm);

public slots:
    void processAudio(float** output, int numFrames);
    void updatePlayhead();

private:
    bool m_initialized = false;
    int m_sampleRate = 44100;
    int m_bufferSize = 256;
    double m_currentTime = 0.0;

    Transport* m_transport;
    Mixer* m_mixer;
    QVector<Track*> m_tracks;

    mutable QMutex m_mutex;
    QTimer* m_timer;

    void setupDefaultTracks();
};

#endif // AUDIOENGINE_H