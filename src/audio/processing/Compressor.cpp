#include "Compressor.h"
#include <algorithm>
#include <cmath>

Compressor::Compressor() {
    m_envelopes.resize(2);
    updateEnvelopeCoefficients();
}

void Compressor::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!inputs || !outputs) return;
    if (numChannels <= 0 || numFrames <= 0) return;

    // Ensure envelopes match channels
    if (m_envelopes.size() != static_cast<size_t>(numChannels)) {
        m_envelopes.resize(numChannels);
        updateEnvelopeCoefficients();
    }

    for (int c = 0; c < numChannels; c++) {
        if (!inputs[c] || !outputs[c]) continue;

        if (inputs[c] != outputs[c]) {
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }

        float* output = outputs[c];
        EnvelopeFollower& env = m_envelopes[c];

        for (int i = 0; i < numFrames; i++) {
            float sample = output[i];

            // RMS approximation (simple absolute value)
            float absSample = std::abs(sample);
            float envIn = absSample;

            // Attack/Release
            float coeff = (envIn > env.envelope) ? env.attackCoeff : env.releaseCoeff;
            env.envelope += coeff * (envIn - env.envelope);

            // Calculate gain reduction
            float envDb = gainToDb(env.envelope + 0.0001f);
            float gainReductionDb = calculateGainReduction(envDb);

            // Apply gain reduction
            float gain = dbToGain(-gainReductionDb);
            output[i] = sample * gain;
        }

        // Apply makeup gain
        float makeupGain = dbToGain(m_autoMakeup ? -calculateGainReduction(m_threshold) : m_makeupGain);
        if (makeupGain != 1.0f) {
            for (int i = 0; i < numFrames; i++) {
                output[i] *= makeupGain;
            }
        }
    }
}

void Compressor::reset() {
    for (auto& env : m_envelopes) {
        env.envelope = 0.0f;
    }
}

void Compressor::setParameter(int index, float value) {
    switch (index) {
        case 0: m_threshold = -60.0f + value * 70.0f; break;
        case 1: m_ratio = 1.0f + value * 19.0f; break;
        case 2: m_attackMs = 0.1f + value * 99.9f; break;
        case 3: m_releaseMs = 1.0f + value * 999.0f; break;
        case 4: m_makeupGain = -20.0f + value * 40.0f; break;
        case 5: m_knee = 0.0f + value * 24.0f; break;
    }
    updateEnvelopeCoefficients();
}

float Compressor::getParameter(int index) const {
    switch (index) {
        case 0: return (m_threshold + 60.0f) / 70.0f;
        case 1: return (m_ratio - 1.0f) / 19.0f;
        case 2: return (m_attackMs - 0.1f) / 99.9f;
        case 3: return (m_releaseMs - 1.0f) / 999.0f;
        case 4: return (m_makeupGain + 20.0f) / 40.0f;
        case 5: return m_knee / 24.0f;
        default: return 0.0f;
    }
}

QString Compressor::getParameterName(int index) const {
    switch (index) {
        case 0: return "Threshold";
        case 1: return "Ratio";
        case 2: return "Attack";
        case 3: return "Release";
        case 4: return "Makeup Gain";
        case 5: return "Knee";
        default: return "";
    }
}

float Compressor::getParameterMin(int index) const { return 0.0f; }
float Compressor::getParameterMax(int index) const { return 1.0f; }

float Compressor::getParameterDefault(int index) const {
    switch (index) {
        case 0: return 0.4f;
        case 1: return 0.5f;
        case 2: return 0.05f;
        case 3: return 0.05f;
        case 4: return 0.5f;
        case 5: return 0.25f;
        default: return 0.0f;
    }
}

void Compressor::updateEnvelopeCoefficients() {
    float attackSec = m_attackMs / 1000.0f;
    float releaseSec = m_releaseMs / 1000.0f;
    float sampleRate = m_sampleRate > 0 ? m_sampleRate : 44100.0f;

    m_envelopes.resize(std::max(1, static_cast<int>(m_envelopes.size())));
    for (auto& env : m_envelopes) {
        env.attackCoeff = 1.0f - std::exp(-1.0f / (attackSec * sampleRate));
        env.releaseCoeff = 1.0f - std::exp(-1.0f / (releaseSec * sampleRate));
    }
}

float Compressor::calculateGainReduction(float levelDb) const {
    if (levelDb < m_threshold - m_knee / 2.0f) {
        return 0.0f;
    }

    if (levelDb < m_threshold + m_knee / 2.0f) {
        float kneeWidth = m_knee;
        float over = levelDb - m_threshold + kneeWidth / 2.0f;
        float reduction = (over * over) / (2.0f * kneeWidth) * (1.0f - 1.0f / m_ratio);
        return reduction;
    }

    float over = levelDb - m_threshold;
    float reduction = over * (1.0f - 1.0f / m_ratio);
    return reduction;
}

float Compressor::dbToGain(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

float Compressor::gainToDb(float gain) const {
    if (gain <= 0.0f) return -120.0f;
    return 20.0f * std::log10(gain);
}
