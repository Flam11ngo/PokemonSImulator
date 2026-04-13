#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>

#include "Battle/Abilities.h"
#include "Battle/Battle.h"
#include "Battle/BattleQueue.h"
#include "Battle/BattleSession.h"
#include "Battle/BuildFromJson.h"
#include "Battle/Moves.h"
#include "Battle/PRNG.h"
#include <nlohmann/json.hpp>

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

bool responseHasErrorFragment(const nlohmann::json& response, const std::string& fragment) {
    if (response.contains("error") && response["error"].is_string()) {
        if (response["error"].get<std::string>().find(fragment) != std::string::npos) {
            return true;
        }
    }
    if (response.contains("errors") && response["errors"].is_array()) {
        for (const auto& error : response["errors"]) {
            if (error.is_string() && error.get<std::string>().find(fragment) != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

nlohmann::json makeSessionPokemonJson(int speciesId, const std::string& abilityName) {
    return nlohmann::json{
        {"speciesID", speciesId},
        {"level", 50},
        {"nature", "hardy"},
        {"ability", abilityName},
        {"moves", nlohmann::json::array({4, 4, 4, 4})}
    };
}

nlohmann::json makeSessionPokemonJson(int speciesId,
                                     const std::string& abilityName,
                                     const nlohmann::json& moves) {
    return nlohmann::json{
        {"speciesID", speciesId},
        {"level", 50},
        {"nature", "hardy"},
        {"ability", abilityName},
        {"moves", moves}
    };
}

int countSpecialEventsByReason(const std::string& reason) {
    std::ifstream in("cache/event.json");
    if (!in.is_open()) {
        return 0;
    }

    nlohmann::json events = nlohmann::json::parse(in, nullptr, false);
    if (!events.is_array()) {
        return 0;
    }

    int count = 0;
    for (const auto& event : events) {
        if (!event.is_object() || !event.contains("details") || !event["details"].is_object()) {
            continue;
        }
        if (!event["details"].contains("reason") || !event["details"]["reason"].is_string()) {
            continue;
        }
        if (event["details"]["reason"].get<std::string>() == reason) {
            ++count;
        }
    }
    return count;
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

TEST(AbilityDataTest, AbilityFactoryPopulatesImmunityMetadata) {
    Ability levitate = getAbility(AbilityType::Levitate);
    ASSERT_EQ(levitate.typeImmunities.size(), 1u);
    EXPECT_EQ(levitate.typeImmunities[0].typeId, static_cast<int>(Type::Ground));
    EXPECT_FALSE(levitate.typeImmunities[0].healInstead);

    Ability waterAbsorb = getAbility(AbilityType::WaterAbsorb);
    ASSERT_EQ(waterAbsorb.typeImmunities.size(), 1u);
    EXPECT_EQ(waterAbsorb.typeImmunities[0].typeId, static_cast<int>(Type::Water));
    EXPECT_TRUE(waterAbsorb.typeImmunities[0].healInstead);
    EXPECT_EQ(waterAbsorb.typeImmunities[0].healPercent, 25);

    Ability insomnia = getAbility(AbilityType::Insomnia);
    ASSERT_EQ(insomnia.statusImmunities.size(), 1u);
    EXPECT_EQ(insomnia.statusImmunities[0].statusId, static_cast<int>(StatusType::Sleep));

    Ability immunity = getAbility(AbilityType::Immunity);
    ASSERT_EQ(immunity.statusImmunities.size(), 2u);
    EXPECT_EQ(immunity.statusImmunities[0].statusId, static_cast<int>(StatusType::Poison));
    EXPECT_EQ(immunity.statusImmunities[1].statusId, static_cast<int>(StatusType::ToxicPoison));
}

TEST(AbilityDataTest, AbilityFactoryPopulatesModifierAndTriggerMetadata) {
    Ability hugePower = getAbility(AbilityType::HugePower);
    ASSERT_EQ(hugePower.statModifiers.size(), 1u);
    EXPECT_EQ(hugePower.statModifiers[0].stat, StatModifier::Attack);
    EXPECT_FLOAT_EQ(hugePower.statModifiers[0].multiplier, 2.0f);

    Ability filter = getAbility(AbilityType::Filter);
    EXPECT_FLOAT_EQ(filter.damageModifier.multiplier, 0.75f);
    EXPECT_FALSE(filter.damageModifier.onDealDamage);

    Ability moxie = getAbility(AbilityType::Moxie);
    EXPECT_TRUE(moxie.hasTrigger(Trigger::OnFaint));

    Species s = makeSpecies(9991, "MoxieUser", Type::Normal, Type::Count, AbilityType::Moxie, AbilityType::None);
    Pokemon user = makePokemon(s, AbilityType::Moxie);
    EXPECT_EQ(user.getStatStage(StatIndex::Attack), 0);
    moxie.executeTrigger(Trigger::OnFaint, &user, nullptr, nullptr);
    EXPECT_EQ(user.getStatStage(StatIndex::Attack), 1);
}

TEST(AbilityDataTest, AbilityFactoryPopulatesEntryAndExitTriggers) {
    Ability regenerator = getAbility(AbilityType::Regenerator);
    Ability naturalCure = getAbility(AbilityType::NaturalCure);
    Ability innerFocus = getAbility(AbilityType::InnerFocus);
    Ability drizzle = getAbility(AbilityType::Drizzle);
    Ability grassySurge = getAbility(AbilityType::GrassySurge);

    EXPECT_TRUE(regenerator.hasTrigger(Trigger::OnExit));
    EXPECT_TRUE(naturalCure.hasTrigger(Trigger::OnExit));
    EXPECT_FALSE(innerFocus.hasTrigger(Trigger::OnExit));
    EXPECT_TRUE(drizzle.hasTrigger(Trigger::OnEntry));
    EXPECT_TRUE(grassySurge.hasTrigger(Trigger::OnEntry));
}

TEST(AbilityDataTest, EntryAndExitAbilityTriggersMutateBattleState) {
    Species sourceSpecies = makeSpecies(9995, "TriggerSource", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(9996, "TriggerTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon source = makePokemon(sourceSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&source));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);

    Ability drizzle = getAbility(AbilityType::Drizzle);
    drizzle.executeTrigger(Trigger::OnEntry, &source, &target, &battle);
    EXPECT_EQ(battle.getWeather().type, WeatherType::Rain);
    EXPECT_TRUE(battle.getWeather().isActive());

    Ability grassySurge = getAbility(AbilityType::GrassySurge);
    grassySurge.executeTrigger(Trigger::OnEntry, &source, &target, &battle);
    EXPECT_EQ(battle.getField().type, FieldType::Grassy);
    EXPECT_TRUE(battle.getField().isActive());

    source.setCurrentHP(source.getMaxHP() / 2);
    source.addStatus(StatusType::Poison);

    Ability regenerator = getAbility(AbilityType::Regenerator);
    Ability naturalCure = getAbility(AbilityType::NaturalCure);
    regenerator.executeTrigger(Trigger::OnExit, &source, nullptr, nullptr);
    EXPECT_GT(source.getCurrentHP(), source.getMaxHP() / 2);

    naturalCure.executeTrigger(Trigger::OnExit, &source, nullptr, nullptr);
    EXPECT_FALSE(source.hasStatus(StatusType::Poison));
}

TEST(AbilityDataTest, ResolveTypeImmunityUsesAbilityDefinition) {
    bool healInstead = false;
    int healPercent = 0;

    EXPECT_TRUE(resolveTypeImmunity(AbilityType::Levitate, Type::Ground, healInstead, healPercent));
    EXPECT_FALSE(healInstead);
    EXPECT_EQ(healPercent, 0);

    EXPECT_TRUE(resolveTypeImmunity(AbilityType::WaterAbsorb, Type::Water, healInstead, healPercent));
    EXPECT_TRUE(healInstead);
    EXPECT_EQ(healPercent, 25);

    EXPECT_FALSE(resolveTypeImmunity(AbilityType::WaterAbsorb, Type::Fire, healInstead, healPercent));
}

TEST(AbilityDataTest, ResolveStatusImmunityUsesAbilityDefinition) {
    EXPECT_TRUE(resolveStatusImmunity(AbilityType::Insomnia, StatusType::Sleep));
    EXPECT_TRUE(resolveStatusImmunity(AbilityType::Immunity, StatusType::Poison));
    EXPECT_TRUE(resolveStatusImmunity(AbilityType::InnerFocus, StatusType::Flinch));

    // Legacy simulator behavior: Vital Spirit also blocks paralysis.
    EXPECT_TRUE(resolveStatusImmunity(AbilityType::VitalSpirit, StatusType::Paralysis));
    EXPECT_FALSE(resolveStatusImmunity(AbilityType::None, StatusType::Sleep));
}

TEST(AbilityDataTest, AbilityFlagHelpersExposeBattleRoutingRules) {
    EXPECT_TRUE(abilitySuppressesWeather(AbilityType::CloudNine));
    EXPECT_FALSE(abilitySuppressesWeather(AbilityType::None));

    EXPECT_TRUE(abilityIgnoresSubstitute(AbilityType::Infiltrator));
    EXPECT_TRUE(abilityIgnoresScreens(AbilityType::Infiltrator));
    EXPECT_FALSE(abilityIgnoresSubstitute(AbilityType::None));
    EXPECT_FALSE(abilityIgnoresScreens(AbilityType::None));

    EXPECT_TRUE(abilityBlocksBerryConsumption(AbilityType::Unnerve));
    EXPECT_FALSE(abilityBlocksBerryConsumption(AbilityType::None));

    EXPECT_TRUE(abilityIgnoresIndirectDamage(AbilityType::MagicGuard));
    EXPECT_FALSE(abilityIgnoresIndirectDamage(AbilityType::None));

    EXPECT_TRUE(abilityCanTypeShift(AbilityType::Protean));
    EXPECT_TRUE(abilityCanTypeShift(AbilityType::Libero));
    EXPECT_FALSE(abilityCanTypeShift(AbilityType::None));

    EXPECT_TRUE(abilityIgnoresOpponentStatStages(AbilityType::Unaware));
    EXPECT_FALSE(abilityIgnoresOpponentStatStages(AbilityType::None));

    EXPECT_FLOAT_EQ(abilityStabBonusMultiplier(AbilityType::Adaptability), 2.0f);
    EXPECT_FLOAT_EQ(abilityStabBonusMultiplier(AbilityType::None), 1.5f);

    Move boostedFlinch("BoostedFlinch", Type::Normal, Category::Physical, 60, 100, 10, MoveEffect::Flinch, 100);
    Move statusMove("StatusMove", Type::Normal, Category::Status, 0, 100, 10, MoveEffect::None, 0);
    EXPECT_TRUE(abilitySuppressesSecondaryEffects(AbilityType::SheerForce, boostedFlinch, true));
    EXPECT_FALSE(abilitySuppressesSecondaryEffects(AbilityType::SheerForce, statusMove, true));
    EXPECT_FALSE(abilitySuppressesSecondaryEffects(AbilityType::None, boostedFlinch, true));

    EXPECT_EQ(abilityStatusMovePriorityBonus(AbilityType::Prankster), 1);
    EXPECT_EQ(abilityStatusMovePriorityBonus(AbilityType::None), 0);

    EXPECT_TRUE(abilityBlocksGenericStatDrops(AbilityType::ClearBody));
    EXPECT_TRUE(abilityBlocksGenericStatDrops(AbilityType::WhiteSmoke));
    EXPECT_FALSE(abilityBlocksGenericStatDrops(AbilityType::None));

    EXPECT_TRUE(abilityBlocksAttackDrops(AbilityType::HyperCutter));
    EXPECT_TRUE(abilityBlocksAccuracyDrops(AbilityType::ClearBody));
    EXPECT_TRUE(abilityBlocksEvasionDrops(AbilityType::KeenEye));

    EXPECT_TRUE(abilityReflectsStatDrops(AbilityType::MirrorArmor));
    EXPECT_FALSE(abilityReflectsStatDrops(AbilityType::None));

    EXPECT_TRUE(abilityLowersOpponentPhysicalAttackAura(AbilityType::TabletsOfRuin));
    EXPECT_TRUE(abilityLowersOpponentSpecialAttackAura(AbilityType::VesselOfRuin));
    EXPECT_TRUE(abilityLowersOpponentDefenseAura(AbilityType::SwordOfRuin));
    EXPECT_TRUE(abilityLowersOpponentSpecialDefenseAura(AbilityType::BeadsOfRuin));
    EXPECT_FALSE(abilityLowersOpponentPhysicalAttackAura(AbilityType::None));

    EXPECT_TRUE(abilityGrantsGroundHazardImmunity(AbilityType::Levitate));
    EXPECT_FALSE(abilityGrantsGroundHazardImmunity(AbilityType::None));

    EXPECT_EQ(abilityTypeImmunityEventReason(AbilityType::WaterAbsorb), "water_absorb");
    EXPECT_EQ(abilityTypeImmunityEventReason(AbilityType::VoltAbsorb), "volt_absorb");
    EXPECT_EQ(abilityTypeImmunityEventReason(AbilityType::Levitate), "ability_immunity");
}

TEST(AbilityDataTest, ApplyTypeImmunityBonusUsesAbilityDefinition) {
    Species sapSpecies = makeSpecies(9992, "SapTarget", Type::Normal, Type::Count, AbilityType::SapSipper, AbilityType::None);
    Species stormSpecies = makeSpecies(9993, "StormTarget", Type::Normal, Type::Count, AbilityType::StormDrain, AbilityType::None);
    Species motorSpecies = makeSpecies(9994, "MotorTarget", Type::Normal, Type::Count, AbilityType::MotorDrive, AbilityType::None);

    Pokemon sapTarget = makePokemon(sapSpecies, AbilityType::SapSipper);
    Pokemon stormTarget = makePokemon(stormSpecies, AbilityType::StormDrain);
    Pokemon motorTarget = makePokemon(motorSpecies, AbilityType::MotorDrive);

    applyTypeImmunityBonus(AbilityType::SapSipper, &sapTarget);
    applyTypeImmunityBonus(AbilityType::StormDrain, &stormTarget);
    applyTypeImmunityBonus(AbilityType::MotorDrive, &motorTarget);

    EXPECT_EQ(sapTarget.getStatStage(StatIndex::Attack), 1);
    EXPECT_EQ(stormTarget.getStatStage(StatIndex::SpecialAttack), 1);
    EXPECT_EQ(motorTarget.getStatStage(StatIndex::Speed), 1);
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

TEST(BattleLogicTest, QueueTieUsesInsertionOrderForDeterminism) {
    Species firstSpecies = makeSpecies(3003, "Zedmon", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species secondSpecies = makeSpecies(3004, "Aammon", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    firstSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 80;
    secondSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 80;

    Pokemon firstPokemon = makePokemon(firstSpecies, AbilityType::None);
    Pokemon secondPokemon = makePokemon(secondSpecies, AbilityType::None);
    Move pound = createMoveByName("Pound");

    BattleQueue queue;
    queue.push(BattleAction::makeAttack(&firstPokemon, &secondPokemon, pound));
    queue.push(BattleAction::makeAttack(&secondPokemon, &firstPokemon, pound));

    const BattleAction first = queue.pop();
    const BattleAction second = queue.pop();

    EXPECT_EQ(first.actor, &firstPokemon);
    EXPECT_EQ(second.actor, &secondPokemon);
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

TEST(AbilityBehaviorTest, InsomniaBlocksSleepStatus) {
    Species sleeperSpecies = makeSpecies(5001, "Sleeper", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(5002, "Target", Type::Normal, Type::Count, AbilityType::Insomnia, AbilityType::None);

    Pokemon attacker = makePokemon(sleeperSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::Insomnia);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move sleepMove("Sleep Powder", Type::Grass, Category::Status, 0, 100, 15, MoveEffect::Sleep, 100);

    battle.processMoveEffects(&attacker, &target, sleepMove);

    EXPECT_FALSE(target.hasStatus(StatusType::Sleep));
}

TEST(AbilityBehaviorTest, GutsBoostsPhysicalDamageWhileStatused) {
    PRNG::setSeed(77);

    Species attackerSpecies = makeSpecies(5003, "GutsUser", Type::Fighting, Type::Count, AbilityType::Guts, AbilityType::None);
    Species defenderSpecies = makeSpecies(5004, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon gutsUser = makePokemon(attackerSpecies, AbilityType::Guts);
    Pokemon controlUser = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(defenderSpecies, AbilityType::None);
    gutsUser.addStatus(StatusType::Poison);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&gutsUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move closeCombat("Close Combat", Type::Fighting, Category::Physical, 120, 100, 5, MoveEffect::None, 100);

    PRNG::setSeed(78);
    const int gutsDamage = battle.calculateDamage(&gutsUser, &target, closeCombat);

    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlUser));
    ASSERT_TRUE(controlSideB.addPokemon(&target));
    Battle controlBattle(controlSideA, controlSideB);

    PRNG::setSeed(78);
    const int controlDamage = controlBattle.calculateDamage(&controlUser, &target, closeCombat);

    EXPECT_GT(gutsDamage, controlDamage);
}

TEST(AbilityBehaviorTest, HugePowerBoostsPhysicalDamage) {
    PRNG::setSeed(81);

    Species attackerSpecies = makeSpecies(5005, "HugePowerUser", Type::Normal, Type::Count, AbilityType::HugePower, AbilityType::None);
    Species defenderSpecies = makeSpecies(5006, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon hugePowerUser = makePokemon(attackerSpecies, AbilityType::HugePower);
    Pokemon controlUser = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(defenderSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&hugePowerUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");

    PRNG::setSeed(82);
    const int hugePowerDamage = battle.calculateDamage(&hugePowerUser, &target, pound);

    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlUser));
    ASSERT_TRUE(controlSideB.addPokemon(&target));
    Battle controlBattle(controlSideA, controlSideB);

    PRNG::setSeed(82);
    const int controlDamage = controlBattle.calculateDamage(&controlUser, &target, pound);

    EXPECT_GT(hugePowerDamage, controlDamage);
}

TEST(AbilityBehaviorTest, ThickFatReducesFireDamage) {
    PRNG::setSeed(91);

    Species attackerSpecies = makeSpecies(5007, "FireUser", Type::Fire, Type::Count, AbilityType::None, AbilityType::None);
    Species thickFatSpecies = makeSpecies(5008, "ThickFatTarget", Type::Normal, Type::Count, AbilityType::ThickFat, AbilityType::None);
    Species normalSpecies = makeSpecies(5009, "NormalTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon thickFatTarget = makePokemon(thickFatSpecies, AbilityType::ThickFat);
    Pokemon normalTarget = makePokemon(normalSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&thickFatTarget));

    Battle battle(sideA, sideB);
    Move fireMove("Flamethrower", Type::Fire, Category::Special, 90, 100, 15, MoveEffect::Burn, 10);

    PRNG::setSeed(92);
    const int reducedDamage = battle.calculateDamage(&attacker, &thickFatTarget, fireMove);

    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&attacker));
    ASSERT_TRUE(controlSideB.addPokemon(&normalTarget));
    Battle controlBattle(controlSideA, controlSideB);

    PRNG::setSeed(92);
    const int normalDamage = controlBattle.calculateDamage(&attacker, &normalTarget, fireMove);

    EXPECT_LT(reducedDamage, normalDamage);
}

TEST(AbilityBehaviorTest, SapSipperAbsorbsGrassAndRaisesAttack) {
    PRNG::setSeed(101);

    Species attackerSpecies = makeSpecies(5010, "GrassUser", Type::Grass, Type::Count, AbilityType::None, AbilityType::None);
    Species sapSpecies = makeSpecies(5011, "SapTarget", Type::Normal, Type::Count, AbilityType::SapSipper, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon sapTarget = makePokemon(sapSpecies, AbilityType::SapSipper);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&sapTarget));

    Battle battle(sideA, sideB);
    Move grassMove("Giga Drain", Type::Grass, Category::Special, 75, 100, 10, MoveEffect::Drain, 50);

    const int attackBefore = sapTarget.getStatStage(StatIndex::Attack);
    const int hpBefore = sapTarget.getCurrentHP();
    const int damage = battle.calculateDamage(&attacker, &sapTarget, grassMove);

    EXPECT_EQ(damage, 0);
    EXPECT_EQ(sapTarget.getCurrentHP(), hpBefore);
    EXPECT_EQ(sapTarget.getStatStage(StatIndex::Attack), attackBefore + 1);
}

TEST(AbilityBehaviorTest, TechnicianBoostsLowPowerMoves) {
    PRNG::setSeed(111);

    Species attackerSpecies = makeSpecies(5012, "TechUser", Type::Normal, Type::Count, AbilityType::Technician, AbilityType::None);
    Species defenderSpecies = makeSpecies(5013, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon technicianUser = makePokemon(attackerSpecies, AbilityType::Technician);
    Pokemon controlUser = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(defenderSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&technicianUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move lowPowerMove("Bite", Type::Dark, Category::Physical, 60, 100, 25, MoveEffect::Flinch, 30);

    PRNG::setSeed(112);
    const int technicianDamage = battle.calculateDamage(&technicianUser, &target, lowPowerMove);

    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlUser));
    ASSERT_TRUE(controlSideB.addPokemon(&target));
    Battle controlBattle(controlSideA, controlSideB);

    PRNG::setSeed(112);
    const int controlDamage = controlBattle.calculateDamage(&controlUser, &target, lowPowerMove);

    EXPECT_GT(technicianDamage, controlDamage);
}

TEST(AbilityBehaviorTest, FilterReducesSuperEffectiveDamage) {
    PRNG::setSeed(121);

    Species attackerSpecies = makeSpecies(5014, "WaterUser", Type::Water, Type::Count, AbilityType::None, AbilityType::None);
    Species filterSpecies = makeSpecies(5015, "FilterTarget", Type::Fire, Type::Rock, AbilityType::Filter, AbilityType::None);
    Species normalSpecies = makeSpecies(5016, "NormalTarget", Type::Fire, Type::Rock, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon filterTarget = makePokemon(filterSpecies, AbilityType::Filter);
    Pokemon normalTarget = makePokemon(normalSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&filterTarget));

    Battle battle(sideA, sideB);
    Move surf("Surf", Type::Water, Category::Special, 90, 100, 15, MoveEffect::None, 100);

    PRNG::setSeed(122);
    const int reducedDamage = battle.calculateDamage(&attacker, &filterTarget, surf);

    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&attacker));
    ASSERT_TRUE(controlSideB.addPokemon(&normalTarget));
    Battle controlBattle(controlSideA, controlSideB);

    PRNG::setSeed(122);
    const int normalDamage = controlBattle.calculateDamage(&attacker, &normalTarget, surf);

    EXPECT_LT(reducedDamage, normalDamage);
}

TEST(AbilityBehaviorTest, SolidRockReducesSuperEffectiveDamage) {
    PRNG::setSeed(131);

    Species attackerSpecies = makeSpecies(5017, "WaterUser", Type::Water, Type::Count, AbilityType::None, AbilityType::None);
    Species solidRockSpecies = makeSpecies(5018, "SolidRockTarget", Type::Fire, Type::Rock, AbilityType::SolidRock, AbilityType::None);
    Species normalSpecies = makeSpecies(5019, "NormalTarget", Type::Fire, Type::Rock, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon solidRockTarget = makePokemon(solidRockSpecies, AbilityType::SolidRock);
    Pokemon normalTarget = makePokemon(normalSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&solidRockTarget));

    Battle battle(sideA, sideB);
    Move surf("Surf", Type::Water, Category::Special, 90, 100, 15, MoveEffect::None, 100);

    PRNG::setSeed(132);
    const int reducedDamage = battle.calculateDamage(&attacker, &solidRockTarget, surf);

    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&attacker));
    ASSERT_TRUE(controlSideB.addPokemon(&normalTarget));
    Battle controlBattle(controlSideA, controlSideB);

    PRNG::setSeed(132);
    const int normalDamage = controlBattle.calculateDamage(&attacker, &normalTarget, surf);

    EXPECT_LT(reducedDamage, normalDamage);
}

TEST(AbilityBehaviorTest, MoxieRaisesAttackAfterKnockout) {
    PRNG::setSeed(141);

    Species attackerSpecies = makeSpecies(5020, "MoxieUser", Type::Dark, Type::Count, AbilityType::Moxie, AbilityType::None);
    Species defenderSpecies = makeSpecies(5021, "FragileTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::Moxie);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);
    defender.setCurrentHP(1);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");

    EXPECT_EQ(attacker.getStatStage(StatIndex::Attack), 0);
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, pound));
    battle.processTurn();
    EXPECT_TRUE(defender.isFainted());
    EXPECT_EQ(attacker.getStatStage(StatIndex::Attack), 1);
}

TEST(AbilityBehaviorTest, InnerFocusBlocksFlinch) {
    Species attackerSpecies = makeSpecies(5022, "FlinchUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(5023, "InnerFocusTarget", Type::Normal, Type::Count, AbilityType::InnerFocus, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::InnerFocus);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move flinchMove("Fake Out", Type::Normal, Category::Physical, 40, 100, 10, MoveEffect::Flinch, 100);

    battle.processMoveEffects(&attacker, &defender, flinchMove);
    EXPECT_FALSE(defender.hasStatus(StatusType::Flinch));
}

TEST(AbilityBehaviorTest, RegeneratorHealsOnSwitchOut) {
    Species regenSpecies = makeSpecies(5024, "RegenUser", Type::Normal, Type::Count, AbilityType::Regenerator, AbilityType::None);
    Species benchSpecies = makeSpecies(5025, "Bench", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species foeSpecies = makeSpecies(5026, "Foe", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon regeneratorUser = makePokemon(regenSpecies, AbilityType::Regenerator);
    Pokemon bench = makePokemon(benchSpecies, AbilityType::None);
    Pokemon foe = makePokemon(foeSpecies, AbilityType::None);

    regeneratorUser.setCurrentHP(regeneratorUser.getMaxHP() / 2);
    const int hpBefore = regeneratorUser.getCurrentHP();
    const int expectedHeal = std::max(1, regeneratorUser.getMaxHP() / 3);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&regeneratorUser));
    ASSERT_TRUE(sideA.addPokemon(&bench));
    ASSERT_TRUE(sideB.addPokemon(&foe));

    Battle battle(sideA, sideB);
    const bool switched = battle.switchPokemon(battle.getSideA(), 1);

    ASSERT_TRUE(switched);
    EXPECT_EQ(regeneratorUser.getCurrentHP(), std::min(regeneratorUser.getMaxHP(), hpBefore + expectedHeal));
}

TEST(AbilityBehaviorTest, NaturalCureClearsStatusOnSwitchOut) {
    Species naturalCureSpecies = makeSpecies(5027, "CureUser", Type::Normal, Type::Count, AbilityType::NaturalCure, AbilityType::None);
    Species benchSpecies = makeSpecies(5028, "Bench", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species foeSpecies = makeSpecies(5029, "Foe", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon naturalCureUser = makePokemon(naturalCureSpecies, AbilityType::NaturalCure);
    Pokemon bench = makePokemon(benchSpecies, AbilityType::None);
    Pokemon foe = makePokemon(foeSpecies, AbilityType::None);

    naturalCureUser.addStatus(StatusType::Poison);
    ASSERT_TRUE(naturalCureUser.hasStatus(StatusType::Poison));

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&naturalCureUser));
    ASSERT_TRUE(sideA.addPokemon(&bench));
    ASSERT_TRUE(sideB.addPokemon(&foe));

    Battle battle(sideA, sideB);
    const bool switched = battle.switchPokemon(battle.getSideA(), 1);

    ASSERT_TRUE(switched);
    EXPECT_FALSE(naturalCureUser.hasStatus(StatusType::Poison));
}

TEST(AbilityBehaviorTest, MagicGuardPreventsWeatherChipDamage) {
    Species magicGuardSpecies = makeSpecies(5030, "MagicGuardUser", Type::Normal, Type::Count, AbilityType::MagicGuard, AbilityType::None);
    Species normalSpecies = makeSpecies(5031, "NormalUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon magicGuardUser = makePokemon(magicGuardSpecies, AbilityType::MagicGuard);
    Pokemon normalUser = makePokemon(normalSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&magicGuardUser));
    ASSERT_TRUE(sideB.addPokemon(&normalUser));

    Battle battle(sideA, sideB);
    battle.getWeather().setWeather(WeatherType::Sandstorm, 5);

    const int magicGuardHpBefore = magicGuardUser.getCurrentHP();
    const int normalHpBefore = normalUser.getCurrentHP();
    battle.processTurn();

    EXPECT_EQ(magicGuardUser.getCurrentHP(), magicGuardHpBefore);
    EXPECT_LT(normalUser.getCurrentHP(), normalHpBefore);
}

TEST(AbilityBehaviorTest, UnawareDefenderIgnoresAttackerBoosts) {
    PRNG::setSeed(151);

    Species attackerSpecies = makeSpecies(5032, "BoostedAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species unawareSpecies = makeSpecies(5033, "UnawareTarget", Type::Normal, Type::Count, AbilityType::Unaware, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon unawareTarget = makePokemon(unawareSpecies, AbilityType::Unaware);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&unawareTarget));
    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");

    PRNG::setSeed(152);
    const int baselineDamage = battle.calculateDamage(&attacker, &unawareTarget, pound);

    attacker.changeStatStage(StatIndex::Attack, 2);
    PRNG::setSeed(152);
    const int boostedDamage = battle.calculateDamage(&attacker, &unawareTarget, pound);

    EXPECT_EQ(boostedDamage, baselineDamage);
}

TEST(AbilityBehaviorTest, UnawareAttackerIgnoresDefenderBoosts) {
    PRNG::setSeed(161);

    Species unawareSpecies = makeSpecies(5034, "UnawareAttacker", Type::Normal, Type::Count, AbilityType::Unaware, AbilityType::None);
    Species controlSpecies = makeSpecies(5035, "ControlAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(5036, "BoostedDefender", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon unawareAttacker = makePokemon(unawareSpecies, AbilityType::Unaware);
    Pokemon controlAttacker = makePokemon(controlSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);
    defender.changeStatStage(StatIndex::Defense, 3);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&unawareAttacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));
    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");

    PRNG::setSeed(162);
    const int unawareDamage = battle.calculateDamage(&unawareAttacker, &defender, pound);

    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlAttacker));
    ASSERT_TRUE(controlSideB.addPokemon(&defender));
    Battle controlBattle(controlSideA, controlSideB);

    PRNG::setSeed(162);
    const int controlDamage = controlBattle.calculateDamage(&controlAttacker, &defender, pound);

    EXPECT_GT(unawareDamage, controlDamage);
}

TEST(AbilityBehaviorTest, PranksterGivesPriorityToStatusMoves) {
    Species pranksterSpecies = makeSpecies(5037, "PranksterUser", Type::Dark, Type::Count, AbilityType::Prankster, AbilityType::None);
    Species fastSpecies = makeSpecies(5038, "FastAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    pranksterSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 30;
    fastSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 130;

    Pokemon pranksterUser = makePokemon(pranksterSpecies, AbilityType::Prankster);
    Pokemon fastAttacker = makePokemon(fastSpecies, AbilityType::None);
    pranksterUser.setCurrentHP(1);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&pranksterUser));
    ASSERT_TRUE(sideB.addPokemon(&fastAttacker));

    Battle battle(sideA, sideB);
    Move trickRoom("Trick Room", Type::Psychic, Category::Status, 0, 100, 5, MoveEffect::None, 100);
    Move pound = createMoveByName("Pound");

    battle.enqueueAction(BattleAction::makeAttack(&pranksterUser, &fastAttacker, trickRoom));
    battle.enqueueAction(BattleAction::makeAttack(&fastAttacker, &pranksterUser, pound));
    battle.processTurn();

    EXPECT_TRUE(battle.getField().isTrickRoom());
    EXPECT_TRUE(pranksterUser.isFainted());
}

TEST(AbilityBehaviorTest, ClearBodyBlocksIntimidateAttackDrop) {
    Species intimidateSpecies = makeSpecies(5039, "Intimidator", Type::Normal, Type::Count, AbilityType::Intimidate, AbilityType::None);
    Species clearBodySpecies = makeSpecies(5040, "ClearBodyTarget", Type::Steel, Type::Count, AbilityType::ClearBody, AbilityType::None);

    Pokemon intimidator = makePokemon(intimidateSpecies, AbilityType::Intimidate);
    Pokemon clearBodyTarget = makePokemon(clearBodySpecies, AbilityType::ClearBody);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&intimidator));
    ASSERT_TRUE(sideB.addPokemon(&clearBodyTarget));

    Battle battle(sideA, sideB);

    EXPECT_EQ(clearBodyTarget.getStatStage(StatIndex::Attack), 0);
}

