#include "EQ.h"
#include <cmath>
#include <algorithm>

EQ::EQ() {
    // Default bands
    Band lowShelf;
    lowShelf.type = Band::LowShelf;
    lowShelf.frequency = 80.0f;
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

    m_filters.resize(2);
    updateFilters();
}

void EQ::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!inputs || !outputs) return;
    if (numChannels <= 0 || numFrames <= 0) return;

    if (m_filters.size() != static_cast<size_t>(numChannels)) {
        m_filters.resize(numChannels);
        for (auto& channel : m_filters) {
            channel.resize(m_bands.size());
        }
        updateFilters();
    }

    for (int c = 0; c < numChannels; c++) {
        if (!inputs[c] || !outputs[c]) continue;

        if (inputs[c] != outputs[c]) {
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }

        for (int b = 0; b < m_bands.size() && b < m_filters[c].size(); b++) {
            if (m_bands[b].enabled && m_filters[c][b].active) {
                m_filters[c][b].filter.process(outputs[c], outputs[c], numFrames);
            }
        }

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
    if (index < 0 || index >= 10) return;

    if (index < 3) {
        int bandIndex = index;
        if (bandIndex < m_bands.size()) {
            m_bands[bandIndex].gain = value * 24.0f - 12.0f;
            updateFilters();
        }
    } else if (index < 6) {
        int bandIndex = index - 3;
        if (bandIndex < m_bands.size()) {
            m_bands[bandIndex].frequency = 20.0f + value * 19980.0f;
            updateFilters();
        }
    } else if (index < 9) {
        int bandIndex = index - 6;
        if (bandIndex < m_bands.size()) {
            m_bands[bandIndex].q = 0.1f + value * 9.9f;
            updateFilters();
        }
    } else if (index == 9) {
        m_masterGain = powf(10.0f, (value * 24.0f - 12.0f) / 20.0f);
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
        float gainDb = 20.0f * log10f(m_masterGain + 0.0001f);
        return (gainDb + 12.0f) / 24.0f;
    }
    return 0.0f;
}

int EQ::getParameterCount() const { return 10; }

QString EQ::getParameterName(int index) const {
    if (index < 0 || index >= 10) return "";
    if (index < 3) return QString("Band %1 Gain").arg(index + 1);
    if (index < 6) return QString("Band %1 Freq").arg(index - 2);
    if (index < 9) return QString("Band %1 Q").arg(index - 5);
    if (index == 9) return "Master Gain";
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
    float sampleRate = m_sampleRate > 0 ? m_sampleRate : 44100.0f;

    for (int c = 0; c < m_filters.size(); c++) {
        m_filters[c].resize(m_bands.size());
        for (int b = 0; b < m_bands.size(); b++) {
            const Band& band = m_bands[b];
            auto& filter = m_filters[c][b];
            filter.active = band.enabled;

            // Calculate biquad coefficients based on band type
            float w0 = 2.0f * M_PI * band.frequency / sampleRate;
            float cos_w0 = cosf(w0);
            float sin_w0 = sinf(w0);
            float alpha = sin_w0 / (2.0f * band.q);

            float a0, a1, a2, b0, b1, b2;

            switch (band.type) {
                case Band::LowShelf: {
                    float A = powf(10.0f, band.gain / 40.0f);
                    float beta = sqrtf(A) / band.q;
                    b0 = A * ((A + 1) + (A - 1) * cos_w0 + 2 * sqrtf(A) * alpha);
                    b1 = -2 * A * ((A - 1) + (A + 1) * cos_w0);
                    b2 = A * ((A + 1) + (A - 1) * cos_w0 - 2 * sqrtf(A) * alpha);
                    a0 = (A + 1) - (A - 1) * cos_w0 + 2 * sqrtf(A) * alpha;
                    a1 = 2 * ((A - 1) - (A + 1) * cos_w0);
                    a2 = (A + 1) - (A - 1) * cos_w0 - 2 * sqrtf(A) * alpha;
                    break;
                }
                case Band::HighShelf: {
                    float A = powf(10.0f, band.gain / 40.0f);
                    float beta = sqrtf(A) / band.q;
                    b0 = A * ((A + 1) - (A - 1) * cos_w0 + 2 * sqrtf(A) * alpha);
                    b1 = 2 * A * ((A - 1) - (A + 1) * cos_w0);
                    b2 = A * ((A + 1) - (A - 1) * cos_w0 - 2 * sqrtf(A) * alpha);
                    a0 = (A + 1) + (A - 1) * cos_w0 + 2 * sqrtf(A) * alpha;
                    a1 = -2 * ((A - 1) + (A + 1) * cos_w0);
                    a2 = (A + 1) + (A - 1) * cos_w0 - 2 * sqrtf(A) * alpha;
                    break;
                }
                case Band::Peaking: {
                    float A = powf(10.0f, band.gain / 40.0f);
                    b0 = 1 + alpha * A;
                    b1 = -2 * cos_w0;
                    b2 = 1 - alpha * A;
                    a0 = 1 + alpha / A;
                    a1 = -2 * cos_w0;
                    a2 = 1 - alpha / A;
                    break;
                }
                case Band::LowPass: {
                    b0 = (1 - cos_w0) / 2;
                    b1 = 1 - cos_w0;
                    b2 = (1 - cos_w0) / 2;
                    a0 = 1 + alpha;
                    a1 = -2 * cos_w0;
                    a2 = 1 - alpha;
                    break;
                }
                case Band::HighPass: {
                    b0 = (1 + cos_w0) / 2;
                    b1 = -(1 + cos_w0);
                    b2 = (1 + cos_w0) / 2;
                    a0 = 1 + alpha;
                    a1 = -2 * cos_w0;
                    a2 = 1 - alpha;
                    break;
                }
                case Band::BandPass: {
                    b0 = alpha;
                    b1 = 0;
                    b2 = -alpha;
                    a0 = 1 + alpha;
                    a1 = -2 * cos_w0;
                    a2 = 1 - alpha;
                    break;
                }
                case Band::Notch: {
                    b0 = 1;
                    b1 = -2 * cos_w0;
                    b2 = 1;
                    a0 = 1 + alpha;
                    a1 = -2 * cos_w0;
                    a2 = 1 - alpha;
                    break;
                }
                default: {
                    // Passthrough
                    b0 = 1; b1 = 0; b2 = 0;
                    a0 = 1; a1 = 0; a2 = 0;
                    break;
                }
            }

            // Normalize
            filter.filter.setCoefficients(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
        }
    }
}
