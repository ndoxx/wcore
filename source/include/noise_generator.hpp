#ifndef NOISEGENERATOR_HPP_INCLUDED
#define NOISEGENERATOR_HPP_INCLUDED

#include <utility>
#include <random>

namespace wcore
{

template <typename NoisePolicy>
class NoiseGenerator2D
{
public:
    NoiseGenerator2D()
    : generator_() {}

    template <typename... Args>
    explicit NoiseGenerator2D(Args&&... args)
    : generator_(std::forward<Args>(args)...) {}   // Perfect forward args to policy constructor

    ~NoiseGenerator2D() {}

    // Find noise policy's floting point type
    typedef decltype(std::declval<NoisePolicy>().operator()(0.0,0.0)) Float;

    void init(std::mt19937& rng)
    {
        generator_.init(rng);
    }

    // Raw noise
    inline Float sample(Float xin, Float yin)
    {
        return generator_(xin,yin);
    }

    Float operator()(Float xin, Float yin)
    {
        return generator_(xin,yin);
    }

    // Smooth filtering by local average        Kernel is 1/16 1/8 1/16
    Float smoothed_sample(Float x, Float y)            // 1/8  1/4 1/8
    {                                                  // 1/16 1/8 1/16
        Float corners = (sample(x-1, y-1)+sample(x+1, y-1)+sample(x-1, y+1)+sample(x+1, y+1))/16;
        Float sides   = (sample(x-1, y)  +sample(x+1, y)  +sample(x, y-1)  +sample(x, y+1)  )/8;
        Float center  =  sample(x, y)/4;

        return corners + sides + center;
    }

    // Scaled sample
    Float scaled_sample(Float x, Float y, Float loBound, Float hiBound)
    {
        return sample(x,y) * (hiBound - loBound)/2 + (hiBound + loBound)/2;
    }

    // Octaved noise for pattern control
    Float octave_noise(Float x, Float y, size_t octaves, Float frequency, Float persistence)
    {
        Float total = 0;
        Float amplitude = 1;

        // We have to keep track of the largest possible amplitude,
        // because each octave adds more, and we need a value in [-1, 1].
        Float maxAmplitude = 0;

        for(size_t ii=0; ii<octaves; ++ii)
        {
            total += sample( x * frequency, y * frequency ) * amplitude;

            frequency *= 1.95;  // Not set to 2.0 exactly, interference patterns are desired to break repetition
            maxAmplitude += amplitude;
            amplitude *= persistence;
        }

        return total / maxAmplitude;
    }

    Float scaled_octave_noise(Float x, Float y, size_t octaves, Float frequency,
                                 Float persistence, Float loBound, Float hiBound)
    {
        return octave_noise(x, y, octaves, frequency, persistence)
        * (hiBound - loBound)/2 + (hiBound + loBound)/2;
    }

    Float marble_noise_2d(Float x, Float y, size_t octaves, Float frequency, Float persistence)
    {
        return cos(x * frequency + octave_noise(x, y, octaves, frequency/3, persistence));
    }

private:
    NoisePolicy generator_;
};

}

#endif // NOISEGENERATOR_HPP_INCLUDED
