#ifndef DELAY_H
#define DELAY_H

#include "FX.h"
#include <vector>
#include <deque>

class Delay : public FX {
public:
    Delay();
    ~Delay() override = default;

    QString getName() const override { return "Delay"; }
    QString getVendor() const override { return "MeowyLoops"; }
    QString getCategory() const override { return "Spatial"; }

    void process(float** inputs, float** outputs, int numChannels, int numFrames) override;
    void reset() override;

    void setParameter(int index, float value) override;
    float getParameter(int index) const override;
    int getParameterCount() const override { return 5; }
    QString getParameterName(int index) const override;
    float getParameterMin(int index) const override;
    float getParameterMax(int index) const override;
    float getParameterDefault(int index) const override;

    // Direct parameter setters
    void setDelayTime(float ms) { m_delayMs = ms; updateDelay(); }
    void setFeedback(float feedback) { m_feedback = feedback; }
    void setWetLevel(float wet) { m_wetLevel = wet; }
    void setDryLevel(float dry) { m_dryLevel = dry; }
    void setFilterFrequency(float freq) { m_filterFreq = freq; }

    float getDelayTime() const { return m_delayMs; }
    float getFeedback() const { return m_feedback; }
    float getWetLevel() const { return m_wetLevel; }
    float getDryLevel() const { return m_dryLevel; }
    float getFilterFrequency() const { return m_filterFreq; }

private:
    struct DelayChannel {
        std::deque<float> buffer;
        int delaySamples = 0;
        float filterState = 0.0f;
    };

    void updateDelay();

    float m_delayMs = 500.0f;
    float m_feedback = 0.3f;
    float m_wetLevel = 0.5f;
    float m_dryLevel = 1.0f;
    float m_filterFreq = 1000.0f;

    std::vector<DelayChannel> m_channels;
};

#endif // DELAY_H