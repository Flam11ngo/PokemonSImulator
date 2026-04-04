#include <gtest/gtest.h>
#include "Battle/Battle.h"  // Assuming Battle.h exists
#include "Battle/Utils.h"  // Assuming Utils.h exists
#include"Battle/Types.h"  // Assuming Types.h defines the Type enum
#include "Battle/Natures.h"
#include "Battle/Pokemon.h"
#include "Battle/Species.h"
#include <cmath>

static int calculateDamage(int level, int power, int attack, int defense, float modifier) {
    float base = ((2.0f * level / 5.0f + 2.0f) * power * attack / defense) / 50.0f + 2.0f;
    return static_cast<int>(std::lround(base * modifier));
}

TEST(DamageTest, PhysicalDamageWithTypeEffectiveness) {
    Species attackerSpecies{
        1,
        "Attacker",
        Type::Fire,
        Type::Count,
        {45, 100, 50, 30, 40, 60},
        {EggGroup::Field},
        {},
        0.5f,
        -1,
        0,
        {AbilityType::Blaze}
    };

    Species defenderSpecies{
        2,
        "Defender",
        Type::Grass,
        Type::Poison,
        {50, 49, 49, 65, 65, 45},
        {EggGroup::Field},
        {},
        0.5f,
        -1,
        0,
        {AbilityType::Overgrow}
    };

    std::array<int, static_cast<int>(StatIndex::Count)> ivs{};
    std::array<int, static_cast<int>(StatIndex::Count)> evs{};
    ivs.fill(0);
    evs.fill(0);

    Pokemon attacker(&attackerSpecies, Nature::Brave, AbilityType::Blaze, 50, ivs, evs);
    Pokemon defender(&defenderSpecies, Nature::Bold, AbilityType::Overgrow, 50, ivs, evs);

    int power = 50;
    float modifier = defender.getTypeEffectiveness(Type::Fire);

    EXPECT_FLOAT_EQ(attacker.getAttack(), 115);
    EXPECT_FLOAT_EQ(defender.getDefense(), 59);
    EXPECT_FLOAT_EQ(modifier, 2.0f);

    int expected = calculateDamage(attacker.getLevel(), power, attacker.getAttack(), defender.getDefense(), modifier);
    EXPECT_EQ(expected, 90);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}