#include "AudioIO.h"
#include <QDebug>

AudioIO::AudioIO(QObject* parent) : QObject(parent), m_callback(nullptr), m_isRunning(false), m_sampleRate(44100), m_bufferSize(256), m_channels(2) {
}

AudioIO::~AudioIO() {
    shutdown();
}

bool AudioIO::initialize(int sampleRate, int bufferSize, int channels) {
    m_sampleRate = sampleRate;
    m_bufferSize = bufferSize;
    m_channels = channels;
    qDebug() << "AudioIO initialized" << sampleRate << bufferSize << channels;
    return true;
}

void AudioIO::shutdown() {
    stop();
    m_callback = nullptr;
}

bool AudioIO::start() {
    if (m_isRunning) return true;
    m_isRunning = true;
    qDebug() << "AudioIO started";
    return true;
}

bool AudioIO::stop() {
    m_isRunning = false;
    qDebug() << "AudioIO stopped";
    return true;
}

void AudioIO::setCallback(std::function<void(float**, float**, int)> cb) {
    m_callback = std::move(cb);
}

bool AudioIO::isRunning() const {
    return m_isRunning;
}

void AudioIO::processCallback(float** input, float** output, int numFrames) {
    if (m_callback && m_isRunning) {
        m_callback(input, output, numFrames);
    }
}
