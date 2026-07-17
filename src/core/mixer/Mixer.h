#ifndef MIXER_H
#define MIXER_H

#include <QObject>
#include <QVector>
#include <QMutex>

class Mixer : public QObject {
    Q_OBJECT

public:
    explicit Mixer(QObject* parent = nullptr);
    ~Mixer();

    void setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }
    void setBufferSize(int bufferSize) { m_bufferSize = bufferSize; }

    void setMasterVolume(float volume);
    float getMasterVolume() const { return m_masterVolume; }

    void reset();

    // Mix a track into the main output
    void mixTrack(float** trackBuffer, float** output, int numFrames, float volume, float pan);

signals:
    void masterVolumeChanged(float volume);

private:
    int m_sampleRate = 44100;
    int m_bufferSize = 256;
    float m_masterVolume = 1.0f;
    mutable QMutex m_mutex;
};

#endif // MIXER_H