#include "MIDI.h"
#include <QDebug>

// RtMidi headers
#include <RtMidi.h>

MIDI::MIDI(QObject* parent)
    : QObject(parent)
    , m_midiIn(nullptr)
    , m_midiOut(nullptr) {
}

MIDI::~MIDI() {
    shutdown();
}

bool MIDI::initialize() {
    if (m_initialized) return true;

    try {
        m_midiIn = new RtMidiIn();
        m_midiOut = new RtMidiOut();
        m_initialized = true;
        qDebug() << "MIDI initialized";
        return true;
    } catch (const std::exception& e) {
        qCritical() << "MIDI init error:" << e.what();
        emit error(QString("MIDI: %1").arg(e.what()));
        return false;
    }
}

void MIDI::shutdown() {
    if (!m_initialized) return;

    closeInput();
    closeOutput();

    if (m_midiIn) {
        delete static_cast<RtMidiIn*>(m_midiIn);
        m_midiIn = nullptr;
    }
    if (m_midiOut) {
        delete static_cast<RtMidiOut*>(m_midiOut);
        m_midiOut = nullptr;
    }

    m_initialized = false;
    qDebug() << "MIDI shutdown";
}

bool MIDI::openInput(int portIndex) {
    if (!m_initialized || !m_midiIn) return false;
    if (m_inputOpen) closeInput();

    try {
        auto* midiIn = static_cast<RtMidiIn*>(m_midiIn);
        midiIn->openPort(portIndex);
        midiIn->setCallback(&MIDI::midiInputCallback, this);
        midiIn->ignoreTypes(false, false, false);

        m_inputOpen = true;
        m_currentInputDevice = QString::fromStdString(midiIn->getPortName(portIndex));
        emit inputOpened(m_currentInputDevice);
        qDebug() << "MIDI input opened:" << m_currentInputDevice;
        return true;
    } catch (const std::exception& e) {
        qCritical() << "MIDI input error:" << e.what();
        emit error(QString("MIDI input: %1").arg(e.what()));
        return false;
    }
}

bool MIDI::openInput(const QString& deviceName) {
    auto devices = getInputDevices();
    for (int i = 0; i < devices.size(); i++) {
        if (devices[i] == deviceName) {
            return openInput(i);
        }
    }
    return false;
}

void MIDI::closeInput() {
    if (!m_inputOpen || !m_midiIn) return;
    auto* midiIn = static_cast<RtMidiIn*>(m_midiIn);
    midiIn->closePort();
    m_inputOpen = false;
    m_currentInputDevice.clear();
    qDebug() << "MIDI input closed";
}

bool MIDI::openOutput(int portIndex) {
    if (!m_initialized || !m_midiOut) return false;
    if (m_outputOpen) closeOutput();

    try {
        auto* midiOut = static_cast<RtMidiOut*>(m_midiOut);
        midiOut->openPort(portIndex);
        m_outputOpen = true;
        m_currentOutputDevice = QString::fromStdString(midiOut->getPortName(portIndex));
        emit outputOpened(m_currentOutputDevice);
        qDebug() << "MIDI output opened:" << m_currentOutputDevice;
        return true;
    } catch (const std::exception& e) {
        qCritical() << "MIDI output error:" << e.what();
        emit error(QString("MIDI output: %1").arg(e.what()));
        return false;
    }
}

bool MIDI::openOutput(const QString& deviceName) {
    auto devices = getOutputDevices();
    for (int i = 0; i < devices.size(); i++) {
        if (devices[i] == deviceName) {
            return openOutput(i);
        }
    }
    return false;
}

void MIDI::closeOutput() {
    if (!m_outputOpen || !m_midiOut) return;
    auto* midiOut = static_cast<RtMidiOut*>(m_midiOut);
    midiOut->closePort();
    m_outputOpen = false;
    m_currentOutputDevice.clear();
    qDebug() << "MIDI output closed";
}

QVector<QString> MIDI::getInputDevices() const {
    QVector<QString> devices;
    if (!m_initialized || !m_midiIn) return devices;

    auto* midiIn = static_cast<RtMidiIn*>(m_midiIn);
    int count = midiIn->getPortCount();
    for (int i = 0; i < count; i++) {
        try {
            devices.append(QString::fromStdString(midiIn->getPortName(i)));
        } catch (...) {}
    }
    return devices;
}

