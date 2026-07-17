#ifndef MATHUTILS_H
#define MATHUTILS_H

#include <cmath>
#include <vector>
#include <algorithm>

namespace MathUtils {

    // Constants
    const double PI = 3.14159265358979323846;
    const double TWO_PI = 2.0 * PI;
    const double HALF_PI = PI / 2.0;

    // Conversion
    inline double degreesToRadians(double degrees) {
        return degrees * PI / 180.0;
    }

    inline double radiansToDegrees(double radians) {
        return radians * 180.0 / PI;
    }

    inline double dbToGain(double db) {
        return std::pow(10.0, db / 20.0);
    }

    inline double gainToDb(double gain) {
        if (gain <= 0.0) return -INFINITY;
        return 20.0 * std::log10(gain);
    }

    inline double midiToFrequency(int midiNote) {
        return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    }

    inline int frequencyToMidi(double frequency) {
        return static_cast<int>(std::round(69.0 + 12.0 * std::log2(frequency / 440.0)));
    }

    // Clamping
    template <typename T>
    inline T clamp(T value, T min, T max) {
        return std::max(min, std::min(value, max));
    }

    // Interpolation
    inline float linearInterpolate(float a, float b, float t) {
        return a + (b - a) * t;
    }

    inline double linearInterpolate(double a, double b, double t) {
        return a + (b - a) * t;
    }

    // Smoothing
    class Smoother {
    public:
        Smoother() : m_value(0.0f), m_target(0.0f), m_speed(0.99f) {}
        Smoother(float speed) : m_value(0.0f), m_target(0.0f), m_speed(speed) {}

        void setTarget(float target) { m_target = target; }
        float getValue() const { return m_value; }
        void reset(float value) { m_value = value; m_target = value; }

        float process() {
            m_value += (m_target - m_value) * (1.0f - m_speed);
            return m_value;
        }

        float process(float speed) {
            m_speed = speed;
            return process();
        }

        void setSpeed(float speed) { m_speed = speed; }

    private:
        float m_value;
        float m_target;
        float m_speed;
    };

    // Biquad filter (for EQ)
    class Biquad {
    public:
        enum Type {
            LowPass,
            HighPass,
            BandPass,
            Notch,
            LowShelf,
            HighShelf,
            Peaking
        };

        Biquad() : m_type(LowPass), m_frequency(1000.0f), m_q(0.707f), m_gain(0.0f) {
            reset();
        }

        Biquad(Type type, float frequency, float q, float gain = 0.0f) : m_type(type), m_frequency(frequency), m_q(q), m_gain(gain) {
            reset();
        }

        void setType(Type type) { m_type = type; updateCoefficients(); }
        void setFrequency(float freq) { m_frequency = freq; updateCoefficients(); }
        void setQ(float q) { m_q = q; updateCoefficients(); }
        void setGain(float gain) { m_gain = gain; updateCoefficients(); }

        void reset() {
            m_x1 = m_x2 = 0.0f;
            m_y1 = m_y2 = 0.0f;
            updateCoefficients();
        }

        float process(float input) {
            float output = m_a0 * input + m_a1 * m_x1 + m_a2 * m_x2 - m_b1 * m_y1 - m_b2 * m_y2;
            m_x2 = m_x1;
            m_x1 = input;
            m_y2 = m_y1;
            m_y1 = output;
            return output;
        }

        void process(float* input, float* output, int numSamples) {
            for (int i = 0; i < numSamples; i++) {
                output[i] = process(input[i]);
            }
        }

    private:
        void updateCoefficients() {
            // TODO: Implement biquad coefficient calculation
            // For now, set to passthrough
            m_a0 = 1.0f;
            m_a1 = 0.0f;
            m_a2 = 0.0f;
            m_b1 = 0.0f;
            m_b2 = 0.0f;
        }

        Type m_type;
        float m_frequency;
        float m_q;
        float m_gain;

        float m_a0, m_a1, m_a2, m_b1, m_b2;
        float m_x1, m_x2, m_y1, m_y2;
    };

} // namespace MathUtils

#endif // MATHUTILS_H