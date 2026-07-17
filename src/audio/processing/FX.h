#ifndef FX_H
#define FX_H

#include <QString>
#include <QVector>
#include <QMap>

class FX {
public:
    virtual ~FX() = default;

    virtual QString getName() const = 0;
    virtual QString getVendor() const = 0;
    virtual QString getCategory() const = 0;

    virtual void process(float** inputs, float** outputs, int numChannels, int numFrames) = 0;
    virtual void reset() = 0;

    virtual void setParameter(int index, float value) = 0;
    virtual float getParameter(int index) const = 0;
    virtual int getParameterCount() const = 0;
    virtual QString getParameterName(int index) const = 0;
    virtual float getParameterMin(int index) const = 0;
    virtual float getParameterMax(int index) const = 0;
    virtual float getParameterDefault(int index) const = 0;

    virtual void setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }
    virtual void setBufferSize(int bufferSize) { m_bufferSize = bufferSize; }

protected:
    int m_sampleRate = 44100;
    int m_bufferSize = 256;
};

#endif // FX_H