#include "EQ.h"
#include <cmath>
#include <QDebug>

EQ::EQ() {
    // Default bands
    Band lowShelf;
    lowShelf.type = Band::LowShelf;
    lowShelf.frequency = 100.0f;
    lowShelf.gain = 0.0f;
    lowShelf.q = 0.707f;
    m_bands.append(lowShelf);

    Band mid;
    mid.type = Band::Peaking;
    mid.frequency = 1000.0f;
    mid.gain = 0.0f;
    mid.q = 1.0f;
    m_bands.append(mid);

    Band highShelf;
    highShelf.type = Band::HighShelf;
    highShelf.frequency = 8000.0f;
    highShelf.gain = 0.0f;
    highShelf.q = 0.707f;
    m_bands.append(highShelf);

    updateFilters();
}

void EQ::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!inputs || !outputs) return;
    if (numChannels <= 0 || numFrames <= 0) return;

    // Ensure filter array matches channels
    if (m_filters.size() != numChannels) {
        m_filters.resize(numChannels);
        for (int c = 0; c < numChannels; c++) {
            m_filters[c].resize(m_bands.size());
        }
    }

    for (int c = 0; c < numChannels; c++) {
        if (!inputs[c] || !outputs[c]) continue;

        // Copy input to output if same buffer
        if (inputs[c] != outputs[c]) {
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }

        // Apply each band
        for (int b = 0; b < m_bands.size() && b < m_filters[c].size(); b++) {
            if (m_bands[b].enabled && m_filters[c][b].active) {
                m_filters[c][b].filter.process(outputs[c], outputs[c], numFrames);
            }
        }

        // Apply master gain
        if (m_masterGain != 1.0f) {
            for (int i = 0; i < numFrames; i++) {
                outputs[c][i] *= m_masterGain;
            }
        }
    }
}

void EQ::reset() {
    for (auto& channel : m_filters) {
        for (auto& filter : channel) {
            filter.filter.reset();
        }
    }
}

void EQ::setParameter(int index, float value) {
    // Parameter mapping:
    // 0-2: band gains
    // 3-5: band frequencies
    // 6-8: band Q values
    // 9: master gain

    if (index < 0 || index >= 10) return;

    if (index < 3) {
        int bandIndex = index;
        if (bandIndex < m_bands.size()) {
            m_bands[bandIndex].gain = value * 24.0f - 12.0f; // -12 to +12 dB
            updateFilters();
        }
    } else if (index < 6) {
        int bandIndex = index - 3;
        if (bandIndex < m_bands.size()) {
            m_bands[bandIndex].frequency = MathUtils::linearInterpolate(20.0f, 20000.0f, value);
            updateFilters();
        }
    } else if (index < 9) {
        int bandIndex = index - 6;
        if (bandIndex < m_bands.size()) {
            m_bands[bandIndex].q = MathUtils::linearInterpolate(0.1f, 10.0f, value);
            updateFilters();
        }
    } else if (index == 9) {
        m_masterGain = MathUtils::dbToGain(value * 24.0f - 12.0f);
    }
}

float EQ::getParameter(int index) const {
    if (index < 0 || index >= 10) return 0.0f;

    if (index < 3) {
        int bandIndex = index;
        if (bandIndex < m_bands.size()) {
            return (m_bands[bandIndex].gain + 12.0f) / 24.0f;
        }
    } else if (index < 6) {
        int bandIndex = index - 3;
        if (bandIndex < m_bands.size()) {
            return (m_bands[bandIndex].frequency - 20.0f) / 19980.0f;
        }
    } else if (index < 9) {
        int bandIndex = index - 6;
        if (bandIndex < m_bands.size()) {
            return (m_bands[bandIndex].q - 0.1f) / 9.9f;
        }
    } else if (index == 9) {
        float gainDb = MathUtils::gainToDb(m_masterGain);
        return (gainDb + 12.0f) / 24.0f;
    }

    return 0.0f;
}

int EQ::getParameterCount() const { return 10; }

QString EQ::getParameterName(int index) const {
    if (index < 0 || index >= 10) return "";

    if (index < 3) {
        return QString("Band %1 Gain").arg(index + 1);
    } else if (index < 6) {
        return QString("Band %1 Freq").arg(index - 2);
    } else if (index < 9) {
        return QString("Band %1 Q").arg(index - 5);
    } else if (index == 9) {
        return "Master Gain";
    }
    return "";
}

float EQ::getParameterMin(int index) const { return 0.0f; }
float EQ::getParameterMax(int index) const { return 1.0f; }
float EQ::getParameterDefault(int index) const { return 0.5f; }

void EQ::setSampleRate(int sampleRate) {
    FX::setSampleRate(sampleRate);
    updateFilters();
}

void EQ::addBand(const Band& band) {
    m_bands.append(band);
    updateFilters();
}

void EQ::removeBand(int index) {
    if (index < 0 || index >= m_bands.size()) return;
    m_bands.removeAt(index);
    updateFilters();
}

void EQ::clearBands() {
    m_bands.clear();
    updateFilters();
}

EQ::Band EQ::getBand(int index) const {
    if (index < 0 || index >= m_bands.size()) return Band();
    return m_bands[index];
}

void EQ::setBand(int index, const Band& band) {
    if (index < 0 || index >= m_bands.size()) return;
    m_bands[index] = band;
    updateFilters();
}

void EQ::setMasterGain(float gain) {
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 4.0f) gain = 4.0f;
    m_masterGain = gain;
}

void EQ::updateFilters() {
    // TODO: Implement actual biquad coefficient calculation
    // For now, just mark filters as active
    for (int c = 0; c < m_filters.size(); c++) {
        m_filters[c].resize(m_bands.size());
        for (int b = 0; b < m_bands.size(); b++) {
            m_filters[c][b].active = m_bands[b].enabled;
        }
    }
}