TEST(AbilityBehaviorTest, DefiantTriggersOnIntimidate) {
    Species intimidateSpecies = makeSpecies(5041, "Intimidator", Type::Normal, Type::Count, AbilityType::Intimidate, AbilityType::None);
    Species defiantSpecies = makeSpecies(5042, "DefiantTarget", Type::Dark, Type::Count, AbilityType::Defiant, AbilityType::None);

    Pokemon intimidator = makePokemon(intimidateSpecies, AbilityType::Intimidate);
    Pokemon defiantTarget = makePokemon(defiantSpecies, AbilityType::Defiant);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&intimidator));
    ASSERT_TRUE(sideB.addPokemon(&defiantTarget));

    Battle battle(sideA, sideB);

    EXPECT_EQ(defiantTarget.getStatStage(StatIndex::Attack), 1);
}

TEST(AbilityBehaviorTest, CompetitiveTriggersOnIntimidate) {
    Species intimidateSpecies = makeSpecies(5043, "Intimidator", Type::Normal, Type::Count, AbilityType::Intimidate, AbilityType::None);
    Species competitiveSpecies = makeSpecies(5044, "CompetitiveTarget", Type::Normal, Type::Count, AbilityType::Competitive, AbilityType::None);

    Pokemon intimidator = makePokemon(intimidateSpecies, AbilityType::Intimidate);
    Pokemon competitiveTarget = makePokemon(competitiveSpecies, AbilityType::Competitive);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&intimidator));
    ASSERT_TRUE(sideB.addPokemon(&competitiveTarget));

    Battle battle(sideA, sideB);

    EXPECT_EQ(competitiveTarget.getStatStage(StatIndex::Attack), -1);
    EXPECT_EQ(competitiveTarget.getStatStage(StatIndex::SpecialAttack), 2);
}

TEST(AbilityBehaviorTest, DefiantTriggersOnStatLoweringMoveEffect) {
    Species attackerSpecies = makeSpecies(5045, "DebuffUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defiantSpecies = makeSpecies(5046, "DefiantTarget", Type::Dark, Type::Count, AbilityType::Defiant, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defiantTarget = makePokemon(defiantSpecies, AbilityType::Defiant);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defiantTarget));

    Battle battle(sideA, sideB);
    Move growl("Growl", Type::Normal, Category::Status, 0, 100, 40, MoveEffect::StatChange, 100, 1, -1);
    battle.processMoveEffects(&attacker, &defiantTarget, growl);

    EXPECT_EQ(defiantTarget.getStatStage(StatIndex::Attack), 1);
}

TEST(AbilityBehaviorTest, WhiteSmokeBlocksIntimidateAttackDrop) {
    Species intimidateSpecies = makeSpecies(5047, "Intimidator", Type::Normal, Type::Count, AbilityType::Intimidate, AbilityType::None);
    Species whiteSmokeSpecies = makeSpecies(5048, "WhiteSmokeTarget", Type::Fire, Type::Count, AbilityType::WhiteSmoke, AbilityType::None);

    Pokemon intimidator = makePokemon(intimidateSpecies, AbilityType::Intimidate);
    Pokemon whiteSmokeTarget = makePokemon(whiteSmokeSpecies, AbilityType::WhiteSmoke);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&intimidator));
    ASSERT_TRUE(sideB.addPokemon(&whiteSmokeTarget));

    Battle battle(sideA, sideB);

    EXPECT_EQ(whiteSmokeTarget.getStatStage(StatIndex::Attack), 0);
}

TEST(AbilityBehaviorTest, HyperCutterBlocksIntimidateAttackDrop) {
    Species intimidateSpecies = makeSpecies(5049, "Intimidator", Type::Normal, Type::Count, AbilityType::Intimidate, AbilityType::None);
    Species hyperCutterSpecies = makeSpecies(5050, "HyperCutterTarget", Type::Bug, Type::Count, AbilityType::HyperCutter, AbilityType::None);

    Pokemon intimidator = makePokemon(intimidateSpecies, AbilityType::Intimidate);
    Pokemon hyperCutterTarget = makePokemon(hyperCutterSpecies, AbilityType::HyperCutter);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&intimidator));
    ASSERT_TRUE(sideB.addPokemon(&hyperCutterTarget));

    Battle battle(sideA, sideB);

    EXPECT_EQ(hyperCutterTarget.getStatStage(StatIndex::Attack), 0);
}

TEST(AbilityBehaviorTest, MirrorArmorReflectsIntimidateAttackDrop) {
    Species intimidateSpecies = makeSpecies(5051, "Intimidator", Type::Normal, Type::Count, AbilityType::Intimidate, AbilityType::None);
    Species mirrorArmorSpecies = makeSpecies(5052, "MirrorArmorTarget", Type::Flying, Type::Steel, AbilityType::MirrorArmor, AbilityType::None);

    Pokemon intimidator = makePokemon(intimidateSpecies, AbilityType::Intimidate);
    Pokemon mirrorArmorTarget = makePokemon(mirrorArmorSpecies, AbilityType::MirrorArmor);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&intimidator));
    ASSERT_TRUE(sideB.addPokemon(&mirrorArmorTarget));

    Battle battle(sideA, sideB);

    EXPECT_EQ(mirrorArmorTarget.getStatStage(StatIndex::Attack), 0);
    EXPECT_EQ(intimidator.getStatStage(StatIndex::Attack), -1);
}

TEST(AbilityBehaviorTest, KeenEyeBlocksSandAttackStyleEvasionRule) {
    Species attackerSpecies = makeSpecies(5053, "AccuracyDropUser", Type::Ground, Type::Count, AbilityType::None, AbilityType::None);
    Species keenEyeSpecies = makeSpecies(5054, "KeenEyeTarget", Type::Normal, Type::Count, AbilityType::KeenEye, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon keenEyeTarget = makePokemon(keenEyeSpecies, AbilityType::KeenEye);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&keenEyeTarget));

    Battle battle(sideA, sideB);
    Move sandAttack = createMoveByName("Sand Attack");
    const int beforeEvasion = keenEyeTarget.getEvasionStage();

    battle.processMoveEffects(&attacker, &keenEyeTarget, sandAttack);

    EXPECT_EQ(keenEyeTarget.getEvasionStage(), beforeEvasion);
}

TEST(AbilityBehaviorTest, DrizzleSetsRainOnEntry) {
    Species drizzleSpecies = makeSpecies(5055, "DrizzleUser", Type::Water, Type::Count, AbilityType::Drizzle, AbilityType::None);
    Species targetSpecies = makeSpecies(5056, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon drizzleUser = makePokemon(drizzleSpecies, AbilityType::Drizzle);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&drizzleUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    EXPECT_EQ(battle.getWeather().type, WeatherType::Rain);
    EXPECT_TRUE(battle.getWeather().isActive());
}

TEST(AbilityBehaviorTest, DroughtSetsSunOnEntry) {
    Species droughtSpecies = makeSpecies(5057, "DroughtUser", Type::Fire, Type::Count, AbilityType::Drought, AbilityType::None);
    Species targetSpecies = makeSpecies(5058, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon droughtUser = makePokemon(droughtSpecies, AbilityType::Drought);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&droughtUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    EXPECT_EQ(battle.getWeather().type, WeatherType::Sun);
    EXPECT_TRUE(battle.getWeather().isActive());
}

TEST(AbilityBehaviorTest, SandStreamSetsSandstormOnEntry) {
    Species sandSpecies = makeSpecies(5059, "SandUser", Type::Rock, Type::Count, AbilityType::SandStream, AbilityType::None);
    Species targetSpecies = makeSpecies(5060, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon sandUser = makePokemon(sandSpecies, AbilityType::SandStream);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&sandUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    EXPECT_EQ(battle.getWeather().type, WeatherType::Sandstorm);
    EXPECT_TRUE(battle.getWeather().isActive());
}

TEST(AbilityBehaviorTest, SnowWarningSetsSnowOnEntry) {
    Species snowSpecies = makeSpecies(5061, "SnowUser", Type::Ice, Type::Count, AbilityType::SnowWarning, AbilityType::None);
    Species targetSpecies = makeSpecies(5062, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon snowUser = makePokemon(snowSpecies, AbilityType::SnowWarning);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&snowUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    EXPECT_EQ(battle.getWeather().type, WeatherType::Snow);
    EXPECT_TRUE(battle.getWeather().isActive());
}

TEST(AbilityBehaviorTest, CloudNineSuppressesWeatherChipAndWeatherBallBoost) {
    PRNG::setSeed(2401);

    Species cloudNineSpecies = makeSpecies(5063, "CloudNineUser", Type::Normal, Type::Count, AbilityType::CloudNine, AbilityType::None);
    Species targetSpecies = makeSpecies(5064, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon cloudNineUser = makePokemon(cloudNineSpecies, AbilityType::CloudNine);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&cloudNineUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    battle.getWeather().setWeather(WeatherType::Sandstorm, 5);

    const int hpA = cloudNineUser.getCurrentHP();
    const int hpB = target.getCurrentHP();
    battle.processTurn();
    EXPECT_EQ(cloudNineUser.getCurrentHP(), hpA);
    EXPECT_EQ(target.getCurrentHP(), hpB);

    Move weatherBall("Weather Ball", Type::Normal, Category::Special, 50, 100, 10, MoveEffect::WeatherBall, 100);
    PRNG::setSeed(2402);
    const int cloudNineDamage = battle.calculateDamage(&cloudNineUser, &target, weatherBall);

    Side controlSideA("CA");
    Side controlSideB("CB");
    Pokemon controlAttacker = makePokemon(cloudNineSpecies, AbilityType::None);
    Pokemon controlTarget = makePokemon(targetSpecies, AbilityType::None);
    ASSERT_TRUE(controlSideA.addPokemon(&controlAttacker));
    ASSERT_TRUE(controlSideB.addPokemon(&controlTarget));
    Battle controlBattle(controlSideA, controlSideB);
    controlBattle.getWeather().setWeather(WeatherType::Sandstorm, 5);

    PRNG::setSeed(2402);
    const int weatherDamage = controlBattle.calculateDamage(&controlAttacker, &controlTarget, weatherBall);
    EXPECT_LT(cloudNineDamage, weatherDamage);
}

TEST(AbilityBehaviorTest, GrassySurgeSetsGrassyTerrainOnEntry) {
    Species surgeSpecies = makeSpecies(5065, "GrassyUser", Type::Grass, Type::Count, AbilityType::GrassySurge, AbilityType::None);
    Species targetSpecies = makeSpecies(5066, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon surgeUser = makePokemon(surgeSpecies, AbilityType::GrassySurge);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&surgeUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    EXPECT_TRUE(battle.getField().isActive());
    EXPECT_EQ(battle.getField().type, FieldType::Grassy);
}

TEST(AbilityBehaviorTest, ElectricSurgeSetsElectricTerrainOnEntry) {
    Species surgeSpecies = makeSpecies(5067, "ElectricUser", Type::Electric, Type::Count, AbilityType::ElectricSurge, AbilityType::None);
    Species targetSpecies = makeSpecies(5068, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon surgeUser = makePokemon(surgeSpecies, AbilityType::ElectricSurge);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&surgeUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    EXPECT_TRUE(battle.getField().isActive());
    EXPECT_EQ(battle.getField().type, FieldType::Electric);
}

TEST(AbilityBehaviorTest, PsychicSurgeSetsPsychicTerrainOnEntry) {
    Species surgeSpecies = makeSpecies(5069, "PsychicUser", Type::Psychic, Type::Count, AbilityType::PsychicSurge, AbilityType::None);
    Species targetSpecies = makeSpecies(5070, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon surgeUser = makePokemon(surgeSpecies, AbilityType::PsychicSurge);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&surgeUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    EXPECT_TRUE(battle.getField().isActive());
    EXPECT_EQ(battle.getField().type, FieldType::Psychic);
}

TEST(AbilityBehaviorTest, MistySurgeSetsMistyTerrainOnEntry) {
    Species surgeSpecies = makeSpecies(5071, "MistyUser", Type::Fairy, Type::Count, AbilityType::MistySurge, AbilityType::None);
    Species targetSpecies = makeSpecies(5072, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon surgeUser = makePokemon(surgeSpecies, AbilityType::MistySurge);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&surgeUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    EXPECT_TRUE(battle.getField().isActive());
    EXPECT_EQ(battle.getField().type, FieldType::Misty);
}

TEST(AbilityBehaviorTest, HadronEngineSetsElectricTerrainAndBoostsSpecialDamage) {
    PRNG::setSeed(2501);

    Species hadronSpecies = makeSpecies(5073, "HadronUser", Type::Electric, Type::Count, AbilityType::HadronEngine, AbilityType::None);
    Species targetSpecies = makeSpecies(5074, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon hadronUser = makePokemon(hadronSpecies, AbilityType::HadronEngine);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&hadronUser));
    ASSERT_TRUE(sideB.addPokemon(&target));
    Battle battle(sideA, sideB);

    EXPECT_TRUE(battle.getField().isActive());
    EXPECT_EQ(battle.getField().type, FieldType::Electric);

    Move thunderbolt("Thunderbolt", Type::Electric, Category::Special, 90, 100, 15, MoveEffect::Paralyze, 10);
    PRNG::setSeed(2502);
    const int hadronDamage = battle.calculateDamage(&hadronUser, &target, thunderbolt);

    Species controlSpecies = makeSpecies(5075, "ControlUser", Type::Electric, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon controlUser = makePokemon(controlSpecies, AbilityType::None);
    Pokemon controlTarget = makePokemon(targetSpecies, AbilityType::None);
    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlUser));
    ASSERT_TRUE(controlSideB.addPokemon(&controlTarget));
    Battle controlBattle(controlSideA, controlSideB);
    controlBattle.getField().setField(FieldType::Electric, 5);

    PRNG::setSeed(2502);
    const int controlDamage = controlBattle.calculateDamage(&controlUser, &controlTarget, thunderbolt);
    EXPECT_GT(hadronDamage, controlDamage);
}

TEST(AbilityBehaviorTest, ProteanChangesUsersTypeWhenAttacking) {
    PRNG::setSeed(2601);

    Species proteanSpecies = makeSpecies(5076, "ProteanUser", Type::Normal, Type::Count, AbilityType::Protean, AbilityType::None);
    Species targetSpecies = makeSpecies(5077, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon proteanUser = makePokemon(proteanSpecies, AbilityType::Protean);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&proteanUser));
    ASSERT_TRUE(sideB.addPokemon(&target));
    Battle battle(sideA, sideB);

    Move ember = createMoveByName("Ember");
    battle.enqueueAction(BattleAction::makeAttack(&proteanUser, &target, ember));
    battle.processTurn();

    EXPECT_EQ(proteanUser.getType1(), Type::Fire);
    EXPECT_EQ(proteanUser.getType2(), Type::Count);
}

TEST(AbilityBehaviorTest, LiberoChangesUsersTypeWhenAttacking) {
    PRNG::setSeed(2602);

    Species liberoSpecies = makeSpecies(5078, "LiberoUser", Type::Normal, Type::Count, AbilityType::Libero, AbilityType::None);
    Species targetSpecies = makeSpecies(5079, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon liberoUser = makePokemon(liberoSpecies, AbilityType::Libero);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&liberoUser));
    ASSERT_TRUE(sideB.addPokemon(&target));
    Battle battle(sideA, sideB);

    Move lowKick("Low Kick", Type::Fighting, Category::Physical, 60, 100, 20, MoveEffect::None, 100);
    battle.enqueueAction(BattleAction::makeAttack(&liberoUser, &target, lowKick));
    battle.processTurn();

    EXPECT_EQ(liberoUser.getType1(), Type::Fighting);
    EXPECT_EQ(liberoUser.getType2(), Type::Count);
}

TEST(AbilityBehaviorTest, AdaptabilityBoostsStabDamage) {
    PRNG::setSeed(2603);

    Species adaptSpecies = makeSpecies(5080, "AdaptUser", Type::Normal, Type::Count, AbilityType::Adaptability, AbilityType::None);
    Species targetSpecies = makeSpecies(5081, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon adaptUser = makePokemon(adaptSpecies, AbilityType::Adaptability);
    Pokemon controlUser = makePokemon(adaptSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Move pound = createMoveByName("Pound");

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&adaptUser));
    ASSERT_TRUE(sideB.addPokemon(&target));
    Battle battle(sideA, sideB);

    PRNG::setSeed(2604);
    const int adaptDamage = battle.calculateDamage(&adaptUser, &target, pound);

    Side controlSideA("CA");
    Side controlSideB("CB");
    Pokemon controlTarget = makePokemon(targetSpecies, AbilityType::None);
    ASSERT_TRUE(controlSideA.addPokemon(&controlUser));
    ASSERT_TRUE(controlSideB.addPokemon(&controlTarget));
    Battle controlBattle(controlSideA, controlSideB);

    PRNG::setSeed(2604);
    const int controlDamage = controlBattle.calculateDamage(&controlUser, &controlTarget, pound);
    EXPECT_GT(adaptDamage, controlDamage);
}

TEST(AbilityBehaviorTest, SheerForceBoostsDamageAndSuppressesSecondaryEffect) {
    PRNG::setSeed(2605);

    Species sheerSpecies = makeSpecies(5082, "SheerUser", Type::Normal, Type::Count, AbilityType::SheerForce, AbilityType::None);
    Species targetSpecies = makeSpecies(5083, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon sheerUser = makePokemon(sheerSpecies, AbilityType::SheerForce);
    Pokemon controlUser = makePokemon(sheerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Move flinchMove("TestFlinch", Type::Normal, Category::Physical, 60, 100, 10, MoveEffect::Flinch, 100);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&sheerUser));
    ASSERT_TRUE(sideB.addPokemon(&target));
    Battle battle(sideA, sideB);

    PRNG::setSeed(2606);
    const int sheerDamage = battle.calculateDamage(&sheerUser, &target, flinchMove);

    Side controlSideA("CA");
    Side controlSideB("CB");
    Pokemon controlTarget = makePokemon(targetSpecies, AbilityType::None);
    ASSERT_TRUE(controlSideA.addPokemon(&controlUser));
    ASSERT_TRUE(controlSideB.addPokemon(&controlTarget));
    Battle controlBattle(controlSideA, controlSideB);

    PRNG::setSeed(2606);
    const int controlDamage = controlBattle.calculateDamage(&controlUser, &controlTarget, flinchMove);
    EXPECT_GT(sheerDamage, controlDamage);

    battle.enqueueAction(BattleAction::makeAttack(&sheerUser, &target, flinchMove));
    battle.processTurn();
    EXPECT_FALSE(target.hasStatus(StatusType::Flinch));
}

TEST(AbilityBehaviorTest, InfiltratorBypassesSubstituteForDamage) {
    PRNG::setSeed(2607);

    Species infiltratorSpecies = makeSpecies(5084, "InfiltratorUser", Type::Normal, Type::Count, AbilityType::Infiltrator, AbilityType::None);
    Species targetSpecies = makeSpecies(5085, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon infiltratorUser = makePokemon(infiltratorSpecies, AbilityType::Infiltrator);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    target.setSubstituteHP(std::max(1, target.getMaxHP() / 4));
    const int substituteBefore = target.getSubstituteHP();
    const int hpBefore = target.getCurrentHP();

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&infiltratorUser));
    ASSERT_TRUE(sideB.addPokemon(&target));
    Battle battle(sideA, sideB);

    Move pound = createMoveByName("Pound");
    battle.enqueueAction(BattleAction::makeAttack(&infiltratorUser, &target, pound));
    battle.processTurn();

    EXPECT_LT(target.getCurrentHP(), hpBefore);
    EXPECT_EQ(target.getSubstituteHP(), substituteBefore);
}

TEST(AbilityBehaviorTest, BeadsOfRuinLowersOpponentsSpecialDefenseForDamage) {
    PRNG::setSeed(2701);

    Species beadsSpecies = makeSpecies(5086, "BeadsUser", Type::Dark, Type::Count, AbilityType::BeadsOfRuin, AbilityType::None);
    Species attackerSpecies = makeSpecies(5087, "SpecialAttacker", Type::Fire, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(5088, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon beadsUser = makePokemon(beadsSpecies, AbilityType::BeadsOfRuin);
    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Move ember = createMoveByName("Ember");

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));
    ASSERT_TRUE(sideB.addPokemon(&beadsUser));
    Battle battle(sideA, sideB);
    ASSERT_TRUE(battle.switchPokemon(battle.getSideB(), 1));

    PRNG::setSeed(2702);
    const int beadsDamage = battle.calculateDamage(&attacker, &target, ember);

    Species controlSpecies = makeSpecies(5089, "Control", Type::Dark, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon controlMon = makePokemon(controlSpecies, AbilityType::None);
    Pokemon controlAttacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon controlTarget = makePokemon(targetSpecies, AbilityType::None);
    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlAttacker));
    ASSERT_TRUE(controlSideB.addPokemon(&controlTarget));
    ASSERT_TRUE(controlSideB.addPokemon(&controlMon));
    Battle controlBattle(controlSideA, controlSideB);
    ASSERT_TRUE(controlBattle.switchPokemon(controlBattle.getSideB(), 1));

    PRNG::setSeed(2702);
    const int controlDamage = controlBattle.calculateDamage(&controlAttacker, &controlTarget, ember);
    EXPECT_GT(beadsDamage, controlDamage);
}

TEST(AbilityBehaviorTest, SwordOfRuinLowersOpponentsDefenseForDamage) {
    PRNG::setSeed(2703);

    Species swordSpecies = makeSpecies(5090, "SwordUser", Type::Dark, Type::Count, AbilityType::SwordOfRuin, AbilityType::None);
    Species attackerSpecies = makeSpecies(5091, "PhysicalAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(5092, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon swordUser = makePokemon(swordSpecies, AbilityType::SwordOfRuin);
    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Move pound = createMoveByName("Pound");

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));
    ASSERT_TRUE(sideB.addPokemon(&swordUser));
    Battle battle(sideA, sideB);
    ASSERT_TRUE(battle.switchPokemon(battle.getSideB(), 1));

    PRNG::setSeed(2704);
    const int swordDamage = battle.calculateDamage(&attacker, &target, pound);

    Species controlSpecies = makeSpecies(5093, "Control", Type::Dark, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon controlMon = makePokemon(controlSpecies, AbilityType::None);
    Pokemon controlAttacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon controlTarget = makePokemon(targetSpecies, AbilityType::None);
    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlAttacker));
    ASSERT_TRUE(controlSideB.addPokemon(&controlTarget));
    ASSERT_TRUE(controlSideB.addPokemon(&controlMon));
    Battle controlBattle(controlSideA, controlSideB);
    ASSERT_TRUE(controlBattle.switchPokemon(controlBattle.getSideB(), 1));

    PRNG::setSeed(2704);
    const int controlDamage = controlBattle.calculateDamage(&controlAttacker, &controlTarget, pound);
    EXPECT_GT(swordDamage, controlDamage);
}

TEST(AbilityBehaviorTest, TabletsOfRuinLowersOpponentsPhysicalAttackForDamage) {
    PRNG::setSeed(2705);

    Species tabletsSpecies = makeSpecies(5094, "TabletsUser", Type::Dark, Type::Count, AbilityType::TabletsOfRuin, AbilityType::None);
    Species attackerSpecies = makeSpecies(5095, "PhysicalAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(5096, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon tabletsUser = makePokemon(tabletsSpecies, AbilityType::TabletsOfRuin);
    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Move pound = createMoveByName("Pound");

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));
    ASSERT_TRUE(sideB.addPokemon(&tabletsUser));
    Battle battle(sideA, sideB);
    ASSERT_TRUE(battle.switchPokemon(battle.getSideB(), 1));

    PRNG::setSeed(2706);
    const int tabletsDamage = battle.calculateDamage(&attacker, &target, pound);

    Species controlSpecies = makeSpecies(5097, "Control", Type::Dark, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon controlMon = makePokemon(controlSpecies, AbilityType::None);
    Pokemon controlAttacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon controlTarget = makePokemon(targetSpecies, AbilityType::None);
    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlAttacker));
    ASSERT_TRUE(controlSideB.addPokemon(&controlTarget));
    ASSERT_TRUE(controlSideB.addPokemon(&controlMon));
    Battle controlBattle(controlSideA, controlSideB);
    ASSERT_TRUE(controlBattle.switchPokemon(controlBattle.getSideB(), 1));

    PRNG::setSeed(2706);
    const int controlDamage = controlBattle.calculateDamage(&controlAttacker, &controlTarget, pound);
    EXPECT_LT(tabletsDamage, controlDamage);
}

