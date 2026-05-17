#pragma once

#include "battle/OnCondition.h"
#include "battle/Status.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class Pokemon;
enum class Type;
class Move;
class Battle;
class BattleContext;
enum class WeatherType;
enum class FieldType;
enum class StatIndex;

struct AbilityDamageContext {
    bool isDamagingMove = false;
    bool isContact = false;
    const Move* move = nullptr;
    BattleContext* context = nullptr;
};

enum class AbilityType {
    None,
    Intimidate,
    Overgrow,
    Blaze,
    Torrent,
    Multiscale,
    Levitate,
    WaterAbsorb,
    VoltAbsorb,
    FlashFire,
    Static,
    PoisonPoint,
    Aftermath,
    Mummy,
    RoughSkin,
    FlameBody,
    Insomnia,
    VitalSpirit,
    Guts,
    HugePower,
    ThickFat,
    MarvelScale,
    SapSipper,
    IronBarbs,
    StormDrain,
    MotorDrive,
    Immunity,
    Technician,
    Filter,
    SolidRock,
    Moxie,
    InnerFocus,
    Regenerator,
    NaturalCure,
    MagicGuard,
    Unaware,
    Prankster,
    ClearBody,
    Defiant,
    Competitive,
    WhiteSmoke,
    MirrorArmor,
    HyperCutter,
    KeenEye,
    Drizzle,
    Drought,
    SandStream,
    SnowWarning,
    CloudNine,
    GrassySurge,
    ElectricSurge,
    PsychicSurge,
    MistySurge,
    HadronEngine,
    Protean,
    Libero,
    Adaptability,
    SheerForce,
    Infiltrator,
    BeadsOfRuin,
    SwordOfRuin,
    TabletsOfRuin,
    VesselOfRuin,
    Unnerve,
    EarthEater,
    Sharpness,
    PurifyingSalt,
    WellBakedBody,
    WindRider,
    ToxicDebris,
    LingeringAroma,
    ArmorTail,
    GoodAsGold,
    Stakeout,
    CudChew,
    MoldBreaker,
    Protosynthesis,
    QuarkDrive,
    SupremeOverlord,
    Triage,
    Steelworker,
    Corrosion,
    Bulletproof,
    WaterBubble,
    Scrappy,
    Contrary,
    SwiftSwim,
    Chlorophyll,
    SandRush,
    SlushRush,
    SurgeSurfer,
    SpeedBoost,
    Sturdy,
    Limber,
    OwnTempo,
    Oblivious,
    SereneGrace,
    IronFist,
    Reckless,
    StrongJaw,
    ToughClaws,
    BattleArmor,
    ShellArmor,
    ShedSkin,
    MagicBounce,
    Hustle,
    Pressure,
    WonderGuard,
    ShadowTag,
    LightningRod,
    Soundproof,
    Trace,
    PurePower,
    CompoundEyes,
    RockHead,
    ShieldDust,
    Simple,
    Synchronize,
    MagnetPull,
    ArenaTrap,
    RainDish,
    StickyHold,
    Damp,
    EarlyBird,
    Unburden,
    AngerPoint,
    Gluttony,
    EffectSpore,
    WaterVeil,
    MagmaArmor,
    LiquidOoze,
    SandVeil,
    Stench,
    CuteCharm,
    Steadfast,
    TangledFeet,
    Rivalry,
    SuctionCups,
    ColorChange,
    Heatproof,
    AirLock,
    SnowCloak,
    Sniper,
    NoGuard,
    SkillLink,
    Hydration,
    PoisonHeal,
    Download,
    Normalize,
    TintedLens,
    Klutz,
    SlowStart,
    Swarm,
    DrySkin,
    SolarPower,
    QuickFeet,
    Stall,
    LeafGuard,
    SuperLuck,
    Anticipation,
    Forewarn,
    IceBody,
    Frisk,
    Pickpocket,
    Defeatist,
    CursedBody,
    WeakArmor,
    HeavyMetal,
    LightMetal,
    ToxicBoost,
    FlareBoost,
    Harvest,
    Overcoat,
    PoisonTouch,
    BigPecks,
    WonderSkin,
    Analytic,
    Illusion,
    Justified,
    Rattled,
    SandForce,
    VictoryStar,
    Count
};

