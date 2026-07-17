#ifndef EQ_H
#define EQ_H

#include "FX.h"
#include "utils/MathUtils.h"
#include <QVector>

class EQ : public FX {
public:
    struct Band {
        enum Type {
            LowShelf,
            HighShelf,
            Peaking,
            LowPass,
            HighPass,
            BandPass,
            Notch
        };

        Type type = Peaking;
        float frequency = 1000.0f;
        float gain = 0.0f;
        float q = 0.707f;
        bool enabled = true;
    };

    EQ();
    ~EQ() override = default;

    QString getName() const override { return "EQ"; }
    QString getVendor() const override { return "MeowyLoops"; }
    QString getCategory() const override { return "EQ"; }

    void process(float** inputs, float** outputs, int numChannels, int numFrames) override;
    void reset() override;

    void setParameter(int index, float value) override;
    float getParameter(int index) const override;
    int getParameterCount() const override;
    QString getParameterName(int index) const override;
    float getParameterMin(int index) const override;
    float getParameterMax(int index) const override;
    float getParameterDefault(int index) const override;

    void setSampleRate(int sampleRate) override;

    // Band management
    void addBand(const Band& band);
    void removeBand(int index);
    void clearBands();
    int getBandCount() const { return m_bands.size(); }
    Band getBand(int index) const;
    void setBand(int index, const Band& band);

    // Master controls
    void setMasterGain(float gain);
    float getMasterGain() const { return m_masterGain; }

private:
    struct BiquadFilter {
        MathUtils::Biquad filter;
        bool active = false;
    };

    QVector<Band> m_bands;
    QVector<QVector<BiquadFilter>> m_filters; // [channel][band]

    float m_masterGain = 1.0f;

    void updateFilters();
};

#endif // EQ_H