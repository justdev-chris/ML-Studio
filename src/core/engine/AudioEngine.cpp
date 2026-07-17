#include "AudioEngine.h"
#include "core/track/Clip.h"
#include <QDebug>
#include <QTimer>
#include <QDateTime>

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
    , m_transport(new Transport(this))
    , m_mixer(new Mixer(this)) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AudioEngine::updatePlayhead);
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

    // Initialize mixer
    m_mixer->setSampleRate(sampleRate);
    m_mixer->setBufferSize(bufferSize);

    // Setup default tracks
    setupDefaultTracks();

    m_initialized = true;
    m_timer->start(50); // Update playhead 20x per second

    qDebug() << "AudioEngine initialized: " << sampleRate << "Hz, " << bufferSize << " samples";
    return true;
}

void AudioEngine::shutdown() {
    if (!m_initialized) return;

    m_timer->stop();

    for (Track* track : m_tracks) {
        delete track;
    }
    m_tracks.clear();

    m_mixer->reset();
    m_initialized = false;
    qDebug() << "AudioEngine shutdown";
}

void AudioEngine::play() {
    if (!m_initialized) return;
    m_transport->play();
    emit transportStateChanged(true);
    qDebug() << "AudioEngine: play";
}

void AudioEngine::stop() {
    if (!m_initialized) return;
    m_transport->stop();
    emit transportStateChanged(false);
    qDebug() << "AudioEngine: stop";
}

void AudioEngine::pause() {
    if (!m_initialized) return;
    m_transport->pause();
    emit transportStateChanged(false);
    qDebug() << "AudioEngine: pause";
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
    emit recordingStateChanged(m_transport->isRecording());
    qDebug() << "AudioEngine: record";
}

void AudioEngine::setPosition(double seconds) {
    if (!m_initialized) return;
    m_transport->setPosition(seconds);
    m_currentTime = seconds;
    emit positionChanged(seconds);
}

Track* AudioEngine::addTrack(TrackType type) {
    Track* track = new Track(type, m_sampleRate, this);
    track->setName(QString("Track %1").arg(m_tracks.size() + 1));
    m_tracks.append(track);
    emit trackAdded(track);
    qDebug() << "AudioEngine: added track " << track->getName();
    return track;
}

void AudioEngine::removeTrack(int index) {
    if (index < 0 || index >= m_tracks.size()) return;
    Track* track = m_tracks.at(index);
    m_tracks.removeAt(index);
    delete track;
    emit trackRemoved(index);
    qDebug() << "AudioEngine: removed track " << index;
}

Track* AudioEngine::getTrack(int index) const {
    if (index < 0 || index >= m_tracks.size()) return nullptr;
    return m_tracks.at(index);
}

void AudioEngine::setMasterVolume(float volume) {
    m_mixer->setMasterVolume(volume);
}

float AudioEngine::getMasterVolume() const {
    return m_mixer->getMasterVolume();
}

void AudioEngine::setAudioDevice(const QString& device) {
    // TODO: Implement PortAudio device selection
    Q_UNUSED(device);
}

QStringList AudioEngine::getAudioDevices() const {
    // TODO: Implement PortAudio device enumeration
    return QStringList();
}

void AudioEngine::setupDefaultTracks() {
    // Create a default audio track
    Track* track = addTrack(TrackType::Audio);
    track->setName("Track 1");
    track->setColor(QColor(70, 130, 180));

    // Add a second track for MIDI
    Track* midiTrack = addTrack(TrackType::MIDI);
    midiTrack->setName("MIDI Track");
    midiTrack->setColor(QColor(180, 130, 70));
}

void AudioEngine::processAudio(float** output, int numFrames) {
    if (!m_initialized) return;
    if (numFrames <= 0) return;

    m_mutex.lock();

    // Clear output buffer
    for (int i = 0; i < 2; i++) {
        if (output[i]) {
            memset(output[i], 0, numFrames * sizeof(float));
        }
    }

    // Process all tracks
    for (Track* track : m_tracks) {
        if (!track->isMuted()) {
            float* trackBuffer[2] = { nullptr, nullptr };
            // Allocate temporary buffer for track processing
            // TODO: Use pre-allocated buffers for efficiency
            track->process(trackBuffer, numFrames);
            m_mixer->mixTrack(trackBuffer, output, numFrames, track->getVolume(), track->getPan());
            // Cleanup
            if (trackBuffer[0]) delete[] trackBuffer[0];
            if (trackBuffer[1]) delete[] trackBuffer[1];
        }
    }

    m_mutex.unlock();
}

void AudioEngine::updatePlayhead() {
    if (!m_initialized || !m_transport->isPlaying()) return;

    double position = m_transport->getPosition();
    emit positionChanged(position);

    // Update track clip positions if needed
    for (Track* track : m_tracks) {
        track->updateClips(position);
    }
}