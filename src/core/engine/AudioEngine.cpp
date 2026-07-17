#include "AudioEngine.h"
#include <QDebug>
#include <QTimer>
#include <cmath>

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
    , m_transport(new Transport(this))
    , m_mixer(new Mixer(this)) {
    connectTransportSignals();
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize(int sampleRate, int bufferSize) {
    if (m_initialized) {
        qWarning() << "AudioEngine already initialized";
        return true;
    }

    m_sampleRate = sampleRate;
    m_bufferSize = bufferSize;

    m_mixer->setSampleRate(sampleRate);
    m_mixer->setBufferSize(bufferSize);

    // Set up audio I/O if available
    if (m_audioIO) {
        m_audioIO->initialize(sampleRate, bufferSize, 2);
        m_audioIO->setCallback([this](float** input, float** output, int numFrames) {
            this->processAudio(input, output, 2, numFrames);
        });
    }

    // Default tracks
    setupDefaultTracks();

    m_initialized = true;
    qDebug() << "AudioEngine initialized:" << sampleRate << "Hz," << bufferSize << "samples";
    return true;
}

void AudioEngine::shutdown() {
    if (!m_initialized) return;

    if (m_audioIO) {
        m_audioIO->shutdown();
    }

    clearTracks();
    m_mixer->reset();
    m_initialized = false;
    qDebug() << "AudioEngine shutdown";
}

void AudioEngine::setAudioIO(AudioIO* audioIO) {
    m_audioIO = audioIO;
    if (m_initialized && m_audioIO) {
        m_audioIO->initialize(m_sampleRate, m_bufferSize, 2);
        m_audioIO->setCallback([this](float** input, float** output, int numFrames) {
            this->processAudio(input, output, 2, numFrames);
        });
    }
}

void AudioEngine::play() {
    if (!m_initialized) return;
    m_transport->play();
    if (m_audioIO && !m_audioIO->isRunning()) {
        m_audioIO->start();
    }
    emit transportStateChanged(true);
}

void AudioEngine::stop() {
    if (!m_initialized) return;
    m_transport->stop();
    if (m_audioIO && m_audioIO->isRunning()) {
        m_audioIO->stop();
    }
    emit transportStateChanged(false);
}

void AudioEngine::pause() {
    if (!m_initialized) return;
    m_transport->pause();
    emit transportStateChanged(false);
}

void AudioEngine::togglePlay() {
    if (m_transport->isPlaying()) {
        pause();
    } else {
        play();
    }
}

void AudioEngine::record() {
    if (!m_initialized) return;
    m_transport->record();
    if (m_transport->isRecording() && m_audioIO && !m_audioIO->isRunning()) {
        m_audioIO->start();
    }
    emit recordingStateChanged(m_transport->isRecording());
}

void AudioEngine::setPosition(double seconds) {
    if (!m_initialized) return;
    m_transport->setPosition(seconds);
}

void AudioEngine::setTempo(double bpm) {
    if (!m_initialized) return;
    m_transport->setTempo(bpm);
}

Track* AudioEngine::addTrack(TrackType type) {
    Track* track = new Track(type, m_sampleRate, this);
    track->setName(QString("Track %1").arg(m_tracks.size() + 1));
    m_tracks.append(track);
    emit trackAdded(track);
    return track;
}

void AudioEngine::removeTrack(int index) {
    if (index < 0 || index >= m_tracks.size()) return;
    Track* track = m_tracks.at(index);
    m_tracks.removeAt(index);
    delete track;
    emit trackRemoved(index);
}

Track* AudioEngine::getTrack(int index) const {
    if (index < 0 || index >= m_tracks.size()) return nullptr;
    return m_tracks.at(index);
}

void AudioEngine::setTracks(const QVector<Track*>& tracks) {
    clearTracks();
    m_tracks = tracks;
    for (Track* track : m_tracks) {
        track->setParent(this);
        emit trackAdded(track);
    }
}

void AudioEngine::setMasterVolume(float volume) {
    m_mixer->setMasterVolume(volume);
}

float AudioEngine::getMasterVolume() const {
    return m_mixer->getMasterVolume();
}

void AudioEngine::setProject(Project* project) {
    m_project = project;
    if (m_project) {
        loadProject(m_project);
    }
}

void AudioEngine::loadProject(Project* project) {
    if (!project) return;

    clearTracks();

    // Copy tracks from project
    for (Track* track : project->getTracks()) {
        Track* newTrack = new Track(track->getType(), m_sampleRate, this);
        newTrack->setName(track->getName());
        newTrack->setColor(track->getColor());
        newTrack->setVolume(track->getVolume());
        newTrack->setPan(track->getPan());
        newTrack->setMuted(track->isMuted());
        newTrack->setSoloed(track->isSoloed());
        newTrack->setRecordArmed(track->isRecordArmed());

        // Copy clips
        for (int i = 0; i < track->getClipCount(); i++) {
            Clip* clip = track->getClip(i);
            if (clip->getType() == ClipType::Audio) {
                AudioClip* audioClip = dynamic_cast<AudioClip*>(clip);
                if (audioClip) {
                    AudioClip* newClip = new AudioClip(newTrack);
                    newClip->setName(audioClip->getName());
                    newClip->setStart(audioClip->getStart());
                    newClip->setLength(audioClip->getLength());
                    newClip->setGain(audioClip->getGain());
                    newClip->setPitch(audioClip->getPitch());
                    // Load audio file
                    newClip->loadFromFile(audioClip->getFilePath());
                    newTrack->addClip(newClip);
                }
            } else if (clip->getType() == ClipType::MIDI) {
                MIDIClip* midiClip = dynamic_cast<MIDIClip*>(clip);
                if (midiClip) {
                    MIDIClip* newClip = new MIDIClip(newTrack);
                    newClip->setName(midiClip->getName());
                    newClip->setStart(midiClip->getStart());
                    newClip->setLength(midiClip->getLength());
                    for (const MIDIEvent& event : midiClip->getEvents()) {
                        newClip->addEvent(event);
                    }
                    newTrack->addClip(newClip);
                }
            }
        }

        m_tracks.append(newTrack);
        emit trackAdded(newTrack);
    }

    m_project = project;
    emit projectLoaded();
}

void AudioEngine::clearProject() {
    clearTracks();
    m_project = nullptr;
    emit projectCleared();
}

void AudioEngine::clearTracks() {
    for (Track* track : m_tracks) {
        delete track;
    }
    m_tracks.clear();
}

void AudioEngine::processAudio(float** input, float** output, int numChannels, int numFrames) {
    if (!m_initialized) return;
    if (numFrames <= 0) return;

    QMutexLocker locker(&m_mutex);

    // Clear output buffers
    for (int c = 0; c < numChannels; c++) {
        if (output[c]) {
            memset(output[c], 0, numFrames * sizeof(float));
        }
    }

    // Process each track
    for (Track* track : m_tracks) {
        if (track->isMuted()) continue;

        // Allocate track buffers
        float* trackBuffer[2] = {nullptr, nullptr};
        trackBuffer[0] = new float[numFrames];
        trackBuffer[1] = new float[numFrames];
        memset(trackBuffer[0], 0, numFrames * sizeof(float));
        memset(trackBuffer[1], 0, numFrames * sizeof(float));

        // Process track
        track->process(trackBuffer, numFrames);

        // Mix into output
        m_mixer->mixTrack(trackBuffer, output, numFrames, track->getVolume(), track->getPan());

        delete[] trackBuffer[0];
        delete[] trackBuffer[1];
    }

    // Update playhead position
    if (m_transport->isPlaying()) {
        m_transport->updatePosition();
    }
}

void AudioEngine::setupDefaultTracks() {
    Track* track = addTrack(TrackType::Audio);
    track->setName("Track 1");
    track->setColor(QColor(70, 130, 180));

    Track* midiTrack = addTrack(TrackType::MIDI);
    midiTrack->setName("MIDI Track");
    midiTrack->setColor(QColor(180, 130, 70));
}

void AudioEngine::connectTransportSignals() {
    connect(m_transport, &Transport::positionChanged,
            this, &AudioEngine::onTransportPositionChanged);
    connect(m_transport, &Transport::playStateChanged,
            this, &AudioEngine::onTransportPlayStateChanged);
    connect(m_transport, &Transport::recordStateChanged,
            this, &AudioEngine::onTransportRecordStateChanged);
    connect(m_transport, &Transport::tempoChanged,
            this, &AudioEngine::onTransportTempoChanged);
}

void AudioEngine::onTransportPositionChanged(double seconds) {
    emit positionChanged(seconds);
    emit playheadMoved(seconds);
}

void AudioEngine::onTransportPlayStateChanged(bool playing) {
    emit transportStateChanged(playing);
}

void AudioEngine::onTransportRecordStateChanged(bool recording) {
    emit recordingStateChanged(recording);
}

void AudioEngine::onTransportTempoChanged(double bpm) {
    emit tempoChanged(bpm);
}
