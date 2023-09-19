#include <string>
#include "common.hpp"

#ifndef WEATHER_HPP
#define WEATHER_HPP

enum Weather : u32
{
    SUNNY,
    CLOUDY,
    RAIN,
    STORM,
    SNOW,
    SNOWSTORM,
    SUNSHINE,
    SANDSTORM,
    MIST,

    INVALID = 255,
};

static const std::string WEATHER_LUT[] = {
    "SUNNY",
    "CLOUDY",
    "RAIN",
    "STORM",
    "SNOW",
    "SNOWSTORM",
    "SUNSHINE",
    "SANDSTORM",
    "MIST",
};

#endif