TEST(AbilityBehaviorTest, VesselOfRuinLowersOpponentsSpecialAttackForDamage) {
    PRNG::setSeed(2707);

    Species vesselSpecies = makeSpecies(5098, "VesselUser", Type::Dark, Type::Count, AbilityType::VesselOfRuin, AbilityType::None);
    Species attackerSpecies = makeSpecies(5099, "SpecialAttacker", Type::Fire, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(5100, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon vesselUser = makePokemon(vesselSpecies, AbilityType::VesselOfRuin);
    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Move ember = createMoveByName("Ember");

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));
    ASSERT_TRUE(sideB.addPokemon(&vesselUser));
    Battle battle(sideA, sideB);
    ASSERT_TRUE(battle.switchPokemon(battle.getSideB(), 1));

    PRNG::setSeed(2708);
    const int vesselDamage = battle.calculateDamage(&attacker, &target, ember);

    Species controlSpecies = makeSpecies(5101, "Control", Type::Dark, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon controlMon = makePokemon(controlSpecies, AbilityType::None);
    Pokemon controlAttacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon controlTarget = makePokemon(targetSpecies, AbilityType::None);
    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlAttacker));
    ASSERT_TRUE(controlSideB.addPokemon(&controlTarget));
    ASSERT_TRUE(controlSideB.addPokemon(&controlMon));
    Battle controlBattle(controlSideA, controlSideB);
    ASSERT_TRUE(controlBattle.switchPokemon(controlBattle.getSideB(), 1));

    PRNG::setSeed(2708);
    const int controlDamage = controlBattle.calculateDamage(&controlAttacker, &controlTarget, ember);
    EXPECT_LT(vesselDamage, controlDamage);
}

TEST(AbilityBehaviorTest, UnnerveBlocksBerryConsumption) {
    Species unnerveSpecies = makeSpecies(5102, "UnnerveUser", Type::Dark, Type::Count, AbilityType::Unnerve, AbilityType::None);
    Species berrySpecies = makeSpecies(5103, "BerryUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon unnerveUser = makePokemon(unnerveSpecies, AbilityType::Unnerve);
    Pokemon berryUser = makePokemon(berrySpecies, AbilityType::None);
    berryUser.holdItem(ItemType::OranBerry);
    berryUser.setCurrentHP(std::max(1, berryUser.getMaxHP() / 2));
    const int hpBefore = berryUser.getCurrentHP();

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&berryUser));
    ASSERT_TRUE(sideB.addPokemon(&unnerveUser));
    Battle battle(sideA, sideB);

    battle.enqueueAction(BattleAction::makeUseItem(&berryUser, &berryUser, ItemType::OranBerry));
    battle.processTurn();

    EXPECT_EQ(berryUser.getCurrentHP(), hpBefore);
    EXPECT_EQ(berryUser.getItemType(), ItemType::OranBerry);
}

TEST(AbilityBehaviorTest, ImmunityBlocksPoisonMoves) {
    Species attackerSpecies = makeSpecies(5104, "PoisonUser", Type::Poison, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(5105, "ImmunityTarget", Type::Normal, Type::Count, AbilityType::Immunity, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::Immunity);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move poisonSting("Poison Sting", Type::Poison, Category::Physical, 15, 100, 35, MoveEffect::Poison, 100);

    battle.processMoveEffects(&attacker, &target, poisonSting);

    EXPECT_FALSE(target.hasStatus(StatusType::Poison));
    EXPECT_FALSE(target.hasStatus(StatusType::ToxicPoison));
}

TEST(AbilityBehaviorTest, StaticRespectsParalysisImmunityRouting) {
    Species staticSpecies = makeSpecies(51031, "StaticUser", Type::Electric, Type::Count, AbilityType::Static, AbilityType::None);
    Species immuneSpecies = makeSpecies(51032, "ParaImmuneTarget", Type::Normal, Type::Count, AbilityType::VitalSpirit, AbilityType::None);

    Pokemon staticUser = makePokemon(staticSpecies, AbilityType::Static);
    Pokemon immuneTarget = makePokemon(immuneSpecies, AbilityType::VitalSpirit);

    Ability staticAbility = getAbility(AbilityType::Static);
    AbilityDamageContext context;
    context.isDamagingMove = true;
    context.isContact = true;

    PRNG::setSeed(5103201);
    for (int i = 0; i < 400; ++i) {
        immuneTarget.removeStatus(StatusType::Paralysis);
        staticAbility.executeTrigger(Trigger::OnDamage, &staticUser, &immuneTarget, &context);
        EXPECT_FALSE(immuneTarget.hasStatus(StatusType::Paralysis));
    }
}

TEST(AbilityBehaviorTest, RoughSkinDealsContactChipToAttacker) {
    PRNG::setSeed(5103251);

    Species attackerSpecies = makeSpecies(51071, "ContactUserRS", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51072, "RoughSkinHolder", Type::Water, Type::Count, AbilityType::RoughSkin, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::RoughSkin);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move tackle = createMoveByName("Tackle");
    ASSERT_FALSE(tackle.getName().empty());

    const int hpBefore = attacker.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, tackle));
    battle.resolveNextAction();

    EXPECT_EQ(attacker.getCurrentHP(), hpBefore - std::max(1, attacker.getMaxHP() / 8));
}

TEST(AbilityBehaviorTest, IronBarbsDealsContactChipToAttacker) {
    PRNG::setSeed(5103261);

    Species attackerSpecies = makeSpecies(51073, "ContactUserIB", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51074, "IronBarbsHolder", Type::Steel, Type::Count, AbilityType::IronBarbs, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::IronBarbs);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move tackle = createMoveByName("Tackle");
    ASSERT_FALSE(tackle.getName().empty());

    const int hpBefore = attacker.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, tackle));
    battle.resolveNextAction();

    EXPECT_EQ(attacker.getCurrentHP(), hpBefore - std::max(1, attacker.getMaxHP() / 8));
}

TEST(AbilityBehaviorTest, FlameBodyCanBurnContactAttacker) {
    Species attackerSpecies = makeSpecies(51075, "ContactUserFB", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51076, "FlameBodyHolder", Type::Fire, Type::Count, AbilityType::FlameBody, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::FlameBody);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move tackle = createMoveByName("Tackle");
    ASSERT_FALSE(tackle.getName().empty());

    bool burned = false;
    for (int i = 0; i < 300 && !burned; ++i) {
        PRNG::setSeed(5103270 + i);
        attacker.removeStatus(StatusType::Burn);
        battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, tackle));
        battle.resolveNextAction();
        burned = attacker.hasStatus(StatusType::Burn);
    }

    EXPECT_TRUE(burned);
}

TEST(AbilityBehaviorTest, PoisonPointCanPoisonContactAttacker) {
    Species attackerSpecies = makeSpecies(51077, "ContactUserPP", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51078, "PoisonPointHolder", Type::Poison, Type::Count, AbilityType::PoisonPoint, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::PoisonPoint);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move tackle = createMoveByName("Tackle");
    ASSERT_FALSE(tackle.getName().empty());

    bool poisoned = false;
    for (int i = 0; i < 300 && !poisoned; ++i) {
        PRNG::setSeed(5103280 + i);
        attacker.removeStatus(StatusType::Poison);
        battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, tackle));
        battle.resolveNextAction();
        poisoned = attacker.hasStatus(StatusType::Poison);
    }

    EXPECT_TRUE(poisoned);
}

TEST(AbilityBehaviorTest, AftermathDamagesContactAttackerOnHolderFaint) {
    PRNG::setSeed(5103291);

    Species attackerSpecies = makeSpecies(51079, "ContactUserAM", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51080, "AftermathHolder", Type::Normal, Type::Count, AbilityType::Aftermath, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::Aftermath);
    defender.setCurrentHP(1);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move tackle = createMoveByName("Tackle");
    ASSERT_FALSE(tackle.getName().empty());

    const int hpBefore = attacker.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, tackle));
    battle.resolveNextAction();

    EXPECT_TRUE(defender.isFainted());
    EXPECT_EQ(attacker.getCurrentHP(), hpBefore - std::max(1, attacker.getMaxHP() / 4));
}

TEST(AbilityBehaviorTest, MummyReplacesContactAttackerAbility) {
    PRNG::setSeed(5103301);

    Species attackerSpecies = makeSpecies(51081, "MummyAttacker", Type::Normal, Type::Count, AbilityType::Blaze, AbilityType::None);
    Species defenderSpecies = makeSpecies(51082, "MummyHolder", Type::Normal, Type::Count, AbilityType::Mummy, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::Blaze);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::Mummy);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move tackle = createMoveByName("Tackle");
    ASSERT_FALSE(tackle.getName().empty());

    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, tackle));
    battle.resolveNextAction();

    EXPECT_EQ(attacker.getAbility(), AbilityType::Mummy);
}

TEST(AbilityBehaviorTest, EarthEaterAbsorbsGroundAndHeals) {
    PRNG::setSeed(5103301);

    Species attackerSpecies = makeSpecies(51033, "GroundUser", Type::Ground, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51034, "EarthEaterTarget", Type::Normal, Type::Count, AbilityType::EarthEater, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::EarthEater);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move groundMove("Ground Test", Type::Ground, Category::Physical, 80, 100, 10, MoveEffect::None, 0);

    defender.setCurrentHP(std::max(1, defender.getMaxHP() / 2));
    const int hpBefore = defender.getCurrentHP();
    const int damage = battle.calculateDamage(&attacker, &defender, groundMove);

    EXPECT_EQ(damage, 0);
    EXPECT_GT(defender.getCurrentHP(), hpBefore);
}

TEST(AbilityBehaviorTest, SharpnessBoostsSlicingMovePower) {
    PRNG::setSeed(5103501);

    Species sharpSpecies = makeSpecies(51035, "SharpUser", Type::Normal, Type::Count, AbilityType::Sharpness, AbilityType::None);
    Species controlSpecies = makeSpecies(51036, "ControlUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(51037, "SharpTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon sharpUser = makePokemon(sharpSpecies, AbilityType::Sharpness);
    Pokemon controlUser = makePokemon(controlSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&sharpUser));
    ASSERT_TRUE(sideB.addPokemon(&target));
    Battle sharpBattle(sideA, sideB);

    Side controlSideA("CA");
    Side controlSideB("CB");
    Pokemon controlTarget = makePokemon(targetSpecies, AbilityType::None);
    ASSERT_TRUE(controlSideA.addPokemon(&controlUser));
    ASSERT_TRUE(controlSideB.addPokemon(&controlTarget));
    Battle controlBattle(controlSideA, controlSideB);

    Move leafBlade("Leaf Blade", Type::Grass, Category::Physical, 90, 100, 15, MoveEffect::None, 0);

    PRNG::setSeed(5103502);
    const int sharpDamage = sharpBattle.calculateDamage(&sharpUser, &target, leafBlade);
    PRNG::setSeed(5103502);
    const int controlDamage = controlBattle.calculateDamage(&controlUser, &controlTarget, leafBlade);

    EXPECT_GT(sharpDamage, controlDamage);
}

TEST(AbilityBehaviorTest, PurifyingSaltBlocksMajorStatusAndHalvesGhostDamage) {
    PRNG::setSeed(5103601);

    Species attackerSpecies = makeSpecies(51038, "GhostUser", Type::Ghost, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51039, "SaltTarget", Type::Psychic, Type::Count, AbilityType::PurifyingSalt, AbilityType::None);
    Species controlSpecies = makeSpecies(51040, "ControlTarget", Type::Psychic, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon purifyingSaltTarget = makePokemon(defenderSpecies, AbilityType::PurifyingSalt);
    Pokemon controlTarget = makePokemon(controlSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&purifyingSaltTarget));
    Battle battle(sideA, sideB);

    Move thunderWave("Thunder Wave", Type::Electric, Category::Status, 0, 100, 20, MoveEffect::Paralyze, 100);
    Move shadowBall("Shadow Ball", Type::Ghost, Category::Special, 80, 100, 15, MoveEffect::None, 0);

    battle.processMoveEffects(&attacker, &purifyingSaltTarget, thunderWave);
    EXPECT_FALSE(purifyingSaltTarget.hasStatus(StatusType::Paralysis));

    Side controlSideA("CA");
    Side controlSideB("CB");
    Pokemon controlAttacker = makePokemon(attackerSpecies, AbilityType::None);
    ASSERT_TRUE(controlSideA.addPokemon(&controlAttacker));
    ASSERT_TRUE(controlSideB.addPokemon(&controlTarget));
    Battle controlBattle(controlSideA, controlSideB);

    PRNG::setSeed(5103602);
    const int reducedDamage = battle.calculateDamage(&attacker, &purifyingSaltTarget, shadowBall);
    PRNG::setSeed(5103602);
    const int normalDamage = controlBattle.calculateDamage(&controlAttacker, &controlTarget, shadowBall);

    EXPECT_LT(reducedDamage, normalDamage);
}

TEST(AbilityBehaviorTest, WellBakedBodyAbsorbsFireAndBoostsDefense) {
    PRNG::setSeed(5103701);

    Species attackerSpecies = makeSpecies(51041, "FireUser", Type::Fire, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51042, "BakedTarget", Type::Normal, Type::Count, AbilityType::WellBakedBody, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::WellBakedBody);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move fireMove("Flamethrower", Type::Fire, Category::Special, 90, 100, 15, MoveEffect::Burn, 10);

    const int defenseBefore = defender.getStatStage(StatIndex::Defense);
    const int hpBefore = defender.getCurrentHP();
    const int damage = battle.calculateDamage(&attacker, &defender, fireMove);

    EXPECT_EQ(damage, 0);
    EXPECT_EQ(defender.getCurrentHP(), hpBefore);
    EXPECT_EQ(defender.getStatStage(StatIndex::Defense), defenseBefore + 2);
}

TEST(AbilityBehaviorTest, WindRiderBlocksWindMovesAndBoostsAttack) {
    PRNG::setSeed(5103801);

    Species attackerSpecies = makeSpecies(51043, "WindUser", Type::Flying, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51044, "WindRiderTarget", Type::Grass, Type::Count, AbilityType::WindRider, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::WindRider);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move hurricane("Hurricane", Type::Flying, Category::Special, 110, 100, 10, MoveEffect::Confuse, 30);

    const int attackBefore = defender.getStatStage(StatIndex::Attack);
    const int hpBefore = defender.getCurrentHP();
    const int damage = battle.calculateDamage(&attacker, &defender, hurricane);

    EXPECT_EQ(damage, 0);
    EXPECT_EQ(defender.getCurrentHP(), hpBefore);
    EXPECT_EQ(defender.getStatStage(StatIndex::Attack), attackBefore + 1);
}

TEST(AbilityBehaviorTest, ToxicDebrisSetsToxicSpikesWhenHitByPhysicalMove) {
    PRNG::setSeed(5103901);

    Species attackerSpecies = makeSpecies(51045, "PhysicalUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51046, "DebrisTarget", Type::Rock, Type::Count, AbilityType::ToxicDebris, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::ToxicDebris);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move tackle("Tackle", Type::Normal, Category::Physical, 40, 100, 35, MoveEffect::None, 0);

    EXPECT_EQ(battle.getSideA().getToxicSpikesLayers(), 0);

    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, tackle));
    battle.resolveNextAction();

    EXPECT_EQ(battle.getSideA().getToxicSpikesLayers(), 1);
}

TEST(AbilityBehaviorTest, LingeringAromaReplacesContactAttackerAbility) {
    PRNG::setSeed(5104001);

    Species attackerSpecies = makeSpecies(51047, "AromaAttacker", Type::Normal, Type::Count, AbilityType::Blaze, AbilityType::None);
    Species defenderSpecies = makeSpecies(51048, "AromaDefender", Type::Grass, Type::Count, AbilityType::LingeringAroma, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::Blaze);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::LingeringAroma);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move tackle = createMoveByName("Tackle");
    ASSERT_FALSE(tackle.getName().empty());

    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, tackle));
    battle.resolveNextAction();

    EXPECT_EQ(attacker.getAbility(), AbilityType::LingeringAroma);
}

TEST(AbilityBehaviorTest, ArmorTailBlocksPriorityMovesAgainstHolder) {
    PRNG::setSeed(5104101);

    Species attackerSpecies = makeSpecies(51049, "PriorityUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51050, "ArmorTailTarget", Type::Psychic, Type::Count, AbilityType::ArmorTail, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::ArmorTail);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move quickAttack = createMoveByName("Quick Attack");
    Move pound = createMoveByName("Pound");
    ASSERT_FALSE(quickAttack.getName().empty());
    ASSERT_FALSE(pound.getName().empty());

    const int hpBeforePriority = defender.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, quickAttack));
    battle.resolveNextAction();
    EXPECT_EQ(defender.getCurrentHP(), hpBeforePriority);

    const int hpBeforeNormal = defender.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, pound));
    battle.resolveNextAction();
    EXPECT_LT(defender.getCurrentHP(), hpBeforeNormal);
}

TEST(AbilityBehaviorTest, GoodAsGoldBlocksOpposingStatusMoves) {
    PRNG::setSeed(5104201);

    Species attackerSpecies = makeSpecies(51051, "StatusUser", Type::Electric, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51052, "GoldTarget", Type::Steel, Type::Ghost, AbilityType::GoodAsGold, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::GoodAsGold);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move thunderWave = createMoveByName("Thunder Wave");
    ASSERT_FALSE(thunderWave.getName().empty());

    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, thunderWave));
    battle.resolveNextAction();

    EXPECT_FALSE(defender.hasStatus(StatusType::Paralysis));
}

TEST(AbilityBehaviorTest, StakeoutBoostsDamageAgainstFreshSwitchInTarget) {
    PRNG::setSeed(5104301);

    Species attackerSpecies = makeSpecies(51053, "StakeoutUser", Type::Dark, Type::Count, AbilityType::Stakeout, AbilityType::None);
    Species targetSpecies = makeSpecies(51054, "SwitchTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species benchSpecies = makeSpecies(51055, "BenchTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::Stakeout);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Pokemon bench = makePokemon(benchSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));
    ASSERT_TRUE(sideB.addPokemon(&bench));

    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");
    ASSERT_FALSE(pound.getName().empty());

    PRNG::setSeed(5104302);
    const int normalDamage = battle.calculateDamage(&attacker, &target, pound);

    ASSERT_TRUE(battle.switchPokemon(battle.getSideB(), 1));
    Pokemon* switchedIn = battle.getSideB().getActivePokemon();
    ASSERT_NE(switchedIn, nullptr);

    PRNG::setSeed(5104302);
    const int boostedDamage = battle.calculateDamage(&attacker, switchedIn, pound);

    EXPECT_GT(boostedDamage, normalDamage);
}