QVector<QString> MIDI::getOutputDevices() const {
    QVector<QString> devices;
    if (!m_initialized || !m_midiOut) return devices;

    auto* midiOut = static_cast<RtMidiOut*>(m_midiOut);
    int count = midiOut->getPortCount();
    for (int i = 0; i < count; i++) {
        try {
            devices.append(QString::fromStdString(midiOut->getPortName(i)));
        } catch (...) {}
    }
    return devices;
}

bool MIDI::sendMessage(const MIDIMessage& message) {
    if (!m_outputOpen || !m_midiOut) return false;

    QByteArray data;
    if (message.type >= 0xF0) {
        data.append(message.type);
        data.append(message.data1);
        data.append(message.data2);
    } else {
        data.append((message.type & 0xF0) | (message.channel & 0x0F));
        data.append(message.data1 & 0x7F);
        data.append(message.data2 & 0x7F);
    }

    try {
        auto* midiOut = static_cast<RtMidiOut*>(m_midiOut);
        midiOut->sendMessage(reinterpret_cast<const unsigned char*>(data.constData()), data.size());
        return true;
    } catch (const std::exception& e) {
        emit error(QString("MIDI send: %1").arg(e.what()));
        return false;
    }
}

bool MIDI::sendNoteOn(int channel, int note, int velocity) {
    MIDIMessage msg;
    msg.type = MIDIMessage::NoteOn;
    msg.channel = channel;
    msg.data1 = note;
    msg.data2 = velocity;
    return sendMessage(msg);
}

bool MIDI::sendNoteOff(int channel, int note, int velocity) {
    MIDIMessage msg;
    msg.type = MIDIMessage::NoteOff;
    msg.channel = channel;
    msg.data1 = note;
    msg.data2 = velocity;
    return sendMessage(msg);
}

bool MIDI::sendControlChange(int channel, int controller, int value) {
    MIDIMessage msg;
    msg.type = MIDIMessage::ControlChange;
    msg.channel = channel;
    msg.data1 = controller;
    msg.data2 = value;
    return sendMessage(msg);
}

bool MIDI::sendProgramChange(int channel, int program) {
    MIDIMessage msg;
    msg.type = MIDIMessage::ProgramChange;
    msg.channel = channel;
    msg.data1 = program;
    msg.data2 = 0;
    return sendMessage(msg);
}

bool MIDI::sendPitchBend(int channel, int value) {
    MIDIMessage msg;
    msg.type = MIDIMessage::PitchBend;
    msg.channel = channel;
    msg.data1 = value & 0x7F;
    msg.data2 = (value >> 7) & 0x7F;
    return sendMessage(msg);
}

void MIDI::processInputMessage(const QByteArray& data, int timestamp) {
    if (data.size() < 1) return;

    MIDIMessage msg;
    msg.timestamp = timestamp;

    unsigned char status = data[0];
    if (status >= 0xF0) {
        msg.type = static_cast<MIDIMessage::Type>(status);
        if (data.size() > 1) msg.data1 = data[1];
        if (data.size() > 2) msg.data2 = data[2];
    } else {
        msg.type = static_cast<MIDIMessage::Type>(status & 0xF0);
        msg.channel = status & 0x0F;
        if (data.size() > 1) msg.data1 = data[1];
        if (data.size() > 2) msg.data2 = data[2];
    }

    if (m_callback) {
        m_callback(msg);
    }
    emit messageReceived(msg);

    // Forward to PianoRoll if connected
    if (msg.type == MIDIMessage::NoteOn && msg.data2 > 0) {
        emit noteOnReceived(msg.data1, msg.data2);
    } else if (msg.type == MIDIMessage::NoteOn && msg.data2 == 0) {
        emit noteOffReceived(msg.data1);
    } else if (msg.type == MIDIMessage::NoteOff) {
        emit noteOffReceived(msg.data1);
    }
}

void MIDI::midiInputCallback(double timeStamp, const unsigned char* message, size_t size, void* userData) {
    auto* midi = static_cast<MIDI*>(userData);
    if (!midi) return;

    QByteArray data(reinterpret_cast<const char*>(message), static_cast<int>(size));
    int timestamp = static_cast<int>(timeStamp * 1000);
    midi->processInputMessage(data, timestamp);
}
