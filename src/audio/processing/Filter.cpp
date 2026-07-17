#include "Filter.h"
#include <algorithm>
#include <cmath>

Filter::Filter() {
    m_channels.resize(2);
    updateFilters();
}

void Filter::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!inputs || !outputs) return;
    if (numChannels <= 0 || numFrames <= 0) return;

    if (m_channels.size() != static_cast<size_t>(numChannels)) {
        m_channels.resize(numChannels);
        updateFilters();
    }

    for (int c = 0; c < numChannels; c++) {
        if (!inputs[c] || !outputs[c]) continue;

        if (inputs[c] != outputs[c]) {
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }

        if (m_channels[c].active) {
            m_channels[c].filter.process(outputs[c], outputs[c], numFrames);
        }
    }
}

void Filter::reset() {
    for (auto& channel : m_channels) {
        channel.filter.reset();
    }
}

void Filter::setParameter(int index, float value) {
    switch (index) {
        case 0: {
            // Type selector (0-6 mapped to enum)
            int typeIndex = static_cast<int>(value * 6.99f);
            if (typeIndex < 0) typeIndex = 0;
            if (typeIndex > 6) typeIndex = 6;
            m_type = static_cast<Type>(typeIndex);
            updateFilters();
            break;
        }
        case 1: {
            m_frequency = 20.0f + value * 19980.0f;
            updateFilters();
            break;
        }
        case 2: {
            m_q = 0.1f + value * 9.9f;
            updateFilters();
            break;
        }
        case 3: {
            m_gain = -24.0f + value * 48.0f;
            updateFilters();
            break;
        }
    }
}

float Filter::getParameter(int index) const {
    switch (index) {
        case 0: return static_cast<float>(m_type) / 6.99f;
        case 1: return (m_frequency - 20.0f) / 19980.0f;
        case 2: return (m_q - 0.1f) / 9.9f;
        case 3: return (m_gain + 24.0f) / 48.0f;
        default: return 0.0f;
    }
}

QString Filter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Type";
        case 1: return "Frequency";
        case 2: return "Q";
        case 3: return "Gain";
        default: return "";
    }
}

float Filter::getParameterMin(int index) const { return 0.0f; }
float Filter::getParameterMax(int index) const { return 1.0f; }

float Filter::getParameterDefault(int index) const {
    switch (index) {
        case 0: return 0.0f; // LowPass
        case 1: return 0.05f; // 1000Hz
        case 2: return 0.06f; // 0.707
        case 3: return 0.5f; // 0dB
        default: return 0.0f;
    }
}

void Filter::updateFilters() {
    MathUtils::Biquad::Type biquadType = mapType(m_type);

    for (auto& channel : m_channels) {
        // In a real implementation, we'd pass the actual parameters to the biquad
        // For now, just mark as active
        channel.active = true;
        channel.filter.setType(biquadType);
        // TODO: Actually implement biquad coefficient calculation
    }
}

MathUtils::Biquad::Type Filter::mapType(Type type) const {
    switch (type) {
        case LowPass:    return MathUtils::Biquad::LowPass;
        case HighPass:   return MathUtils::Biquad::HighPass;
        case BandPass:   return MathUtils::Biquad::BandPass;
        case Notch:      return MathUtils::Biquad::Notch;
        case LowShelf:   return MathUtils::Biquad::LowShelf;
        case HighShelf:  return MathUtils::Biquad::HighShelf;
        case Peaking:    return MathUtils::Biquad::Peaking;
        default:         return MathUtils::Biquad::LowPass;
    }
}