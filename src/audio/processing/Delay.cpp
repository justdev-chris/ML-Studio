#include "Delay.h"
#include <algorithm>
#include <cmath>

Delay::Delay() {
    m_channels.resize(2); // Stereo
    updateDelay();
}

void Delay::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!inputs || !outputs) return;
    if (numChannels <= 0 || numFrames <= 0) return;

    // Ensure channels match
    if (m_channels.size() != static_cast<size_t>(numChannels)) {
        m_channels.resize(numChannels);
        updateDelay();
    }

    for (int c = 0; c < numChannels; c++) {
        if (!inputs[c] || !outputs[c]) continue;

        if (inputs[c] != outputs[c]) {
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }

        float* output = outputs[c];
        DelayChannel& channel = m_channels[c];

        for (int i = 0; i < numFrames; i++) {
            float input = output[i];
            float delayed = 0.0f;

            // Read from delay buffer
            if (!channel.buffer.empty()) {
                delayed = channel.buffer.front();
                channel.buffer.pop_front();

                // Simple low-pass filter on feedback
                float filterState = channel.filterState;
                float alpha = 1.0f / (1.0f + 2.0f * M_PI * m_filterFreq / m_sampleRate);
                filterState += alpha * (delayed - filterState);
                channel.filterState = filterState;
                delayed = filterState;
            }

            // Write to delay buffer (input + feedback)
            float writeSample = input + delayed * m_feedback;
            channel.buffer.push_back(writeSample);

            // Keep buffer at correct size
            while (channel.buffer.size() > channel.delaySamples) {
                channel.buffer.pop_front();
            }

            // Mix dry/wet
            output[i] = input * m_dryLevel + delayed * m_wetLevel;
        }
    }
}

void Delay::reset() {
    for (auto& channel : m_channels) {
        channel.buffer.clear();
        channel.filterState = 0.0f;
    }
}

void Delay::setParameter(int index, float value) {
    switch (index) {
        case 0: m_delayMs = 1.0f + value * 1999.0f; break;   // 1-2000ms
        case 1: m_feedback = value * 0.95f; break;           // 0-0.95
        case 2: m_wetLevel = value; break;                   // 0-1
        case 3: m_dryLevel = value; break;                   // 0-1
        case 4: m_filterFreq = 20.0f + value * 19980.0f; break; // 20-20000Hz
    }
    updateDelay();
}

float Delay::getParameter(int index) const {
    switch (index) {
        case 0: return (m_delayMs - 1.0f) / 1999.0f;
        case 1: return m_feedback / 0.95f;
        case 2: return m_wetLevel;
        case 3: return m_dryLevel;
        case 4: return (m_filterFreq - 20.0f) / 19980.0f;
        default: return 0.0f;
    }
}

QString Delay::getParameterName(int index) const {
    switch (index) {
        case 0: return "Delay Time";
        case 1: return "Feedback";
        case 2: return "Wet Level";
        case 3: return "Dry Level";
        case 4: return "Filter Freq";
        default: return "";
    }
}

float Delay::getParameterMin(int index) const { return 0.0f; }
float Delay::getParameterMax(int index) const { return 1.0f; }

float Delay::getParameterDefault(int index) const {
    switch (index) {
        case 0: return 0.25f; // 500ms
        case 1: return 0.3f;  // 30%
        case 2: return 0.5f;  // 50%
        case 3: return 0.8f;  // 80%
        case 4: return 0.05f; // 1000Hz
        default: return 0.0f;
    }
}

void Delay::updateDelay() {
    float sampleRate = m_sampleRate > 0 ? m_sampleRate : 44100.0f;
    int delaySamples = static_cast<int>(m_delayMs * sampleRate / 1000.0f);
    if (delaySamples < 1) delaySamples = 1;

    for (auto& channel : m_channels) {
        channel.delaySamples = delaySamples;
        channel.buffer.assign(delaySamples, 0.0f);
        channel.filterState = 0.0f;
    }
}