TEST(AbilityBehaviorTest, SupremeOverlordBoostScalesWithFaintedAllies) {
    PRNG::setSeed(5104401);

    Species attackerSpecies = makeSpecies(51056, "OverlordUser", Type::Dark, Type::Count, AbilityType::SupremeOverlord, AbilityType::None);
    Species allySpeciesA = makeSpecies(51057, "FaintedAllyA", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species allySpeciesB = makeSpecies(51058, "FaintedAllyB", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(51059, "OverlordTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::SupremeOverlord);
    Pokemon allyA = makePokemon(allySpeciesA, AbilityType::None);
    Pokemon allyB = makePokemon(allySpeciesB, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideA.addPokemon(&allyA));
    ASSERT_TRUE(sideA.addPokemon(&allyB));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");
    ASSERT_FALSE(pound.getName().empty());

    PRNG::setSeed(5104402);
    const int normalDamage = battle.calculateDamage(&attacker, &target, pound);

    allyA.setCurrentHP(0);
    allyB.setCurrentHP(0);

    PRNG::setSeed(5104402);
    const int boostedDamage = battle.calculateDamage(&attacker, &target, pound);

    EXPECT_GT(boostedDamage, normalDamage);
}

TEST(AbilityBehaviorTest, ProtosynthesisBoostsStrongestAttackStatInSun) {
    PRNG::setSeed(5104501);

    Species attackerSpecies = makeSpecies(51060, "ProtoUser", Type::Normal, Type::Count, AbilityType::Protosynthesis, AbilityType::None);
    Species targetSpecies = makeSpecies(51061, "ProtoTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    attackerSpecies.baseStats = {100, 150, 80, 70, 70, 70};

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::Protosynthesis);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");
    ASSERT_FALSE(pound.getName().empty());

    PRNG::setSeed(5104502);
    const int clearDamage = battle.calculateDamage(&attacker, &target, pound);

    battle.getWeather().setWeather(WeatherType::Sun, 5);

    PRNG::setSeed(5104502);
    const int sunDamage = battle.calculateDamage(&attacker, &target, pound);

    EXPECT_GT(sunDamage, clearDamage);
}

TEST(AbilityBehaviorTest, QuarkDriveBoostsStrongestDefenseStatOnElectricTerrain) {
    PRNG::setSeed(5104601);

    Species attackerSpecies = makeSpecies(51062, "QuarkAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51063, "QuarkDefender", Type::Normal, Type::Count, AbilityType::QuarkDrive, AbilityType::None);
    defenderSpecies.baseStats = {100, 70, 160, 70, 70, 70};

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::QuarkDrive);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");
    ASSERT_FALSE(pound.getName().empty());

    PRNG::setSeed(5104602);
    const int clearDamage = battle.calculateDamage(&attacker, &defender, pound);

    battle.getField().setField(FieldType::Electric, 5);

    PRNG::setSeed(5104602);
    const int terrainDamage = battle.calculateDamage(&attacker, &defender, pound);

    EXPECT_LT(terrainDamage, clearDamage);
}

TEST(AbilityBehaviorTest, CudChewReplaysBerryAtEndOfNextTurn) {
    PRNG::setSeed(5104701);

    Species attackerSpecies = makeSpecies(51064, "CudAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51065, "CudChewUser", Type::Normal, Type::Count, AbilityType::CudChew, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::CudChew);
    defender.holdItem(ItemType::OranBerry);
    defender.setCurrentHP(90);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");
    ASSERT_FALSE(pound.getName().empty());

    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, pound));
    battle.enqueueAction(BattleAction::makePass(&defender));
    battle.processTurn();

    ASSERT_EQ(defender.getItemType(), ItemType::None);
    const int hpAfterFirstTurn = defender.getCurrentHP();

    battle.enqueueAction(BattleAction::makePass(&attacker));
    battle.enqueueAction(BattleAction::makePass(&defender));
    battle.processTurn();

    EXPECT_EQ(defender.getCurrentHP(), std::min(defender.getMaxHP(), hpAfterFirstTurn + 10));
}

TEST(AbilityBehaviorTest, MoldBreakerBypassesWindRiderImmunity) {
    PRNG::setSeed(5104801);

    Species moldBreakerSpecies = makeSpecies(51066, "MoldBreakerUser", Type::Flying, Type::Count, AbilityType::MoldBreaker, AbilityType::None);
    Species controlSpecies = makeSpecies(51067, "ControlUser", Type::Flying, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(51068, "WindRiderHolder", Type::Grass, Type::Count, AbilityType::WindRider, AbilityType::None);

    Pokemon moldBreakerUser = makePokemon(moldBreakerSpecies, AbilityType::MoldBreaker);
    Pokemon controlUser = makePokemon(controlSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::WindRider);
    Pokemon controlDefender = makePokemon(defenderSpecies, AbilityType::WindRider);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&moldBreakerUser));
    ASSERT_TRUE(sideB.addPokemon(&defender));
    Battle moldBreakerBattle(sideA, sideB);

    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlUser));
    ASSERT_TRUE(controlSideB.addPokemon(&controlDefender));
    Battle controlBattle(controlSideA, controlSideB);

    Move hurricane = createMoveByName("Hurricane");
    ASSERT_FALSE(hurricane.getName().empty());

    PRNG::setSeed(5104802);
    const int bypassDamage = moldBreakerBattle.calculateDamage(&moldBreakerUser, &defender, hurricane);
    PRNG::setSeed(5104802);
    const int blockedDamage = controlBattle.calculateDamage(&controlUser, &controlDefender, hurricane);

    EXPECT_GT(bypassDamage, 0);
    EXPECT_EQ(blockedDamage, 0);
}

TEST(AbilityBehaviorTest, MoldBreakerBypassesGoodAsGoldStatusBlock) {
    PRNG::setSeed(5104901);

    Species attackerSpecies = makeSpecies(51069, "MoldBreakerStatusUser", Type::Electric, Type::Count, AbilityType::MoldBreaker, AbilityType::None);
    Species defenderSpecies = makeSpecies(51070, "GoldHolder", Type::Steel, Type::Ghost, AbilityType::GoodAsGold, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::MoldBreaker);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::GoodAsGold);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move thunderWave = createMoveByName("Thunder Wave");
    ASSERT_FALSE(thunderWave.getName().empty());

    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, thunderWave));
    battle.resolveNextAction();

    EXPECT_TRUE(defender.hasStatus(StatusType::Paralysis));
}

