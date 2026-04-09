#include "Moves.h"

Move::Move()
    : data{0, "", "", "", Type::Normal, Category::Status, 0, 100, 0, MoveEffect::None, 0, 0, 0, 0, Target::Opponent}, maxPP(0) {}

Move::Move(const MoveData& data)
    : data(data), maxPP(data.pp) {}

Move::Move(std::string name, Type type, Category category, int power, int accuracy, int pp)
    : data{0, std::move(name), "", "", type, category, power, accuracy, pp, MoveEffect::None, 0, 0, 0, 0, Target::Opponent}, maxPP(pp) {}

Move::Move(std::string name, Type type, Category category, int power, int accuracy, int pp, MoveEffect effect, int effectChance, int effectParam1, int effectParam2, int priority, Target target)
    : data{0, std::move(name), "", "", type, category, power, accuracy, pp, effect, effectChance, effectParam1, effectParam2, priority, target}, maxPP(pp) {}

Move::Move(const Move& other)
    : data(other.data), maxPP(other.maxPP) {}

Move& Move::operator=(const Move& other) {
    if (this != &other) {
        data = other.data;
        maxPP = other.maxPP;
    }
    return *this;
}

Move::~Move() {}
