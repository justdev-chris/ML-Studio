#include "MIDI.h"
#include <QDebug>
#include <QTimer>
#include <RtMidi.h>

bool MIDIMessage::isValid() const { return type >= 0x80 && type <= 0xFF; }

QString MIDIMessage::toString() const {
    static const char* names[] = {"NoteOff", "NoteOn", "PolyPressure", "ControlChange", "ProgramChange",
                                  "ChannelPressure", "PitchBend", "SysEx"};
    return QString("%1 ch%2 d1=%3 d2=%4").arg(names[type >> 4]).arg(channel).arg(data1).arg(data2);
}

MIDI::MIDI(QObject* parent) : QObject(parent), m_midiIn(nullptr), m_midiOut(nullptr), m_transport(nullptr), m_clockTimer(nullptr) {}
MIDI::~MIDI() { shutdown(); }

bool MIDI::initialize() {
    if (m_initialized) return true;
    try {
        m_midiIn = new RtMidiIn();
        m_midiOut = new RtMidiOut();
        m_initialized = true;

        // Setup clock timer
        m_clockTimer = new QTimer(this);
        connect(m_clockTimer, &QTimer::timeout, this, &MIDI::sendClock);
        m_clockTimer->setInterval(20833); // 24ppq at 120 BPM (in microseconds)
        m_clockTimer->stop();

        return true;
    } catch (const std::exception& e) {
        emit error(QString("MIDI init: %1").arg(e.what()));
        return false;
    }
}

void MIDI::shutdown() {
    if (!m_initialized) return;
    closeInput();
    closeOutput();
    if (m_clockTimer) {
        m_clockTimer->stop();
        delete m_clockTimer;
        m_clockTimer = nullptr;
    }
    delete static_cast<RtMidiIn*>(m_midiIn);
    delete static_cast<RtMidiOut*>(m_midiOut);
    m_midiIn = m_midiOut = nullptr;
    m_initialized = false;
    m_transport = nullptr;
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
        return true;
    } catch (const std::exception& e) {
        emit error(QString("MIDI input: %1").arg(e.what()));
        return false;
    }
}

bool MIDI::openInput(const QString& deviceName) {
    auto devices = getInputDevices();
    for (int i = 0; i < devices.size(); i++) {
        if (devices[i] == deviceName) return openInput(i);
    }
    return false;
}

void MIDI::closeInput() {
    if (!m_inputOpen || !m_midiIn) return;
    static_cast<RtMidiIn*>(m_midiIn)->closePort();
    m_inputOpen = false;
    m_currentInputDevice.clear();
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
        return true;
    } catch (const std::exception& e) {
        emit error(QString("MIDI output: %1").arg(e.what()));
        return false;
    }
}

bool MIDI::openOutput(const QString& deviceName) {
    auto devices = getOutputDevices();
    for (int i = 0; i < devices.size(); i++) {
        if (devices[i] == deviceName) return openOutput(i);
    }
    return false;
}

void MIDI::closeOutput() {
    if (!m_outputOpen || !m_midiOut) return;
    static_cast<RtMidiOut*>(m_midiOut)->closePort();
    m_outputOpen = false;
    m_currentOutputDevice.clear();
}

QVector<QString> MIDI::getInputDevices() const {
    QVector<QString> devices;
    if (!m_initialized || !m_midiIn) return devices;
    auto* midiIn = static_cast<RtMidiIn*>(m_midiIn);
    int count = midiIn->getPortCount();
    for (int i = 0; i < count; i++) {
        try { devices.append(QString::fromStdString(midiIn->getPortName(i))); }
        catch (...) {}
    }
    return devices;
}

QVector<QString> MIDI::getOutputDevices() const {
    QVector<QString> devices;
    if (!m_initialized || !m_midiOut) return devices;
    auto* midiOut = static_cast<RtMidiOut*>(m_midiOut);
    int count = midiOut->getPortCount();
    for (int i = 0; i < count; i++) {
        try { devices.append(QString::fromStdString(midiOut->getPortName(i))); }
        catch (...) {}
    }
    return devices;
}

bool MIDI::sendMessage(const MIDIMessage& message) {
    if (!m_outputOpen || !m_midiOut) return false;
    QByteArray data;
    if (message.type >= 0xF0) {
        data.append((char)message.type);
        data.append((char)message.data1);
        data.append((char)message.data2);
    } else {
        data.append((char)((message.type & 0xF0) | (message.channel & 0x0F)));
        data.append((char)(message.data1 & 0x7F));
        data.append((char)(message.data2 & 0x7F));
    }
    try {
        auto* midiOut = static_cast<RtMidiOut*>(m_midiOut);
        midiOut->sendMessage((const unsigned char*)data.constData(), data.size());
        return true;
    } catch (const std::exception& e) {
        emit error(QString("MIDI send: %1").arg(e.what()));
        return false;
    }
}

