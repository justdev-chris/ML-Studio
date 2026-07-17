#include "Reverb.h"
#include <algorithm>
#include <cmath>
#include <random>

Reverb::Reverb() {
    m_delayLines.resize(NUM_COMB_FILTERS + NUM_ALLPASS_FILTERS);
    m_filterStates.resize(NUM_COMB_FILTERS, 0.0f);
    updateDelayLines();
}

Reverb::~Reverb() {}

void Reverb::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!inputs || !outputs) return;
    if (numChannels <= 0 || numFrames <= 0) return;

    int monoFrames = std::min(2, numChannels);

    for (int c = 0; c < monoFrames; c++) {
        if (!inputs[c] || !outputs[c]) continue;

        if (inputs[c] != outputs[c]) {
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }

        float* output = outputs[c];

        for (int i = 0; i < numFrames; i++) {
            float input = output[i];

            // Predelay
            m_predelaySamples = static_cast<int>(m_predelayMs * m_sampleRate / 1000.0f);
            if (m_predelaySamples > 0) {
                // Simple predelay buffer
                static std::deque<float> predelayBuffer;
                predelayBuffer.push_back(input);
                if (predelayBuffer.size() > m_predelaySamples) {
                    input = predelayBuffer.front();
                    predelayBuffer.pop_front();
                } else {
                    input = 0.0f;
                }
            }

            // Comb filters
            float wet = 0.0f;
            int combOffset = 0;
            for (int d = 0; d < NUM_COMB_FILTERS; d++) {
                DelayLine& line = m_delayLines[combOffset + d];
                if (line.delaySamples > 0 && !line.buffer.empty()) {
                    float sample = line.buffer.front();
                    line.buffer.pop_front();
                    float outputSample = sample;
                    float filterState = m_filterStates[d];
                    // Simple low-pass damping
                    filterState += m_damping * (outputSample - filterState);
                    m_filterStates[d] = filterState;
                    outputSample = filterState;
                    // Feedback
                    line.buffer.push_back(input + outputSample * 0.7f);
                    // Keep buffer size
                    while (line.buffer.size() > line.delaySamples) {
                        line.buffer.pop_front();
                    }
                    wet += outputSample;
                }
            }

            // Allpass filters (simplified)
            int allpassOffset = NUM_COMB_FILTERS;
            for (int d = 0; d < NUM_ALLPASS_FILTERS; d++) {
                DelayLine& line = m_delayLines[allpassOffset + d];
                if (line.delaySamples > 0 && !line.buffer.empty()) {
                    float sample = line.buffer.front();
                    line.buffer.pop_front();
                    float outputSample = -sample + input * 0.5f;
                    line.buffer.push_back(input + sample * 0.5f);
                    while (line.buffer.size() > line.delaySamples) {
                        line.buffer.pop_front();
                    }
                    input = outputSample;
                    wet += outputSample;
                }
            }

            // Mix dry/wet
            float dry = input * m_dryLevel;
            wet *= m_wetLevel / (NUM_COMB_FILTERS + NUM_ALLPASS_FILTERS);
            output[i] = dry + wet;
        }
    }
}

void Reverb::reset() {
    for (auto& line : m_delayLines) {
        line.buffer.clear();
    }
    std::fill(m_filterStates.begin(), m_filterStates.end(), 0.0f);
}

void Reverb::setParameter(int index, float value) {
    switch (index) {
        case 0: m_roomSize = value; break;
        case 1: m_damping = value; break;
        case 2: m_wetLevel = value; break;
        case 3: m_dryLevel = value; break;
        case 4: m_predelayMs = 0.1f + value * 99.9f; break;
    }
    updateDelayLines();
}

float Reverb::getParameter(int index) const {
    switch (index) {
        case 0: return m_roomSize;
        case 1: return m_damping;
        case 2: return m_wetLevel;
        case 3: return m_dryLevel;
        case 4: return (m_predelayMs - 0.1f) / 99.9f;
        default: return 0.0f;
    }
}

QString Reverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Room Size";
        case 1: return "Damping";
        case 2: return "Wet Level";
        case 3: return "Dry Level";
        case 4: return "Predelay";
        default: return "";
    }
}

float Reverb::getParameterMin(int index) const { return 0.0f; }
float Reverb::getParameterMax(int index) const { return 1.0f; }

float Reverb::getParameterDefault(int index) const {
    switch (index) {
        case 0: return 0.5f;
        case 1: return 0.3f;
        case 2: return 0.3f;
        case 3: return 0.8f;
        case 4: return 0.2f;
        default: return 0.0f;
    }
}

void Reverb::updateDelayLines() {
    // Prime numbers for comb filter delays
    static const int combDelays[NUM_COMB_FILTERS] = {
        1111, 1187, 1277, 1319, 1423, 1481, 1553, 1601
    };
    static const int allpassDelays[NUM_ALLPASS_FILTERS] = {
        559, 619, 683, 743
    };

    float sampleRate = m_sampleRate > 0 ? m_sampleRate : 44100.0f;
    float scale = 0.5f + m_roomSize * 1.5f;

    for (int i = 0; i < NUM_COMB_FILTERS; i++) {
        int delay = static_cast<int>(combDelays[i] * scale);
        m_delayLines[i].delaySamples = delay;
        m_delayLines[i].buffer.assign(delay, 0.0f);
    }

    for (int i = 0; i < NUM_ALLPASS_FILTERS; i++) {
        int delay = static_cast<int>(allpassDelays[i] * scale);
        m_delayLines[NUM_COMB_FILTERS + i].delaySamples = delay;
        m_delayLines[NUM_COMB_FILTERS + i].buffer.assign(delay, 0.0f);
    }
}