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
