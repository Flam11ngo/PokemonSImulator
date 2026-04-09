#include <gtest/gtest.h>

#include "Battle/Abilities.h"
#include "Battle/Battle.h"
#include "Battle/BattleQueue.h"
#include "Battle/BuildFromJson.h"
#include "Battle/Moves.h"
#include "Battle/PRNG.h"

namespace {
Species makeSpecies(int id, const std::string& name, Type t1, Type t2, AbilityType ability, AbilityType hidden) {
    Species s;
    s.id = id;
    s.name = name;
    s.height = 10;
    s.weight = 100;
    s.type1 = t1;
    s.type2 = t2;
    s.baseStats = {100, 100, 100, 100, 100, 100};
    s.maleRatio = 0.5f;
    s.nextEvolutionID = -1;
    s.evolutionLevel = 0;
    s.abilities = {ability};
    s.hiddenAbility = hidden;
    return s;
}

Pokemon makePokemon(const Species& species, AbilityType ability) {
    std::array<int, static_cast<int>(StatIndex::Count)> ivs{};
    std::array<int, static_cast<int>(StatIndex::Count)> evs{};
    ivs.fill(31);
    evs.fill(0);
    return Pokemon(species, Nature::Hardy, ability, false, 50, ivs, evs);
}
}  // namespace

TEST(MoveDataTest, ProtectMoveLoadedWithExpectedEffect) {
    MoveData protect = getMoveDataByName("protect");
    EXPECT_EQ(protect.apiName, "protect");
    EXPECT_EQ(protect.effect, MoveEffect::Safeguard);
    EXPECT_EQ(protect.category, Category::Status);
}

TEST(MoveDataTest, QuickAttackLoadedWithPriorityAndTarget) {
    MoveData quickAttack = getMoveDataByName("Quick Attack");
    EXPECT_EQ(quickAttack.priority, 1);
    EXPECT_EQ(quickAttack.target, Target::Opponent);

    Move move = createMoveByName("Quick Attack");
    EXPECT_EQ(move.getPriority(), 1);
    EXPECT_EQ(move.getTarget(), Target::Opponent);
}

TEST(MoveDataTest, DragonDanceLoadedWithSelfTarget) {
    MoveData dragonDance = getMoveDataByName("Dragon Dance");
    EXPECT_EQ(dragonDance.priority, 0);
    EXPECT_EQ(dragonDance.target, Target::Self);

    Move move = createMoveByName("Dragon Dance");
    EXPECT_EQ(move.getPriority(), 0);
    EXPECT_EQ(move.getTarget(), Target::Self);
}

TEST(AbilityDataTest, BlazeNameMapsToBlazeType) {
    AbilityData blaze = getAbilityDataByName("blaze");
    EXPECT_EQ(blaze.type, AbilityType::Blaze);
    EXPECT_FALSE(blaze.name.empty());
}

TEST(AbilityDataTest, AbilityFactoryUsesCanonicalAbilityData) {
    AbilityData intimidateData = getAbilityData(AbilityType::Intimidate);
    Ability ability = getAbility(AbilityType::Intimidate);

    EXPECT_EQ(ability.getType(), AbilityType::Intimidate);
    EXPECT_EQ(ability.getName(), intimidateData.name);
    EXPECT_TRUE(ability.hasTrigger(Trigger::OnEntry));
}

TEST(BuildFromJsonTest, CharizardBuildHasFourMoves) {
    Pokemon p = BuildFromJson::loadPokemonFromFile("data/charizard.json");
    EXPECT_EQ(p.getMoves().size(), 4u);
}