struct StatModifier {
    enum Stat { Attack, Defense, SpAttack, SpDefense, Speed, Accuracy, Evasion };
    Stat stat;
    float multiplier;  // 倍率
    int flatBonus;     // 固定加成
};


struct TypeImmunity {
    int typeId;  // 属性ID
    bool healInstead;  // 是否回复HP
    int healPercent;   // 回复百分比
};

struct StatusImmunity {
    int statusId;  // 状态ID
};

struct DamageModifier {
    float multiplier;
    bool onDealDamage;   // true: 造成伤害时, false: 受到伤害时
};

struct AbilityData {
    int id;
    std::string name;
    std::string apiName;
    std::string description;
    AbilityType type;
};

class Ability {
private:
    AbilityData data;

public:
    // 1. 基础效果（多个触发时机）
    // context is BattleContext* for OnEntry/OnExit; AbilityDamageContext* for OnDamage
    std::unordered_map<Trigger, std::function<void(Pokemon* self, Pokemon* opponent, void* context)>> effects;

    // 2. 数值修改
    std::vector<StatModifier> statModifiers;

    // 3. 免疫效果
    std::vector<TypeImmunity> typeImmunities;
    std::vector<StatusImmunity> statusImmunities;

    // 4. 伤害修改 (TODO: replace with outgoingDamageModifiers / incomingDamageModifiers)
    DamageModifier damageModifier;

    // 5. 被动效果标志（替代 regHelperOnly 和分散的 abilityXxx() 辅助函数）
    struct PassiveFlags {
        bool suppressesWeather = false;
        bool ignoresSubstitute = false;
        bool ignoresScreens = false;
        bool blocksBerryConsumption = false;
        bool ignoresIndirectDamage = false;
        bool canTypeShift = false;
        bool ignoresOpponentStatStages = false;
        bool blocksGenericStatDrops = false;
        bool blocksAttackDrops = false;
        bool blocksAccuracyDrops = false;
        bool blocksEvasionDrops = false;
        bool reflectsStatDrops = false;
        bool lowersOppPhysAtkAura = false;
        bool lowersOppSpAtkAura = false;
        bool lowersOppDefAura = false;
        bool lowersOppSpDefAura = false;
        bool grantsGroundHazardImmunity = false;
        bool blocksPriorityTargetedMoves = false;
        bool blocksStatusMovesFromOpponents = false;
        bool ignoresTargetAbility = false;
        bool overridesGhostImmunity = false;
        bool reversesStatChanges = false;
        bool overridesPoisonTypeImmunity = false;
        bool hasCudChew = false;
        bool hasParadoxBoost = false;
        bool preventsTaunt = false;
        bool preventsInfatuation = false;
        bool sturdyEndure = false;
        bool blocksCriticalHits = false;
        bool reflectsStatusMoves = false;
        bool drainsOpponentPP = false;
        bool wonderGuard = false;
        bool trapsOpponent = false;
        bool redirectsElectricMoves = false;
        bool blocksSoundMoves = false;
        bool copiesOpponentAbility = false;
        bool doublesAttack = false;
        bool preventsRecoil = false;
        bool blocksMoveSecondaryEffects = false;
        bool mirrorsStatus = false;
        bool trapsSteelTypes = false;
        bool trapsGrounded = false;
        bool healsInRain = false;
        bool preventsItemLoss = false;
        bool preventsExplosion = false;
        bool halvesSleepTurns = false;
        bool speedDoubledWithoutItem = false;
        bool attackMaxedOnCrit = false;
        bool earlyBerryConsumption = false;
        bool inflictsRandomContactStatus = false;
        bool preventsBurn = false;
        bool preventsFreeze = false;
        bool invertsDrainingHeal = false;
        bool boostsEvasionInSand = false;
        bool flinchOnHit = false;
        bool infatuatesOnContact = false;
        bool speedBoostWhenFlinched = false;
        bool evasionDoubleWhenConfused = false;
        bool rivalryDamageModifier = false;
        bool preventsForcedSwitch = false;
        bool colorChangeOnHit = false;
        bool fireResistance = false;
        bool negatesWeather = false;
        bool evasionInSnow = false;
        bool sniperCritBoost = false;
        bool noGuardAlwaysHit = false;
        bool skillLinkMaxHits = false;
        bool hydrationHealsStatus = false;
        bool poisonHealRecovery = false;
        bool downloadStatBoost = false;
        bool normalizeAllNormal = false;
        bool tintedLensBoost = false;
        bool klutzNoItem = false;
        bool slowStartHalved = false;
        bool swarmBugBoost = false;
        bool drySkinEffects = false;
        bool solarPowerBoost = false;
        bool quickFeetSpeedBoost = false;
        bool alwaysMovesLast = false;
        bool leafGuardSun = false;
        bool superLuckCrit = false;
        bool anticipationShudder = false;
        bool forewarnReveal = false;
        bool iceBodyHailHeal = false;
        bool friskRevealsItem = false;
        bool pickpocketStealsItem = false;
        bool defeatistDebuff = false;
        bool cursedBodyDisable = false;
        bool weakArmorStatShift = false;
        bool heavyMetalWeightDouble = false;
        bool lightMetalWeightHalf = false;
        bool toxicBoostAttack = false;
        bool flareBoostSpAttack = false;
        bool harvestRecyclesBerry = false;
        bool overcoatPowderWeather = false;
        bool poisonTouchContact = false;
        bool bigPecksPreventDefDrop = false;
        bool wonderSkinReducedAccuracy = false;
        bool analyticMoveLastBoost = false;
        bool illusionDisguise = false;
        bool justifiedDarkBoost = false;
        bool rattledSpeedBoost = false;
        bool sandForceBoost = false;
        bool victoryStarAccuracy = false;