TEST(AbilityBehaviorTest, ImmunityBlocksToxicSpikesPoisonOnSwitchIn) {
    Species setupSpecies = makeSpecies(5106, "HazardUser", Type::Poison, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(5107, "ImmunityTarget", Type::Normal, Type::Count, AbilityType::Immunity, AbilityType::None);
    Species benchSpecies = makeSpecies(5108, "Bench", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon setter = makePokemon(setupSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::Immunity);
    Pokemon bench = makePokemon(benchSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&setter));
    ASSERT_TRUE(sideB.addPokemon(&target));
    ASSERT_TRUE(sideB.addPokemon(&bench));

    Battle battle(sideA, sideB);
    Move toxicSpikes("Toxic Spikes", Type::Poison, Category::Status, 0, 100, 20, MoveEffect::None, 100);

    battle.processMoveEffects(&setter, &target, toxicSpikes);
    ASSERT_TRUE(battle.switchPokemon(battle.getSideB(), 1));

    EXPECT_FALSE(target.hasStatus(StatusType::Poison));
    EXPECT_FALSE(target.hasStatus(StatusType::ToxicPoison));
}

TEST(MoveBehaviorTest, TrickRoomSetsFieldAndReversesQueueOrder) {
    Species fastSpecies = makeSpecies(5001, "FastMon", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species slowSpecies = makeSpecies(5002, "SlowMon", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    fastSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 130;
    slowSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 30;

    Pokemon fast = makePokemon(fastSpecies, AbilityType::None);
    Pokemon slow = makePokemon(slowSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&fast));
    ASSERT_TRUE(sideB.addPokemon(&slow));

    Battle battle(sideA, sideB);
    Move trickRoom("Trick Room", Type::Psychic, Category::Status, 0, 100, 5, MoveEffect::None, 100);
    battle.processMoveEffects(&slow, &fast, trickRoom);

    EXPECT_TRUE(battle.getField().isTrickRoom());
    EXPECT_EQ(battle.getField().type, FieldType::TrickRoom);
    EXPECT_EQ(battle.getField().duration, 5);

    Move pound = createMoveByName("Pound");
    BattleQueue queue;
    queue.push(BattleAction::makeAttack(&fast, &slow, pound), true);
    queue.push(BattleAction::makeAttack(&slow, &fast, pound), true);

    const BattleAction first = queue.pop();
    const BattleAction second = queue.pop();
    EXPECT_EQ(first.actor, &slow);
    EXPECT_EQ(second.actor, &fast);
}

TEST(MoveBehaviorTest, HeavySlamUsesWeightScalingForDamage) {
    PRNG::setSeed(404);

    Species heavySpecies = makeSpecies(6001, "Heavy", Type::Steel, Type::Count, AbilityType::None, AbilityType::None);
    Species lightSpecies = makeSpecies(6002, "Light", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species bulkySpecies = makeSpecies(6003, "Bulky", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    heavySpecies.weight = 1200;
    lightSpecies.weight = 120;
    bulkySpecies.weight = 900;

    Pokemon attacker = makePokemon(heavySpecies, AbilityType::None);
    Pokemon lightTarget = makePokemon(lightSpecies, AbilityType::None);
    Pokemon bulkyTarget = makePokemon(bulkySpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&lightTarget));

    Battle battle(sideA, sideB);
    Move heavySlam("Heavy Slam", Type::Steel, Category::Physical, 1, 100, 10, MoveEffect::None, 100);

    PRNG::setSeed(405);
    const int damageVsLight = battle.calculateDamage(&attacker, &lightTarget, heavySlam);
    PRNG::setSeed(405);
    const int damageVsBulky = battle.calculateDamage(&attacker, &bulkyTarget, heavySlam);

    EXPECT_GT(damageVsLight, damageVsBulky);
    EXPECT_GT(damageVsLight, 0);
}

TEST(MoveBehaviorTest, UturnSwitchesOutAfterDamage) {
    PRNG::setSeed(505);

    Species attackerSpecies = makeSpecies(7001, "UturnUser", Type::Bug, Type::Count, AbilityType::None, AbilityType::None);
    Species teammateSpecies = makeSpecies(7002, "BenchMate", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(7003, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon teammate = makePokemon(teammateSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideA.addPokemon(&teammate));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move uturn("U-turn", Type::Bug, Category::Physical, 70, 100, 20, MoveEffect::None, 100);

    battle.processMoveEffects(&attacker, &target, uturn);

    EXPECT_EQ(battle.getSideA().getActivePokemon(), &teammate);
}

TEST(MoveBehaviorTest, BatonPassTransfersStatStagesToSwitchInPokemon) {
    PRNG::setSeed(606);

    Species passerSpecies = makeSpecies(8001, "Passer", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species receiverSpecies = makeSpecies(8002, "Receiver", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8003, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon passer = makePokemon(passerSpecies, AbilityType::None);
    Pokemon receiver = makePokemon(receiverSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    passer.changeStatStage(StatIndex::Attack, 2);
    passer.changeStatStage(StatIndex::Speed, 1);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&passer));
    ASSERT_TRUE(sideA.addPokemon(&receiver));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move batonPass("Baton Pass", Type::Normal, Category::Status, 0, 100, 40, MoveEffect::None, 100);

    battle.processMoveEffects(&passer, &target, batonPass);

    Pokemon* activeAfterPass = battle.getSideA().getActivePokemon();
    ASSERT_EQ(activeAfterPass, &receiver);
    EXPECT_EQ(activeAfterPass->getStatStage(StatIndex::Attack), 2);
    EXPECT_EQ(activeAfterPass->getStatStage(StatIndex::Speed), 1);
}

TEST(MoveBehaviorTest, WhirlwindAndRoarForceTargetSwitchAndRespectSubstitute) {
    Species userSpecies = makeSpecies(8101, "ForceSwitchUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8102, "ForcedTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species benchSpecies = makeSpecies(8103, "BenchTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species blockedUserSpecies = makeSpecies(8104, "BlockedUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species blockedTargetSpecies = makeSpecies(8105, "BlockedTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species blockedBenchSpecies = makeSpecies(8106, "BlockedBench", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Pokemon bench = makePokemon(benchSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));
    ASSERT_TRUE(sideB.addPokemon(&bench));

    Battle battle(sideA, sideB);
    Move whirlwind = createMoveByName("Whirlwind");
    Move roar = createMoveByName("Roar");

    battle.processMoveEffects(&user, &target, whirlwind);
    EXPECT_EQ(battle.getSideB().getActivePokemon(), &bench);

    Pokemon blockedUser = makePokemon(blockedUserSpecies, AbilityType::None);
    Pokemon blockedTarget = makePokemon(blockedTargetSpecies, AbilityType::None);
    Pokemon blockedBench = makePokemon(blockedBenchSpecies, AbilityType::None);
    blockedTarget.setSubstituteHP(25);

    Side blockedSideA("BlockedA");
    Side blockedSideB("BlockedB");
    ASSERT_TRUE(blockedSideA.addPokemon(&blockedUser));
    ASSERT_TRUE(blockedSideB.addPokemon(&blockedTarget));
    ASSERT_TRUE(blockedSideB.addPokemon(&blockedBench));

    Battle blockedBattle(blockedSideA, blockedSideB);
    blockedBattle.processMoveEffects(&blockedUser, &blockedTarget, roar);

    EXPECT_EQ(blockedBattle.getSideB().getActivePokemon(), &blockedTarget);
    EXPECT_EQ(blockedTarget.getSubstituteHP(), 25);
}

TEST(MoveBehaviorTest, WhirlwindDoesNothingWithoutBench) {
    Species userSpecies = makeSpecies(8111, "SoloUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8112, "SoloTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move whirlwind = createMoveByName("Whirlwind");

    battle.processMoveEffects(&user, &target, whirlwind);

    EXPECT_EQ(battle.getSideB().getActivePokemon(), &target);
}

TEST(MoveBehaviorTest, DragonTailAndCircleThrowForceSwitchAfterDamage) {
    Species attackerSpecies = makeSpecies(8113, "DragTailUser", Type::Dragon, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8114, "DragTailTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species benchSpecies = makeSpecies(8115, "DragTailBench", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Pokemon bench = makePokemon(benchSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));
    ASSERT_TRUE(sideB.addPokemon(&bench));

    Battle battle(sideA, sideB);
    Move dragonTail = createMoveByName("Dragon Tail");
    Move circleThrow = createMoveByName("Circle Throw");

    ASSERT_FALSE(dragonTail.getName().empty());
    ASSERT_FALSE(circleThrow.getName().empty());

    battle.enqueueAction(BattleAction::makeAttack(&attacker, &target, dragonTail));
    battle.resolveNextAction();
    EXPECT_EQ(battle.getSideB().getActivePokemon(), &bench);

    Pokemon target2 = makePokemon(targetSpecies, AbilityType::None);
    Pokemon bench2 = makePokemon(benchSpecies, AbilityType::None);
    Side sideB2("B2");
    ASSERT_TRUE(sideB2.addPokemon(&target2));
    ASSERT_TRUE(sideB2.addPokemon(&bench2));

    Battle battle2(sideA, sideB2);
    battle2.enqueueAction(BattleAction::makeAttack(&attacker, &target2, circleThrow));
    battle2.resolveNextAction();
    EXPECT_EQ(battle2.getSideB().getActivePokemon(), &bench2);
}

TEST(MoveBehaviorTest, ForcedSwitchMovesRespectSubstituteAndIngrain) {
    Species attackerSpecies = makeSpecies(8116, "TrapUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8117, "TrapTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species benchSpecies = makeSpecies(8118, "TrapBench", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Pokemon bench = makePokemon(benchSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));
    ASSERT_TRUE(sideB.addPokemon(&bench));

    Battle substituteBattle(sideA, sideB);
    Move whirlwind = createMoveByName("Whirlwind");
    target.setSubstituteHP(25);
    substituteBattle.processMoveEffects(&attacker, &target, whirlwind);
    EXPECT_EQ(substituteBattle.getSideB().getActivePokemon(), &target);

    Battle ingrainBattle(sideA, sideB);
    target.clearSubstitute();
    ingrainBattle.processMoveEffects(&target, &attacker, createMoveByName("Ingrain"));
    ingrainBattle.processMoveEffects(&attacker, &target, whirlwind);
    EXPECT_EQ(ingrainBattle.getSideB().getActivePokemon(), &target);
}

TEST(MoveBehaviorTest, SupersonicAndConfuseRayApplyConfusionAndRespectSubstitute) {
    Species userSpecies = makeSpecies(8121, "ConfuseUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8122, "ConfusedTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species blockedUserSpecies = makeSpecies(8123, "BlockedConfuseUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species blockedTargetSpecies = makeSpecies(8124, "BlockedConfuseTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move supersonic = createMoveByName("Supersonic");

    battle.processMoveEffects(&user, &target, supersonic);
    EXPECT_TRUE(target.hasStatus(StatusType::Confusion));

    Pokemon blockedUser = makePokemon(blockedUserSpecies, AbilityType::None);
    Pokemon blockedTarget = makePokemon(blockedTargetSpecies, AbilityType::None);
    blockedTarget.setSubstituteHP(25);

    Side blockedSideA("BlockedA");
    Side blockedSideB("BlockedB");
    ASSERT_TRUE(blockedSideA.addPokemon(&blockedUser));
    ASSERT_TRUE(blockedSideB.addPokemon(&blockedTarget));

    Battle blockedBattle(blockedSideA, blockedSideB);
    Move confuseRay = createMoveByName("Confuse Ray");

    blockedBattle.processMoveEffects(&blockedUser, &blockedTarget, confuseRay);
    EXPECT_FALSE(blockedTarget.hasStatus(StatusType::Confusion));
}

TEST(MoveBehaviorTest, MimicCopiesTargetsLastUsedMoveIntoMimicSlot) {
    Species mimicUserSpecies = makeSpecies(8125, "MimicUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species mimicTargetSpecies = makeSpecies(8126, "MimicTarget", Type::Fire, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon mimicUser = makePokemon(mimicUserSpecies, AbilityType::None);
    Pokemon mimicTarget = makePokemon(mimicTargetSpecies, AbilityType::None);

    Move mimic = createMoveByName("Mimic");
    ASSERT_FALSE(mimic.getName().empty());
    mimicUser.addMove(mimic);
    mimicUser.addMove(createMoveByName("Pound"));

    Move ember = createMoveByName("Ember");
    ASSERT_FALSE(ember.getName().empty());
    mimicTarget.addMove(ember);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&mimicUser));
    ASSERT_TRUE(sideB.addPokemon(&mimicTarget));

    Battle battle(sideA, sideB);

    battle.enqueueAction(BattleAction::makeAttack(&mimicTarget, &mimicUser, ember));
    battle.resolveNextAction();

    battle.processMoveEffects(&mimicUser, &mimicTarget, mimic);

    const auto& updatedMoves = mimicUser.getMoves();
    ASSERT_GE(updatedMoves.size(), 1u);
    EXPECT_EQ(updatedMoves[0].getName(), "Ember");
    EXPECT_EQ(updatedMoves[0].getPP(), 5);
}

TEST(MoveBehaviorTest, TrickRolePlayAndSkillSwapApplyExpectedEffects) {
    Species trickUserSpecies = makeSpecies(8127, "TrickUser", Type::Psychic, Type::Count, AbilityType::Blaze, AbilityType::None);
    Species trickTargetSpecies = makeSpecies(8128, "TrickTarget", Type::Normal, Type::Count, AbilityType::Torrent, AbilityType::None);

    Pokemon trickUser = makePokemon(trickUserSpecies, AbilityType::Blaze);
    Pokemon trickTarget = makePokemon(trickTargetSpecies, AbilityType::Torrent);
    trickUser.setItemType(ItemType::ChoiceScarf);
    trickTarget.setItemType(ItemType::Leftovers);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&trickUser));
    ASSERT_TRUE(sideB.addPokemon(&trickTarget));

    Battle battle(sideA, sideB);

    Move trick = createMoveByName("Trick");
    Move rolePlay = createMoveByName("Role Play");
    Move skillSwap = createMoveByName("Skill Swap");

    ASSERT_FALSE(trick.getName().empty());
    ASSERT_FALSE(rolePlay.getName().empty());
    ASSERT_FALSE(skillSwap.getName().empty());

    battle.processMoveEffects(&trickUser, &trickTarget, trick);
    EXPECT_EQ(trickUser.getItemType(), ItemType::Leftovers);
    EXPECT_EQ(trickTarget.getItemType(), ItemType::ChoiceScarf);

    battle.processMoveEffects(&trickUser, &trickTarget, rolePlay);
    EXPECT_EQ(trickUser.getAbility(), AbilityType::Torrent);

    trickUser.setAbility(AbilityType::Blaze);
    trickTarget.setAbility(AbilityType::Torrent);
    battle.processMoveEffects(&trickUser, &trickTarget, skillSwap);
    EXPECT_EQ(trickUser.getAbility(), AbilityType::Torrent);
    EXPECT_EQ(trickTarget.getAbility(), AbilityType::Blaze);

    Pokemon blockedUser = makePokemon(trickUserSpecies, AbilityType::Blaze);
    Pokemon blockedTarget = makePokemon(trickTargetSpecies, AbilityType::Torrent);
    blockedTarget.setSubstituteHP(30);
    blockedUser.setItemType(ItemType::ChoiceSpecs);
    blockedTarget.setItemType(ItemType::SitrusBerry);

    Side blockedSideA("BlockedA");
    Side blockedSideB("BlockedB");
    ASSERT_TRUE(blockedSideA.addPokemon(&blockedUser));
    ASSERT_TRUE(blockedSideB.addPokemon(&blockedTarget));

    Battle blockedBattle(blockedSideA, blockedSideB);
    blockedBattle.processMoveEffects(&blockedUser, &blockedTarget, trick);
    blockedBattle.processMoveEffects(&blockedUser, &blockedTarget, rolePlay);

    EXPECT_EQ(blockedUser.getItemType(), ItemType::ChoiceSpecs);
    EXPECT_EQ(blockedTarget.getItemType(), ItemType::SitrusBerry);
    EXPECT_EQ(blockedUser.getAbility(), AbilityType::Blaze);
}

TEST(MoveBehaviorTest, TransformCopiesTypesAndMovesAndSketchCopiesLastUsedMove) {
    Species transformUserSpecies = makeSpecies(8133, "TransformUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species transformTargetSpecies = makeSpecies(8134, "TransformTarget", Type::Fire, Type::Flying, AbilityType::None, AbilityType::None);

    Pokemon transformUser = makePokemon(transformUserSpecies, AbilityType::None);
    Pokemon transformTarget = makePokemon(transformTargetSpecies, AbilityType::None);

    Move transformMove = createMoveByName("Transform");
    ASSERT_FALSE(transformMove.getName().empty());
    transformUser.addMove(transformMove);
    transformUser.addMove(createMoveByName("Splash"));
    transformTarget.addMove(createMoveByName("Flamethrower"));
    transformTarget.addMove(createMoveByName("Air Slash"));

    Side transformSideA("TransformA");
    Side transformSideB("TransformB");
    ASSERT_TRUE(transformSideA.addPokemon(&transformUser));
    ASSERT_TRUE(transformSideB.addPokemon(&transformTarget));
    Battle transformBattle(transformSideA, transformSideB);

    transformBattle.processMoveEffects(&transformUser, &transformTarget, transformMove);
    EXPECT_EQ(transformUser.getType1(), Type::Fire);
    EXPECT_EQ(transformUser.getType2(), Type::Flying);
    ASSERT_FALSE(transformUser.getMoves().empty());
    EXPECT_EQ(transformUser.getMoves().front().getName(), "Flamethrower");
    EXPECT_EQ(transformUser.getMoves().front().getPP(), 5);

    Species sketchUserSpecies = makeSpecies(8135, "SketchUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species sketchTargetSpecies = makeSpecies(8136, "SketchTarget", Type::Electric, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon sketchUser = makePokemon(sketchUserSpecies, AbilityType::None);
    Pokemon sketchTarget = makePokemon(sketchTargetSpecies, AbilityType::None);

    Move sketchMove = createMoveByName("Sketch");
    Move thunderbolt = createMoveByName("Thunderbolt");
    Move splash = createMoveByName("Splash");
    ASSERT_FALSE(sketchMove.getName().empty());
    ASSERT_FALSE(thunderbolt.getName().empty());
    sketchUser.addMove(sketchMove);
    sketchUser.addMove(splash);
    sketchTarget.addMove(thunderbolt);

    Side sketchSideA("SketchA");
    Side sketchSideB("SketchB");
    ASSERT_TRUE(sketchSideA.addPokemon(&sketchUser));
    ASSERT_TRUE(sketchSideB.addPokemon(&sketchTarget));
    Battle sketchBattle(sketchSideA, sketchSideB);

    sketchBattle.enqueueAction(BattleAction::makeAttack(&sketchTarget, &sketchUser, thunderbolt));
    sketchBattle.enqueueAction(BattleAction::makeAttack(&sketchUser, &sketchTarget, splash));
    sketchBattle.processTurn();

    sketchBattle.processMoveEffects(&sketchUser, &sketchTarget, sketchMove);
    const std::vector<Move>& sketchMoves = sketchUser.getMoves();
    ASSERT_GE(sketchMoves.size(), 1U);
    EXPECT_EQ(sketchMoves[0].getName(), "Thunderbolt");
}

TEST(MoveBehaviorTest, ForesightOdorSleuthAndMiracleEyeEnablePreviouslyImmuneDamage) {
    Species ghostSpecies = makeSpecies(8129, "GhostTarget", Type::Ghost, Type::Count, AbilityType::None, AbilityType::None);
    Species normalSpecies = makeSpecies(8130, "NormalUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species darkSpecies = makeSpecies(8131, "DarkTarget", Type::Dark, Type::Count, AbilityType::None, AbilityType::None);
    Species psychicSpecies = makeSpecies(8132, "PsychicUser", Type::Psychic, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon normalUser = makePokemon(normalSpecies, AbilityType::None);
    Pokemon ghostTarget = makePokemon(ghostSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&normalUser));
    ASSERT_TRUE(sideB.addPokemon(&ghostTarget));

    Battle battle(sideA, sideB);
    Move bodySlam("Body Slam", Type::Normal, Category::Physical, 85, 100, 15, MoveEffect::None, 0);
    Move lowKick("Low Kick", Type::Fighting, Category::Physical, 50, 100, 20, MoveEffect::None, 0);
    Move foresight = createMoveByName("Foresight");
    Move odorSleuth = createMoveByName("Odor Sleuth");

    ASSERT_FALSE(foresight.getName().empty());
    ASSERT_FALSE(odorSleuth.getName().empty());

    EXPECT_EQ(battle.calculateDamage(&normalUser, &ghostTarget, bodySlam), 0);
    EXPECT_EQ(battle.calculateDamage(&normalUser, &ghostTarget, lowKick), 0);

    battle.processMoveEffects(&normalUser, &ghostTarget, foresight);
    EXPECT_GT(battle.calculateDamage(&normalUser, &ghostTarget, bodySlam), 0);

    battle.processMoveEffects(&normalUser, &ghostTarget, odorSleuth);
    EXPECT_GT(battle.calculateDamage(&normalUser, &ghostTarget, lowKick), 0);

    Pokemon psychicUser = makePokemon(psychicSpecies, AbilityType::None);
    Pokemon darkTarget = makePokemon(darkSpecies, AbilityType::None);
    Side sideC("C");
    Side sideD("D");
    ASSERT_TRUE(sideC.addPokemon(&psychicUser));
    ASSERT_TRUE(sideD.addPokemon(&darkTarget));

    Battle miracleBattle(sideC, sideD);
    Move psychicMove("Psychic", Type::Psychic, Category::Special, 90, 100, 10, MoveEffect::None, 0);
    Move miracleEye = createMoveByName("Miracle Eye");
    ASSERT_FALSE(miracleEye.getName().empty());

    EXPECT_EQ(miracleBattle.calculateDamage(&psychicUser, &darkTarget, psychicMove), 0);
    miracleBattle.processMoveEffects(&psychicUser, &darkTarget, miracleEye);
    EXPECT_GT(miracleBattle.calculateDamage(&psychicUser, &darkTarget, psychicMove), 0);

    Pokemon blockedUser = makePokemon(normalSpecies, AbilityType::None);
    Pokemon blockedGhostTarget = makePokemon(ghostSpecies, AbilityType::None);
    blockedGhostTarget.setSubstituteHP(30);
    Side blockedSideA("BlockedA");
    Side blockedSideB("BlockedB");
    ASSERT_TRUE(blockedSideA.addPokemon(&blockedUser));
    ASSERT_TRUE(blockedSideB.addPokemon(&blockedGhostTarget));
    Battle blockedBattle(blockedSideA, blockedSideB);

    blockedBattle.processMoveEffects(&blockedUser, &blockedGhostTarget, foresight);
    EXPECT_EQ(blockedBattle.calculateDamage(&blockedUser, &blockedGhostTarget, bodySlam), 0);
}

TEST(MoveBehaviorTest, DisableBlocksTargetsLastUsedMoveAndMistBlocksStatDrops) {
    PRNG::setSeed(8201);

    Species mistUserSpecies = makeSpecies(8131, "MistUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species mistTargetSpecies = makeSpecies(8132, "MistTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species disableUserSpecies = makeSpecies(8133, "DisableUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon mistUser = makePokemon(mistUserSpecies, AbilityType::None);
    Pokemon mistTarget = makePokemon(mistTargetSpecies, AbilityType::None);
    Pokemon disableUser = makePokemon(disableUserSpecies, AbilityType::None);

    Side mistSideA("MistA");
    Side mistSideB("MistB");
    ASSERT_TRUE(mistSideA.addPokemon(&mistUser));
    ASSERT_TRUE(mistSideA.addPokemon(&mistTarget));
    ASSERT_TRUE(mistSideB.addPokemon(&disableUser));

    Battle mistBattle(mistSideA, mistSideB);
    Move mist = createMoveByName("Mist");
    Move sandAttack = createMoveByName("Sand Attack");

    mistBattle.processMoveEffects(&mistUser, &mistTarget, mist);
    mistBattle.processMoveEffects(&disableUser, &mistTarget, sandAttack);
    EXPECT_EQ(mistTarget.getEvasionStage(), 0);

    Move thunderbolt = createMoveByName("Thunderbolt");
    mistBattle.enqueueAction(BattleAction::makeAttack(&mistTarget, &disableUser, thunderbolt));
    mistBattle.resolveNextAction();

    Move disable = createMoveByName("Disable");
    mistBattle.processMoveEffects(&mistUser, &mistTarget, disable);

    const int disableUserHpBefore = disableUser.getCurrentHP();
    mistBattle.enqueueAction(BattleAction::makeAttack(&mistTarget, &disableUser, thunderbolt));
    mistBattle.resolveNextAction();

    EXPECT_EQ(disableUser.getCurrentHP(), disableUserHpBefore);
}

TEST(MoveBehaviorTest, DoubleTeamMinimizeAndSplashUpdateEvasionAndDoNothingElse) {
    Species userSpecies = makeSpecies(8141, "EvasionUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8142, "EvasionTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move doubleTeam = createMoveByName("Double Team");
    Move minimize = createMoveByName("Minimize");
    Move splash = createMoveByName("Splash");

    battle.processMoveEffects(&user, &target, doubleTeam);
    EXPECT_EQ(user.getEvasionStage(), 1);

    battle.processMoveEffects(&user, &target, minimize);
    EXPECT_EQ(user.getEvasionStage(), 3);

    const int hpBeforeSplash = user.getCurrentHP();
    battle.processMoveEffects(&user, &target, splash);
    EXPECT_EQ(user.getCurrentHP(), hpBeforeSplash);
    EXPECT_EQ(user.getEvasionStage(), 3);
}

TEST(MoveBehaviorTest, TeleportSwitchesUserToNextBenchPokemon) {
    Species teleporterSpecies = makeSpecies(8151, "Teleporter", Type::Psychic, Type::Count, AbilityType::None, AbilityType::None);
    Species benchSpecies = makeSpecies(8152, "BenchMate", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8153, "TeleportTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon teleporter = makePokemon(teleporterSpecies, AbilityType::None);
    Pokemon benchMate = makePokemon(benchSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&teleporter));
    ASSERT_TRUE(sideA.addPokemon(&benchMate));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move teleport = createMoveByName("Teleport");

    battle.processMoveEffects(&teleporter, &target, teleport);
    EXPECT_EQ(battle.getSideA().getActivePokemon(), &benchMate);
}

TEST(MoveBehaviorTest, KinesisSweetScentAndPainSplitApplyExpectedEffects) {
    Species userSpecies = makeSpecies(8161, "UtilityUser", Type::Psychic, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8162, "UtilityTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move kinesis = createMoveByName("Kinesis");
    Move sweetScent = createMoveByName("Sweet Scent");
    Move painSplit = createMoveByName("Pain Split");

    battle.processMoveEffects(&user, &target, kinesis);
    EXPECT_EQ(target.getAccuracyStage(), -1);

    battle.processMoveEffects(&user, &target, sweetScent);
    EXPECT_EQ(target.getEvasionStage(), 2);

    user.setCurrentHP(40);
    target.setCurrentHP(100);
    battle.processMoveEffects(&user, &target, painSplit);
    EXPECT_EQ(user.getCurrentHP(), 70);
    EXPECT_EQ(target.getCurrentHP(), 70);
}

TEST(MoveBehaviorTest, RecoveryVariantsAndFocusEnergyMetronomeMirrorMoveWork) {
    Species userSpecies = makeSpecies(8171, "UtilityBatchUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8172, "UtilityBatchTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move slackOff = createMoveByName("Slack Off");
    Move morningSun = createMoveByName("Morning Sun");
    Move synthesis = createMoveByName("Synthesis");
    Move moonlight = createMoveByName("Moonlight");

    user.setCurrentHP(40);
    battle.processMoveEffects(&user, &target, slackOff);
    EXPECT_EQ(user.getCurrentHP(), 40 + user.getMaxHP() / 2);

    user.setCurrentHP(40);
    battle.getWeather().setWeather(WeatherType::Sun, 5);
    battle.processMoveEffects(&user, &target, morningSun);
    EXPECT_EQ(user.getCurrentHP(), std::min(user.getMaxHP(), 40 + (user.getMaxHP() * 2) / 3));

    user.setCurrentHP(80);
    battle.getWeather().setWeather(WeatherType::Rain, 5);
    battle.processMoveEffects(&user, &target, synthesis);
    EXPECT_EQ(user.getCurrentHP(), std::min(user.getMaxHP(), 80 + user.getMaxHP() / 4));

    user.setCurrentHP(50);
    battle.getWeather().setWeather(WeatherType::Clear, 0);
    battle.processMoveEffects(&user, &target, moonlight);
    EXPECT_EQ(user.getCurrentHP(), std::min(user.getMaxHP(), 50 + user.getMaxHP() / 2));

    Move focusEnergy = createMoveByName("Focus Energy");
    Move pound = createMoveByName("Pound");

    Species critUserSpecies = makeSpecies(8173, "CritUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species critTargetSpecies = makeSpecies(8174, "CritTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon normalCritUser = makePokemon(critUserSpecies, AbilityType::None);
    Pokemon focusCritUser = makePokemon(critUserSpecies, AbilityType::None);
    Pokemon normalCritTarget = makePokemon(critTargetSpecies, AbilityType::None);
    Pokemon focusCritTarget = makePokemon(critTargetSpecies, AbilityType::None);

    Side normalSideA("NormalCritA");
    Side normalSideB("NormalCritB");
    ASSERT_TRUE(normalSideA.addPokemon(&normalCritUser));
    ASSERT_TRUE(normalSideB.addPokemon(&normalCritTarget));
    Battle normalCritBattle(normalSideA, normalSideB);

    Side focusSideA("FocusCritA");
    Side focusSideB("FocusCritB");
    ASSERT_TRUE(focusSideA.addPokemon(&focusCritUser));
    ASSERT_TRUE(focusSideB.addPokemon(&focusCritTarget));
    Battle focusCritBattle(focusSideA, focusSideB);
    focusCritBattle.processMoveEffects(&focusCritUser, &focusCritTarget, focusEnergy);

    int normalTotalDamage = 0;
    int focusTotalDamage = 0;
    for (uint32_t seed = 1; seed <= 150; ++seed) {
        PRNG::setSeed(seed);
        normalTotalDamage += normalCritBattle.calculateDamage(&normalCritUser, &normalCritTarget, pound);

        PRNG::setSeed(seed);
        focusTotalDamage += focusCritBattle.calculateDamage(&focusCritUser, &focusCritTarget, pound);
    }
    EXPECT_GT(focusTotalDamage, normalTotalDamage);

    Move metronome = createMoveByName("Metronome");
    target.setCurrentHP(target.getMaxHP());
    PRNG::setSeed(1234);
    battle.processMoveEffects(&user, &target, metronome);
    EXPECT_LT(target.getCurrentHP(), target.getMaxHP());

    Move confuseRay = createMoveByName("Confuse Ray");
    Move mirrorMove = createMoveByName("Mirror Move");

    target.removeStatus(StatusType::Confusion);
    user.removeStatus(StatusType::Confusion);

    battle.enqueueAction(BattleAction::makeAttack(&target, &user, confuseRay));
    battle.resolveNextAction();
    EXPECT_TRUE(user.hasStatus(StatusType::Confusion));

    user.removeStatus(StatusType::Confusion);
    target.removeStatus(StatusType::Confusion);
    battle.processMoveEffects(&user, &target, mirrorMove);
    EXPECT_TRUE(target.hasStatus(StatusType::Confusion));
}

TEST(MoveBehaviorTest, SafeguardPsychUpHealBellAndAromatherapyApplyExpectedEffects) {
    Species userSpecies = makeSpecies(8181, "SupportUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species teammateSpecies = makeSpecies(8182, "SupportMate", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8183, "SupportTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon teammate = makePokemon(teammateSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideA.addPokemon(&teammate));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move safeguard = createMoveByName("Safeguard");
    Move thunderWave = createMoveByName("Thunder Wave");
    Move psychUp = createMoveByName("Psych Up");
    Move healBell = createMoveByName("Heal Bell");
    Move aromatherapy = createMoveByName("Aromatherapy");

    battle.processMoveEffects(&user, &target, safeguard);
    battle.processMoveEffects(&target, &user, thunderWave);
    EXPECT_FALSE(user.hasStatus(StatusType::Paralysis));

    target.changeStatStage(StatIndex::Attack, 2);
    target.changeStatStage(StatIndex::Speed, 1);
    target.changeAccuracyStage(-1);
    target.changeEvasionStage(2);

    battle.processMoveEffects(&user, &target, psychUp);
    EXPECT_EQ(user.getStatStage(StatIndex::Attack), 2);
    EXPECT_EQ(user.getStatStage(StatIndex::Speed), 1);
    EXPECT_EQ(user.getAccuracyStage(), -1);
    EXPECT_EQ(user.getEvasionStage(), 2);

    user.addStatus(StatusType::Poison);
    teammate.addStatus(StatusType::Burn);
    battle.processMoveEffects(&user, &target, healBell);
    EXPECT_FALSE(user.hasStatus(StatusType::Poison));
    EXPECT_FALSE(teammate.hasStatus(StatusType::Burn));

    user.addStatus(StatusType::Paralysis);
    teammate.addStatus(StatusType::Sleep, 2);
    battle.processMoveEffects(&user, &target, aromatherapy);
    EXPECT_FALSE(user.hasStatus(StatusType::Paralysis));
    EXPECT_FALSE(teammate.hasStatus(StatusType::Sleep));
}

TEST(MoveBehaviorTest, YawnTauntAndTormentControlMoveFlow) {
    PRNG::setSeed(8281);

    Species userSpecies = makeSpecies(8191, "ControlUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8192, "ControlTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move yawn = createMoveByName("Yawn");
    Move taunt = createMoveByName("Taunt");
    Move torment = createMoveByName("Torment");
    Move thunderWave = createMoveByName("Thunder Wave");
    Move pound = createMoveByName("Pound");

    battle.processMoveEffects(&user, &target, yawn);
    EXPECT_FALSE(target.hasStatus(StatusType::Sleep));
    battle.processTurn();
    EXPECT_FALSE(target.hasStatus(StatusType::Sleep));
    battle.processTurn();
    EXPECT_TRUE(target.hasStatus(StatusType::Sleep));
    target.clearStatuses();

    battle.processMoveEffects(&user, &target, taunt);
    const int userHpBeforeTauntedMove = user.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, thunderWave));
    battle.resolveNextAction();
    EXPECT_EQ(user.getCurrentHP(), userHpBeforeTauntedMove);
    EXPECT_FALSE(user.hasStatus(StatusType::Paralysis));

    battle.processMoveEffects(&user, &target, torment);
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
    battle.resolveNextAction();
    const int hpAfterFirstPound = user.getCurrentHP();

    battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
    battle.resolveNextAction();
    EXPECT_EQ(user.getCurrentHP(), hpAfterFirstPound);
}

TEST(MoveBehaviorTest, TauntExpiresAndAllowsStatusMovesAgain) {
    PRNG::setSeed(8282);

    Species userSpecies = makeSpecies(8193, "TauntUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8194, "TauntedTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move taunt = createMoveByName("Taunt");
    Move thunderWave = createMoveByName("Thunder Wave");

    ASSERT_FALSE(taunt.getName().empty());
    ASSERT_FALSE(thunderWave.getName().empty());

    battle.processMoveEffects(&user, &target, taunt);

    battle.enqueueAction(BattleAction::makeAttack(&target, &user, thunderWave));
    battle.resolveNextAction();
    EXPECT_FALSE(user.hasStatus(StatusType::Paralysis));

    for (int i = 0; i < 3; ++i) {
        battle.enqueueAction(BattleAction::makePass(&user));
        battle.enqueueAction(BattleAction::makePass(&target));
        battle.processTurn();
    }

    battle.enqueueAction(BattleAction::makeAttack(&target, &user, thunderWave));
    battle.resolveNextAction();
    EXPECT_TRUE(user.hasStatus(StatusType::Paralysis));
}

TEST(MoveBehaviorTest, TormentExpiresAndAllowsRepeatedMoveAgain) {
    PRNG::setSeed(8283);

    Species userSpecies = makeSpecies(8195, "TormentUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8196, "TormentedTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move torment = createMoveByName("Torment");
    Move pound = createMoveByName("Pound");

    ASSERT_FALSE(torment.getName().empty());
    ASSERT_FALSE(pound.getName().empty());

    battle.processMoveEffects(&user, &target, torment);

    const int hpBeforeFirstPound = user.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
    battle.resolveNextAction();
    EXPECT_LT(user.getCurrentHP(), hpBeforeFirstPound);

    const int hpBeforeBlockedPound = user.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
    battle.resolveNextAction();
    EXPECT_EQ(user.getCurrentHP(), hpBeforeBlockedPound);

    for (int i = 0; i < 3; ++i) {
        battle.enqueueAction(BattleAction::makePass(&user));
        battle.enqueueAction(BattleAction::makePass(&target));
        battle.processTurn();
    }

    const int hpBeforeAllowedPound = user.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
    battle.resolveNextAction();
    EXPECT_LT(user.getCurrentHP(), hpBeforeAllowedPound);
}

TEST(MoveBehaviorTest, DisableExpiresAndAllowsDisabledMoveAgain) {
    PRNG::setSeed(8284);

    Species userSpecies = makeSpecies(8197, "DisableApplier", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8198, "DisabledTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move disable = createMoveByName("Disable");
    Move pound = createMoveByName("Pound");

    ASSERT_FALSE(disable.getName().empty());
    ASSERT_FALSE(pound.getName().empty());

    const int hpBeforeFirstPound = user.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
    battle.resolveNextAction();
    EXPECT_LT(user.getCurrentHP(), hpBeforeFirstPound);

    battle.processMoveEffects(&user, &target, disable);

    const int hpBeforeBlockedPound = user.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
    battle.resolveNextAction();
    EXPECT_EQ(user.getCurrentHP(), hpBeforeBlockedPound);

    for (int i = 0; i < 4; ++i) {
        battle.enqueueAction(BattleAction::makePass(&user));
        battle.enqueueAction(BattleAction::makePass(&target));
        battle.processTurn();
    }

    const int hpBeforeAllowedPound = user.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
    battle.resolveNextAction();
    EXPECT_LT(user.getCurrentHP(), hpBeforeAllowedPound);
}

TEST(MoveBehaviorTest, RefreshCamouflageAndImprisonApplyExpectedEffects) {
    PRNG::setSeed(8291);

    Species userSpecies = makeSpecies(8201, "ImprisonUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8202, "ImprisonTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    user.addMove(createMoveByName("Pound"));
    user.addMove(createMoveByName("Imprison"));
    target.addMove(createMoveByName("Pound"));

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move refresh = createMoveByName("Refresh");
    Move camouflage = createMoveByName("Camouflage");
    Move electricTerrain = createMoveByName("Electric Terrain");
    Move imprison = createMoveByName("Imprison");
    Move pound = createMoveByName("Pound");

    user.addStatus(StatusType::Paralysis);
    user.addStatus(StatusType::Poison);
    battle.processMoveEffects(&user, &target, refresh);
    EXPECT_FALSE(user.hasStatus(StatusType::Paralysis));
    EXPECT_FALSE(user.hasStatus(StatusType::Poison));

    battle.processMoveEffects(&user, &target, electricTerrain);
    battle.processMoveEffects(&user, &target, camouflage);
    EXPECT_EQ(user.getType1(), Type::Electric);
    EXPECT_EQ(user.getType2(), Type::Count);

    battle.processMoveEffects(&user, &target, imprison);
    const int userHpBeforeBlockedPound = user.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
    battle.resolveNextAction();
    EXPECT_EQ(user.getCurrentHP(), userHpBeforeBlockedPound);
}

TEST(MoveBehaviorTest, WishAndConversionApplyExpectedEffects) {
    PRNG::setSeed(8301);

    Species userSpecies = makeSpecies(8211, "ConversionUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8212, "ConversionTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    user.addMove(createMoveByName("Thunderbolt"));

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move wish = createMoveByName("Wish");
    Move conversion = createMoveByName("Conversion");
    Move thunderbolt = createMoveByName("Thunderbolt");

    user.setCurrentHP(40);
    battle.processMoveEffects(&user, &target, wish);
    battle.processTurn();
    EXPECT_EQ(user.getCurrentHP(), 40);
    battle.processTurn();
    EXPECT_GT(user.getCurrentHP(), 40);

    battle.processMoveEffects(&user, &target, conversion);
    EXPECT_EQ(user.getType1(), Type::Electric);
    EXPECT_EQ(user.getType2(), Type::Count);
}

TEST(MoveBehaviorTest, ShoreUpAndHealPulseApplyExpectedRecovery) {
    Species userSpecies = makeSpecies(8213, "RecoveryUser", Type::Ground, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8214, "RecoveryTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move shoreUp = createMoveByName("Shore Up");
    Move healPulse = createMoveByName("Heal Pulse");

    user.setCurrentHP(40);
    battle.getWeather().setWeather(WeatherType::Sandstorm, 5);
    battle.processMoveEffects(&user, &target, shoreUp);
    EXPECT_EQ(user.getCurrentHP(), 40 + std::max(1, (user.getMaxHP() * 2) / 3));

    target.setCurrentHP(30);
    battle.processMoveEffects(&user, &target, healPulse);
    EXPECT_EQ(target.getCurrentHP(), 30 + std::max(1, target.getMaxHP() / 2));
}

TEST(MoveBehaviorTest, HealingWishAndLunarDanceRestoreNextSwitchInPokemon) {
    Species leadSpecies = makeSpecies(8215, "WishLead", Type::Psychic, Type::Count, AbilityType::None, AbilityType::None);
    Species benchSpecies = makeSpecies(8216, "WishBench", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8217, "WishTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    {
        Pokemon lead = makePokemon(leadSpecies, AbilityType::None);
        Pokemon bench = makePokemon(benchSpecies, AbilityType::None);
        Pokemon target = makePokemon(targetSpecies, AbilityType::None);

        Side sideA("A");
        Side sideB("B");
        ASSERT_TRUE(sideA.addPokemon(&lead));
        ASSERT_TRUE(sideA.addPokemon(&bench));
        ASSERT_TRUE(sideB.addPokemon(&target));

        Battle battle(sideA, sideB);
        Move healingWish = createMoveByName("Healing Wish");
        bench.setCurrentHP(20);
        bench.addStatus(StatusType::Burn);

        battle.processMoveEffects(&lead, &target, healingWish);
        EXPECT_TRUE(lead.isFainted());
        ASSERT_TRUE(battle.switchPokemon(battle.getSideA(), 1));
        EXPECT_EQ(bench.getCurrentHP(), bench.getMaxHP());
        EXPECT_FALSE(bench.hasStatus(StatusType::Burn));
    }

    {
        Pokemon lead = makePokemon(leadSpecies, AbilityType::None);
        Pokemon bench = makePokemon(benchSpecies, AbilityType::None);
        Pokemon target = makePokemon(targetSpecies, AbilityType::None);

        Move pound = createMoveByName("Pound");
        ASSERT_FALSE(pound.getName().empty());
        ASSERT_GT(pound.getMaxPP(), 1);
        pound.setPP(1);
        bench.addMove(pound);

        Side sideA("A");
        Side sideB("B");
        ASSERT_TRUE(sideA.addPokemon(&lead));
        ASSERT_TRUE(sideA.addPokemon(&bench));
        ASSERT_TRUE(sideB.addPokemon(&target));

        Battle battle(sideA, sideB);
        Move lunarDance = createMoveByName("Lunar Dance");

        bench.setCurrentHP(18);
        bench.addStatus(StatusType::Poison);
        std::vector<Move> reducedPpMoves = bench.getMoves();
        ASSERT_FALSE(reducedPpMoves.empty());
        reducedPpMoves[0].setPP(1);
        const int moveMaxPp = reducedPpMoves[0].getMaxPP();
        bench.replaceMoves(reducedPpMoves);

        battle.processMoveEffects(&lead, &target, lunarDance);
        EXPECT_TRUE(lead.isFainted());
        ASSERT_TRUE(battle.switchPokemon(battle.getSideA(), 1));
        EXPECT_EQ(bench.getCurrentHP(), bench.getMaxHP());
        EXPECT_FALSE(bench.hasStatus(StatusType::Poison));
        ASSERT_FALSE(bench.getMoves().empty());
        EXPECT_EQ(bench.getMoves()[0].getPP(), moveMaxPp);
    }
}

TEST(MoveBehaviorTest, LifeDewAndLunarBlessingApplyExpectedRecovery) {
    Species userSpecies = makeSpecies(8218, "BlessingUser", Type::Psychic, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8219, "BlessingTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move lifeDew = createMoveByName("Life Dew");
    Move lunarBlessing = createMoveByName("Lunar Blessing");

    ASSERT_FALSE(lifeDew.getName().empty());
    ASSERT_FALSE(lunarBlessing.getName().empty());

    user.setCurrentHP(40);
    battle.processMoveEffects(&user, &target, lifeDew);
    EXPECT_EQ(user.getCurrentHP(), 40 + std::max(1, user.getMaxHP() / 4));

    user.setCurrentHP(50);
    user.addStatus(StatusType::Poison);
    battle.processMoveEffects(&user, &target, lunarBlessing);
    EXPECT_EQ(user.getCurrentHP(), 50 + std::max(1, user.getMaxHP() / 4));
    EXPECT_FALSE(user.hasStatus(StatusType::Poison));
}

TEST(MoveBehaviorTest, HealBlockPreventsHealingMovesDuringDuration) {
    PRNG::setSeed(8361);

    Species userSpecies = makeSpecies(8220, "HealBlockUser", Type::Psychic, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8221, "HealBlockTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move healBlock = createMoveByName("Heal Block");
    Move recover = createMoveByName("Recover");
    Move roost = createMoveByName("Roost");

    ASSERT_FALSE(healBlock.getName().empty());
    ASSERT_FALSE(recover.getName().empty());
    ASSERT_FALSE(roost.getName().empty());

    target.setCurrentHP(40);
    battle.processMoveEffects(&user, &target, healBlock);

    const int hpBeforeBlockedRecover = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, recover));
    battle.resolveNextAction();
    EXPECT_EQ(target.getCurrentHP(), hpBeforeBlockedRecover);

    const int hpBeforeBlockedRoost = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, roost));
    battle.resolveNextAction();
    EXPECT_EQ(target.getCurrentHP(), hpBeforeBlockedRoost);

    for (int i = 0; i < 5; ++i) {
        battle.enqueueAction(BattleAction::makePass(&user));
        battle.enqueueAction(BattleAction::makePass(&target));
        battle.processTurn();
    }

    const int hpBeforeAllowedRecover = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, recover));
    battle.resolveNextAction();
    EXPECT_GT(target.getCurrentHP(), hpBeforeAllowedRecover);
}

TEST(MoveBehaviorTest, HealBlockBlocksWishAndLifeDewThenExpires) {
    PRNG::setSeed(8362);

    Species userSpecies = makeSpecies(8224, "HealBlockCaster", Type::Psychic, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8225, "HealBlockVictim", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move healBlock = createMoveByName("Heal Block");
    Move lifeDew = createMoveByName("Life Dew");
    Move wish = createMoveByName("Wish");

    ASSERT_FALSE(healBlock.getName().empty());
    ASSERT_FALSE(lifeDew.getName().empty());
    ASSERT_FALSE(wish.getName().empty());

    target.setCurrentHP(40);
    battle.processMoveEffects(&user, &target, healBlock);

    const int hpBeforeBlockedLifeDew = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makePass(&user));
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, lifeDew));
    battle.processTurn();
    EXPECT_EQ(target.getCurrentHP(), hpBeforeBlockedLifeDew);

    const int hpBeforeBlockedWish = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makePass(&user));
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, wish));
    battle.processTurn();
    EXPECT_EQ(target.getCurrentHP(), hpBeforeBlockedWish);

    battle.enqueueAction(BattleAction::makePass(&user));
    battle.enqueueAction(BattleAction::makePass(&target));
    battle.processTurn();
    battle.enqueueAction(BattleAction::makePass(&user));
    battle.enqueueAction(BattleAction::makePass(&target));
    battle.processTurn();
    EXPECT_EQ(target.getCurrentHP(), hpBeforeBlockedWish);

    for (int i = 0; i < 5; ++i) {
        battle.enqueueAction(BattleAction::makePass(&user));
        battle.enqueueAction(BattleAction::makePass(&target));
        battle.processTurn();
    }

    const int hpBeforeAllowedLifeDew = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makePass(&user));
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, lifeDew));
    battle.processTurn();
    EXPECT_GT(target.getCurrentHP(), hpBeforeAllowedLifeDew);
}

TEST(MoveBehaviorTest, HealBlockBlocksLunarBlessingThenExpires) {
    PRNG::setSeed(8363);

    Species userSpecies = makeSpecies(8226, "HealBlockCaster2", Type::Psychic, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8227, "HealBlockVictim2", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move healBlock = createMoveByName("Heal Block");
    Move lunarBlessing = createMoveByName("Lunar Blessing");

    ASSERT_FALSE(healBlock.getName().empty());
    ASSERT_FALSE(lunarBlessing.getName().empty());

    target.setCurrentHP(40);
    target.addStatus(StatusType::Poison);
    battle.processMoveEffects(&user, &target, healBlock);

    const int hpBeforeBlockedLunarBlessing = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makePass(&user));
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, lunarBlessing));
    battle.processTurn();
    EXPECT_EQ(target.getCurrentHP(), hpBeforeBlockedLunarBlessing);
    EXPECT_TRUE(target.hasStatus(StatusType::Poison));

    for (int i = 0; i < 5; ++i) {
        battle.enqueueAction(BattleAction::makePass(&user));
        battle.enqueueAction(BattleAction::makePass(&target));
        battle.processTurn();
    }

    const int hpBeforeAllowedLunarBlessing = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makePass(&user));
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, lunarBlessing));
    battle.processTurn();
    EXPECT_GT(target.getCurrentHP(), hpBeforeAllowedLunarBlessing);
    EXPECT_FALSE(target.hasStatus(StatusType::Poison));
}

TEST(MoveBehaviorTest, EmbargoBlocksItemUseUntilItExpires) {
    Species userSpecies = makeSpecies(8222, "EmbargoUser", Type::Dark, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8223, "EmbargoTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    target.holdItem(ItemType::OranBerry);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move embargo = createMoveByName("Embargo");
    ASSERT_FALSE(embargo.getName().empty());

    target.setCurrentHP(std::max(1, target.getMaxHP() / 2));
    const int hpBeforeBlockedItem = target.getCurrentHP();

    battle.processMoveEffects(&user, &target, embargo);
    battle.enqueueAction(BattleAction::makeUseItem(&target, &target, ItemType::OranBerry));
    battle.processTurn();
    EXPECT_EQ(target.getCurrentHP(), hpBeforeBlockedItem);
    EXPECT_EQ(target.getItemType(), ItemType::OranBerry);
    EXPECT_EQ(countSpecialEventsByReason("embargo"), 1);

    for (int i = 0; i < 5; ++i) {
        battle.enqueueAction(BattleAction::makePass(&user));
        battle.enqueueAction(BattleAction::makePass(&target));
        battle.processTurn();
    }

    const int hpBeforeAllowedItem = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makeUseItem(&target, &target, ItemType::OranBerry));
    battle.processTurn();
    EXPECT_EQ(target.getCurrentHP(), hpBeforeAllowedItem);
    EXPECT_EQ(target.getItemType(), ItemType::OranBerry);
    EXPECT_EQ(countSpecialEventsByReason("embargo"), 1);
}

TEST(MoveBehaviorTest, EndureLockOnAndSpiteApplyExpectedEffects) {
    PRNG::setSeed(8311);

    Species userSpecies = makeSpecies(8221, "ControlUser2", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8222, "ControlTarget2", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    user.addMove(createMoveByName("Endure"));
    user.addMove(createMoveByName("Lock-On"));
    user.addMove(createMoveByName("Spite"));
    target.addMove(createMoveByName("Pound"));

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move endure = createMoveByName("Endure");
    Move lockOn = createMoveByName("Lock-On");
    Move spite = createMoveByName("Spite");
    Move pound = createMoveByName("Pound");
    Move fatalHit("Fatal Hit", Type::Normal, Category::Physical, 500, 100, 5, MoveEffect::None, 0);
    Move impossibleHit("Impossible Hit", Type::Normal, Category::Physical, 80, 0, 5, MoveEffect::None, 0);

    battle.processMoveEffects(&user, &target, endure);
    const int hpBeforeFatalHit = user.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, fatalHit));
    battle.resolveNextAction();
    EXPECT_EQ(user.getCurrentHP(), 1);
    EXPECT_LT(user.getCurrentHP(), hpBeforeFatalHit);

    battle.processMoveEffects(&user, &target, lockOn);
    const int targetHpBeforeForcedHit = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&user, &target, impossibleHit));
    battle.resolveNextAction();
    EXPECT_LT(target.getCurrentHP(), targetHpBeforeForcedHit);

    const auto findMovePp = [](const Pokemon& pokemon, const std::string& moveName) {
        for (const Move& move : pokemon.getMoves()) {
            if (move.getName() == moveName) {
                return move.getPP();
            }
        }
        return -1;
    };

    const int ppBeforeSpite = findMovePp(target, "Pound");
    battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
    battle.resolveNextAction();
    battle.processMoveEffects(&user, &target, spite);
    const int ppAfterSpite = findMovePp(target, "Pound");
    ASSERT_GE(ppBeforeSpite, 0);
    ASSERT_GE(ppAfterSpite, 0);
    EXPECT_EQ(ppAfterSpite, std::max(0, ppBeforeSpite - 4));
}

TEST(MoveBehaviorTest, RestSweetKissAndNightmareApplyExpectedEffects) {
    PRNG::setSeed(8321);

    Species userSpecies = makeSpecies(8231, "SleepCaster", Type::Psychic, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8232, "SleepTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move rest = createMoveByName("Rest");
    Move sweetKiss = createMoveByName("Sweet Kiss");
    Move nightmare = createMoveByName("Nightmare");

    user.setCurrentHP(30);
    user.addStatus(StatusType::Burn);
    battle.processMoveEffects(&user, &target, rest);
    EXPECT_EQ(user.getCurrentHP(), user.getMaxHP());
    EXPECT_TRUE(user.hasStatus(StatusType::Sleep));
    EXPECT_FALSE(user.hasStatus(StatusType::Burn));

    battle.processMoveEffects(&user, &target, sweetKiss);
    EXPECT_TRUE(target.hasStatus(StatusType::Confusion));

    target.addStatus(StatusType::Sleep, 2);
    const int hpBeforeNightmareTick = target.getCurrentHP();
    battle.processMoveEffects(&user, &target, nightmare);
    battle.processTurn();
    EXPECT_LT(target.getCurrentHP(), hpBeforeNightmareTick);
}

TEST(MoveBehaviorTest, SpiderWebMeanLookAndBlockPreventTargetSwitch) {
    PRNG::setSeed(8331);

    Species userSpecies = makeSpecies(8241, "TrapUser", Type::Bug, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8242, "TrapTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species benchSpecies = makeSpecies(8243, "BenchMon", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    auto runTrapCase = [&](const std::string& moveName) {
        Pokemon user = makePokemon(userSpecies, AbilityType::None);
        Pokemon target = makePokemon(targetSpecies, AbilityType::None);
        Pokemon bench = makePokemon(benchSpecies, AbilityType::None);

        Side sideA("A");
        Side sideB("B");
        ASSERT_TRUE(sideA.addPokemon(&user));
        ASSERT_TRUE(sideB.addPokemon(&target));
        ASSERT_TRUE(sideB.addPokemon(&bench));

        Battle battle(sideA, sideB);
        Move trapMove = createMoveByName(moveName);
        battle.processMoveEffects(&user, &target, trapMove);

        const int targetSideIndexBefore = battle.getSideB().getActiveIndex();
        battle.enqueueAction(BattleAction::makeSwitch(&target, 1));
        battle.resolveNextAction();
        EXPECT_EQ(battle.getSideB().getActiveIndex(), targetSideIndexBefore);
    };

    runTrapCase("Spider Web");
    runTrapCase("Mean Look");
    runTrapCase("Block");
}

TEST(MoveBehaviorTest, BellyDrumRoostIngrainPerishSongAndTeeterDanceApplyExpectedEffects) {
    PRNG::setSeed(8341);

    Species userSpecies = makeSpecies(8251, "BatchUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8252, "BatchTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species benchSpecies = makeSpecies(8253, "BatchBench", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    Pokemon bench = makePokemon(benchSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));
    ASSERT_TRUE(sideB.addPokemon(&bench));

    Battle battle(sideA, sideB);
    Move bellyDrum = createMoveByName("Belly Drum");
    Move roost = createMoveByName("Roost");
    Move ingrain = createMoveByName("Ingrain");
    Move perishSong = createMoveByName("Perish Song");
    Move teeterDance = createMoveByName("Teeter Dance");

    const int hpBeforeBellyDrum = user.getCurrentHP();
    battle.processMoveEffects(&user, &target, bellyDrum);
    EXPECT_LT(user.getCurrentHP(), hpBeforeBellyDrum);
    EXPECT_EQ(user.getStatStage(StatIndex::Attack), 6);

    user.setCurrentHP(20);
    battle.processMoveEffects(&user, &target, roost);
    EXPECT_GT(user.getCurrentHP(), 20);

    user.setCurrentHP(40);
    battle.processMoveEffects(&user, &target, ingrain);
    const int hpBeforeIngrainTick = user.getCurrentHP();
    battle.processTurn();
    EXPECT_GT(user.getCurrentHP(), hpBeforeIngrainTick);

    const int sideAIndexBeforeSwitch = battle.getSideA().getActiveIndex();
    battle.enqueueAction(BattleAction::makeSwitch(&user, 1));
    battle.resolveNextAction();
    EXPECT_EQ(battle.getSideA().getActiveIndex(), sideAIndexBeforeSwitch);

    battle.processMoveEffects(&user, &target, teeterDance);
    EXPECT_TRUE(target.hasStatus(StatusType::Confusion));

    battle.processMoveEffects(&user, &target, perishSong);
    EXPECT_FALSE(user.isFainted());
    EXPECT_FALSE(target.isFainted());
    battle.processTurn();
    battle.processTurn();
    battle.processTurn();
    EXPECT_TRUE(user.isFainted());
    EXPECT_TRUE(target.isFainted());
}

TEST(MoveBehaviorTest, AttractDestinyBondGrudgeCurseAndSleepTalkApplyExpectedEffects) {
    PRNG::setSeed(8351);

    Species userSpecies = makeSpecies(8261, "StateUser", Type::Ghost, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(8262, "StateTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Move attract = createMoveByName("Attract");
    Move destinyBond = createMoveByName("Destiny Bond");
    Move grudge = createMoveByName("Grudge");
    Move curse = createMoveByName("Curse");
    Move sleepTalk = createMoveByName("Sleep Talk");
    Move pound = createMoveByName("Pound");
    Move bite = createMoveByName("Bite");
    Move fatalHit("Fatal Hit", Type::Dark, Category::Physical, 500, 100, 5, MoveEffect::None, 0);

    {
        Pokemon user = makePokemon(userSpecies, AbilityType::None);
        Pokemon target = makePokemon(targetSpecies, AbilityType::None);
        Side sideA("A");
        Side sideB("B");
        ASSERT_TRUE(sideA.addPokemon(&user));
        ASSERT_TRUE(sideB.addPokemon(&target));
        Battle battle(sideA, sideB);

        battle.processMoveEffects(&user, &target, attract);
        const int userHpBeforeInfatuationAction = user.getCurrentHP();
        battle.enqueueAction(BattleAction::makeAttack(&target, &user, pound));
        battle.resolveNextAction();
        EXPECT_EQ(user.getCurrentHP(), userHpBeforeInfatuationAction);
    }

    {
        Pokemon user = makePokemon(userSpecies, AbilityType::None);
        Pokemon target = makePokemon(targetSpecies, AbilityType::None);
        Side sideA("A");
        Side sideB("B");
        ASSERT_TRUE(sideA.addPokemon(&user));
        ASSERT_TRUE(sideB.addPokemon(&target));
        Battle battle(sideA, sideB);

        battle.processMoveEffects(&user, &target, destinyBond);
        battle.enqueueAction(BattleAction::makeAttack(&target, &user, fatalHit));
        battle.resolveNextAction();
        EXPECT_TRUE(user.isFainted());
        EXPECT_TRUE(target.isFainted());
    }

    {
        Pokemon grudgeUser = makePokemon(userSpecies, AbilityType::None);
        Pokemon grudgeTarget = makePokemon(targetSpecies, AbilityType::None);
        grudgeTarget.addMove(bite);
        Side gSideA("GA");
        Side gSideB("GB");
        ASSERT_TRUE(gSideA.addPokemon(&grudgeUser));
        ASSERT_TRUE(gSideB.addPokemon(&grudgeTarget));
        Battle grudgeBattle(gSideA, gSideB);
        grudgeUser.setCurrentHP(1);

        const auto findMovePp = [](const Pokemon& pokemon, const std::string& moveName) {
            for (const Move& move : pokemon.getMoves()) {
                if (move.getName() == moveName) {
                    return move.getPP();
                }
            }
            return -1;
        };

        grudgeBattle.processMoveEffects(&grudgeUser, &grudgeTarget, grudge);
        grudgeBattle.enqueueAction(BattleAction::makeAttack(&grudgeTarget, &grudgeUser, bite));
        grudgeBattle.resolveNextAction();
        EXPECT_EQ(findMovePp(grudgeTarget, "Bite"), 0);
    }

    {
        Pokemon curseUser = makePokemon(userSpecies, AbilityType::None);
        Pokemon curseTarget = makePokemon(targetSpecies, AbilityType::None);
        Side cSideA("CA");
        Side cSideB("CB");
        ASSERT_TRUE(cSideA.addPokemon(&curseUser));
        ASSERT_TRUE(cSideB.addPokemon(&curseTarget));
        Battle curseBattle(cSideA, cSideB);

        const int userHpBeforeCurse = curseUser.getCurrentHP();
        const int targetHpBeforeCurseTick = curseTarget.getCurrentHP();
        curseBattle.processMoveEffects(&curseUser, &curseTarget, curse);
        EXPECT_LT(curseUser.getCurrentHP(), userHpBeforeCurse);
        curseBattle.processTurn();
        EXPECT_LT(curseTarget.getCurrentHP(), targetHpBeforeCurseTick);
    }

    {
        Pokemon sleepTalkUser = makePokemon(userSpecies, AbilityType::None);
        Pokemon sleepTalkTarget = makePokemon(targetSpecies, AbilityType::None);
        sleepTalkUser.addMove(createMoveByName("Pound"));
        sleepTalkUser.addMove(createMoveByName("Sleep Talk"));
        sleepTalkUser.addStatus(StatusType::Sleep, 2);
        Side sSideA("SA");
        Side sSideB("SB");
        ASSERT_TRUE(sSideA.addPokemon(&sleepTalkUser));
        ASSERT_TRUE(sSideB.addPokemon(&sleepTalkTarget));
        Battle sleepTalkBattle(sSideA, sSideB);

        const int sleepTalkTargetHpBefore = sleepTalkTarget.getCurrentHP();
        sleepTalkBattle.processMoveEffects(&sleepTalkUser, &sleepTalkTarget, sleepTalk);
        EXPECT_LT(sleepTalkTarget.getCurrentHP(), sleepTalkTargetHpBefore);
    }
}

TEST(MoveBehaviorTest, KnockOffRemovesItemAndGetsDamageBoostAgainstHeldItem) {
    PRNG::setSeed(701);

    Species attackerSpecies = makeSpecies(9001, "KnockUser", Type::Dark, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(9002, "ItemTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon withItem = makePokemon(targetSpecies, AbilityType::None);
    Pokemon withoutItem = makePokemon(targetSpecies, AbilityType::None);
    withItem.holdItem(ItemType::Leftovers);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&withItem));

    Battle battle(sideA, sideB);
    Move knockOff("Knock Off", Type::Dark, Category::Physical, 65, 100, 20, MoveEffect::KnockOff, 100);

    PRNG::setSeed(702);
    const int damageWithItem = battle.calculateDamage(&attacker, &withItem, knockOff);
    PRNG::setSeed(702);
    const int damageWithoutItem = battle.calculateDamage(&attacker, &withoutItem, knockOff);

    EXPECT_GT(damageWithItem, damageWithoutItem);

    battle.processMoveEffects(&attacker, &withItem, knockOff);
    EXPECT_EQ(withItem.getItemType(), ItemType::None);
}

TEST(MoveBehaviorTest, WeatherBallChangesWithWeatherAndBoostsDamage) {
    PRNG::setSeed(801);

    Species attackerSpecies = makeSpecies(9101, "WeatherUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(9102, "FireTarget", Type::Fire, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move weatherBall("Weather Ball", Type::Normal, Category::Special, 50, 100, 10, MoveEffect::WeatherBall, 100);

    PRNG::setSeed(802);
    const int clearDamage = battle.calculateDamage(&attacker, &target, weatherBall);

    battle.getWeather().setWeather(WeatherType::Rain, 5);
    PRNG::setSeed(802);
    const int rainyDamage = battle.calculateDamage(&attacker, &target, weatherBall);

    EXPECT_GT(rainyDamage, clearDamage);
}

TEST(MoveBehaviorTest, DigNeedsChargeTurnAndSecondTurnDealsDamage) {
    PRNG::setSeed(901);

    Species diggerSpecies = makeSpecies(9201, "Digger", Type::Ground, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(9202, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon digger = makePokemon(diggerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&digger));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move dig("Dig", Type::Ground, Category::Physical, 80, 100, 10, MoveEffect::Dig, 100);
    Move pound = createMoveByName("Pound");

    const int targetHpBeforeCharge = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&digger, &target, dig));
    battle.resolveNextAction();
    EXPECT_EQ(target.getCurrentHP(), targetHpBeforeCharge);

    const int diggerHpBeforePound = digger.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &digger, pound));
    battle.resolveNextAction();
    EXPECT_EQ(digger.getCurrentHP(), diggerHpBeforePound);

    const int targetHpBeforeHit = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&digger, &target, dig));
    battle.resolveNextAction();
    EXPECT_LT(target.getCurrentHP(), targetHpBeforeHit);
}

TEST(MoveBehaviorTest, EncoreForcesTargetToRepeatLastUsedMove) {
    PRNG::setSeed(1001);

    Species encoreSpecies = makeSpecies(9301, "EncoreUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(9302, "EncoreTarget", Type::Fire, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon encoreUser = makePokemon(encoreSpecies, AbilityType::None);
    Pokemon encoreTarget = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&encoreUser));
    ASSERT_TRUE(sideB.addPokemon(&encoreTarget));

    Battle battle(sideA, sideB);

    Move ember("Ember", Type::Fire, Category::Special, 40, 100, 25, MoveEffect::Burn, 0);
    Move pound = createMoveByName("Pound");
    Move encore("Encore", Type::Normal, Category::Status, 0, 100, 5, MoveEffect::Encore, 100);

    battle.enqueueAction(BattleAction::makeAttack(&encoreTarget, &encoreUser, ember));
    battle.resolveNextAction();
    
    battle.processMoveEffects(&encoreUser, &encoreTarget, encore);

    const int hpBefore = encoreUser.getCurrentHP();
    PRNG::setSeed(1002);
    const int expectedEmberDamage = battle.calculateDamage(&encoreTarget, &encoreUser, ember);

    encoreUser.setCurrentHP(hpBefore);
    PRNG::setSeed(1002);
    battle.enqueueAction(BattleAction::makeAttack(&encoreTarget, &encoreUser, pound));
    battle.resolveNextAction();
    const int actualDamage = hpBefore - encoreUser.getCurrentHP();

    EXPECT_EQ(actualDamage, expectedEmberDamage);
}

TEST(MoveBehaviorTest, RoundSecondUseInSameTurnHasHigherDamage) {
    PRNG::setSeed(1101);

    Species fastSpecies = makeSpecies(9401, "RoundFast", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species slowSpecies = makeSpecies(9402, "RoundSlow", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    fastSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 120;
    slowSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 60;

    Pokemon fast = makePokemon(fastSpecies, AbilityType::None);
    Pokemon slow = makePokemon(slowSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&fast));
    ASSERT_TRUE(sideB.addPokemon(&slow));

    Battle battle(sideA, sideB);
    Move round("Round", Type::Normal, Category::Special, 60, 100, 15, MoveEffect::Round, 100);

    const int slowHpBefore = slow.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&fast, &slow, round));
    battle.resolveNextAction();
    const int firstDamage = slowHpBefore - slow.getCurrentHP();

    const int fastHpBefore = fast.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&slow, &fast, round));
    battle.resolveNextAction();
    const int secondDamage = fastHpBefore - fast.getCurrentHP();

    EXPECT_GT(secondDamage, firstDamage);
}

TEST(MoveBehaviorTest, RoundCountsAsUsedEvenWhenItMisses) {
    PRNG::setSeed(1111);

    Species fastSpecies = makeSpecies(9403, "RoundMissFast", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species slowSpecies = makeSpecies(9404, "RoundMissSlow", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon fast = makePokemon(fastSpecies, AbilityType::None);
    Pokemon slow = makePokemon(slowSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&fast));
    ASSERT_TRUE(sideB.addPokemon(&slow));

    Battle battle(sideA, sideB);
    Move roundMiss("Round", Type::Normal, Category::Special, 60, 0, 15, MoveEffect::Round, 100);
    Move roundHit("Round", Type::Normal, Category::Special, 60, 100, 15, MoveEffect::Round, 100);

    battle.enqueueAction(BattleAction::makeAttack(&fast, &slow, roundMiss));
    battle.resolveNextAction();

    PRNG::setSeed(1112);
    const int hpBeforeBoostedHit = slow.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&fast, &slow, roundHit));
    battle.resolveNextAction();
    const int boostedDamage = hpBeforeBoostedHit - slow.getCurrentHP();

    Species controlFastSpecies = makeSpecies(9405, "RoundControlFast", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species controlSlowSpecies = makeSpecies(9406, "RoundControlSlow", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Pokemon controlFast = makePokemon(controlFastSpecies, AbilityType::None);
    Pokemon controlSlow = makePokemon(controlSlowSpecies, AbilityType::None);

    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlFast));
    ASSERT_TRUE(controlSideB.addPokemon(&controlSlow));

    Battle controlBattle(controlSideA, controlSideB);
    PRNG::setSeed(1112);
    const int controlHpBefore = controlSlow.getCurrentHP();
    controlBattle.enqueueAction(BattleAction::makeAttack(&controlFast, &controlSlow, roundHit));
    controlBattle.resolveNextAction();
    const int controlDamage = controlHpBefore - controlSlow.getCurrentHP();

    EXPECT_GT(boostedDamage, controlDamage);
}

TEST(MoveBehaviorTest, PopulationBombMatchesTenIndependentHitsAtFullAccuracy) {
    PRNG::setSeed(1701);

    Species attackerSpecies = makeSpecies(10301, "BombUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(10302, "BombTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move populationBomb("Population Bomb", Type::Normal, Category::Physical, 20, 100, 10, MoveEffect::None, 100);
    Move singleHit("Single Hit", Type::Normal, Category::Physical, 20, 100, 35, MoveEffect::None, 100);

    const int hpBeforePopulationBomb = defender.getCurrentHP();
    PRNG::setSeed(1702);
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, populationBomb));
    battle.resolveNextAction();
    const int populationBombDamage = hpBeforePopulationBomb - defender.getCurrentHP();

    Pokemon controlAttacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon controlDefender = makePokemon(defenderSpecies, AbilityType::None);
    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlAttacker));
    ASSERT_TRUE(controlSideB.addPokemon(&controlDefender));
    Battle controlBattle(controlSideA, controlSideB);

    const int hpBeforeControl = controlDefender.getCurrentHP();
    PRNG::setSeed(1702);
    for (int i = 0; i < 10; ++i) {
        controlBattle.enqueueAction(BattleAction::makeAttack(&controlAttacker, &controlDefender, singleHit));
        controlBattle.resolveNextAction();
    }
    const int controlDamage = hpBeforeControl - controlDefender.getCurrentHP();

    EXPECT_EQ(populationBombDamage, controlDamage);
}