TEST(BuildFromJsonTest, SelfContainedPokemonJsonLoadsPokemonInstance) {
        const std::string jsonText = R"json(
        {
            "species": {
                "id": 9001,
                "name": "Unitmon",
                "height": 10,
                "weight": 100,
                "type1": "fire",
                "type2": "count",
                "baseStats": [39, 52, 43, 60, 50, 65],
                "abilities": [66],
                "maleRatio": 0.5,
                "nextEvolutionID": -1,
                "evolutionLevel": 0
            },
            "level": 42,
            "nature": "adamant",
            "ability": "blaze",
            "item": {"name": "Leftovers"},
            "ivs": {
                "hp": 31,
                "attack": 31,
                "defense": 31,
                "specialAttack": 31,
                "specialDefense": 31,
                "speed": 31
            },
            "evs": {
                "hp": 0,
                "attack": 252,
                "defense": 0,
                "specialAttack": 0,
                "specialDefense": 0,
                "speed": 252
            },
            "moves": ["Quick Attack", {"name": "Pound"}, "Protect"]
        }
        )json";

        Pokemon p = BuildFromJson::loadPokemonFromString(jsonText);
        EXPECT_EQ(p.getSpecies().id, 9001);
        EXPECT_EQ(p.getLevel(), 42);
        EXPECT_EQ(p.getAbility(), AbilityType::Blaze);
        EXPECT_EQ(p.getItemType(), ItemType::Leftovers);
        ASSERT_EQ(p.getMoves().size(), 3u);
        EXPECT_EQ(p.getMoves()[0].getName(), "Quick Attack");
        EXPECT_EQ(p.getMoves()[1].getName(), "Pound");
        EXPECT_EQ(p.getMoves()[2].getName(), "Protect");
}

TEST(BattleLogicTest, ToxicPoisonEffectApplied) {
    PRNG::setSeed(42);

    Species sa = makeSpecies(1001, "Attacker", Type::Poison, Type::Count, AbilityType::None, AbilityType::None);
    Species sd = makeSpecies(1002, "Defender", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon attacker = makePokemon(sa, AbilityType::None);
    Pokemon defender = makePokemon(sd, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);

    Move toxic("Toxic", Type::Poison, Category::Status, 0, 100, 10, MoveEffect::Poison, 100, 2, 0);
    battle.processMoveEffects(&attacker, &defender, toxic);

    EXPECT_TRUE(defender.hasStatus(StatusType::ToxicPoison));
}

TEST(BattleLogicTest, IntimidateLowersOpponentAttackStageOnEntry) {
    PRNG::setSeed(7);

    Species si = makeSpecies(2001, "Intimidator", Type::Normal, Type::Count, AbilityType::Intimidate, AbilityType::None);
    Species so = makeSpecies(2002, "Opponent", Type::Normal, Type::Count, AbilityType::Blaze, AbilityType::None);

    Pokemon intimidator = makePokemon(si, AbilityType::Intimidate);
    Pokemon opponent = makePokemon(so, AbilityType::Blaze);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&intimidator));
    ASSERT_TRUE(sideB.addPokemon(&opponent));

    EXPECT_EQ(opponent.getStatStage(StatIndex::Attack), 0);
    Battle battle(sideA, sideB);
    (void)battle;
    EXPECT_EQ(opponent.getStatStage(StatIndex::Attack), -1);
}

TEST(BattleLogicTest, MovePriorityOutranksSpeedInQueue) {
    Species fastSpecies = makeSpecies(3001, "Fastmon", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species slowSpecies = makeSpecies(3002, "Slowmon", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    fastSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 120;
    slowSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 30;

    Pokemon fastPokemon = makePokemon(fastSpecies, AbilityType::None);
    Pokemon slowPokemon = makePokemon(slowSpecies, AbilityType::None);

    Move quickAttack = createMoveByName("Quick Attack");
    Move pound = createMoveByName("Pound");

    BattleQueue queue;
    queue.push(BattleAction::makeAttack(&slowPokemon, &fastPokemon, pound));
    queue.push(BattleAction::makeAttack(&fastPokemon, &slowPokemon, quickAttack));

    BattleAction first = queue.pop();
    BattleAction second = queue.pop();

    EXPECT_EQ(first.move.getName(), "Quick Attack");
    EXPECT_EQ(first.movePriority, 1);
    EXPECT_EQ(second.move.getName(), "Pound");
    EXPECT_EQ(second.movePriority, 0);
}

TEST(BattleLogicTest, DragonDanceRaisesAttackAndSpeed) {
    PRNG::setSeed(21);

    Species dragonSpecies = makeSpecies(4001, "DragonUser", Type::Dragon, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(4002, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon attacker = makePokemon(dragonSpecies, AbilityType::None);
    Pokemon defender = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move dragonDance = createMoveByName("Dragon Dance");

    EXPECT_EQ(attacker.getStatStage(StatIndex::Attack), 0);
    EXPECT_EQ(attacker.getStatStage(StatIndex::Speed), 0);

    battle.processMoveEffects(&attacker, &defender, dragonDance);

    EXPECT_EQ(attacker.getStatStage(StatIndex::Attack), 1);
    EXPECT_EQ(attacker.getStatStage(StatIndex::Speed), 1);
}
