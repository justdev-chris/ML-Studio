#include "MathUtils.h"
#include <cmath>
#include <cstring>

namespace MathUtils {

void Biquad::updateCoefficients() {
    if (m_sampleRate <= 0) return;

    float w0 = 2.0f * M_PI * m_frequency / m_sampleRate;
    float cos_w0 = std::cos(w0);
    float sin_w0 = std::sin(w0);
    float alpha = sin_w0 / (2.0f * m_q);

    float A = std::pow(10.0f, m_gain / 40.0f);

    switch (m_type) {
        case LowPass: {
            float b0 = (1.0f - cos_w0) / 2.0f;
            float b1 = 1.0f - cos_w0;
            float b2 = (1.0f - cos_w0) / 2.0f;
            float a0 = 1.0f + alpha;
            float a1 = -2.0f * cos_w0;
            float a2 = 1.0f - alpha;
            m_b0 = b0 / a0; m_b1 = b1 / a0; m_b2 = b2 / a0;
            m_a1 = a1 / a0; m_a2 = a2 / a0;
            break;
        }
        case HighPass: {
            float b0 = (1.0f + cos_w0) / 2.0f;
            float b1 = -(1.0f + cos_w0);
            float b2 = (1.0f + cos_w0) / 2.0f;
            float a0 = 1.0f + alpha;
            float a1 = -2.0f * cos_w0;
            float a2 = 1.0f - alpha;
            m_b0 = b0 / a0; m_b1 = b1 / a0; m_b2 = b2 / a0;
            m_a1 = a1 / a0; m_a2 = a2 / a0;
            break;
        }
        case BandPass: {
            float b0 = alpha;
            float b1 = 0.0f;
            float b2 = -alpha;
            float a0 = 1.0f + alpha;
            float a1 = -2.0f * cos_w0;
            float a2 = 1.0f - alpha;
            m_b0 = b0 / a0; m_b1 = b1 / a0; m_b2 = b2 / a0;
            m_a1 = a1 / a0; m_a2 = a2 / a0;
            break;
        }
        case Notch: {
            float b0 = 1.0f;
            float b1 = -2.0f * cos_w0;
            float b2 = 1.0f;
            float a0 = 1.0f + alpha;
            float a1 = -2.0f * cos_w0;
            float a2 = 1.0f - alpha;
            m_b0 = b0 / a0; m_b1 = b1 / a0; m_b2 = b2 / a0;
            m_a1 = a1 / a0; m_a2 = a2 / a0;
            break;
        }
        case LowShelf: {
            float beta = std::sqrt(A) / m_q;
            float b0 = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * std::sqrt(A) * alpha);
            float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_w0);
            float b2 = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * std::sqrt(A) * alpha);
            float a0 = (A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * std::sqrt(A) * alpha;
            float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_w0);
            float a2 = (A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * std::sqrt(A) * alpha;
            m_b0 = b0 / a0; m_b1 = b1 / a0; m_b2 = b2 / a0;
            m_a1 = a1 / a0; m_a2 = a2 / a0;
            break;
        }
        case HighShelf: {
            float beta = std::sqrt(A) / m_q;
            float b0 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * std::sqrt(A) * alpha);
            float b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_w0);
            float b2 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * std::sqrt(A) * alpha);
            float a0 = (A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * std::sqrt(A) * alpha;
            float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_w0);
            float a2 = (A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * std::sqrt(A) * alpha;
            m_b0 = b0 / a0; m_b1 = b1 / a0; m_b2 = b2 / a0;
            m_a1 = a1 / a0; m_a2 = a2 / a0;
            break;
        }
        case Peaking: {
            float b0 = 1.0f + alpha * A;
            float b1 = -2.0f * cos_w0;
            float b2 = 1.0f - alpha * A;
            float a0 = 1.0f + alpha / A;
            float a1 = -2.0f * cos_w0;
            float a2 = 1.0f - alpha / A;
            m_b0 = b0 / a0; m_b1 = b1 / a0; m_b2 = b2 / a0;
            m_a1 = a1 / a0; m_a2 = a2 / a0;
            break;
        }
        default: {
            m_b0 = 1.0f; m_b1 = 0.0f; m_b2 = 0.0f;
            m_a1 = 0.0f; m_a2 = 0.0f;
            break;
        }
    }
}

float Biquad::process(float input) {
    float output = m_b0 * input + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
    m_x2 = m_x1;
    m_x1 = input;
    m_y2 = m_y1;
    m_y1 = output;
    return output;
}

void Biquad::process(float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        output[i] = process(input[i]);
    }
}

void Biquad::reset() {
    m_x1 = 0.0f;
    m_x2 = 0.0f;
    m_y1 = 0.0f;
    m_y2 = 0.0f;
}

void Biquad::setCoefficients(float b0, float b1, float b2, float a1, float a2) {
    m_b0 = b0;
    m_b1 = b1;
    m_b2 = b2;
    m_a1 = a1;
    m_a2 = a2;
}

void Biquad::setFrequency(float frequency) {
    m_frequency = frequency;
    updateCoefficients();
}

void Biquad::setQ(float q) {
    m_q = q;
    updateCoefficients();
}

void Biquad::setGain(float gain) {
    m_gain = gain;
    updateCoefficients();
}

void Biquad::setType(Type type) {
    m_type = type;
    updateCoefficients();
}

void Biquad::setSampleRate(float sampleRate) {
    m_sampleRate = sampleRate;
    updateCoefficients();
}

} // namespace MathUtils