TEST(MoveBehaviorTest, PopulationBombStopsAfterFirstMiss) {
    Species attackerSpecies = makeSpecies(10303, "BombUserMiss", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(10304, "BombTargetMiss", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move populationBombAlwaysMiss("Population Bomb", Type::Normal, Category::Physical, 20, 0, 10, MoveEffect::None, 100);

    const int hpBefore = defender.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, populationBombAlwaysMiss));
    battle.resolveNextAction();

    EXPECT_EQ(defender.getCurrentHP(), hpBefore);
}

TEST(MoveBehaviorTest, FlyChargeTurnIsSemiInvulnerableExceptForHurricane) {
    PRNG::setSeed(1801);

    Species flyerSpecies = makeSpecies(10311, "Flyer", Type::Flying, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(10312, "FlyTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon flyer = makePokemon(flyerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&flyer));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move fly("Fly", Type::Flying, Category::Physical, 90, 95, 15, MoveEffect::None, 100);
    Move pound("Pound", Type::Normal, Category::Physical, 40, 100, 35, MoveEffect::None, 100);
    Move hurricane("Hurricane", Type::Flying, Category::Special, 110, 100, 10, MoveEffect::None, 100);

    battle.enqueueAction(BattleAction::makeAttack(&flyer, &target, fly));
    battle.resolveNextAction();

    const int hpBeforePound = flyer.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &flyer, pound));
    battle.resolveNextAction();
    EXPECT_EQ(flyer.getCurrentHP(), hpBeforePound);

    PRNG::setSeed(1802);
    const int hpBeforeHurricane = flyer.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &flyer, hurricane));
    battle.resolveNextAction();
    EXPECT_LT(flyer.getCurrentHP(), hpBeforeHurricane);
}

