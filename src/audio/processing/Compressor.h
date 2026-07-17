#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include "FX.h"
#include <vector>
#include <cmath>

class Compressor : public FX {
public:
    Compressor();
    ~Compressor() override = default;

    QString getName() const override { return "Compressor"; }
    QString getVendor() const override { return "MeowyLoops"; }
    QString getCategory() const override { return "Dynamics"; }

    void process(float** inputs, float** outputs, int numChannels, int numFrames) override;
    void reset() override;

    void setParameter(int index, float value) override;
    float getParameter(int index) const override;
    int getParameterCount() const override { return 6; }
    QString getParameterName(int index) const override;
    float getParameterMin(int index) const override;
    float getParameterMax(int index) const override;
    float getParameterDefault(int index) const override;

    // Parameter setters (for UI)
    void setThreshold(float db) { m_threshold = db; }
    void setRatio(float ratio) { m_ratio = ratio; }
    void setAttack(float ms) { m_attackMs = ms; }
    void setRelease(float ms) { m_releaseMs = ms; }
    void setMakeupGain(float db) { m_makeupGain = db; }
    void setKnee(float db) { m_knee = db; }
    void setAutoMakeup(bool autoMakeup) { m_autoMakeup = autoMakeup; }

    float getThreshold() const { return m_threshold; }
    float getRatio() const { return m_ratio; }
    float getAttack() const { return m_attackMs; }
    float getRelease() const { return m_releaseMs; }
    float getMakeupGain() const { return m_makeupGain; }
    float getKnee() const { return m_knee; }
    bool getAutoMakeup() const { return m_autoMakeup; }

private:
    struct EnvelopeFollower {
        float envelope = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
    };

    float m_threshold = -20.0f;      // dB
    float m_ratio = 4.0f;            // 1:4
    float m_attackMs = 5.0f;         // ms
    float m_releaseMs = 50.0f;       // ms
    float m_makeupGain = 0.0f;       // dB
    float m_knee = 6.0f;             // dB
    bool m_autoMakeup = true;

    std::vector<EnvelopeFollower> m_envelopes;

    void updateEnvelopeCoefficients();
    float calculateGainReduction(float levelDb) const;
    float dbToGain(float db) const;
    float gainToDb(float gain) const;
};

#endif // COMPRESSOR_H