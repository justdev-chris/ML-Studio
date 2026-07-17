#ifndef FILTER_H
#define FILTER_H

#include "FX.h"
#include "utils/MathUtils.h"
#include <vector>

class Filter : public FX {
public:
    enum Type {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        LowShelf,
        HighShelf,
        Peaking
    };

    Filter();
    ~Filter() override = default;

    QString getName() const override { return "Filter"; }
    QString getVendor() const override { return "MeowyLoops"; }
    QString getCategory() const override { return "EQ/Filters"; }

    void process(float** inputs, float** outputs, int numChannels, int numFrames) override;
    void reset() override;

    void setParameter(int index, float value) override;
    float getParameter(int index) const override;
    int getParameterCount() const override { return 4; }
    QString getParameterName(int index) const override;
    float getParameterMin(int index) const override;
    float getParameterMax(int index) const override;
    float getParameterDefault(int index) const override;

    // Direct parameter setters
    void setType(Type type) { m_type = type; updateFilters(); }
    void setFrequency(float freq) { m_frequency = freq; updateFilters(); }
    void setQ(float q) { m_q = q; updateFilters(); }
    void setGain(float gain) { m_gain = gain; updateFilters(); }

    Type getType() const { return m_type; }
    float getFrequency() const { return m_frequency; }
    float getQ() const { return m_q; }
    float getGain() const { return m_gain; }

private:
    struct FilterChannel {
        MathUtils::Biquad filter;
        bool active = false;
    };

    void updateFilters();

    Type m_type = LowPass;
    float m_frequency = 1000.0f;
    float m_q = 0.707f;
    float m_gain = 0.0f;

    std::vector<FilterChannel> m_channels;

    MathUtils::Biquad::Type mapType(Type type) const;
};

#endif // FILTER_H