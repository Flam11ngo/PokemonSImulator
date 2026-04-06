#include "Moves.h"

Move::Move()
    : name(""), type(Type::Normal), category(Category::Status), power(0), accuracy(100), pp(0), maxPP(0), effect(MoveEffect::None), effectChance(0), effectParam1(0), effectParam2(0) {}

Move::Move(std::string name, Type type, Category category, int power, int accuracy, int pp)
    : name(std::move(name)), type(type), category(category), power(power), accuracy(accuracy), pp(pp), maxPP(pp), effect(MoveEffect::None), effectChance(0), effectParam1(0), effectParam2(0) {}

Move::Move(std::string name, Type type, Category category, int power, int accuracy, int pp, MoveEffect effect, int effectChance, int effectParam1, int effectParam2)
    : name(std::move(name)), type(type), category(category), power(power), accuracy(accuracy), pp(pp), maxPP(pp), effect(effect), effectChance(effectChance), effectParam1(effectParam1), effectParam2(effectParam2) {}

Move::Move(const Move& other)
    : name(other.name), type(other.type), category(other.category), power(other.power), accuracy(other.accuracy), pp(other.pp), maxPP(other.maxPP), effect(other.effect), effectChance(other.effectChance), effectParam1(other.effectParam1), effectParam2(other.effectParam2) {}

Move& Move::operator=(const Move& other) {
    if (this != &other) {
        name = other.name;
        type = other.type;
        category = other.category;
        power = other.power;
        accuracy = other.accuracy;
        pp = other.pp;
        maxPP = other.maxPP;
        effect = other.effect;
        effectChance = other.effectChance;
        effectParam1 = other.effectParam1;
        effectParam2 = other.effectParam2;
    }
    return *this;
}

Move::~Move() {}
