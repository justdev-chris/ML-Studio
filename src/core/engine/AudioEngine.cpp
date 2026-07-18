#include "AudioEngine.h"
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <cmath>
#include <sndfile.h>

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
    , m_transport(new Transport(this))
    , m_mixer(new Mixer(this))
    , m_recordingClip(nullptr)
    , m_recordingTrack(nullptr) {
    connectTransportSignals();
    m_recordBuffer.reserve(44100 * 60 * 10);
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize(int sampleRate, int bufferSize) {
    if (m_initialized) return true;
    m_sampleRate = sampleRate;
    m_bufferSize = bufferSize;
    m_mixer->setSampleRate(sampleRate);
    m_mixer->setBufferSize(bufferSize);

    if (m_audioIO) {
        m_audioIO->initialize(sampleRate, bufferSize, 2);
        m_audioIO->setCallback([this](float** input, float** output, int numFrames) {
            this->processAudio(input, output, 2, numFrames);
        });
    }

    setupDefaultTracks();
    m_initialized = true;
    return true;
}

void AudioEngine::shutdown() {
    if (!m_initialized) return;
    if (m_audioIO && m_audioIO->isRunning()) m_audioIO->stop();
    if (m_audioIO) m_audioIO->shutdown();
    clearTracks();
    for (FX* fx : m_masterInserts) delete fx;
    m_masterInserts.clear();
    m_mixer->reset();
    m_initialized = false;
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
    if (m_audioIO && !m_audioIO->isRunning()) m_audioIO->start();
    emit transportStateChanged(true);
}

void AudioEngine::stop() {
    if (!m_initialized) return;
    m_transport->stop();
    if (m_recordingClip) {
        finalizeRecording();
    }
    emit transportStateChanged(false);
}

void AudioEngine::pause() {
    if (!m_initialized) return;
    m_transport->pause();
    emit transportStateChanged(false);
}

void AudioEngine::togglePlay() {
    if (m_transport->isPlaying()) pause();
    else play();
}

void AudioEngine::record(bool armed) {
    if (!m_initialized) return;
    if (armed && !m_transport->isPlaying()) play();
    m_transport->setRecordArmed(armed);
    emit recordingStateChanged(armed);
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

void AudioEngine::addMasterInsert(FX* effect) {
    if (!effect) return;
    effect->setSampleRate(m_sampleRate);
    effect->setBufferSize(m_bufferSize);
    m_masterInserts.append(effect);
}

void AudioEngine::removeMasterInsert(int index) {
    if (index < 0 || index >= m_masterInserts.size()) return;
    FX* fx = m_masterInserts.at(index);
    m_masterInserts.removeAt(index);
    delete fx;
}

void AudioEngine::clearMasterInserts() {
    for (FX* fx : m_masterInserts) delete fx;
    m_masterInserts.clear();
}

void AudioEngine::setProject(Project* project) {
    m_project = project;
    if (m_project) loadProject(m_project);
}

void AudioEngine::loadProject(Project* project) {
    if (!project) return;
    clearTracks();
    clearMasterInserts();

    for (Track* track : project->getTracks()) {
        Track* newTrack = new Track(track->getType(), m_sampleRate, this);
        newTrack->setName(track->getName());
        newTrack->setColor(track->getColor());
        newTrack->setVolume(track->getVolume());
        newTrack->setPan(track->getPan());
        newTrack->setMuted(track->isMuted());
        newTrack->setSoloed(track->isSoloed());
        newTrack->setRecordArmed(track->isRecordArmed());

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

        // Restore inserts
        for (FX* insert : track->getInserts()) {
            newTrack->addInsert(insert);
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
    for (Track* track : m_tracks) delete track;
    m_tracks.clear();
}

void AudioEngine::setRecordPath(const QString& path) {
    m_recordPath = path;
    QDir dir;
    if (!dir.exists(path)) dir.mkpath(path);
}

void AudioEngine::processAudio(float** input, float** output, int numChannels, int numFrames) {
    if (!m_initialized || numFrames <= 0) return;
    QMutexLocker locker(&m_mutex);

    for (int c = 0; c < numChannels; c++) {
        if (output[c]) memset(output[c], 0, numFrames * sizeof(float));
    }

    // Handle recording
    handleRecording(input, numChannels, numFrames);

    // Process tracks
    float* trackBuffer[2] = {nullptr, nullptr};
    float* processedBuffer[2] = {nullptr, nullptr};

    for (Track* track : m_tracks) {
        if (track->isMuted()) continue;

        trackBuffer[0] = new float[numFrames];
        trackBuffer[1] = new float[numFrames];
        memset(trackBuffer[0], 0, numFrames * sizeof(float));
        memset(trackBuffer[1], 0, numFrames * sizeof(float));

        processedBuffer[0] = new float[numFrames];
        processedBuffer[1] = new float[numFrames];
        memset(processedBuffer[0], 0, numFrames * sizeof(float));
        memset(processedBuffer[1], 0, numFrames * sizeof(float));

        track->process(trackBuffer, numFrames);

        // Apply track inserts
        if (track->getInsertCount() > 0) {
            memcpy(processedBuffer[0], trackBuffer[0], numFrames * sizeof(float));
            memcpy(processedBuffer[1], trackBuffer[1], numFrames * sizeof(float));
            for (FX* insert : track->getInserts()) {
                insert->process(processedBuffer, processedBuffer, numChannels, numFrames);
            }
        } else {
            memcpy(processedBuffer[0], trackBuffer[0], numFrames * sizeof(float));
            memcpy(processedBuffer[1], trackBuffer[1], numFrames * sizeof(float));
        }

        m_mixer->mixTrack(processedBuffer, output, numFrames, track->getVolume(), track->getPan());

        delete[] trackBuffer[0];
        delete[] trackBuffer[1];
        delete[] processedBuffer[0];
        delete[] processedBuffer[1];
    }

    // Process master inserts
    processMasterInserts(output, numChannels, numFrames);

    // Update playhead
    if (m_transport->isPlaying()) {
        m_transport->updatePosition();
    }
}

void AudioEngine::processMasterInserts(float** buffer, int numChannels, int numFrames) {
    if (m_masterInserts.isEmpty()) return;
    float* tempBuffer[2] = {nullptr, nullptr};
    tempBuffer[0] = new float[numFrames];
    tempBuffer[1] = new float[numFrames];

    for (FX* fx : m_masterInserts) {
        memcpy(tempBuffer[0], buffer[0], numFrames * sizeof(float));
        memcpy(tempBuffer[1], buffer[1], numFrames * sizeof(float));
        fx->process(tempBuffer, buffer, numChannels, numFrames);
    }

    delete[] tempBuffer[0];
    delete[] tempBuffer[1];
}

void AudioEngine::handleRecording(float** input, int numChannels, int numFrames) {
    if (!m_transport->isRecording()) {
        if (m_recordingClip) {
            finalizeRecording();
        }
        return;
    }

    Track* armedTrack = nullptr;
    for (Track* track : m_tracks) {
        if (track->isRecordArmed() && track->getType() == TrackType::Audio) {
            armedTrack = track;
            break;
        }
    }
    if (!armedTrack || !input[0]) return;

    if (!m_recordingClip || m_recordingTrack != armedTrack) {
        m_recordingTrack = armedTrack;
        m_recordingClip = new AudioClip(armedTrack);
        m_recordingClip->setName("Recording " + QDateTime::currentDateTime().toString("hhmmss"));
        m_recordingClip->setStart(m_transport->getPosition() * m_sampleRate);
        m_recordBuffer.clear();
        armedTrack->addClip(m_recordingClip);
    }

    int numSamples = numFrames * numChannels;
    for (int i = 0; i < numSamples; i++) {
        m_recordBuffer.append(input[i % numChannels]);
    }
    m_recordingClip->setLength(m_recordBuffer.size());
    emit recordingProgress(m_transport->getPosition());
}

void AudioEngine::finalizeRecording() {
    if (!m_recordingClip || m_recordBuffer.isEmpty()) {
        m_recordingClip = nullptr;
        m_recordingTrack = nullptr;
        m_recordBuffer.clear();
        return;
    }

    if (!m_recordPath.isEmpty()) {
        QString fileName = m_recordPath + "/" + m_recordingClip->getName() + ".wav";
        SF_INFO info;
        info.samplerate = m_sampleRate;
        info.channels = 2;
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
        SNDFILE* file = sf_open(fileName.toUtf8().constData(), SFM_WRITE, &info);
        if (file) {
            sf_write_float(file, m_recordBuffer.data(), m_recordBuffer.size());
            sf_close(file);
            qDebug() << "Recording saved to:" << fileName;
        } else {
            qWarning() << "Failed to save recording:" << fileName;
        }
    }

    m_recordingClip->setAudioData(m_recordBuffer);
    m_recordingClip->setLength(m_recordBuffer.size());

    m_recordingClip = nullptr;
    m_recordingTrack = nullptr;
    m_recordBuffer.clear();
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
    m_currentTime = seconds;
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