bool MIDI::sendNoteOn(int channel, int note, int velocity) {
    MIDIMessage msg; msg.type = MIDIMessage::NoteOn; msg.channel = channel; msg.data1 = note; msg.data2 = velocity;
    return sendMessage(msg);
}

bool MIDI::sendNoteOff(int channel, int note, int velocity) {
    MIDIMessage msg; msg.type = MIDIMessage::NoteOff; msg.channel = channel; msg.data1 = note; msg.data2 = velocity;
    return sendMessage(msg);
}

bool MIDI::sendControlChange(int channel, int controller, int value) {
    MIDIMessage msg; msg.type = MIDIMessage::ControlChange; msg.channel = channel; msg.data1 = controller; msg.data2 = value;
    return sendMessage(msg);
}

bool MIDI::sendProgramChange(int channel, int program) {
    MIDIMessage msg; msg.type = MIDIMessage::ProgramChange; msg.channel = channel; msg.data1 = program; msg.data2 = 0;
    return sendMessage(msg);
}

bool MIDI::sendPitchBend(int channel, int value) {
    MIDIMessage msg; msg.type = MIDIMessage::PitchBend; msg.channel = channel; msg.data1 = value & 0x7F; msg.data2 = (value >> 7) & 0x7F;
    return sendMessage(msg);
}

void MIDI::sendClock() {
    if (!m_outputOpen) return;
    MIDIMessage msg;
    msg.type = MIDIMessage::Clock;
    sendMessage(msg);
}

void MIDI::sendStart() {
    if (!m_outputOpen) return;
    MIDIMessage msg;
    msg.type = MIDIMessage::Start;
    sendMessage(msg);
}

void MIDI::sendContinue() {
    if (!m_outputOpen) return;
    MIDIMessage msg;
    msg.type = MIDIMessage::Continue;
    sendMessage(msg);
}

void MIDI::sendStop() {
    if (!m_outputOpen) return;
    MIDIMessage msg;
    msg.type = MIDIMessage::Stop;
    sendMessage(msg);
}

void MIDI::setTransport(Transport* transport) {
    m_transport = transport;
    if (!transport) {
        if (m_clockTimer) m_clockTimer->stop();
        return;
    }

    connect(transport, &Transport::playStateChanged, this, [this](bool playing) {
        if (playing) {
            sendStart();
            if (m_clockTimer) {
                // Calculate timer interval based on tempo
                double bpm = m_transport->getTempo();
                int interval = static_cast<int>(60000000.0 / (bpm * 24.0)); // 24ppq
                m_clockTimer->setInterval(interval);
                m_clockTimer->start();
            }
        } else {
            sendStop();
            if (m_clockTimer) m_clockTimer->stop();
        }
    });

    connect(transport, &Transport::tempoChanged, this, [this](double bpm) {
        if (m_clockTimer && m_transport && m_transport->isPlaying()) {
            int interval = static_cast<int>(60000000.0 / (bpm * 24.0));
            m_clockTimer->setInterval(interval);
        }
    });

    connect(transport, &Transport::positionChanged, this, [this](double position) {
        // Optionally send song position pointer (SMPTE)
        // This would be implemented for advanced sync
        Q_UNUSED(position);
    });
}

void MIDI::processInputMessage(const QByteArray& data, int timestamp) {
    if (data.isEmpty()) return;
    MIDIMessage msg; msg.timestamp = timestamp;
    unsigned char status = data[0];

    if (status >= 0xF0) {
        msg.type = (MIDIMessage::Type)status;
        if (data.size() > 1) msg.data1 = data[1];
        if (data.size() > 2) msg.data2 = data[2];
    } else {
        msg.type = (MIDIMessage::Type)(status & 0xF0);
        msg.channel = status & 0x0F;
        if (data.size() > 1) msg.data1 = data[1];
        if (data.size() > 2) msg.data2 = data[2];
    }

    emit messageReceived(msg);
    switch (msg.type) {
        case MIDIMessage::NoteOn:
            if (msg.data2 > 0) emit noteOnReceived(msg.data1, msg.data2);
            else emit noteOffReceived(msg.data1);
            break;
        case MIDIMessage::NoteOff:
            emit noteOffReceived(msg.data1);
            break;
        case MIDIMessage::ControlChange:
            emit controlChangeReceived(msg.data1, msg.data2);
            break;
        case MIDIMessage::PitchBend:
            emit pitchBendReceived(msg.data1 | (msg.data2 << 7));
            break;
        default: break;
    }
}

void MIDI::midiInputCallback(double timeStamp, const unsigned char* message, size_t size, void* userData) {
    auto* midi = static_cast<MIDI*>(userData);
    if (!midi) return;
    midi->processInputMessage(QByteArray((const char*)message, size), (int)(timeStamp * 1000));
}