        float stabBonusMultiplier = 1.5f;
        int statusMovePriorityBonus = 0;

        // Stat-drop reaction: boosts own Attack (Defiant) or SpecialAttack (Competitive)
        bool statDropReactionBoostsAttack = false;
        bool statDropReactionBoostsSpAttack = false;
        int statDropReactionStages = 0;
    } passive;

    Ability() = default;
    explicit Ability(const AbilityData& abilityData) : data(abilityData) {}

    const AbilityData& getData() const { return data; }
    const std::string& getName() const { return data.name; }
    AbilityType getType() const { return data.type; }
    void setData(const AbilityData& abilityData) { data = abilityData; }

    // 辅助方法
    bool hasTrigger(Trigger trigger) const {
        return effects.find(trigger) != effects.end();
    }

    void executeTrigger(Trigger trigger, Pokemon* self, Pokemon* opponent, void* context = nullptr) const {
        auto it = effects.find(trigger);
        if (it != effects.end() && it->second) {
            it->second(self, opponent, context);
        }
    }
};

// Get ability struct by type
Ability getAbility(AbilityType type);

// Centralized type-immunity helpers consumed by battle flow.
bool resolveTypeImmunity(AbilityType abilityType, Type moveType, bool& healInstead, int& healPercent);
bool resolveStatusImmunity(AbilityType abilityType, StatusType status);
void applyTypeImmunityBonus(AbilityType abilityType, Pokemon* self);

// Centralized ability flag helpers for battle flow branching.
bool abilitySuppressesWeather(AbilityType abilityType);
bool abilityIgnoresSubstitute(AbilityType abilityType);
bool abilityIgnoresScreens(AbilityType abilityType);
bool abilityBlocksBerryConsumption(AbilityType abilityType);
bool abilityIgnoresIndirectDamage(AbilityType abilityType);
bool abilityCanTypeShift(AbilityType abilityType);
bool abilityIgnoresOpponentStatStages(AbilityType abilityType);
float abilityStabBonusMultiplier(AbilityType abilityType);
bool abilitySuppressesSecondaryEffects(AbilityType abilityType, const Move& move, bool sheerForceBoostedMove);
int abilityStatusMovePriorityBonus(AbilityType abilityType);
bool abilityBlocksGenericStatDrops(AbilityType abilityType);
bool abilityBlocksAttackDrops(AbilityType abilityType);
bool abilityBlocksAccuracyDrops(AbilityType abilityType);
bool abilityBlocksEvasionDrops(AbilityType abilityType);
bool abilityReflectsStatDrops(AbilityType abilityType);
void applyStatLoweredReaction(AbilityType abilityType, Pokemon* self);
bool abilityLowersOpponentPhysicalAttackAura(AbilityType abilityType);
bool abilityLowersOpponentSpecialAttackAura(AbilityType abilityType);
bool abilityLowersOpponentDefenseAura(AbilityType abilityType);
bool abilityLowersOpponentSpecialDefenseAura(AbilityType abilityType);
bool abilityGrantsGroundHazardImmunity(AbilityType abilityType);
std::string abilityTypeImmunityEventReason(AbilityType abilityType);

