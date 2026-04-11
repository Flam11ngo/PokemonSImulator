#include <gtest/gtest.h>

#include <algorithm>

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
