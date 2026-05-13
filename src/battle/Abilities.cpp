#include "battle/Abilities.h"
#include "battle/Battle.h"
#include "battle/Pokemon.h" // For Pokemon class
#include "battle/PRNG.h"
#include "battle/GameRegistry.h"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace {
std::string normalizeToken(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch == ' ' || ch == '-' || ch == '_' || ch == '\'') {
            continue;
        }
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

bool isSlicingMove(const Move& move) {
    const std::string key = normalizeToken(move.getName());
    return key == "aerialace"
        || key == "airslash"
        || key == "aquacutter"
        || key == "bitterblade"
        || key == "ceaselessedge"
        || key == "crosspoison"
        || key == "cut"
        || key == "furycutter"
        || key == "leafblade"
        || key == "nightslash"
        || key == "populationbomb"
        || key == "psyblade"
        || key == "razorleaf"
        || key == "razorwind"
        || key == "sacredsword"
        || key == "slash"
        || key == "solarblade"
        || key == "xscissor";
}

    bool isWindMove(const Move& move) {
        const std::string key = normalizeToken(move.getName());
        return key == "gust"
        || key == "hurricane"
        || key == "heatwave"
        || key == "icywind"
        || key == "twister"
        || key == "fairywind"
        || key == "bleakwindstorm"
        || key == "wildboltstorm";
    }

    StatIndex strongestParadoxStat(const Pokemon* self) {
        if (!self) {
            return StatIndex::Attack;
        }

        const int attack = self->getAttack();
        const int defense = self->getDefense();
        const int specialAttack = self->getSpecialAttack();
        const int specialDefense = self->getSpecialDefense();
        const int speed = self->getSpeed();

        int maxValue = attack;
        StatIndex strongest = StatIndex::Attack;
        if (defense > maxValue) {
            maxValue = defense;
            strongest = StatIndex::Defense;
        }
        if (specialAttack > maxValue) {
            maxValue = specialAttack;
            strongest = StatIndex::SpecialAttack;
        }
        if (specialDefense > maxValue) {
            maxValue = specialDefense;
            strongest = StatIndex::SpecialDefense;
        }
        if (speed > maxValue) {
            strongest = StatIndex::Speed;
        }
        return strongest;
    }

    bool paradoxBoostActive(AbilityType abilityType, WeatherType weatherType, FieldType fieldType, bool itemActivated) {
        if (itemActivated) {
            return true;
        }
        if (abilityType == AbilityType::Protosynthesis) {
            return weatherType == WeatherType::Sun;
        }
        if (abilityType == AbilityType::QuarkDrive) {
            return fieldType == FieldType::Electric;
        }
        return false;
    }
}

std::string getAbilityName(AbilityType type) {
    switch (type) {
        case AbilityType::Intimidate: return "Intimidate";
        case AbilityType::Overgrow: return "Overgrow";
        case AbilityType::Blaze: return "Blaze";
        case AbilityType::Torrent: return "Torrent";
        case AbilityType::Multiscale: return "Multiscale";
        case AbilityType::Levitate: return "Levitate";
        case AbilityType::WaterAbsorb: return "Water Absorb";
        case AbilityType::VoltAbsorb: return "Volt Absorb";
        case AbilityType::FlashFire: return "Flash Fire";
        case AbilityType::Static: return "Static";
        case AbilityType::PoisonPoint: return "Poison Point";
        case AbilityType::Aftermath: return "Aftermath";
        case AbilityType::Mummy: return "Mummy";
        case AbilityType::RoughSkin: return "Rough Skin";
        case AbilityType::FlameBody: return "Flame Body";
        case AbilityType::Insomnia: return "Insomnia";
        case AbilityType::VitalSpirit: return "Vital Spirit";
        case AbilityType::Guts: return "Guts";
        case AbilityType::HugePower: return "Huge Power";
        case AbilityType::ThickFat: return "Thick Fat";
        case AbilityType::MarvelScale: return "Marvel Scale";
        case AbilityType::SapSipper: return "Sap Sipper";
        case AbilityType::IronBarbs: return "Iron Barbs";
        case AbilityType::StormDrain: return "Storm Drain";
        case AbilityType::MotorDrive: return "Motor Drive";
        case AbilityType::Immunity: return "Immunity";
        case AbilityType::Technician: return "Technician";
        case AbilityType::Filter: return "Filter";
        case AbilityType::SolidRock: return "Solid Rock";
        case AbilityType::Moxie: return "Moxie";
        case AbilityType::InnerFocus: return "Inner Focus";
        case AbilityType::Regenerator: return "Regenerator";
        case AbilityType::NaturalCure: return "Natural Cure";
        case AbilityType::MagicGuard: return "Magic Guard";
        case AbilityType::Unaware: return "Unaware";
        case AbilityType::Prankster: return "Prankster";
        case AbilityType::ClearBody: return "Clear Body";
        case AbilityType::Defiant: return "Defiant";
        case AbilityType::Competitive: return "Competitive";
        case AbilityType::WhiteSmoke: return "White Smoke";
        case AbilityType::MirrorArmor: return "Mirror Armor";
        case AbilityType::HyperCutter: return "Hyper Cutter";
        case AbilityType::KeenEye: return "Keen Eye";
        case AbilityType::Drizzle: return "Drizzle";
        case AbilityType::Drought: return "Drought";
        case AbilityType::SandStream: return "Sand Stream";
        case AbilityType::SnowWarning: return "Snow Warning";
        case AbilityType::CloudNine: return "Cloud Nine";
        case AbilityType::GrassySurge: return "Grassy Surge";
        case AbilityType::ElectricSurge: return "Electric Surge";
        case AbilityType::PsychicSurge: return "Psychic Surge";
        case AbilityType::MistySurge: return "Misty Surge";
        case AbilityType::HadronEngine: return "Hadron Engine";
        case AbilityType::Protean: return "Protean";
        case AbilityType::Libero: return "Libero";
        case AbilityType::Adaptability: return "Adaptability";
        case AbilityType::SheerForce: return "Sheer Force";
        case AbilityType::Infiltrator: return "Infiltrator";
        case AbilityType::BeadsOfRuin: return "Beads of Ruin";
        case AbilityType::SwordOfRuin: return "Sword of Ruin";
        case AbilityType::TabletsOfRuin: return "Tablets of Ruin";
        case AbilityType::VesselOfRuin: return "Vessel of Ruin";
        case AbilityType::Unnerve: return "Unnerve";
        case AbilityType::EarthEater: return "Earth Eater";
        case AbilityType::Sharpness: return "Sharpness";
        case AbilityType::PurifyingSalt: return "Purifying Salt";
        case AbilityType::WellBakedBody: return "Well-Baked Body";
        case AbilityType::WindRider: return "Wind Rider";
        case AbilityType::ToxicDebris: return "Toxic Debris";
        case AbilityType::LingeringAroma: return "Lingering Aroma";
        case AbilityType::ArmorTail: return "Armor Tail";
        case AbilityType::GoodAsGold: return "Good as Gold";
        case AbilityType::Stakeout: return "Stakeout";
        case AbilityType::CudChew: return "Cud Chew";
        case AbilityType::MoldBreaker: return "Mold Breaker";
        case AbilityType::Protosynthesis: return "Protosynthesis";
        case AbilityType::QuarkDrive: return "Quark Drive";
        case AbilityType::SupremeOverlord: return "Supreme Overlord";
        case AbilityType::Triage: return "Triage";
        case AbilityType::Steelworker: return "Steelworker";
        case AbilityType::Corrosion: return "Corrosion";
        case AbilityType::Bulletproof: return "Bulletproof";
        case AbilityType::WaterBubble: return "Water Bubble";
        case AbilityType::Scrappy: return "Scrappy";
        case AbilityType::Contrary: return "Contrary";
        default: return "None";
    }
}

