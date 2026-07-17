#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QVector>
#include <QMutex>
#include <atomic>

#include "core/track/Track.h"
#include "core/mixer/Mixer.h"
#include "core/transport/Transport.h"
#include "audio/io/AudioIO.h"
#include "project/Project.h"

class AudioEngine : public QObject {
    Q_OBJECT

public:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine();

    // Initialization
    bool initialize(int sampleRate = 44100, int bufferSize = 256);
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    // Audio I/O
    void setAudioIO(AudioIO* audioIO);
    AudioIO* getAudioIO() const { return m_audioIO; }

    // Transport
    void play();
    void stop();
    void pause();
    void togglePlay();
    void record();
    bool isPlaying() const { return m_transport->isPlaying(); }
    bool isRecording() const { return m_transport->isRecording(); }
    double getCurrentPosition() const { return m_transport->getPosition(); }
    void setPosition(double seconds);
    void setTempo(double bpm);
    double getTempo() const { return m_transport->getTempo(); }

    // Tracks
    Track* addTrack(TrackType type = TrackType::Audio);
    void removeTrack(int index);
    Track* getTrack(int index) const;
    int getTrackCount() const { return m_tracks.size(); }
    QVector<Track*>& getTracks() { return m_tracks; }
    void setTracks(const QVector<Track*>& tracks);

    // Mixer
    Mixer* getMixer() { return m_mixer; }
    void setMasterVolume(float volume);
    float getMasterVolume() const;

    // Project
    void setProject(Project* project);
    Project* getProject() const { return m_project; }
    void loadProject(Project* project);
    void clearProject();

    // Audio processing (called by AudioIO callback)
    void processAudio(float** input, float** output, int numChannels, int numFrames);

signals:
    void trackAdded(Track* track);
    void trackRemoved(int index);
    void trackMoved(int from, int to);
    void playheadMoved(double seconds);
    void transportStateChanged(bool playing);
    void recordingStateChanged(bool recording);
    void positionChanged(double seconds);
    void tempoChanged(double bpm);
    void projectLoaded();
    void projectCleared();
    void error(const QString& message);

private slots:
    void onTransportPositionChanged(double seconds);
    void onTransportPlayStateChanged(bool playing);
    void onTransportRecordStateChanged(bool recording);
    void onTransportTempoChanged(double bpm);

private:
    bool m_initialized = false;
    int m_sampleRate = 44100;
    int m_bufferSize = 256;

    Transport* m_transport;
    Mixer* m_mixer;
    AudioIO* m_audioIO = nullptr;
    Project* m_project = nullptr;

    QVector<Track*> m_tracks;
    mutable QMutex m_mutex;

    void setupDefaultTracks();
    void connectTransportSignals();
    void clearTracks();
};

#endif // AUDIOENGINE_H