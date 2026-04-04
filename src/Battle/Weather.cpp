#include "Battle/Weather.h"

void Weather::setWeather(WeatherType newType, int turns) {
    type = newType;
    duration = turns > 0 ? turns : 0;
}

void Weather::tick() {
    if (duration > 0) {
        --duration;
        if (duration == 0) {
            type = WeatherType::Clear;
        }
    }
}

bool Weather::isActive() const {
    return type != WeatherType::Clear && duration > 0;
}

std::string Weather::getName() const {
    switch (type) {
        case WeatherType::Rain: return "Rain";
        case WeatherType::Sun: return "Sun";
        case WeatherType::Sandstorm: return "Sandstorm";
        case WeatherType::Hail: return "Hail";
        case WeatherType::Snow: return "Snow";
        default: return "Clear";
    }
}

float Weather::applyDamageModifier(Type targetType) const {
    switch (type) {
        case WeatherType::Rain:
            if (targetType == Type::Fire) return 0.5f;
            if (targetType == Type::Water) return 1.5f;
            break;
        case WeatherType::Sun:
            if (targetType == Type::Water) return 0.5f;
            if (targetType == Type::Fire) return 1.5f;
            break;
        default:
            break;
    }
    return 1.0f;
}