// Centralized ability formula helpers consumed by damage pipeline.
float abilityAttackStatMultiplier(AbilityType abilityType, const Move& move, bool hasMajorStatus, bool electricTerrainActive);
float abilityDefenseStatMultiplier(AbilityType abilityType, const Move& move, bool hasMajorStatus);
int applyAbilityPowerModifier(AbilityType abilityType, const Move& move, int basePower, bool sheerForceBoostedMove);
float abilityOutgoingDamageMultiplier(AbilityType abilityType, const Move& move, int currentHp, int maxHp, bool targetJustSwitchedIn, int faintedAllies);
float abilityIncomingDamageMultiplier(AbilityType abilityType, const Move& move, float typeEffectiveness, int currentHp, int maxHp);
bool abilityBlocksMoveDamage(AbilityType abilityType, const Move& move);
bool abilityBlocksPriorityTargetedMoves(AbilityType abilityType);
bool abilityBlocksStatusMovesFromOpponents(AbilityType abilityType);
bool abilityIgnoresTargetAbility(AbilityType abilityType);
bool abilityOverridesGhostImmunity(AbilityType abilityType);
bool abilityReversesStatChanges(AbilityType abilityType);
bool abilityOverridesPoisonTypeImmunity(AbilityType abilityType);
bool abilityHasCudChew(AbilityType abilityType);
float abilityParadoxStatMultiplier(AbilityType abilityType, const Pokemon* self, StatIndex stat, WeatherType weatherType, FieldType fieldType);
float abilityWeatherSpeedMultiplier(AbilityType abilityType, WeatherType weatherType);

