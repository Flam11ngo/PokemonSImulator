#pragma once

#include "Types.h"
#include <string>

enum class WeatherType {
    Clear,
    Rain,
    Sun,
    Sandstorm,
    Hail,
    Snow,
    Count
};

struct Weather {
    WeatherType type = WeatherType::Clear;
    int duration = 0;

    void setWeather(WeatherType newType, int turns);
    void tick();
    bool isActive() const;
    std::string getName() const;
    float applyDamageModifier(Type targetType) const;
};
