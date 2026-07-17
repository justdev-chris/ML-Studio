#ifndef MIDI_H
#define MIDI_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QMutex>
#include <functional>

struct MIDIMessage {
    enum Type {
        NoteOff = 0x80,
        NoteOn = 0x90,
        PolyPressure = 0xA0,
        ControlChange = 0xB0,
        ProgramChange = 0xC0,
        ChannelPressure = 0xD0,
        PitchBend = 0xE0,
        SystemExclusive = 0xF0,
        TimeCode = 0xF1,
        SongPosition = 0xF2,
        SongSelect = 0xF3,
        TuneRequest = 0xF6,
        SystemExclusiveEnd = 0xF7,
        Clock = 0xF8,
        Start = 0xFA,
        Continue = 0xFB,
        Stop = 0xFC,
        ActiveSensing = 0xFE,
        SystemReset = 0xFF
    };

    Type type;
    int channel = 0;
    int data1 = 0;
    int data2 = 0;
    int timestamp = 0;

    bool isValid() const;
    QString toString() const;
};

class MIDI : public QObject {
    Q_OBJECT

public:
    explicit MIDI(QObject* parent = nullptr);
    ~MIDI();

    bool initialize();
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    bool openInput(int portIndex);
    bool openInput(const QString& deviceName);
    void closeInput();
    bool isInputOpen() const { return m_inputOpen; }

    bool openOutput(int portIndex);
    bool openOutput(const QString& deviceName);
    void closeOutput();
    bool isOutputOpen() const { return m_outputOpen; }

    QVector<QString> getInputDevices() const;
    QVector<QString> getOutputDevices() const;

    bool sendMessage(const MIDIMessage& message);
    bool sendNoteOn(int channel, int note, int velocity);
    bool sendNoteOff(int channel, int note, int velocity = 0);
    bool sendControlChange(int channel, int controller, int value);
    bool sendProgramChange(int channel, int program);
    bool sendPitchBend(int channel, int value);
    bool sendSystemExclusive(const QByteArray& data);

    // Transport sync
    void sendClock();
    void sendStart();
    void sendContinue();
    void sendStop();

    using MIDICallback = std::function<void(const MIDIMessage&)>;
    void setCallback(MIDICallback callback) { m_callback = callback; }

signals:
    void messageReceived(const MIDIMessage& message);
    void noteOnReceived(int note, int velocity);
    void noteOffReceived(int note);
    void controlChangeReceived(int controller, int value);
    void pitchBendReceived(int value);
    void inputOpened(const QString& device);
    void outputOpened(const QString& device);
    void error(const QString& message);

private:
    bool m_initialized = false;
    bool m_inputOpen = false;
    bool m_outputOpen = false;

    QString m_currentInputDevice;
    QString m_currentOutputDevice;

    MIDICallback m_callback;
    mutable QMutex m_mutex;

    void* m_midiIn = nullptr;
    void* m_midiOut = nullptr;

    void processInputMessage(const QByteArray& data, int timestamp);
    static void midiInputCallback(double timeStamp, const unsigned char* message, size_t size, void* userData);

    void handleNoteOn(int note, int velocity);
    void handleNoteOff(int note);
    void handleControlChange(int controller, int value);
    void handlePitchBend(int value);
};

#endif // MIDI_H