TEST(MoveBehaviorTest, FixedTwoHitMoveMatchesTwoIndependentSingleHits) {
    PRNG::setSeed(1901);

    Species attackerSpecies = makeSpecies(10321, "DoubleUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(10322, "DoubleTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move doubleHit("Double Hit", Type::Normal, Category::Physical, 35, 100, 10, MoveEffect::None, 100);
    Move singleHit("Single Hit", Type::Normal, Category::Physical, 35, 100, 35, MoveEffect::None, 100);

    const int hpBeforeDoubleHit = defender.getCurrentHP();
    PRNG::setSeed(1902);
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, doubleHit));
    battle.resolveNextAction();
    const int doubleHitDamage = hpBeforeDoubleHit - defender.getCurrentHP();

    Pokemon controlAttacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon controlDefender = makePokemon(defenderSpecies, AbilityType::None);
    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlAttacker));
    ASSERT_TRUE(controlSideB.addPokemon(&controlDefender));

    Battle controlBattle(controlSideA, controlSideB);
    const int hpBeforeControl = controlDefender.getCurrentHP();
    PRNG::setSeed(1902);
    controlBattle.enqueueAction(BattleAction::makeAttack(&controlAttacker, &controlDefender, singleHit));
    controlBattle.resolveNextAction();
    controlBattle.enqueueAction(BattleAction::makeAttack(&controlAttacker, &controlDefender, singleHit));
    controlBattle.resolveNextAction();
    const int controlDamage = hpBeforeControl - controlDefender.getCurrentHP();

    EXPECT_EQ(doubleHitDamage, controlDamage);
}

TEST(MoveBehaviorTest, DigAllowsEarthquakeToHitUndergroundTarget) {
    PRNG::setSeed(1121);

    Species diggerSpecies = makeSpecies(9203, "DigEarthquakeUser", Type::Ground, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(9204, "DigEarthquakeTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon digger = makePokemon(diggerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&digger));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move dig("Dig", Type::Ground, Category::Physical, 80, 100, 10, MoveEffect::Dig, 100);
    Move earthquake = createMoveByName("Earthquake");

    battle.enqueueAction(BattleAction::makeAttack(&digger, &target, dig));
    battle.resolveNextAction();

    const int diggerHpBeforeEarthquake = digger.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&target, &digger, earthquake));
    battle.resolveNextAction();

    EXPECT_LT(digger.getCurrentHP(), diggerHpBeforeEarthquake);
}

TEST(MoveBehaviorTest, PursuitTriggersOnOpponentSwitch) {
    PRNG::setSeed(1201);

    Species pursuerSpecies = makeSpecies(9501, "Pursuer", Type::Dark, Type::Count, AbilityType::None, AbilityType::None);
    Species switcherSpecies = makeSpecies(9502, "Switcher", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species benchSpecies = makeSpecies(9503, "Bench", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon pursuer = makePokemon(pursuerSpecies, AbilityType::None);
    Pokemon switcher = makePokemon(switcherSpecies, AbilityType::None);
    Pokemon bench = makePokemon(benchSpecies, AbilityType::None);
    Move pursuit("Pursuit", Type::Dark, Category::Physical, 40, 100, 20, MoveEffect::Pursuit, 100);
    pursuer.addMove(pursuit);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&pursuer));
    ASSERT_TRUE(sideB.addPokemon(&switcher));
    ASSERT_TRUE(sideB.addPokemon(&bench));

    Battle battle(sideA, sideB);

    const int hpBeforeSwitch = switcher.getCurrentHP();
    battle.enqueueAction(BattleAction::makeSwitch(&switcher, 1));
    battle.resolveNextAction();

    EXPECT_LT(switcher.getCurrentHP(), hpBeforeSwitch);
    EXPECT_EQ(battle.getSideB().getActivePokemon(), &bench);
}

TEST(MoveBehaviorTest, RecoveryMovesHealUserByHalfMaxHpAndCapAtMax) {
    Species healerSpecies = makeSpecies(9601, "Healer", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(9602, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon healer = makePokemon(healerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&healer));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);

    const int maxHp = healer.getMaxHP();
    const int halfHeal = std::max(1, maxHp / 2);

    healer.setCurrentHP(1);
    Move recover("Recover", Type::Normal, Category::Status, 0, 100, 10, MoveEffect::None, 100);
    battle.processMoveEffects(&healer, &target, recover);
    EXPECT_EQ(healer.getCurrentHP(), std::min(maxHp, 1 + halfHeal));

    healer.setCurrentHP(1);
    Move softBoiled("Soft Boiled", Type::Normal, Category::Status, 0, 100, 10, MoveEffect::None, 100);
    battle.processMoveEffects(&healer, &target, softBoiled);
    EXPECT_EQ(healer.getCurrentHP(), std::min(maxHp, 1 + halfHeal));

    healer.setCurrentHP(maxHp - 1);
    Move milkDrink("Milk Drink", Type::Normal, Category::Status, 0, 100, 10, MoveEffect::None, 100);
    battle.processMoveEffects(&healer, &target, milkDrink);
    EXPECT_EQ(healer.getCurrentHP(), maxHp);
}

TEST(MoveBehaviorTest, HazeResetsBothActivePokemonStatStages) {
    Species leftSpecies = makeSpecies(9701, "Left", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species rightSpecies = makeSpecies(9702, "Right", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon left = makePokemon(leftSpecies, AbilityType::None);
    Pokemon right = makePokemon(rightSpecies, AbilityType::None);

    left.changeStatStage(StatIndex::Attack, 2);
    left.changeStatStage(StatIndex::Speed, -1);
    right.changeStatStage(StatIndex::Defense, 3);
    right.changeStatStage(StatIndex::SpecialAttack, -2);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&left));
    ASSERT_TRUE(sideB.addPokemon(&right));

    Battle battle(sideA, sideB);
    Move haze("Haze", Type::Ice, Category::Status, 0, 100, 30, MoveEffect::None, 100);

    battle.processMoveEffects(&left, &right, haze);

    EXPECT_EQ(left.getStatStage(StatIndex::Attack), 0);
    EXPECT_EQ(left.getStatStage(StatIndex::Speed), 0);
    EXPECT_EQ(right.getStatStage(StatIndex::Defense), 0);
    EXPECT_EQ(right.getStatStage(StatIndex::SpecialAttack), 0);
}

TEST(MoveBehaviorTest, ReflectAndLightScreenReduceMatchingDamage) {
    PRNG::setSeed(1301);

    Species attackerSpecies = makeSpecies(9801, "Attacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(9802, "Defender", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);

    Move physicalMove("Pound", Type::Normal, Category::Physical, 40, 100, 35, MoveEffect::None, 100);
    Move specialMove("Water Gun", Type::Water, Category::Special, 40, 100, 25, MoveEffect::None, 100);
    Move reflect("Reflect", Type::Psychic, Category::Status, 0, 100, 20, MoveEffect::None, 100);
    Move lightScreen("Light Screen", Type::Psychic, Category::Status, 0, 100, 30, MoveEffect::None, 100);

    PRNG::setSeed(1302);
    const int physicalDamageWithoutReflect = battle.calculateDamage(&attacker, &defender, physicalMove);
    battle.processMoveEffects(&defender, &attacker, reflect);
    PRNG::setSeed(1302);
    const int physicalDamageWithReflect = battle.calculateDamage(&attacker, &defender, physicalMove);

    EXPECT_GT(physicalDamageWithoutReflect, physicalDamageWithReflect);

    battle.processMoveEffects(&defender, &attacker, lightScreen);
    PRNG::setSeed(1302);
    const int specialDamageWithLightScreen = battle.calculateDamage(&attacker, &defender, specialMove);
    battle.getSideB().setLightScreenTurns(0);
    PRNG::setSeed(1302);
    const int specialDamageWithoutLightScreen = battle.calculateDamage(&attacker, &defender, specialMove);

    EXPECT_GT(specialDamageWithoutLightScreen, specialDamageWithLightScreen);
}

TEST(MoveBehaviorTest, LeechSeedDrainsTargetAndHealsSourceOnFieldEffects) {
    Species seederSpecies = makeSpecies(9901, "Seeder", Type::Grass, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(9902, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon seeder = makePokemon(seederSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    seeder.setCurrentHP(seeder.getMaxHP() - 20);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&seeder));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move leechSeed("Leech Seed", Type::Grass, Category::Status, 0, 90, 10, MoveEffect::None, 100);

    battle.processMoveEffects(&seeder, &target, leechSeed);

    const int seederHpBefore = seeder.getCurrentHP();
    const int targetHpBefore = target.getCurrentHP();
    battle.applyFieldEffects();

    EXPECT_LT(target.getCurrentHP(), targetHpBefore);
    EXPECT_GT(seeder.getCurrentHP(), seederHpBefore);
    EXPECT_EQ(target.getLeechSeedSource(), &seeder);
}

TEST(MoveBehaviorTest, SubstituteBlocksStatusMovesAndAbsorbsDamage) {
    PRNG::setSeed(1401);

    Species substituteSpecies = makeSpecies(10001, "SubUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species attackerSpecies = makeSpecies(10002, "Attacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon substituteUser = makePokemon(substituteSpecies, AbilityType::None);
    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&substituteUser));
    ASSERT_TRUE(sideB.addPokemon(&attacker));

    Battle battle(sideA, sideB);
    Move substitute("Substitute", Type::Normal, Category::Status, 0, 100, 10, MoveEffect::None, 100);
    Move leechSeed("Leech Seed", Type::Grass, Category::Status, 0, 90, 10, MoveEffect::None, 100);
    Move pound("Pound", Type::Normal, Category::Physical, 40, 100, 35, MoveEffect::None, 100);

    const int hpBeforeSubstitute = substituteUser.getCurrentHP();
    battle.processMoveEffects(&substituteUser, &attacker, substitute);

    EXPECT_LT(substituteUser.getCurrentHP(), hpBeforeSubstitute);
    EXPECT_GT(substituteUser.getSubstituteHP(), 0);

    battle.processMoveEffects(&attacker, &substituteUser, leechSeed);
    EXPECT_EQ(substituteUser.getLeechSeedSource(), nullptr);

    const int hpBeforeAttack = substituteUser.getCurrentHP();
    const int substituteHpBeforeAttack = substituteUser.getSubstituteHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &substituteUser, pound));
    battle.resolveNextAction();

    EXPECT_EQ(substituteUser.getCurrentHP(), hpBeforeAttack);
    EXPECT_LT(substituteUser.getSubstituteHP(), substituteHpBeforeAttack);
}

TEST(MoveBehaviorTest, SandAttackRaisesEvasionAndCausesLowAccuracyMoveToMiss) {
    PRNG::setSeed(0);

    Species attackerSpecies = makeSpecies(10101, "Attacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(10102, "Defender", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move sandAttack("Sand Attack", Type::Ground, Category::Status, 0, 100, 15, MoveEffect::None, 100);
    Move lowAccuracyMove("Focus Blast", Type::Fighting, Category::Special, 120, 50, 5, MoveEffect::None, 100);

    for (int i = 0; i < 6; ++i) {
        battle.processMoveEffects(&attacker, &defender, sandAttack);
    }

    EXPECT_EQ(defender.getEvasionStage(), 6);

    const int hpBeforeMiss = defender.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, lowAccuracyMove));
    battle.resolveNextAction();

    EXPECT_EQ(defender.getCurrentHP(), hpBeforeMiss);
}

TEST(MoveBehaviorTest, GravityBoostsAccuracyForLowAccuracyMove) {
    auto countHitsOverSeries = [](uint32_t seed, bool useGravity, int attempts) {
        Species attackerSpecies = makeSpecies(10103, "GravityAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
        Species defenderSpecies = makeSpecies(10104, "GravityDefender", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

        Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
        Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);

        Side sideA("A");
        Side sideB("B");
        EXPECT_TRUE(sideA.addPokemon(&attacker));
        EXPECT_TRUE(sideB.addPokemon(&defender));

        Battle battle(sideA, sideB);
        Move lowAccuracyMove("Gravity Probe", Type::Normal, Category::Physical, 50, 60, 10, MoveEffect::None, 100);
        Move gravity = createMoveByName("Gravity");
        EXPECT_FALSE(gravity.getName().empty());

        if (useGravity) {
            battle.processMoveEffects(&attacker, &defender, gravity);
        }

        PRNG::setSeed(seed);
        int hitCount = 0;
        for (int i = 0; i < attempts; ++i) {
            defender.setCurrentHP(defender.getMaxHP());
            const int hpBefore = defender.getCurrentHP();
            battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, lowAccuracyMove));
            battle.resolveNextAction();
            if (defender.getCurrentHP() < hpBefore) {
                ++hitCount;
            }
        }
        return hitCount;
    };

    const int attempts = 120;
    const int noGravityHits = countHitsOverSeries(424242, false, attempts);
    const int gravityHits = countHitsOverSeries(424242, true, attempts);

    EXPECT_LT(noGravityHits, gravityHits);
    EXPECT_EQ(gravityHits, attempts);
}

TEST(MoveBehaviorTest, TerrainMovesModifyDamageAndBlockStatusInMistyTerrain) {
    PRNG::setSeed(1601);

    Species attackerSpecies = makeSpecies(10201, "TerrainAttacker", Type::Dragon, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(10202, "TerrainDefender", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);

    Move thunderbolt("Thunderbolt", Type::Electric, Category::Special, 90, 100, 15, MoveEffect::None, 100);
    Move pound("Pound", Type::Normal, Category::Physical, 40, 100, 35, MoveEffect::None, 100);
    Move dragonClaw("Dragon Claw", Type::Dragon, Category::Physical, 80, 100, 15, MoveEffect::None, 100);
    Move toxic("Toxic", Type::Poison, Category::Status, 0, 100, 10, MoveEffect::Poison, 100, 2, 0);
    Move electricTerrain("Electric Terrain", Type::Electric, Category::Status, 0, 100, 10, MoveEffect::None, 100);
    Move psychicTerrain("Psychic Terrain", Type::Psychic, Category::Status, 0, 100, 10, MoveEffect::None, 100);
    Move mistyTerrain("Misty Terrain", Type::Fairy, Category::Status, 0, 100, 10, MoveEffect::None, 100);

    PRNG::setSeed(1602);
    const int electricDamageWithoutTerrain = battle.calculateDamage(&attacker, &defender, thunderbolt);
    battle.processMoveEffects(&attacker, &defender, electricTerrain);
    EXPECT_EQ(battle.getField().type, FieldType::Electric);
    EXPECT_TRUE(battle.getField().isActive());

    PRNG::setSeed(1602);
    const int electricDamageWithTerrain = battle.calculateDamage(&attacker, &defender, thunderbolt);
    EXPECT_GT(electricDamageWithTerrain, electricDamageWithoutTerrain);

    PRNG::setSeed(1602);
    battle.getField().setField(FieldType::None, 0);
    const int physicalDamageWithoutTerrain = battle.calculateDamage(&attacker, &defender, pound);
    battle.processMoveEffects(&attacker, &defender, psychicTerrain);
    EXPECT_EQ(battle.getField().type, FieldType::Psychic);

    PRNG::setSeed(1602);
    const int physicalDamageWithTerrain = battle.calculateDamage(&attacker, &defender, pound);
    EXPECT_GT(physicalDamageWithoutTerrain, physicalDamageWithTerrain);

    PRNG::setSeed(1602);
    battle.getField().setField(FieldType::None, 0);
    const int dragonDamageWithoutMisty = battle.calculateDamage(&attacker, &defender, dragonClaw);
    battle.processMoveEffects(&attacker, &defender, mistyTerrain);
    EXPECT_EQ(battle.getField().type, FieldType::Misty);

    PRNG::setSeed(1602);
    const int dragonDamageWithMisty = battle.calculateDamage(&attacker, &defender, dragonClaw);
    EXPECT_GT(dragonDamageWithoutMisty, dragonDamageWithMisty);

    battle.processMoveEffects(&attacker, &defender, toxic);
    EXPECT_FALSE(defender.hasStatus(StatusType::Poison));
    EXPECT_FALSE(defender.hasStatus(StatusType::ToxicPoison));
}

TEST(MoveBehaviorTest, DetectProtectsUserFromOpponentAttack) {
    Species defenderSpecies = makeSpecies(10401, "DetectUser", Type::Fighting, Type::Count, AbilityType::None, AbilityType::None);
    Species attackerSpecies = makeSpecies(10402, "Attacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon detectUser = makePokemon(defenderSpecies, AbilityType::None);
    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&detectUser));
    ASSERT_TRUE(sideB.addPokemon(&attacker));

    Battle battle(sideA, sideB);
    Move detect = createMoveByName("Detect");
    Move pound = createMoveByName("Pound");

    battle.enqueueAction(BattleAction::makeAttack(&detectUser, &detectUser, detect));
    battle.resolveNextAction();

    const int hpBeforePound = detectUser.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &detectUser, pound));
    battle.resolveNextAction();

    EXPECT_EQ(detectUser.getCurrentHP(), hpBeforePound);
}

TEST(MoveBehaviorTest, ProtectFamilyConsecutiveUseHasFailurePenalty) {
    Species defenderSpecies = makeSpecies(104021, "ProtectUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species attackerSpecies = makeSpecies(104022, "PenaltyAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Move detect = createMoveByName("Detect");
    Move pound = createMoveByName("Pound");
    ASSERT_FALSE(detect.getName().empty());
    ASSERT_FALSE(pound.getName().empty());

    bool observedSecondUseFailure = false;
    for (int seed = 1; seed <= 256 && !observedSecondUseFailure; ++seed) {
        PRNG::setSeed(static_cast<uint32_t>(10402200 + seed));

        Pokemon detectUser = makePokemon(defenderSpecies, AbilityType::None);
        Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);

        Side sideA("A");
        Side sideB("B");
        ASSERT_TRUE(sideA.addPokemon(&detectUser));
        ASSERT_TRUE(sideB.addPokemon(&attacker));

        Battle battle(sideA, sideB);

        battle.enqueueAction(BattleAction::makeAttack(&detectUser, &detectUser, detect));
        battle.resolveNextAction();
        const int hpBeforeFirstAttack = detectUser.getCurrentHP();
        battle.enqueueAction(BattleAction::makeAttack(&attacker, &detectUser, pound));
        battle.resolveNextAction();
        EXPECT_EQ(detectUser.getCurrentHP(), hpBeforeFirstAttack);

        battle.enqueueAction(BattleAction::makePass(&detectUser));
        battle.enqueueAction(BattleAction::makePass(&attacker));
        battle.processTurn();

        battle.enqueueAction(BattleAction::makeAttack(&detectUser, &detectUser, detect));
        battle.resolveNextAction();
        const int hpBeforeSecondAttack = detectUser.getCurrentHP();
        battle.enqueueAction(BattleAction::makeAttack(&attacker, &detectUser, pound));
        battle.resolveNextAction();

        if (detectUser.getCurrentHP() < hpBeforeSecondAttack) {
            observedSecondUseFailure = true;
        }
    }

    EXPECT_TRUE(observedSecondUseFailure);
}

TEST(MoveBehaviorTest, ProtectVariantsApplyContactPunishments) {
    Species defenderSpecies = makeSpecies(10403, "ShieldUser", Type::Fighting, Type::Count, AbilityType::None, AbilityType::None);
    Species attackerSpecies = makeSpecies(10404, "ContactAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);
    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&defender));
    ASSERT_TRUE(sideB.addPokemon(&attacker));

    Move pound = createMoveByName("Pound");

    {
        Battle battle(sideA, sideB);
        Move spikyShield("Spiky Shield", Type::Grass, Category::Status, 0, 100, 10, MoveEffect::None, 100);
        battle.processMoveEffects(&defender, &attacker, spikyShield);
        const int hpBefore = attacker.getCurrentHP();
        const int damage = battle.calculateDamage(&attacker, &defender, pound);
        EXPECT_EQ(damage, 0);
        EXPECT_LT(attacker.getCurrentHP(), hpBefore);
    }

    attacker.setCurrentHP(attacker.getMaxHP());
    attacker.clearStatuses();
    defender.setCurrentHP(defender.getMaxHP());

    {
        Battle battle(sideA, sideB);
        Move kingsShield("King's Shield", Type::Steel, Category::Status, 0, 100, 10, MoveEffect::None, 100);
        battle.processMoveEffects(&defender, &attacker, kingsShield);
        const int damage = battle.calculateDamage(&attacker, &defender, pound);
        EXPECT_EQ(damage, 0);
        EXPECT_EQ(attacker.getStatStage(StatIndex::Attack), -2);
    }

    attacker.setCurrentHP(attacker.getMaxHP());
    attacker.clearStatuses();
    defender.setCurrentHP(defender.getMaxHP());

    {
        Battle battle(sideA, sideB);
        Move banefulBunker("Baneful Bunker", Type::Poison, Category::Status, 0, 100, 10, MoveEffect::None, 100);
        battle.processMoveEffects(&defender, &attacker, banefulBunker);
        const int damage = battle.calculateDamage(&attacker, &defender, pound);
        EXPECT_EQ(damage, 0);
        EXPECT_TRUE(attacker.hasStatus(StatusType::Poison));
    }

    attacker.setCurrentHP(attacker.getMaxHP());
    attacker.clearStatuses();
    defender.setCurrentHP(defender.getMaxHP());

    {
        Battle battle(sideA, sideB);
        Move burningBulwark("Burning Bulwark", Type::Fire, Category::Status, 0, 100, 10, MoveEffect::None, 100);
        battle.processMoveEffects(&defender, &attacker, burningBulwark);
        const int damage = battle.calculateDamage(&attacker, &defender, pound);
        EXPECT_EQ(damage, 0);
        EXPECT_TRUE(attacker.hasStatus(StatusType::Burn));
    }

    attacker.setCurrentHP(attacker.getMaxHP());
    attacker.clearStatuses();
    defender.setCurrentHP(defender.getMaxHP());
    attacker.changeStatStage(StatIndex::Attack, 2);

    {
        Battle battle(sideA, sideB);
        Move obstruct("Obstruct", Type::Dark, Category::Status, 0, 100, 10, MoveEffect::None, 100);
        battle.processMoveEffects(&defender, &attacker, obstruct);
        const int damage = battle.calculateDamage(&attacker, &defender, pound);
        EXPECT_EQ(damage, 0);
        EXPECT_EQ(attacker.getStatStage(StatIndex::Attack), -2);
    }
}

