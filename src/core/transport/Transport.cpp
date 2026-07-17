#include "Transport.h"
#include <QDebug>
#include <cmath>

Transport::Transport(QObject* parent)
    : QObject(parent) {
    m_timer.start();
}

Transport::~Transport() {}

void Transport::play() {
    if (m_playing) return;
    m_playing = true;
    m_paused = false;
    m_timer.restart();
    m_timerOffset = m_position;
    emit playStateChanged(true);
    qDebug() << "Transport: play";
}

void Transport::stop() {
    m_playing = false;
    m_recording = false;
    m_paused = false;
    m_position = 0.0;
    emit playStateChanged(false);
    emit recordStateChanged(false);
    emit positionChanged(0.0);
    qDebug() << "Transport: stop";
}

void Transport::pause() {
    if (!m_playing) return;
    m_playing = false;
    m_paused = true;
    // Store current position
    updatePosition();
    emit playStateChanged(false);
    qDebug() << "Transport: pause at " << m_position;
}

void Transport::record() {
    m_recording = !m_recording;
    if (m_recording && !m_playing) {
        play();
    }
    emit recordStateChanged(m_recording);
    qDebug() << "Transport: record " << (m_recording ? "on" : "off");
}

void Transport::togglePlay() {
    if (m_playing) {
        pause();
    } else {
        play();
    }
}

void Transport::setPosition(double seconds) {
    if (seconds < 0.0) seconds = 0.0;
    m_position = seconds;
    m_timerOffset = seconds;
    if (m_playing) {
        m_timer.restart();
    }
    emit positionChanged(seconds);
    qDebug() << "Transport: set position to " << seconds;
}

void Transport::setTempo(double bpm) {
    if (bpm < 10.0) bpm = 10.0;
    if (bpm > 500.0) bpm = 500.0;
    if (m_tempo != bpm) {
        m_tempo = bpm;
        emit tempoChanged(bpm);
        qDebug() << "Transport: tempo set to " << bpm;
    }
}

void Transport::setTimeSignature(int beats, int noteValue) {
    if (beats < 1) beats = 1;
    if (beats > 16) beats = 16;
    if (noteValue < 1) noteValue = 1;
    if (noteValue > 16) noteValue = 16;
    m_beats = beats;
    m_noteValue = noteValue;
}

void Transport::setLooping(bool looping) {
    m_looping = looping;
    emit loopStateChanged(looping);
}

void Transport::setLoopRange(double start, double end) {
    if (start < 0.0) start = 0.0;
    if (end <= start) end = start + 1.0;
    m_loopStart = start;
    m_loopEnd = end;
    emit loopRangeChanged(start, end);
}

double Transport::getTempoAt(double time) const {
    // TODO: Tempo automation
    return m_tempo;
}

void Transport::updatePosition() {
    if (!m_playing) return;
    double elapsed = m_timer.elapsed() / 1000.0;
    double newPosition = m_timerOffset + elapsed;
    if (newPosition != m_position) {
        m_position = newPosition;

        // Loop handling
        if (m_looping && m_position > m_loopEnd) {
            m_position = m_loopStart;
            m_timer.restart();
            m_timerOffset = m_loopStart;
            emit positionChanged(m_position);
            return;
        }

        emit positionChanged(m_position);
    }
}