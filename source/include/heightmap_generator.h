#ifndef HEIGHTMAP_GENERATOR_H
#define HEIGHTMAP_GENERATOR_H

#include "noise_generator.hpp"
#include "noise_policy.hpp"

#include <cstdint>

namespace rapidxml
{
    template<class Ch> class xml_node;
}

struct SimplexNoiseProps
{
public:
    uint32_t seed = 0;
    uint32_t octaves = 10;
    float  frequency = 0.01f;
    float  persistence = 0.1f;
    float  loBound = 0.0f;
    float  hiBound = 1.0f;
    float  startX = 0.0f;
    float  startZ = 0.0f;
    float  scale = 1.0f;

    void parse_xml(rapidxml::xml_node<char>* node);
};

struct DropletErosionProps
{
public:
    float Kq = 10.0f;       // Soil carry capacity
    float Kw = 0.001f;      // Water evaporation speed
    float Kr = 0.9f;        // Erosion speed
    float Kd = 0.02f;       // Deposition speed
    float Ki = 0.1f;        // Direction inertia
    float Kg = 40.0f;       // Gravity
    float minSlope = 0.05f; // Soil carry slope contrib lower bound
    float epsilon = 1e-3;   // For horizontal ground detection
    uint32_t seed = 42;     // For RNG
    uint32_t iterations = 100;

    void parse_xml(rapidxml::xml_node<char>* node);
};

struct PlateauErosionProps
{
public:
    uint32_t iterations = 20;
    float talus = 0.4f;
    float fraction = 0.5f;

    void parse_xml(rapidxml::xml_node<char>* node);
};

class HeightMap;

class HeightmapGenerator
{
public:

    static void init_simplex_generator(std::mt19937& rng);
    static void heightmap_from_simplex_noise(HeightMap& hm, const SimplexNoiseProps& info);

    static void erode(HeightMap& hm, const PlateauErosionProps& props);
    static void erode_droplets(HeightMap& hmap, const DropletErosionProps& params);

private:
    static NoiseGenerator2D<SimplexNoise<>> RNG_simplex_;
};

#endif // HEIGHTMAP_GENERATOR_H