TEST(MoveBehaviorTest, ProtectFamilySharesConsecutivePenaltyAcrossVariants) {
    Species defenderSpecies = makeSpecies(104023, "ProtectChainUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species attackerSpecies = makeSpecies(104024, "ProtectChainAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Move detect = createMoveByName("Detect");
    Move spikyShield = createMoveByName("Spiky Shield");
    Move pound = createMoveByName("Pound");
    ASSERT_FALSE(detect.getName().empty());
    ASSERT_FALSE(spikyShield.getName().empty());
    ASSERT_FALSE(pound.getName().empty());

    bool observedSecondUseFailure = false;
    for (int seed = 1; seed <= 256 && !observedSecondUseFailure; ++seed) {
        PRNG::setSeed(static_cast<uint32_t>(10402400 + seed));

        Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);
        Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);

        Side sideA("A");
        Side sideB("B");
        ASSERT_TRUE(sideA.addPokemon(&defender));
        ASSERT_TRUE(sideB.addPokemon(&attacker));

        Battle battle(sideA, sideB);

        battle.enqueueAction(BattleAction::makeAttack(&defender, &defender, detect));
        battle.resolveNextAction();
        const int hpBeforeFirstAttack = defender.getCurrentHP();
        battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, pound));
        battle.resolveNextAction();
        EXPECT_EQ(defender.getCurrentHP(), hpBeforeFirstAttack);

        battle.enqueueAction(BattleAction::makePass(&defender));
        battle.enqueueAction(BattleAction::makePass(&attacker));
        battle.processTurn();

        battle.enqueueAction(BattleAction::makeAttack(&defender, &defender, spikyShield));
        battle.resolveNextAction();
        const int hpBeforeSecondAttack = defender.getCurrentHP();
        battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, pound));
        battle.resolveNextAction();

        if (defender.getCurrentHP() < hpBeforeSecondAttack) {
            observedSecondUseFailure = true;
        }
    }

    EXPECT_TRUE(observedSecondUseFailure);
}

TEST(MoveBehaviorTest, QuickGuardBlocksPriorityMovesButNotNormalPriority) {
    Species guarderSpecies = makeSpecies(10405, "QuickGuardUser", Type::Fighting, Type::Count, AbilityType::None, AbilityType::None);
    Species attackerSpecies = makeSpecies(10406, "PriorityAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon guarder = makePokemon(guarderSpecies, AbilityType::None);
    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&guarder));
    ASSERT_TRUE(sideB.addPokemon(&attacker));

    Battle battle(sideA, sideB);
    Move quickGuard = createMoveByName("Quick Guard");
    Move quickAttack = createMoveByName("Quick Attack");
    Move pound = createMoveByName("Pound");

    ASSERT_FALSE(quickGuard.getName().empty());
    ASSERT_FALSE(quickAttack.getName().empty());
    ASSERT_FALSE(pound.getName().empty());

    battle.processMoveEffects(&guarder, &attacker, quickGuard);
    const int hpBeforeQuickAttack = guarder.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &guarder, quickAttack));
    battle.resolveNextAction();
    EXPECT_EQ(guarder.getCurrentHP(), hpBeforeQuickAttack);

    battle.enqueueAction(BattleAction::makePass(&guarder));
    battle.enqueueAction(BattleAction::makePass(&attacker));
    battle.processTurn();

    const int hpBeforePound = guarder.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &guarder, pound));
    battle.resolveNextAction();
    EXPECT_LT(guarder.getCurrentHP(), hpBeforePound);
}

TEST(MoveBehaviorTest, WideGuardBlocksSpreadTargetMoves) {
    Species guarderSpecies = makeSpecies(10407, "WideGuardUser", Type::Rock, Type::Count, AbilityType::None, AbilityType::None);
    Species attackerSpecies = makeSpecies(10408, "SpreadAttacker", Type::Ground, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon guarder = makePokemon(guarderSpecies, AbilityType::None);
    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&guarder));
    ASSERT_TRUE(sideB.addPokemon(&attacker));

    Battle battle(sideA, sideB);
    Move wideGuard = createMoveByName("Wide Guard");
    Move spreadMove("Spread Move", Type::Ground, Category::Physical, 100, 100, 10, MoveEffect::None, 0, 0, 0, 0, Target::AllOpponents);

    ASSERT_FALSE(wideGuard.getName().empty());

    battle.processMoveEffects(&guarder, &attacker, wideGuard);
    const int hpBeforeEarthquake = guarder.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &guarder, spreadMove));
    battle.resolveNextAction();
    EXPECT_EQ(guarder.getCurrentHP(), hpBeforeEarthquake);
}

TEST(MoveBehaviorTest, FlinchPreventsTargetFromMovingThatTurn) {
    PRNG::setSeed(2301);

    Species fastSpecies = makeSpecies(10541, "FlinchUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species slowSpecies = makeSpecies(10542, "FlinchTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    fastSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 130;
    slowSpecies.baseStats[static_cast<int>(StatIndex::Speed)] = 30;

    Pokemon fast = makePokemon(fastSpecies, AbilityType::None);
    Pokemon slow = makePokemon(slowSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&fast));
    ASSERT_TRUE(sideB.addPokemon(&slow));

    Battle battle(sideA, sideB);
    Move flinchMove("Rock Slide", Type::Rock, Category::Physical, 75, 100, 10, MoveEffect::Flinch, 100);
    Move pound = createMoveByName("Pound");

    const int fastHpBefore = fast.getCurrentHP();
    const int slowHpBefore = slow.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&fast, &slow, flinchMove));
    battle.enqueueAction(BattleAction::makeAttack(&slow, &fast, pound));
    battle.processTurn();

    EXPECT_LT(slow.getCurrentHP(), slowHpBefore);
    EXPECT_EQ(fast.getCurrentHP(), fastHpBefore);
    EXPECT_TRUE(slow.getStatuses().empty());
}

TEST(MoveBehaviorTest, WeatherSettingMovesApplyExpectedWeatherState) {
    Species leftSpecies = makeSpecies(10411, "WeatherLeft", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species rightSpecies = makeSpecies(10412, "WeatherRight", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon left = makePokemon(leftSpecies, AbilityType::None);
    Pokemon right = makePokemon(rightSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&left));
    ASSERT_TRUE(sideB.addPokemon(&right));

    Battle battle(sideA, sideB);

    Move rainDance = createMoveByName("Rain Dance");
    Move sunnyDay = createMoveByName("Sunny Day");
    Move sandstorm = createMoveByName("Sandstorm");
    Move hail = createMoveByName("Hail");

    battle.processMoveEffects(&left, &right, rainDance);
    EXPECT_EQ(battle.getWeather().type, WeatherType::Rain);
    EXPECT_EQ(battle.getWeather().duration, 5);

    battle.processMoveEffects(&left, &right, sunnyDay);
    EXPECT_EQ(battle.getWeather().type, WeatherType::Sun);
    EXPECT_EQ(battle.getWeather().duration, 5);

    battle.processMoveEffects(&left, &right, sandstorm);
    EXPECT_EQ(battle.getWeather().type, WeatherType::Sandstorm);
    EXPECT_EQ(battle.getWeather().duration, 5);

    battle.processMoveEffects(&left, &right, hail);
    EXPECT_EQ(battle.getWeather().type, WeatherType::Hail);
    EXPECT_EQ(battle.getWeather().duration, 5);
}

TEST(MoveBehaviorTest, EntryHazardsApplyOnSwitchIn) {
    Species setterSpecies = makeSpecies(10421, "Setter", Type::Ground, Type::Count, AbilityType::None, AbilityType::None);
    Species activeSpecies = makeSpecies(10422, "ActiveTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species switchInSpecies = makeSpecies(10423, "SwitchInTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon setter = makePokemon(setterSpecies, AbilityType::None);
    Pokemon activeTarget = makePokemon(activeSpecies, AbilityType::None);
    Pokemon switchInTarget = makePokemon(switchInSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&setter));
    ASSERT_TRUE(sideB.addPokemon(&activeTarget));
    ASSERT_TRUE(sideB.addPokemon(&switchInTarget));

    Battle battle(sideA, sideB);

    Move spikes = createMoveByName("Spikes");
    Move toxicSpikes = createMoveByName("Toxic Spikes");
    Move stealthRock = createMoveByName("Stealth Rock");

    battle.processMoveEffects(&setter, &activeTarget, spikes);
    battle.processMoveEffects(&setter, &activeTarget, toxicSpikes);
    battle.processMoveEffects(&setter, &activeTarget, stealthRock);

    EXPECT_EQ(battle.getSideB().getSpikesLayers(), 1);
    EXPECT_EQ(battle.getSideB().getToxicSpikesLayers(), 1);
    EXPECT_TRUE(battle.getSideB().hasStealthRock());

    const int hpBeforeSwitchIn = switchInTarget.getCurrentHP();
    ASSERT_TRUE(battle.switchPokemon(battle.getSideB(), 1));

    EXPECT_LT(switchInTarget.getCurrentHP(), hpBeforeSwitchIn);
    EXPECT_TRUE(switchInTarget.hasStatus(StatusType::Poison));
}

TEST(MoveBehaviorTest, DefogClearsBothSidesHazardsAndCourtChangeSwapsThem) {
    Species setterSpecies = makeSpecies(10424, "Setter2", Type::Flying, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(10425, "Target2", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon setter = makePokemon(setterSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&setter));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);

    Move spikes = createMoveByName("Spikes");
    Move stealthRock = createMoveByName("Stealth Rock");
    Move defog = createMoveByName("Defog");
    Move courtChange = createMoveByName("Court Change");
    Move mudSport = createMoveByName("Mud Sport");
    Move waterSport = createMoveByName("Water Sport");

    battle.processMoveEffects(&setter, &target, spikes);
    battle.processMoveEffects(&setter, &target, stealthRock);
    battle.processMoveEffects(&setter, &target, mudSport);
    battle.processMoveEffects(&target, &setter, waterSport);
    battle.getSideA().setReflectTurns(5);
    battle.getSideB().setLightScreenTurns(5);

    battle.processMoveEffects(&setter, &target, defog);

    EXPECT_EQ(battle.getSideA().getSpikesLayers(), 0);
    EXPECT_EQ(battle.getSideB().getSpikesLayers(), 0);
    EXPECT_FALSE(battle.getSideA().hasStealthRock());
    EXPECT_FALSE(battle.getSideB().hasStealthRock());
    EXPECT_FALSE(battle.getSideA().hasReflect());
    EXPECT_FALSE(battle.getSideB().hasLightScreen());
    EXPECT_FALSE(battle.getSideA().hasMudSport());
    EXPECT_FALSE(battle.getSideA().hasWaterSport());
    EXPECT_FALSE(battle.getSideB().hasMudSport());
    EXPECT_FALSE(battle.getSideB().hasWaterSport());

    battle.processMoveEffects(&setter, &target, spikes);
    battle.processMoveEffects(&setter, &target, stealthRock);
    battle.getSideA().setReflectTurns(5);
    battle.getSideB().setLightScreenTurns(5);
    battle.getSideA().setMudSportTurns(4);
    battle.getSideB().setWaterSportTurns(3);
    battle.getSideA().addToxicSpikesLayer();
    battle.getSideB().addSpikesLayer();

    battle.processMoveEffects(&setter, &target, courtChange);

    EXPECT_EQ(battle.getSideA().getSpikesLayers(), 2);
    EXPECT_EQ(battle.getSideA().getToxicSpikesLayers(), 0);
    EXPECT_TRUE(battle.getSideA().hasStealthRock());
    EXPECT_FALSE(battle.getSideA().hasReflect());
    EXPECT_TRUE(battle.getSideA().hasLightScreen());
    EXPECT_FALSE(battle.getSideA().hasMudSport());
    EXPECT_TRUE(battle.getSideA().hasWaterSport());
    EXPECT_EQ(battle.getSideA().getWaterSportTurns(), 3);

    EXPECT_EQ(battle.getSideB().getSpikesLayers(), 0);
    EXPECT_EQ(battle.getSideB().getToxicSpikesLayers(), 1);
    EXPECT_TRUE(battle.getSideB().hasReflect());
    EXPECT_FALSE(battle.getSideB().hasLightScreen());
    EXPECT_FALSE(battle.getSideB().hasStealthRock());
    EXPECT_TRUE(battle.getSideB().hasMudSport());
    EXPECT_FALSE(battle.getSideB().hasWaterSport());
    EXPECT_EQ(battle.getSideB().getMudSportTurns(), 4);
}

TEST(MoveBehaviorTest, MudSportAndWaterSportReduceDamageAndExpireAfterFiveTurns) {
    PRNG::setSeed(24001);

    Species userSpecies = makeSpecies(10601, "SportUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(10602, "SportTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon user = makePokemon(userSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&user));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move mudSport = createMoveByName("Mud Sport");
    Move waterSport = createMoveByName("Water Sport");
    ASSERT_FALSE(mudSport.getName().empty());
    ASSERT_FALSE(waterSport.getName().empty());

    Move electricMove("Test Electric", Type::Electric, Category::Special, 90, 100, 10, MoveEffect::None, 100);
    Move fireMove("Test Fire", Type::Fire, Category::Special, 90, 100, 10, MoveEffect::None, 100);

    PRNG::setSeed(24002);
    const int electricBaseline = battle.calculateDamage(&user, &target, electricMove);
    PRNG::setSeed(24003);
    const int fireBaseline = battle.calculateDamage(&user, &target, fireMove);

    battle.processMoveEffects(&user, &target, mudSport);
    battle.processMoveEffects(&user, &target, waterSport);
    EXPECT_TRUE(battle.getSideA().hasMudSport());
    EXPECT_TRUE(battle.getSideA().hasWaterSport());

    PRNG::setSeed(24002);
    const int electricReduced = battle.calculateDamage(&user, &target, electricMove);
    PRNG::setSeed(24003);
    const int fireReduced = battle.calculateDamage(&user, &target, fireMove);

    EXPECT_LT(electricReduced, electricBaseline);
    EXPECT_LT(fireReduced, fireBaseline);

    for (int i = 0; i < 5; ++i) {
        battle.enqueueAction(BattleAction::makePass(&user));
        battle.enqueueAction(BattleAction::makePass(&target));
        battle.processTurn();
    }

    EXPECT_FALSE(battle.getSideA().hasMudSport());
    EXPECT_FALSE(battle.getSideA().hasWaterSport());
}

TEST(MoveBehaviorTest, SurgingStrikesMatchesThreeIndependentHits) {
    PRNG::setSeed(1951);

    Species attackerSpecies = makeSpecies(10431, "SurgingUser", Type::Water, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(10432, "SurgingTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move surgingStrikes = createMoveByName("Surging Strikes");
    Move singleHit("Single Hit", Type::Water, Category::Physical, 25, 100, 35, MoveEffect::None, 100);

    const int hpBeforeSurging = defender.getCurrentHP();
    PRNG::setSeed(1952);
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, surgingStrikes));
    battle.resolveNextAction();
    const int surgingDamage = hpBeforeSurging - defender.getCurrentHP();

    Pokemon controlAttacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon controlDefender = makePokemon(defenderSpecies, AbilityType::None);
    Side controlSideA("CA");
    Side controlSideB("CB");
    ASSERT_TRUE(controlSideA.addPokemon(&controlAttacker));
    ASSERT_TRUE(controlSideB.addPokemon(&controlDefender));

    Battle controlBattle(controlSideA, controlSideB);
    const int hpBeforeControl = controlDefender.getCurrentHP();
    PRNG::setSeed(1952);
    controlBattle.enqueueAction(BattleAction::makeAttack(&controlAttacker, &controlDefender, singleHit));
    controlBattle.resolveNextAction();
    controlBattle.enqueueAction(BattleAction::makeAttack(&controlAttacker, &controlDefender, singleHit));
    controlBattle.resolveNextAction();
    controlBattle.enqueueAction(BattleAction::makeAttack(&controlAttacker, &controlDefender, singleHit));
    controlBattle.resolveNextAction();
    const int controlDamage = hpBeforeControl - controlDefender.getCurrentHP();

    EXPECT_EQ(surgingDamage, controlDamage);
}

TEST(BattleEventTest, ActionHooksEmitBeforeAndAfterResolve) {
    Species attackerSpecies = makeSpecies(10441, "EventAttacker", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species defenderSpecies = makeSpecies(10442, "EventDefender", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon defender = makePokemon(defenderSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&defender));

    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");

    int beforeCount = 0;
    int afterCount = 0;
    int callbackId = battle.getEventSystem().registerActionEventCallback([&](const RuntimeActionEvent& event) {
        if (event.phase == RuntimeEventPhase::BeforeActionResolve) {
            ++beforeCount;
        } else if (event.phase == RuntimeEventPhase::AfterActionResolve) {
            ++afterCount;
        }
    });

    ASSERT_GT(callbackId, 0);

    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, pound));
    battle.resolveNextAction();

    EXPECT_EQ(beforeCount, 1);
    EXPECT_EQ(afterCount, 1);

    EXPECT_TRUE(battle.getEventSystem().unregisterActionEventCallback(callbackId));

    battle.enqueueAction(BattleAction::makeAttack(&attacker, &defender, pound));
    battle.resolveNextAction();

    EXPECT_EQ(beforeCount, 1);
    EXPECT_EQ(afterCount, 1);
}

TEST(MoveBehaviorTest, RestHealsUserToFullAndAppliesSleep) {
    PRNG::setSeed(2001);

    Species restSpecies = makeSpecies(10501, "RestUser", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(10502, "RestTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon restUser = makePokemon(restSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);
    restUser.setCurrentHP(std::max(1, restUser.getMaxHP() - 20));

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&restUser));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move rest("Rest", Type::Psychic, Category::Status, 0, 100, 10, MoveEffect::None, 100, 0, 0, 0, Target::Self);

    battle.processMoveEffects(&restUser, &target, rest);

    EXPECT_EQ(restUser.getCurrentHP(), restUser.getMaxHP());
    EXPECT_TRUE(restUser.hasStatus(StatusType::Sleep));
}

TEST(MoveBehaviorTest, SleepPreventsActionUntilItExpires) {
    Species sleeperSpecies = makeSpecies(10511, "Sleeper", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(10512, "Target", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon sleeper = makePokemon(sleeperSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&sleeper));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move pound = createMoveByName("Pound");

    sleeper.addStatus(StatusType::Sleep, 2);

    const int hpBeforeFirstTurn = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&sleeper, &target, pound));
    battle.processTurn();
    EXPECT_EQ(target.getCurrentHP(), hpBeforeFirstTurn);
    EXPECT_TRUE(sleeper.hasStatus(StatusType::Sleep));

    const int hpBeforeSecondTurn = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&sleeper, &target, pound));
    battle.processTurn();
    EXPECT_LT(target.getCurrentHP(), hpBeforeSecondTurn);
    EXPECT_FALSE(sleeper.hasStatus(StatusType::Sleep));
}

TEST(MoveBehaviorTest, JumpKickMissDealsCrashRecoilToUser) {
    PRNG::setSeed(2101);

    Species attackerSpecies = makeSpecies(10521, "JumpKickUser", Type::Fighting, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(10522, "JumpKickTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move jumpKick("Jump Kick", Type::Fighting, Category::Physical, 100, 0, 10, MoveEffect::None, 100);

    const int attackerHpBefore = attacker.getCurrentHP();
    const int targetHpBefore = target.getCurrentHP();
    battle.enqueueAction(BattleAction::makeAttack(&attacker, &target, jumpKick));
    battle.resolveNextAction();

    EXPECT_EQ(target.getCurrentHP(), targetHpBefore);
    EXPECT_EQ(attacker.getCurrentHP(), attackerHpBefore - std::max(1, attacker.getMaxHP() / 2));
}

TEST(MoveBehaviorTest, CloseCombatLowersUsersAttackAndDefenseOnHit) {
    PRNG::setSeed(2201);

    Species attackerSpecies = makeSpecies(10531, "CloseCombatUser", Type::Fighting, Type::Count, AbilityType::None, AbilityType::None);
    Species targetSpecies = makeSpecies(10532, "CloseCombatTarget", Type::Normal, Type::Count, AbilityType::None, AbilityType::None);

    Pokemon attacker = makePokemon(attackerSpecies, AbilityType::None);
    Pokemon target = makePokemon(targetSpecies, AbilityType::None);

    Side sideA("A");
    Side sideB("B");
    ASSERT_TRUE(sideA.addPokemon(&attacker));
    ASSERT_TRUE(sideB.addPokemon(&target));

    Battle battle(sideA, sideB);
    Move closeCombat("Close Combat", Type::Fighting, Category::Physical, 120, 100, 5, MoveEffect::None, 100);

    EXPECT_EQ(attacker.getStatStage(StatIndex::Attack), 0);
    EXPECT_EQ(attacker.getStatStage(StatIndex::Defense), 0);

    battle.processMoveEffects(&attacker, &target, closeCombat);

    EXPECT_EQ(attacker.getStatStage(StatIndex::Attack), -1);
    EXPECT_EQ(attacker.getStatStage(StatIndex::Defense), -1);
}

TEST(BattleSessionTest, RejectsDuplicateActionFromSameSideInSingles) {
    const nlohmann::json initRequest = {
        {"side_a", {{"name", "A"}, {"pokemon", nlohmann::json::array({makeSessionPokemonJson(25, "static")})}}},
        {"side_b", {{"name", "B"}, {"pokemon", nlohmann::json::array({makeSessionPokemonJson(4, "blaze")})}}}
    };

    std::string error;
    auto session = BattleSession::createFromJson(initRequest, &error);
    ASSERT_TRUE(session.has_value()) << error;

    const nlohmann::json turnRequest = {
        {"actions", nlohmann::json::array({
            {{"side", "a"}, {"type", "attack"}, {"move_index", 0}},
            {{"side", "a"}, {"type", "pass"}}
        })}
    };

    const nlohmann::json response = session->processTurn(turnRequest);
    EXPECT_FALSE(response.value("ok", true));
    EXPECT_TRUE(responseHasErrorFragment(response, "duplicate action for side a"));
}

TEST(BattleSessionTest, RejectsAttackWithBothMoveIndexAndMoveName) {
    const nlohmann::json initRequest = {
        {"side_a", {{"name", "A"}, {"pokemon", nlohmann::json::array({makeSessionPokemonJson(25, "static")})}}},
        {"side_b", {{"name", "B"}, {"pokemon", nlohmann::json::array({makeSessionPokemonJson(4, "blaze")})}}}
    };

    std::string error;
    auto session = BattleSession::createFromJson(initRequest, &error);
    ASSERT_TRUE(session.has_value()) << error;

    const nlohmann::json turnRequest = {
        {"actions", nlohmann::json::array({
            {{"side", "a"}, {"type", "attack"}, {"move_index", 0}, {"move_name", "Pound"}},
            {{"side", "b"}, {"type", "pass"}}
        })}
    };

    const nlohmann::json response = session->processTurn(turnRequest);
    EXPECT_FALSE(response.value("ok", true));
    EXPECT_TRUE(responseHasErrorFragment(response, "cannot specify both move_index and move_name"));
}

TEST(BattleSessionTest, RejectsBenchActorForAttackInSinglesMode) {
    const nlohmann::json initRequest = {
        {"side_a", {{"name", "A"}, {"pokemon", nlohmann::json::array({
            makeSessionPokemonJson(25, "static"),
            makeSessionPokemonJson(1, "overgrow")
        })}}},
        {"side_b", {{"name", "B"}, {"pokemon", nlohmann::json::array({makeSessionPokemonJson(4, "blaze")})}}}
    };

    std::string error;
    auto session = BattleSession::createFromJson(initRequest, &error);
    ASSERT_TRUE(session.has_value()) << error;

    const nlohmann::json turnRequest = {
        {"actions", nlohmann::json::array({
            {{"side", "a"}, {"type", "attack"}, {"actor_index", 1}, {"move_index", 0}},
            {{"side", "b"}, {"type", "pass"}}
        })}
    };

    const nlohmann::json response = session->processTurn(turnRequest);
    EXPECT_FALSE(response.value("ok", true));
    EXPECT_TRUE(responseHasErrorFragment(response, "actor_index must reference the active pokemon"));
}

TEST(BattleSessionTest, SeededSessionsProduceIdenticalRngDependentResults) {
    const nlohmann::json thunderWaveMoves = nlohmann::json::array({"Thunder Wave", "Pound", "Pound", "Pound"});
    const nlohmann::json initRequest = {
        {"seed", 1337},
        {"side_a", {{"name", "A"}, {"pokemon", nlohmann::json::array({
            makeSessionPokemonJson(25, "static", thunderWaveMoves)
        })}}},
        {"side_b", {{"name", "B"}, {"pokemon", nlohmann::json::array({
            makeSessionPokemonJson(1, "overgrow")
        })}}}
    };

    std::string errorA;
    auto sessionA = BattleSession::createFromJson(initRequest, &errorA);
    ASSERT_TRUE(sessionA.has_value()) << errorA;
    const nlohmann::json responseA = sessionA->processTurn({
        {"actions", nlohmann::json::array({
            {{"side", "a"}, {"type", "attack"}, {"move_name", "Thunder Wave"}},
            {{"side", "b"}, {"type", "pass"}}
        })}
    });
    ASSERT_TRUE(responseA.value("ok", false));

    std::string errorB;
    auto sessionB = BattleSession::createFromJson(initRequest, &errorB);
    ASSERT_TRUE(sessionB.has_value()) << errorB;
    const nlohmann::json responseB = sessionB->processTurn({
        {"actions", nlohmann::json::array({
            {{"side", "a"}, {"type", "attack"}, {"move_name", "Thunder Wave"}},
            {{"side", "b"}, {"type", "pass"}}
        })}
    });
    ASSERT_TRUE(responseB.value("ok", false));

    EXPECT_EQ(responseA["state"], responseB["state"]);
}