// Forward declaration for grounded check
struct RuntimeMoveState;
bool isPokemonGrounded(const Pokemon* pokemon, const RuntimeMoveState& runtimeState);
bool hasMagnetRiseEffect(const Pokemon* pokemon, const RuntimeMoveState& runtimeState);
bool abilityHasSturdy(AbilityType abilityType);
bool abilityPreventsTaunt(AbilityType abilityType);
bool abilityPreventsInfatuation(AbilityType abilityType);
bool abilityBlocksCriticalHits(AbilityType abilityType);
bool abilityReflectsStatusMoves(AbilityType abilityType);
bool abilityDrainsOpponentPP(AbilityType abilityType);
bool abilityHasWonderGuard(AbilityType abilityType);
bool abilityTrapsOpponent(AbilityType abilityType);
bool abilityRedirectsElectricMoves(AbilityType abilityType);
bool abilityBlocksSoundMoves(AbilityType abilityType);
bool abilityCopiesOpponentAbility(AbilityType abilityType);
bool abilityDoublesAttack(AbilityType abilityType);
float abilityAccuracyBoost(AbilityType abilityType);
bool abilityPreventsRecoil(AbilityType abilityType);
bool abilityBlocksMoveSecondaryEffects(AbilityType abilityType);
bool abilityMirrorsStatus(AbilityType abilityType);
bool abilityTrapsSteelTypes(AbilityType abilityType);
bool abilityTrapsGrounded(AbilityType abilityType);
bool abilityHealsInRain(AbilityType abilityType);
bool abilityPreventsItemLoss(AbilityType abilityType);
bool abilityPreventsExplosion(AbilityType abilityType);
bool abilityHalvesSleepTurns(AbilityType abilityType);
bool abilitySpeedDoubledWithoutItem(AbilityType abilityType);
bool abilityAttackMaxedOnCrit(AbilityType abilityType);
bool abilityEarlyBerryConsumption(AbilityType abilityType);
bool abilityInflictsRandomContactStatus(AbilityType abilityType);
bool abilityPreventsBurn(AbilityType abilityType);
bool abilityPreventsFreeze(AbilityType abilityType);
bool abilityInvertsDrainingHeal(AbilityType abilityType);
bool abilityBoostsEvasionInSand(AbilityType abilityType);
bool abilityFlinchOnHit(AbilityType abilityType);
bool abilityInfatuatesOnContact(AbilityType abilityType);
bool abilitySpeedBoostWhenFlinched(AbilityType abilityType);
bool abilityEvasionDoubleWhenConfused(AbilityType abilityType);
bool abilityRivalryDamageModifier(AbilityType abilityType);
bool abilityPreventsForcedSwitch(AbilityType abilityType);
bool abilityColorChangeOnHit(AbilityType abilityType);
bool abilityFireResistance(AbilityType abilityType);
bool abilityNegatesWeather(AbilityType abilityType);
bool abilityEvasionInSnow(AbilityType abilityType);
bool abilitySniperCritBoost(AbilityType abilityType);
bool abilityNoGuardAlwaysHit(AbilityType abilityType);
bool abilitySkillLinkMaxHits(AbilityType abilityType);
bool abilityHydrationHealsStatus(AbilityType abilityType);
bool abilityPoisonHealRecovery(AbilityType abilityType);
bool abilityDownloadStatBoost(AbilityType abilityType);
bool abilityNormalizeAllNormal(AbilityType abilityType);
bool abilityTintedLensBoost(AbilityType abilityType);
bool abilityKlutzNoItem(AbilityType abilityType);
bool abilitySlowStartHalved(AbilityType abilityType);
bool abilitySwarmBugBoost(AbilityType abilityType);
bool abilityDrySkinEffects(AbilityType abilityType);
bool abilitySolarPowerBoost(AbilityType abilityType);
bool abilityQuickFeetSpeedBoost(AbilityType abilityType);
bool abilityAbilityAlwaysMovesLast(AbilityType abilityType);
bool abilityLeafGuardSun(AbilityType abilityType);
bool abilitySuperLuckCrit(AbilityType abilityType);
bool abilityAnticipationShudder(AbilityType abilityType);
bool abilityForewarnReveal(AbilityType abilityType);
bool abilityIceBodyHailHeal(AbilityType abilityType);
bool abilityFriskRevealsItem(AbilityType abilityType);
bool abilityPickpocketStealsItem(AbilityType abilityType);
bool abilityDefeatistDebuff(AbilityType abilityType);
bool abilityCursedBodyDisable(AbilityType abilityType);
bool abilityWeakArmorStatShift(AbilityType abilityType);
bool abilityHeavyMetalWeightDouble(AbilityType abilityType);
bool abilityLightMetalWeightHalf(AbilityType abilityType);
bool abilityToxicBoostAttack(AbilityType abilityType);
bool abilityFlareBoostSpAttack(AbilityType abilityType);
bool abilityHarvestRecyclesBerry(AbilityType abilityType);
bool abilityOvercoatPowderWeather(AbilityType abilityType);
bool abilityPoisonTouchContact(AbilityType abilityType);
bool abilityBigPecksPreventDefDrop(AbilityType abilityType);
bool abilityWonderSkinReducedAccuracy(AbilityType abilityType);
bool abilityAnalyticMoveLastBoost(AbilityType abilityType);
bool abilityIllusionDisguise(AbilityType abilityType);
bool abilityJustifiedDarkBoost(AbilityType abilityType);
bool abilityRattledSpeedBoost(AbilityType abilityType);
bool abilitySandForceBoost(AbilityType abilityType);
bool abilityVictoryStarAccuracy(AbilityType abilityType);

// Convert between ability enum and canonical display name.
std::string getAbilityName(AbilityType type);
AbilityType getAbilityTypeByName(const std::string& name);

// Ability data lookup and conversion helpers.
AbilityData getAbilityDataById(int id);
AbilityData getAbilityDataByName(const std::string& name);
AbilityData getAbilityData(AbilityType type);
AbilityType getAbilityTypeById(int id);
AbilityType getAbilityTypeByNameFromData(const std::string& name);

// Prefetch referenced ability entries and persist to data/abilities.json.
bool prefetchAbilitiesFromPokeAPI(bool refreshExisting = false);

// Get all abilities for a Pokemon (in case multiple)
std::vector<Ability> getAbilitiesForPokemon(AbilityType type);

// Register all ability builders into the game registry.
class GameRegistry;
void initializeCoreAbilities(GameRegistry& registry);
