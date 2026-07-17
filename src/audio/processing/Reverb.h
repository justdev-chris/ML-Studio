#ifndef REVERB_H
#define REVERB_H

#include "FX.h"
#include <vector>
#include <deque>

class Reverb : public FX {
public:
    Reverb();
    ~Reverb() override;

    QString getName() const override { return "Reverb"; }
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

    void setRoomSize(float size) { m_roomSize = size; }
    void setDamping(float damping) { m_damping = damping; }
    void setWetLevel(float wet) { m_wetLevel = wet; }
    void setDryLevel(float dry) { m_dryLevel = dry; }
    void setPredelay(float ms) { m_predelayMs = ms; }

private:
    struct DelayLine {
        std::deque<float> buffer;
        int delaySamples = 0;
    };

    void updateDelayLines();

    float m_roomSize = 0.5f;
    float m_damping = 0.3f;
    float m_wetLevel = 0.3f;
    float m_dryLevel = 0.8f;
    float m_predelayMs = 20.0f;

    std::vector<DelayLine> m_delayLines;
    std::vector<float> m_filterStates;
    int m_predelaySamples = 0;
    int m_predelayReadPos = 0;

    static constexpr int NUM_COMB_FILTERS = 8;
    static constexpr int NUM_ALLPASS_FILTERS = 4;
};

#endif // REVERB_H