AbilityType getAbilityTypeByName(const std::string& name) {
    std::string key;
    key.reserve(name.size());
    for (char ch : name) {
        if (ch == ' ' || ch == '-' || ch == '_' || ch == '\'') {
            continue;
        }
        key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (key == "intimidate") return AbilityType::Intimidate;
    if (key == "overgrow") return AbilityType::Overgrow;
    if (key == "blaze") return AbilityType::Blaze;
    if (key == "torrent") return AbilityType::Torrent;
    if (key == "multiscale") return AbilityType::Multiscale;
    if (key == "levitate") return AbilityType::Levitate;
    if (key == "waterabsorb") return AbilityType::WaterAbsorb;
    if (key == "voltabsorb") return AbilityType::VoltAbsorb;
    if (key == "flashfire") return AbilityType::FlashFire;
    if (key == "static") return AbilityType::Static;
    if (key == "poisonpoint") return AbilityType::PoisonPoint;
    if (key == "aftermath") return AbilityType::Aftermath;
    if (key == "mummy") return AbilityType::Mummy;
    if (key == "roughskin") return AbilityType::RoughSkin;
    if (key == "flamebody") return AbilityType::FlameBody;
    if (key == "insomnia") return AbilityType::Insomnia;
    if (key == "vitalspirit") return AbilityType::VitalSpirit;
    if (key == "guts") return AbilityType::Guts;
    if (key == "hugepower") return AbilityType::HugePower;
    if (key == "thickfat") return AbilityType::ThickFat;
    if (key == "marvelscale") return AbilityType::MarvelScale;
    if (key == "sapsipper") return AbilityType::SapSipper;
    if (key == "ironbarbs") return AbilityType::IronBarbs;
    if (key == "stormdrain") return AbilityType::StormDrain;
    if (key == "motordrive") return AbilityType::MotorDrive;
    if (key == "immunity") return AbilityType::Immunity;
    if (key == "technician") return AbilityType::Technician;
    if (key == "filter") return AbilityType::Filter;
    if (key == "solidrock") return AbilityType::SolidRock;
    if (key == "moxie") return AbilityType::Moxie;
    if (key == "innerfocus") return AbilityType::InnerFocus;
    if (key == "regenerator") return AbilityType::Regenerator;
    if (key == "naturalcure") return AbilityType::NaturalCure;
    if (key == "magicguard") return AbilityType::MagicGuard;
    if (key == "unaware") return AbilityType::Unaware;
    if (key == "prankster") return AbilityType::Prankster;
    if (key == "clearbody") return AbilityType::ClearBody;
    if (key == "defiant") return AbilityType::Defiant;
    if (key == "competitive") return AbilityType::Competitive;
    if (key == "whitesmoke") return AbilityType::WhiteSmoke;
    if (key == "mirrorarmor") return AbilityType::MirrorArmor;
    if (key == "hypercutter") return AbilityType::HyperCutter;
    if (key == "keeneye") return AbilityType::KeenEye;
    if (key == "drizzle") return AbilityType::Drizzle;
    if (key == "drought") return AbilityType::Drought;
    if (key == "sandstream") return AbilityType::SandStream;
    if (key == "snowwarning") return AbilityType::SnowWarning;
    if (key == "cloudnine") return AbilityType::CloudNine;
    if (key == "grassysurge") return AbilityType::GrassySurge;
    if (key == "electricsurge") return AbilityType::ElectricSurge;
    if (key == "psychicsurge") return AbilityType::PsychicSurge;
    if (key == "mistysurge") return AbilityType::MistySurge;
    if (key == "hadronengine") return AbilityType::HadronEngine;
    if (key == "protean") return AbilityType::Protean;
    if (key == "libero") return AbilityType::Libero;
    if (key == "adaptability") return AbilityType::Adaptability;
    if (key == "sheerforce") return AbilityType::SheerForce;
    if (key == "infiltrator") return AbilityType::Infiltrator;
    if (key == "beadsofruin") return AbilityType::BeadsOfRuin;
    if (key == "swordofruin") return AbilityType::SwordOfRuin;
    if (key == "tabletsofruin") return AbilityType::TabletsOfRuin;
    if (key == "vesselofruin") return AbilityType::VesselOfRuin;
    if (key == "unnerve") return AbilityType::Unnerve;
    if (key == "eartheater") return AbilityType::EarthEater;
    if (key == "sharpness") return AbilityType::Sharpness;
    if (key == "purifyingsalt") return AbilityType::PurifyingSalt;
    if (key == "wellbakedbody") return AbilityType::WellBakedBody;
    if (key == "windrider") return AbilityType::WindRider;
    if (key == "toxicdebris") return AbilityType::ToxicDebris;
    if (key == "lingeringaroma") return AbilityType::LingeringAroma;
    if (key == "armortail") return AbilityType::ArmorTail;
    if (key == "goodasgold") return AbilityType::GoodAsGold;
    if (key == "stakeout") return AbilityType::Stakeout;
    if (key == "cudchew") return AbilityType::CudChew;
    if (key == "moldbreaker") return AbilityType::MoldBreaker;
    if (key == "protosynthesis") return AbilityType::Protosynthesis;
    if (key == "quarkdrive") return AbilityType::QuarkDrive;
    if (key == "supremeoverlord") return AbilityType::SupremeOverlord;
    if (key == "triage") return AbilityType::Triage;
    if (key == "steelworker") return AbilityType::Steelworker;
    if (key == "corrosion") return AbilityType::Corrosion;
    if (key == "bulletproof") return AbilityType::Bulletproof;
    if (key == "waterbubble") return AbilityType::WaterBubble;
    if (key == "scrappy") return AbilityType::Scrappy;
    if (key == "contrary") return AbilityType::Contrary;
    return AbilityType::None;
}

Ability getAbility(AbilityType type) {
    return GameRegistry::instance().getAbility(type);
}

bool resolveTypeImmunity(AbilityType abilityType, Type moveType, bool& healInstead, int& healPercent) {
    healInstead = false;
    healPercent = 0;

    const Ability ability = getAbility(abilityType);
    for (const TypeImmunity& immunity : ability.typeImmunities) {
        if (immunity.typeId != static_cast<int>(moveType)) {
            continue;
        }
        healInstead = immunity.healInstead;
        healPercent = immunity.healPercent;
        return true;
    }

    return false;
}

bool resolveStatusImmunity(AbilityType abilityType, StatusType status) {
    // Preserve existing simulator behavior: Vital Spirit currently blocks paralysis.
    if (abilityType == AbilityType::VitalSpirit && status == StatusType::Paralysis) {
        return true;
    }

    const Ability ability = getAbility(abilityType);
    for (const StatusImmunity& immunity : ability.statusImmunities) {
        if (immunity.statusId == static_cast<int>(status)) {
            return true;
        }
    }

    return false;
}

bool abilitySuppressesWeather(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.suppressesWeather;
}

bool abilityIgnoresSubstitute(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.ignoresSubstitute;
}

bool abilityIgnoresScreens(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.ignoresScreens;
}

bool abilityBlocksBerryConsumption(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.blocksBerryConsumption;
}

bool abilityIgnoresIndirectDamage(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.ignoresIndirectDamage;
}

bool abilityCanTypeShift(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.canTypeShift;
}

bool abilityIgnoresOpponentStatStages(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.ignoresOpponentStatStages;
}

float abilityStabBonusMultiplier(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.stabBonusMultiplier;
}

bool abilitySuppressesSecondaryEffects(AbilityType abilityType, const Move& move, bool sheerForceBoostedMove) {
    return abilityType == AbilityType::SheerForce && sheerForceBoostedMove && move.getCategory() != Category::Status;
}

int abilityStatusMovePriorityBonus(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.statusMovePriorityBonus;
}

bool abilityBlocksGenericStatDrops(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.blocksGenericStatDrops;
}

bool abilityBlocksAttackDrops(AbilityType abilityType) {
    const auto& p = GameRegistry::instance().getAbility(abilityType).passive;
    return p.blocksGenericStatDrops || p.blocksAttackDrops;
}

bool abilityBlocksAccuracyDrops(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.blocksAccuracyDrops;
}

bool abilityBlocksEvasionDrops(AbilityType abilityType) {
    const auto& p = GameRegistry::instance().getAbility(abilityType).passive;
    return p.blocksGenericStatDrops || p.blocksEvasionDrops;
}

bool abilityReflectsStatDrops(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.reflectsStatDrops;
}

void applyStatLoweredReaction(AbilityType abilityType, Pokemon* self) {
    if (!self) return;
    const auto& p = GameRegistry::instance().getAbility(abilityType).passive;
    if (p.statDropReactionBoostsAttack) {
        self->changeStatStage(StatIndex::Attack, p.statDropReactionStages);
    } else if (p.statDropReactionBoostsSpAttack) {
        self->changeStatStage(StatIndex::SpecialAttack, p.statDropReactionStages);
    }
}

bool abilityLowersOpponentPhysicalAttackAura(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.lowersOppPhysAtkAura;
}

bool abilityLowersOpponentSpecialAttackAura(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.lowersOppSpAtkAura;
}

bool abilityLowersOpponentDefenseAura(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.lowersOppDefAura;
}

bool abilityLowersOpponentSpecialDefenseAura(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.lowersOppSpDefAura;
}

bool abilityGrantsGroundHazardImmunity(AbilityType abilityType) {
    bool healInstead = false;
    int healPercent = 0;
    return resolveTypeImmunity(abilityType, Type::Ground, healInstead, healPercent);
}

std::string abilityTypeImmunityEventReason(AbilityType abilityType) {
    if (abilityType == AbilityType::WaterAbsorb) return "water_absorb";
    if (abilityType == AbilityType::VoltAbsorb) return "volt_absorb";
    return "ability_immunity";
}

void applyTypeImmunityBonus(AbilityType abilityType, Pokemon* self) {
    if (!self) {
        return;
    }

    switch (abilityType) {
        case AbilityType::SapSipper:
            self->changeStatStage(StatIndex::Attack, 1);
            break;
        case AbilityType::StormDrain:
            self->changeStatStage(StatIndex::SpecialAttack, 1);
            break;
        case AbilityType::MotorDrive:
            self->changeStatStage(StatIndex::Speed, 1);
            break;
        case AbilityType::WellBakedBody:
            self->changeStatStage(StatIndex::Defense, 2);
            break;
        case AbilityType::WindRider:
            self->changeStatStage(StatIndex::Attack, 1);
            break;
        default:
            break;
    }
}

float abilityAttackStatMultiplier(AbilityType abilityType, const Move& move, bool hasMajorStatus, bool electricTerrainActive) {
    if (move.getCategory() == Category::Physical) {
        if (abilityType == AbilityType::HugePower) {
            return 2.0f;
        }
        if (abilityType == AbilityType::Guts && hasMajorStatus) {
            return 1.5f;
        }
    }

    if (move.getCategory() == Category::Special
        && abilityType == AbilityType::HadronEngine
        && electricTerrainActive) {
        return 1.3f;
    }

    return 1.0f;
}

float abilityDefenseStatMultiplier(AbilityType abilityType, const Move& move, bool hasMajorStatus) {
    (void)move;
    if (abilityType == AbilityType::MarvelScale && hasMajorStatus) {
        return 1.5f;
    }
    return 1.0f;
}

int applyAbilityPowerModifier(AbilityType abilityType, const Move& move, int basePower, bool sheerForceBoostedMove) {
    int modifiedPower = basePower;
    if (abilityType == AbilityType::Technician
        && move.getCategory() != Category::Status
        && modifiedPower <= 60) {
        modifiedPower = static_cast<int>(std::lround(modifiedPower * 1.5f));
    }

    if (abilityType == AbilityType::Sharpness
        && move.getCategory() != Category::Status
        && isSlicingMove(move)) {
        modifiedPower = static_cast<int>(std::lround(modifiedPower * 1.5f));
    }

    if (abilityType == AbilityType::SheerForce && sheerForceBoostedMove) {
        modifiedPower = static_cast<int>(std::lround(modifiedPower * 1.3f));
    }

    return modifiedPower;
}

float abilityOutgoingDamageMultiplier(AbilityType abilityType, const Move& move, int currentHp, int maxHp, bool targetJustSwitchedIn, int faintedAllies) {
    if (maxHp <= 0) {
        return 1.0f;
    }

    const bool lowHpBoostRange = currentHp * 3 <= maxHp;
    if (abilityType == AbilityType::Blaze && move.getType() == Type::Fire && lowHpBoostRange) {
        return 1.5f;
    }
    if (abilityType == AbilityType::Torrent && move.getType() == Type::Water && lowHpBoostRange) {
        return 1.5f;
    }
    if (abilityType == AbilityType::Overgrow && move.getType() == Type::Grass && lowHpBoostRange) {
        return 1.5f;
    }
    if (abilityType == AbilityType::FlashFire && move.getType() == Type::Fire) {
        return 1.5f;
    }
    if (abilityType == AbilityType::Stakeout && targetJustSwitchedIn) {
        return 2.0f;
    }
    if (abilityType == AbilityType::SupremeOverlord && faintedAllies > 0) {
        const int clamped = std::min(5, faintedAllies);
        return 1.0f + 0.1f * static_cast<float>(clamped);
    }
    if (abilityType == AbilityType::Steelworker && move.getType() == Type::Steel) {
        return 1.5f;
    }
    if (abilityType == AbilityType::WaterBubble && move.getType() == Type::Water) {
        return 2.0f;
    }

    return 1.0f;
}

float abilityIncomingDamageMultiplier(AbilityType abilityType, const Move& move, float typeEffectiveness, int currentHp, int maxHp) {
    float multiplier = 1.0f;

    if (abilityType == AbilityType::ThickFat && (move.getType() == Type::Fire || move.getType() == Type::Ice)) {
        multiplier *= 0.5f;
    }
    if ((abilityType == AbilityType::Filter || abilityType == AbilityType::SolidRock) && typeEffectiveness > 1.0f) {
        multiplier *= 0.75f;
    }
    if (abilityType == AbilityType::Multiscale && maxHp > 0 && currentHp == maxHp) {
        multiplier *= 0.5f;
    }
    if (abilityType == AbilityType::PurifyingSalt && move.getType() == Type::Ghost) {
        multiplier *= 0.5f;
    }
    if (abilityType == AbilityType::WaterBubble && move.getType() == Type::Fire) {
        multiplier *= 0.5f;
    }

    return multiplier;
}

namespace {
bool isBallBombMove(const Move& move) {
    const std::string key = normalizeToken(move.getName());
    return key == "acidspray" || key == "aeroblast" || key == "appleacid"
        || key == "aurasphere" || key == "barrage" || key == "beakblast"
        || key == "bulletseed" || key == "eggbomb" || key == "electroball"
        || key == "energyball" || key == "focusblast" || key == "gyroball"
        || key == "iceshard" || key == "magnetbomb" || key == "mistball"
        || key == "mudbomb" || key == "octazooka" || key == "pollenpuff"
        || key == "pyroball" || key == "rockblast" || key == "rockwrecker"
        || key == "seedbomb" || key == "secretsword" || key == "shadowball"
        || key == "sludgebomb" || key == "weatherball" || key == "zapcannon";
}
}

bool abilityBlocksMoveDamage(AbilityType abilityType, const Move& move) {
    if (abilityType == AbilityType::WindRider && isWindMove(move)) {
        return true;
    }
    if (abilityType == AbilityType::Bulletproof && isBallBombMove(move)) {
        return true;
    }
    return false;
}

bool abilityBlocksPriorityTargetedMoves(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.blocksPriorityTargetedMoves;
}

bool abilityBlocksStatusMovesFromOpponents(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.blocksStatusMovesFromOpponents;
}

bool abilityIgnoresTargetAbility(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.ignoresTargetAbility;
}

bool abilityOverridesGhostImmunity(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.overridesGhostImmunity;
}

bool abilityReversesStatChanges(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.reversesStatChanges;
}

bool abilityOverridesPoisonTypeImmunity(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.overridesPoisonTypeImmunity;
}

bool abilityHasCudChew(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.hasCudChew;
}

float abilityParadoxStatMultiplier(AbilityType abilityType, const Pokemon* self, StatIndex stat, WeatherType weatherType, FieldType fieldType) {
    const bool itemActivated = self ? self->isParadoxActive() : false;
    if (!paradoxBoostActive(abilityType, weatherType, fieldType, itemActivated)) {
        return 1.0f;
    }

    const StatIndex strongest = strongestParadoxStat(self);
    if (strongest != stat) {
        return 1.0f;
    }

    return strongest == StatIndex::Speed ? 1.5f : 1.3f;
}

void initializeCoreAbilities(GameRegistry& registry) {
    // Helpers for shared patterns
    auto regWeatherSetter = [&registry](AbilityType type, WeatherType weather) {
        registry.registerAbilityBuilder(type,
            [weather](Ability& a, AddTypeImmunity, AddStatusImmunity) {
                a.effects[Trigger::OnEntry] = [weather](Pokemon*, Pokemon*, void* context) {
                    BattleContext* battle = static_cast<BattleContext*>(context);
                    if (battle) battle->getWeather().setWeather(weather, 5);
                };
            });
    };
    auto regTerrainSetter = [&registry](AbilityType type, FieldType field) {
        registry.registerAbilityBuilder(type,
            [field](Ability& a, AddTypeImmunity, AddStatusImmunity) {
                a.effects[Trigger::OnEntry] = [field](Pokemon*, Pokemon*, void* context) {
                    BattleContext* battle = static_cast<BattleContext*>(context);
                    if (battle) battle->getField().setField(field, 5);
                };
            });
    };
    // Shared contact-callback helpers
    auto contactGuard = [](const AbilityDamageContext* dc) {
        return !dc || !dc->isDamagingMove || !dc->isContact;
    };

    // === Intimidate ===
    registry.registerAbilityBuilder(AbilityType::Intimidate,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnEntry] = [](Pokemon* self, Pokemon* opponent, void*) {
                if (!opponent || !self) return;
                if (!opponent->isFainted()) {
                    const AbilityType oppAbil = opponent->getAbility();
                    const Ability& oppAbility = GameRegistry::instance().getAbility(oppAbil);
                    if (oppAbility.passive.blocksGenericStatDrops || oppAbility.passive.blocksAttackDrops) return;
                    if (oppAbility.passive.reflectsStatDrops) {
                        self->changeStatStage(StatIndex::Attack, -1);
                        return;
                    }
                    const int before = opponent->getStatStage(StatIndex::Attack);
                    opponent->changeStatStage(StatIndex::Attack, -1);
                    if (opponent->getStatStage(StatIndex::Attack) < before) {
                        if (oppAbility.passive.statDropReactionBoostsAttack) {
                            opponent->changeStatStage(StatIndex::Attack, oppAbility.passive.statDropReactionStages);
                        } else if (oppAbility.passive.statDropReactionBoostsSpAttack) {
                            opponent->changeStatStage(StatIndex::SpecialAttack, oppAbility.passive.statDropReactionStages);
                        }
                    }
                }
            };
        });

    // === Damage-modifier-only abilities ===
    auto regDmgMod = [&registry](AbilityType type, float mult, bool onDeal) {
        registry.registerAbilityBuilder(type,
            [mult, onDeal](Ability& a, AddTypeImmunity, AddStatusImmunity) {
                a.damageModifier = {mult, onDeal};
            });
    };
    regDmgMod(AbilityType::Overgrow, 1.5f, true);
    regDmgMod(AbilityType::Blaze,    1.5f, true);
    regDmgMod(AbilityType::Torrent,  1.5f, true);
    regDmgMod(AbilityType::Multiscale, 0.5f, false);
    regDmgMod(AbilityType::Guts,     1.5f, true);
    regDmgMod(AbilityType::ThickFat, 0.5f, false);
    regDmgMod(AbilityType::Technician, 1.5f, true);
    regDmgMod(AbilityType::Filter,  0.75f, false);
    regDmgMod(AbilityType::SolidRock, 0.75f, false);
    regDmgMod(AbilityType::SheerForce, 1.3f, true);

    // === Type immunities ===
    auto regTypeImm = [&registry](AbilityType type, Type moveType, bool heal, int healPct) {
        registry.registerAbilityBuilder(type,
            [moveType, heal, healPct](Ability&, AddTypeImmunity addT, AddStatusImmunity) {
                addT(moveType, heal, healPct);
            });
    };
    regTypeImm(AbilityType::Levitate,     Type::Ground,    false, 0);
    regTypeImm(AbilityType::WaterAbsorb,  Type::Water,     true, 25);
    regTypeImm(AbilityType::VoltAbsorb,   Type::Electric,  true, 25);
    regTypeImm(AbilityType::SapSipper,    Type::Grass,     false, 0);
    regTypeImm(AbilityType::StormDrain,   Type::Water,     false, 0);
    regTypeImm(AbilityType::MotorDrive,    Type::Electric,  false, 0);
    regTypeImm(AbilityType::EarthEater,   Type::Ground,    true, 25);
    regTypeImm(AbilityType::WellBakedBody, Type::Fire,     false, 0);

    // FlashFire: immunity + damage boost
    registry.registerAbilityBuilder(AbilityType::FlashFire,
        [](Ability& a, AddTypeImmunity addT, AddStatusImmunity) {
            addT(Type::Fire, false, 0);
            a.damageModifier = {1.5f, true};
        });

    // === Status immunities ===
    auto regStatusImm = [&registry](AbilityType type, StatusType status) {
        registry.registerAbilityBuilder(type,
            [status](Ability&, AddTypeImmunity, AddStatusImmunity addS) {
                addS(status);
            });
    };
    regStatusImm(AbilityType::Insomnia,    StatusType::Sleep);
    regStatusImm(AbilityType::VitalSpirit, StatusType::Sleep);
    regStatusImm(AbilityType::InnerFocus,  StatusType::Flinch);

    // Immunity: Poison + ToxicPoison
    registry.registerAbilityBuilder(AbilityType::Immunity,
        [](Ability&, AddTypeImmunity, AddStatusImmunity addS) {
            addS(StatusType::Poison);
            addS(StatusType::ToxicPoison);
        });

    // PurifyingSalt: blocks all major status + halves Ghost damage (handled in helper)
    registry.registerAbilityBuilder(AbilityType::PurifyingSalt,
        [](Ability&, AddTypeImmunity, AddStatusImmunity addS) {
            addS(StatusType::Burn);
            addS(StatusType::Freeze);
            addS(StatusType::Paralysis);
            addS(StatusType::Poison);
            addS(StatusType::Sleep);
            addS(StatusType::ToxicPoison);
        });

    // WaterBubble: burn immunity + fire resistance / water boost (handled in helpers)
    registry.registerAbilityBuilder(AbilityType::WaterBubble,
        [](Ability&, AddTypeImmunity, AddStatusImmunity addS) {
            addS(StatusType::Burn);
        });

    // === Stat modifiers ===
    registry.registerAbilityBuilder(AbilityType::HugePower,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.statModifiers.push_back({StatModifier::Attack, 2.0f, 0});
        });
    registry.registerAbilityBuilder(AbilityType::MarvelScale,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.statModifiers.push_back({StatModifier::Defense, 1.5f, 0});
        });

    // === Contact-based effects (OnDamage) ===
    registry.registerAbilityBuilder(AbilityType::Static,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self || !opponent) return;
                const auto* dc = static_cast<const AbilityDamageContext*>(context);
                if (!dc || !dc->isDamagingMove || !dc->isContact) return;
                if (PRNG::nextInt(0, 100) >= 30) return;
                if (resolveStatusImmunity(opponent->getAbility(), StatusType::Paralysis)) return;
                opponent->addStatus(StatusType::Paralysis);
            };
        });
    registry.registerAbilityBuilder(AbilityType::PoisonPoint,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self || !opponent) return;
                const auto* dc = static_cast<const AbilityDamageContext*>(context);
                if (!dc || !dc->isDamagingMove || !dc->isContact) return;
                if (PRNG::nextInt(0, 100) >= 30) return;
                if (resolveStatusImmunity(opponent->getAbility(), StatusType::Poison)) return;
                opponent->addStatus(StatusType::Poison);
            };
        });
    registry.registerAbilityBuilder(AbilityType::FlameBody,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self || !opponent) return;
                const auto* dc = static_cast<const AbilityDamageContext*>(context);
                if (!dc || !dc->isDamagingMove || !dc->isContact) return;
                if (PRNG::nextInt(0, 100) >= 30) return;
                if (resolveStatusImmunity(opponent->getAbility(), StatusType::Burn)) return;
                opponent->addStatus(StatusType::Burn);
            };
        });

    // Contact chip (1/8) — RoughSkin, IronBarbs
    auto regContactChip = [&registry](AbilityType type, int fraction) {
        registry.registerAbilityBuilder(type,
            [fraction](Ability& a, AddTypeImmunity, AddStatusImmunity) {
                a.effects[Trigger::OnDamage] = [fraction](Pokemon* self, Pokemon* opponent, void* context) {
                    if (!self || !opponent) return;
                    const auto* dc = static_cast<const AbilityDamageContext*>(context);
                    if (!dc || !dc->isDamagingMove || !dc->isContact) return;
                    if (opponent->isFainted()) return;
                    const int chip = std::max(1, opponent->getMaxHP() / fraction);
                    opponent->setCurrentHP(opponent->getCurrentHP() - chip);
                };
            });
    };
    regContactChip(AbilityType::RoughSkin, 8);
    regContactChip(AbilityType::IronBarbs, 8);

    // Aftermath: 1/4 chip only when holder faints
    registry.registerAbilityBuilder(AbilityType::Aftermath,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self || !opponent) return;
                const auto* dc = static_cast<const AbilityDamageContext*>(context);
                if (!dc || !dc->isDamagingMove || !dc->isContact) return;
                if (!self->isFainted() || opponent->isFainted()) return;
                const int chip = std::max(1, opponent->getMaxHP() / 4);
                opponent->setCurrentHP(opponent->getCurrentHP() - chip);
            };
        });

    // Mummy: replaces contact attacker's ability
    registry.registerAbilityBuilder(AbilityType::Mummy,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self || !opponent) return;
                const auto* dc = static_cast<const AbilityDamageContext*>(context);
                if (!dc || !dc->isDamagingMove || !dc->isContact) return;
                if (opponent->isFainted()) return;
                if (opponent->getAbility() != AbilityType::Mummy)
                    opponent->setAbility(AbilityType::Mummy);
            };
        });

    // LingeringAroma: replaces contact attacker's ability
    registry.registerAbilityBuilder(AbilityType::LingeringAroma,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self || !opponent) return;
                const auto* dc = static_cast<const AbilityDamageContext*>(context);
                if (!dc || !dc->isDamagingMove || !dc->isContact) return;
                if (opponent->isFainted()) return;
                if (opponent->getAbility() != AbilityType::LingeringAroma)
                    opponent->setAbility(AbilityType::LingeringAroma);
            };
        });

    // ToxicDebris: sets toxic spikes when hit by physical move
    registry.registerAbilityBuilder(AbilityType::ToxicDebris,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self || !opponent) return;
                const auto* dc = static_cast<const AbilityDamageContext*>(context);
                if (!dc || !dc->isDamagingMove || !dc->move || !dc->context) return;
                if (dc->move->getCategory() != Category::Physical) return;
                Side* oppSide = dc->context->findSideForPokemon(opponent);
                if (oppSide) oppSide->addToxicSpikesLayer();
            };
        });

    // === OnFaint: Moxie ===
    registry.registerAbilityBuilder(AbilityType::Moxie,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnFaint] = [](Pokemon* self, Pokemon*, void*) {
                if (self) self->changeStatStage(StatIndex::Attack, 1);
            };
        });

    // === OnExit: Regenerator, NaturalCure ===
    registry.registerAbilityBuilder(AbilityType::Regenerator,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnExit] = [](Pokemon* self, Pokemon*, void*) {
                if (!self || self->isFainted()) return;
                const int heal = std::max(1, self->getMaxHP() / 3);
                self->setCurrentHP(self->getCurrentHP() + heal);
            };
        });
    registry.registerAbilityBuilder(AbilityType::NaturalCure,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnExit] = [](Pokemon* self, Pokemon*, void*) {
                if (!self || self->isFainted()) return;
                self->clearStatuses();
            };
        });

    // === Weather setters ===
    regWeatherSetter(AbilityType::Drizzle,     WeatherType::Rain);
    regWeatherSetter(AbilityType::Drought,     WeatherType::Sun);
    regWeatherSetter(AbilityType::SandStream,  WeatherType::Sandstorm);
    regWeatherSetter(AbilityType::SnowWarning, WeatherType::Snow);

    // === Terrain/Field setters ===
    regTerrainSetter(AbilityType::GrassySurge,    FieldType::Grassy);
    regTerrainSetter(AbilityType::ElectricSurge,  FieldType::Electric);
    regTerrainSetter(AbilityType::PsychicSurge,   FieldType::Psychic);
    regTerrainSetter(AbilityType::MistySurge,     FieldType::Misty);
    regTerrainSetter(AbilityType::HadronEngine,   FieldType::Electric);

    // === Abilities with passive effects only (formerly regHelperOnly) ===
    auto regPassive = [&registry](AbilityType type, std::function<void(Ability::PassiveFlags&)> setFlags) {
        registry.registerAbilityBuilder(type,
            [setFlags](Ability& a, AddTypeImmunity, AddStatusImmunity) {
                setFlags(a.passive);
            });
    };

    regPassive(AbilityType::MagicGuard, [](auto& p) { p.ignoresIndirectDamage = true; });
    regPassive(AbilityType::Unaware, [](auto& p) { p.ignoresOpponentStatStages = true; });
    regPassive(AbilityType::Prankster, [](auto& p) { p.statusMovePriorityBonus = 1; });
    regPassive(AbilityType::ClearBody, [](auto& p) {
        p.blocksGenericStatDrops = true;
        p.blocksAccuracyDrops = true;
    });
    regPassive(AbilityType::Defiant, [](auto& p) {
        p.statDropReactionBoostsAttack = true;
        p.statDropReactionStages = 2;
    });
    regPassive(AbilityType::Competitive, [](auto& p) {
        p.statDropReactionBoostsSpAttack = true;
        p.statDropReactionStages = 2;
    });
    regPassive(AbilityType::WhiteSmoke, [](auto& p) {
        p.blocksGenericStatDrops = true;
        p.blocksAccuracyDrops = true;
    });
    regPassive(AbilityType::MirrorArmor, [](auto& p) { p.reflectsStatDrops = true; });
    regPassive(AbilityType::HyperCutter, [](auto& p) { p.blocksAttackDrops = true; });
    regPassive(AbilityType::KeenEye, [](auto& p) { p.blocksEvasionDrops = true; });
    regPassive(AbilityType::CloudNine, [](auto& p) { p.suppressesWeather = true; });
    regPassive(AbilityType::Protean, [](auto& p) { p.canTypeShift = true; });
    regPassive(AbilityType::Libero, [](auto& p) { p.canTypeShift = true; });
    regPassive(AbilityType::Adaptability, [](auto& p) { p.stabBonusMultiplier = 2.0f; });
    regPassive(AbilityType::Infiltrator, [](auto& p) {
        p.ignoresSubstitute = true;
        p.ignoresScreens = true;
    });
    regPassive(AbilityType::BeadsOfRuin, [](auto& p) { p.lowersOppSpDefAura = true; });
    regPassive(AbilityType::SwordOfRuin, [](auto& p) { p.lowersOppDefAura = true; });
    regPassive(AbilityType::TabletsOfRuin, [](auto& p) { p.lowersOppPhysAtkAura = true; });
    regPassive(AbilityType::VesselOfRuin, [](auto& p) { p.lowersOppSpAtkAura = true; });
    regPassive(AbilityType::Unnerve, [](auto& p) { p.blocksBerryConsumption = true; });
    // WindRider: blocks wind-move damage (handled in abilityBlocksMoveDamage) and boosts Attack
    regPassive(AbilityType::WindRider, [](auto& p) {});
    regPassive(AbilityType::ArmorTail, [](auto& p) { p.blocksPriorityTargetedMoves = true; });
    regPassive(AbilityType::GoodAsGold, [](auto& p) { p.blocksStatusMovesFromOpponents = true; });
    regPassive(AbilityType::CudChew, [](auto& p) { p.hasCudChew = true; });
    regPassive(AbilityType::MoldBreaker, [](auto& p) { p.ignoresTargetAbility = true; });
    regPassive(AbilityType::Protosynthesis, [](auto& p) { p.hasParadoxBoost = true; });
    regPassive(AbilityType::QuarkDrive, [](auto& p) { p.hasParadoxBoost = true; });
    regPassive(AbilityType::Triage, [](auto& p) { p.statusMovePriorityBonus = 3; });
    regPassive(AbilityType::Corrosion, [](auto& p) { p.overridesPoisonTypeImmunity = true; });
    regPassive(AbilityType::Scrappy, [](auto& p) { p.overridesGhostImmunity = true; });
    regPassive(AbilityType::Contrary, [](auto& p) { p.reversesStatChanges = true; });

    // Abilities that need runtime context (damage multipliers keyed on move/HP/state).
    // These abilities are self-describing via damageModifier + passive flags;
    // the remaining runtime checks live in Battle's calculateDamage helpers.
    regPassive(AbilityType::Sharpness, [](auto& p) {});
    regPassive(AbilityType::Stakeout, [](auto& p) {});
    regPassive(AbilityType::SupremeOverlord, [](auto& p) {});
    regPassive(AbilityType::Steelworker, [](auto& p) {});
    regPassive(AbilityType::Bulletproof, [](auto& p) {});
}

std::vector<Ability> getAbilitiesForPokemon(AbilityType type) {
    // For now, each Pokemon has one ability
    std::vector<Ability> abilities;
    abilities.push_back(getAbility(type));
    return abilities;
}