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

bool isPunchingMove(const Move& move) {
    const std::string key = normalizeToken(move.getName());
    return key == "bulletpunch" || key == "cometpunch" || key == "dizzypunch"
        || key == "doubleironbash" || key == "drainpunch" || key == "dynamicpunch"
        || key == "firepunch" || key == "focuspunch" || key == "hammerarm"
        || key == "icehammer" || key == "icepunch" || key == "machpunch"
        || key == "megapunch" || key == "meteormash" || key == "plasmafists"
        || key == "poweruppunch" || key == "shadowpunch" || key == "skyuppercut"
        || key == "thunderpunch" || key == "surgingstrikes";
}

bool isBitingMove(const Move& move) {
    const std::string key = normalizeToken(move.getName());
    return key == "bite" || key == "crunch" || key == "firefang"
        || key == "icefang" || key == "thunderfang" || key == "psychicfangs"
        || key == "fishiousrend" || key == "hyperfang" || key == "poisonfang"
        || key == "jawlock";
}

bool hasRecoilMove(const Move& move) {
    const std::string key = normalizeToken(move.getName());
    return key == "bravebird" || key == "doubleedge" || key == "flareblitz"
        || key == "headcharge" || key == "headsmash" || key == "hijumpkick"
        || key == "steelbeam" || key == "submission" || key == "takedown"
        || key == "volttackle" || key == "wildcharge" || key == "woodhammer"
        || key == "chloroblast" || key == "wavecrash" || key == "jumpkick";
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
        case AbilityType::SwiftSwim: return "Swift Swim";
        case AbilityType::Chlorophyll: return "Chlorophyll";
        case AbilityType::SandRush: return "Sand Rush";
        case AbilityType::SlushRush: return "Slush Rush";
        case AbilityType::SurgeSurfer: return "Surge Surfer";
        case AbilityType::SpeedBoost: return "Speed Boost";
        case AbilityType::Sturdy: return "Sturdy";
        case AbilityType::Limber: return "Limber";
        case AbilityType::OwnTempo: return "Own Tempo";
        case AbilityType::Oblivious: return "Oblivious";
        case AbilityType::SereneGrace: return "Serene Grace";
        case AbilityType::IronFist: return "Iron Fist";
        case AbilityType::Reckless: return "Reckless";
        case AbilityType::StrongJaw: return "Strong Jaw";
        case AbilityType::ToughClaws: return "Tough Claws";
        case AbilityType::BattleArmor: return "Battle Armor";
        case AbilityType::ShellArmor: return "Shell Armor";
        case AbilityType::ShedSkin: return "Shed Skin";
        case AbilityType::MagicBounce: return "Magic Bounce";
        case AbilityType::Hustle: return "Hustle";
        case AbilityType::Pressure: return "Pressure";
        case AbilityType::WonderGuard: return "Wonder Guard";
        case AbilityType::ShadowTag: return "Shadow Tag";
        case AbilityType::LightningRod: return "Lightning Rod";
        case AbilityType::Soundproof: return "Soundproof";
        case AbilityType::Trace: return "Trace";
        case AbilityType::PurePower: return "Pure Power";
        case AbilityType::CompoundEyes: return "Compound Eyes";
        case AbilityType::RockHead: return "Rock Head";
        case AbilityType::ShieldDust: return "Shield Dust";
        case AbilityType::Simple: return "Simple";
        case AbilityType::Synchronize: return "Synchronize";
        case AbilityType::MagnetPull: return "Magnet Pull";
        case AbilityType::ArenaTrap: return "Arena Trap";
        case AbilityType::RainDish: return "Rain Dish";
        case AbilityType::StickyHold: return "Sticky Hold";
        case AbilityType::Damp: return "Damp";
        case AbilityType::EarlyBird: return "Early Bird";
        case AbilityType::Unburden: return "Unburden";
        case AbilityType::AngerPoint: return "Anger Point";
        case AbilityType::Gluttony: return "Gluttony";
        case AbilityType::EffectSpore: return "Effect Spore";
        case AbilityType::WaterVeil: return "Water Veil";
        case AbilityType::MagmaArmor: return "Magma Armor";
        case AbilityType::LiquidOoze: return "Liquid Ooze";
        case AbilityType::SandVeil: return "Sand Veil";
        case AbilityType::Stench: return "Stench";
        case AbilityType::CuteCharm: return "Cute Charm";
        case AbilityType::Steadfast: return "Steadfast";
        case AbilityType::TangledFeet: return "Tangled Feet";
        case AbilityType::Rivalry: return "Rivalry";
        case AbilityType::SuctionCups: return "Suction Cups";
        case AbilityType::ColorChange: return "Color Change";
        case AbilityType::Heatproof: return "Heatproof";
        case AbilityType::AirLock: return "Air Lock";
        case AbilityType::SnowCloak: return "Snow Cloak";
        case AbilityType::Sniper: return "Sniper";
        case AbilityType::NoGuard: return "No Guard";
        case AbilityType::SkillLink: return "Skill Link";
        case AbilityType::Hydration: return "Hydration";
        case AbilityType::PoisonHeal: return "Poison Heal";
        case AbilityType::Download: return "Download";
        case AbilityType::Normalize: return "Normalize";
        case AbilityType::TintedLens: return "Tinted Lens";
        case AbilityType::Klutz: return "Klutz";
        case AbilityType::SlowStart: return "Slow Start";
        case AbilityType::Swarm: return "Swarm";
        case AbilityType::DrySkin: return "Dry Skin";
        case AbilityType::SolarPower: return "Solar Power";
        case AbilityType::QuickFeet: return "Quick Feet";
        case AbilityType::Stall: return "Stall";
        case AbilityType::LeafGuard: return "Leaf Guard";
        case AbilityType::SuperLuck: return "Super Luck";
        case AbilityType::Anticipation: return "Anticipation";
        case AbilityType::Forewarn: return "Forewarn";
        case AbilityType::IceBody: return "Ice Body";
        case AbilityType::Frisk: return "Frisk";
        case AbilityType::Pickpocket: return "Pickpocket";
        case AbilityType::Defeatist: return "Defeatist";
        case AbilityType::CursedBody: return "Cursed Body";
        case AbilityType::WeakArmor: return "Weak Armor";
        case AbilityType::HeavyMetal: return "Heavy Metal";
        case AbilityType::LightMetal: return "Light Metal";
        case AbilityType::ToxicBoost: return "Toxic Boost";
        case AbilityType::FlareBoost: return "Flare Boost";
        case AbilityType::Harvest: return "Harvest";
        case AbilityType::Overcoat: return "Overcoat";
        case AbilityType::PoisonTouch: return "Poison Touch";
        case AbilityType::BigPecks: return "Big Pecks";
        case AbilityType::WonderSkin: return "Wonder Skin";
        case AbilityType::Analytic: return "Analytic";
        case AbilityType::Illusion: return "Illusion";
        case AbilityType::Justified: return "Justified";
        case AbilityType::Rattled: return "Rattled";
        case AbilityType::SandForce: return "Sand Force";
        case AbilityType::VictoryStar: return "Victory Star";
        case AbilityType::Plus: return "Plus";
        case AbilityType::Minus: return "Minus";
        case AbilityType::Forecast: return "Forecast";
        case AbilityType::FlowerGift: return "Flower Gift";
        case AbilityType::BadDreams: return "Bad Dreams";
        case AbilityType::Moody: return "Moody";
        case AbilityType::Imposter: return "Imposter";
        case AbilityType::Turboblaze: return "Turboblaze";
        case AbilityType::Teravolt: return "Teravolt";
        case AbilityType::AromaVeil: return "Aroma Veil";
        case AbilityType::FurCoat: return "Fur Coat";
        case AbilityType::Magician: return "Magician";
        case AbilityType::Refrigerate: return "Refrigerate";
        case AbilityType::SweetVeil: return "Sweet Veil";
        case AbilityType::GaleWings: return "Gale Wings";
        case AbilityType::MegaLauncher: return "Mega Launcher";
        case AbilityType::Pixilate: return "Pixilate";
        case AbilityType::Gooey: return "Gooey";
        case AbilityType::Aerilate: return "Aerilate";
        case AbilityType::ParentalBond: return "Parental Bond";
        case AbilityType::Stamina: return "Stamina";
        case AbilityType::Merciless: return "Merciless";
        case AbilityType::Berserk: return "Berserk";
        case AbilityType::LongReach: return "Long Reach";
        case AbilityType::LiquidVoice: return "Liquid Voice";
        case AbilityType::Galvanize: return "Galvanize";
        case AbilityType::QueenlyMajesty: return "Queenly Majesty";
        case AbilityType::Dancer: return "Dancer";
        case AbilityType::Battery: return "Battery";
        case AbilityType::DarkAura: return "Dark Aura";
        case AbilityType::FairyAura: return "Fairy Aura";
        case AbilityType::AuraBreak: return "Aura Break";
        case AbilityType::PrimordialSea: return "Primordial Sea";
        case AbilityType::DesolateLand: return "Desolate Land";
        case AbilityType::DeltaStream: return "Delta Stream";
        case AbilityType::Healer: return "Healer";
        case AbilityType::FriendGuard: return "Friend Guard";
        case AbilityType::Telepathy: return "Telepathy";
        case AbilityType::GrassPelt: return "Grass Pelt";
        case AbilityType::Symbiosis: return "Symbiosis";
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
    if (key == "swiftswim") return AbilityType::SwiftSwim;
    if (key == "swiftswim") return AbilityType::SwiftSwim;
    if (key == "chlorophyll") return AbilityType::Chlorophyll;
    if (key == "sandrush") return AbilityType::SandRush;
    if (key == "slushrush") return AbilityType::SlushRush;
    if (key == "surgesurfer") return AbilityType::SurgeSurfer;
    if (key == "speedboost") return AbilityType::SpeedBoost;
    if (key == "sturdy") return AbilityType::Sturdy;
    if (key == "limber") return AbilityType::Limber;
    if (key == "owntempo") return AbilityType::OwnTempo;
    if (key == "oblivious") return AbilityType::Oblivious;
    if (key == "serenegrace") return AbilityType::SereneGrace;
    if (key == "ironfist") return AbilityType::IronFist;
    if (key == "reckless") return AbilityType::Reckless;
    if (key == "strongjaw") return AbilityType::StrongJaw;
    if (key == "toughclaws") return AbilityType::ToughClaws;
    if (key == "battlearmor") return AbilityType::BattleArmor;
    if (key == "shellarmor") return AbilityType::ShellArmor;
    if (key == "shedskin") return AbilityType::ShedSkin;
    if (key == "magicbounce") return AbilityType::MagicBounce;
    if (key == "hustle") return AbilityType::Hustle;
    if (key == "pressure") return AbilityType::Pressure;
    if (key == "wonderguard") return AbilityType::WonderGuard;
    if (key == "shadowtag") return AbilityType::ShadowTag;
    if (key == "lightningrod") return AbilityType::LightningRod;
    if (key == "soundproof") return AbilityType::Soundproof;
    if (key == "trace") return AbilityType::Trace;
    if (key == "purepower") return AbilityType::PurePower;
    if (key == "compoundeyes") return AbilityType::CompoundEyes;
    if (key == "rockhead") return AbilityType::RockHead;
    if (key == "shielddust") return AbilityType::ShieldDust;
    if (key == "simple") return AbilityType::Simple;
    if (key == "synchronize") return AbilityType::Synchronize;
    if (key == "magnetpull") return AbilityType::MagnetPull;
    if (key == "arenatrap") return AbilityType::ArenaTrap;
    if (key == "raindish") return AbilityType::RainDish;
    if (key == "stickyhold") return AbilityType::StickyHold;
    if (key == "damp") return AbilityType::Damp;
    if (key == "earlybird") return AbilityType::EarlyBird;
    if (key == "unburden") return AbilityType::Unburden;
    if (key == "angerpoint") return AbilityType::AngerPoint;
    if (key == "gluttony") return AbilityType::Gluttony;
    if (key == "effectspore") return AbilityType::EffectSpore;
    if (key == "waterveil") return AbilityType::WaterVeil;
    if (key == "magmaarmor") return AbilityType::MagmaArmor;
    if (key == "liquidooze") return AbilityType::LiquidOoze;
    if (key == "sandveil") return AbilityType::SandVeil;
    if (key == "stench") return AbilityType::Stench;
    if (key == "cutecharm") return AbilityType::CuteCharm;
    if (key == "steadfast") return AbilityType::Steadfast;
    if (key == "tangledfeet") return AbilityType::TangledFeet;
    if (key == "rivalry") return AbilityType::Rivalry;
    if (key == "suctioncups") return AbilityType::SuctionCups;
    if (key == "colorchange") return AbilityType::ColorChange;
    if (key == "heatproof") return AbilityType::Heatproof;
    if (key == "airlock") return AbilityType::AirLock;
    if (key == "snowcloak") return AbilityType::SnowCloak;
    if (key == "sniper") return AbilityType::Sniper;
    if (key == "noguard") return AbilityType::NoGuard;
    if (key == "skilllink") return AbilityType::SkillLink;
    if (key == "hydration") return AbilityType::Hydration;
    if (key == "poisonheal") return AbilityType::PoisonHeal;
    if (key == "download") return AbilityType::Download;
    if (key == "normalize") return AbilityType::Normalize;
    if (key == "tintedlens") return AbilityType::TintedLens;
    if (key == "klutz") return AbilityType::Klutz;
    if (key == "slowstart") return AbilityType::SlowStart;
    if (key == "swarm") return AbilityType::Swarm;
    if (key == "dryskin") return AbilityType::DrySkin;
    if (key == "solarpower") return AbilityType::SolarPower;
    if (key == "quickfeet") return AbilityType::QuickFeet;
    if (key == "stall") return AbilityType::Stall;
    if (key == "leafguard") return AbilityType::LeafGuard;
    if (key == "superluck") return AbilityType::SuperLuck;
    if (key == "anticipation") return AbilityType::Anticipation;
    if (key == "forewarn") return AbilityType::Forewarn;
    if (key == "icebody") return AbilityType::IceBody;
    if (key == "frisk") return AbilityType::Frisk;
    if (key == "pickpocket") return AbilityType::Pickpocket;
    if (key == "defeatist") return AbilityType::Defeatist;
    if (key == "cursedbody") return AbilityType::CursedBody;
    if (key == "weakarmor") return AbilityType::WeakArmor;
    if (key == "heavymetal") return AbilityType::HeavyMetal;
    if (key == "lightmetal") return AbilityType::LightMetal;
    if (key == "toxicboost") return AbilityType::ToxicBoost;
    if (key == "flareboost") return AbilityType::FlareBoost;
    if (key == "harvest") return AbilityType::Harvest;
    if (key == "overcoat") return AbilityType::Overcoat;
    if (key == "poisontouch") return AbilityType::PoisonTouch;
    if (key == "bigpecks") return AbilityType::BigPecks;
    if (key == "wonderskin") return AbilityType::WonderSkin;
    if (key == "analytic") return AbilityType::Analytic;
    if (key == "illusion") return AbilityType::Illusion;
    if (key == "justified") return AbilityType::Justified;
    if (key == "rattled") return AbilityType::Rattled;
    if (key == "sandforce") return AbilityType::SandForce;
    if (key == "victorystar") return AbilityType::VictoryStar;
    if (key == "plus") return AbilityType::Plus;
    if (key == "minus") return AbilityType::Minus;
    if (key == "forecast") return AbilityType::Forecast;
    if (key == "flowergift") return AbilityType::FlowerGift;
    if (key == "baddreams") return AbilityType::BadDreams;
    if (key == "moody") return AbilityType::Moody;
    if (key == "imposter") return AbilityType::Imposter;
    if (key == "turboblaze") return AbilityType::Turboblaze;
    if (key == "teravolt") return AbilityType::Teravolt;
    if (key == "aromaveil") return AbilityType::AromaVeil;
    if (key == "furcoat") return AbilityType::FurCoat;
    if (key == "magician") return AbilityType::Magician;
    if (key == "refrigerate") return AbilityType::Refrigerate;
    if (key == "sweetveil") return AbilityType::SweetVeil;
    if (key == "galewings") return AbilityType::GaleWings;
    if (key == "megalauncher") return AbilityType::MegaLauncher;
    if (key == "pixilate") return AbilityType::Pixilate;
    if (key == "gooey") return AbilityType::Gooey;
    if (key == "aerilate") return AbilityType::Aerilate;
    if (key == "parentalbond") return AbilityType::ParentalBond;
    if (key == "stamina") return AbilityType::Stamina;
    if (key == "merciless") return AbilityType::Merciless;
    if (key == "berserk") return AbilityType::Berserk;
    if (key == "longreach") return AbilityType::LongReach;
    if (key == "liquidvoice") return AbilityType::LiquidVoice;
    if (key == "galvanize") return AbilityType::Galvanize;
    if (key == "queenlymajesty") return AbilityType::QueenlyMajesty;
    if (key == "dancer") return AbilityType::Dancer;
    if (key == "battery") return AbilityType::Battery;
    if (key == "darkaura") return AbilityType::DarkAura;
    if (key == "fairyaura") return AbilityType::FairyAura;
    if (key == "aurabreak") return AbilityType::AuraBreak;
    if (key == "primordialsea") return AbilityType::PrimordialSea;
    if (key == "desolateland") return AbilityType::DesolateLand;
    if (key == "deltastream") return AbilityType::DeltaStream;
    if (key == "healer") return AbilityType::Healer;
    if (key == "friendguard") return AbilityType::FriendGuard;
    if (key == "telepathy") return AbilityType::Telepathy;
    if (key == "grasspelt") return AbilityType::GrassPelt;
    if (key == "symbiosis") return AbilityType::Symbiosis;
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

bool isPokemonGrounded(const Pokemon* pokemon, const RuntimeMoveState& runtimeState) {
    if (!pokemon) return false;

    const bool isFlyingType = pokemon->getType1() == Type::Flying || pokemon->getType2() == Type::Flying;
    if (isFlyingType) return false;

    if (abilityGrantsGroundHazardImmunity(pokemon->getAbility())) return false;

    // Magnet Rise provides temporary grounding immunity
    const auto magnetIt = runtimeState.magnetRiseTurns.find(const_cast<Pokemon*>(pokemon));
    if (magnetIt != runtimeState.magnetRiseTurns.end() && magnetIt->second > 0) return false;

    return true;
}

bool hasMagnetRiseEffect(const Pokemon* pokemon, const RuntimeMoveState& runtimeState) {
    if (!pokemon) return false;
    const auto magnetIt = runtimeState.magnetRiseTurns.find(const_cast<Pokemon*>(pokemon));
    return magnetIt != runtimeState.magnetRiseTurns.end() && magnetIt->second > 0;
}

bool abilityHasSturdy(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.sturdyEndure;
}

bool abilityPreventsTaunt(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.preventsTaunt;
}

bool abilityPreventsInfatuation(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.preventsInfatuation;
}

bool abilityBlocksCriticalHits(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.blocksCriticalHits;
}

bool abilityReflectsStatusMoves(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.reflectsStatusMoves;
}

bool abilityDrainsOpponentPP(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.drainsOpponentPP;
}

bool abilityHasWonderGuard(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.wonderGuard;
}

bool abilityTrapsOpponent(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.trapsOpponent;
}

bool abilityRedirectsElectricMoves(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.redirectsElectricMoves;
}

bool abilityBlocksSoundMoves(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.blocksSoundMoves;
}

bool abilityCopiesOpponentAbility(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.copiesOpponentAbility;
}

bool abilityDoublesAttack(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.doublesAttack;
}

float abilityAccuracyBoost(AbilityType abilityType) {
    if (abilityType == AbilityType::CompoundEyes) return 1.3f;
    return 1.0f;
}

bool abilityPreventsRecoil(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.preventsRecoil;
}

bool abilityBlocksMoveSecondaryEffects(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.blocksMoveSecondaryEffects;
}

bool abilityMirrorsStatus(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.mirrorsStatus;
}

bool abilityTrapsSteelTypes(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.trapsSteelTypes;
}

bool abilityTrapsGrounded(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.trapsGrounded;
}

bool abilityHealsInRain(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.healsInRain;
}

bool abilityPreventsItemLoss(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.preventsItemLoss;
}

bool abilityPreventsExplosion(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.preventsExplosion;
}

bool abilityHalvesSleepTurns(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.halvesSleepTurns;
}

bool abilitySpeedDoubledWithoutItem(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.speedDoubledWithoutItem;
}

bool abilityAttackMaxedOnCrit(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.attackMaxedOnCrit;
}

bool abilityEarlyBerryConsumption(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.earlyBerryConsumption;
}

bool abilityInflictsRandomContactStatus(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.inflictsRandomContactStatus;
}

bool abilityPreventsBurn(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.preventsBurn;
}

bool abilityPreventsFreeze(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.preventsFreeze;
}

bool abilityInvertsDrainingHeal(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.invertsDrainingHeal;
}

bool abilityBoostsEvasionInSand(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.boostsEvasionInSand;
}

bool abilityFlinchOnHit(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.flinchOnHit;
}

bool abilityInfatuatesOnContact(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.infatuatesOnContact;
}

bool abilitySpeedBoostWhenFlinched(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.speedBoostWhenFlinched;
}

bool abilityEvasionDoubleWhenConfused(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.evasionDoubleWhenConfused;
}

bool abilityRivalryDamageModifier(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.rivalryDamageModifier;
}

bool abilityPreventsForcedSwitch(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.preventsForcedSwitch;
}

bool abilityColorChangeOnHit(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.colorChangeOnHit;
}

bool abilityFireResistance(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.fireResistance;
}

bool abilityNegatesWeather(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.negatesWeather;
}

bool abilityEvasionInSnow(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.evasionInSnow;
}

bool abilitySniperCritBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.sniperCritBoost;
}

bool abilityNoGuardAlwaysHit(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.noGuardAlwaysHit;
}

bool abilitySkillLinkMaxHits(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.skillLinkMaxHits;
}

bool abilityHydrationHealsStatus(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.hydrationHealsStatus;
}

bool abilityPoisonHealRecovery(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.poisonHealRecovery;
}

bool abilityDownloadStatBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.downloadStatBoost;
}

bool abilityNormalizeAllNormal(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.normalizeAllNormal;
}

bool abilityTintedLensBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.tintedLensBoost;
}

bool abilityKlutzNoItem(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.klutzNoItem;
}

bool abilitySlowStartHalved(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.slowStartHalved;
}

bool abilitySwarmBugBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.swarmBugBoost;
}

bool abilityDrySkinEffects(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.drySkinEffects;
}

bool abilitySolarPowerBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.solarPowerBoost;
}

bool abilityQuickFeetSpeedBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.quickFeetSpeedBoost;
}

bool abilityAbilityAlwaysMovesLast(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.alwaysMovesLast;
}

bool abilityLeafGuardSun(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.leafGuardSun;
}

bool abilitySuperLuckCrit(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.superLuckCrit;
}

bool abilityAnticipationShudder(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.anticipationShudder;
}

bool abilityForewarnReveal(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.forewarnReveal;
}

bool abilityIceBodyHailHeal(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.iceBodyHailHeal;
}

bool abilityFriskRevealsItem(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.friskRevealsItem;
}

bool abilityPickpocketStealsItem(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.pickpocketStealsItem;
}

bool abilityDefeatistDebuff(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.defeatistDebuff;
}

bool abilityCursedBodyDisable(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.cursedBodyDisable;
}

bool abilityWeakArmorStatShift(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.weakArmorStatShift;
}

bool abilityHeavyMetalWeightDouble(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.heavyMetalWeightDouble;
}

bool abilityLightMetalWeightHalf(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.lightMetalWeightHalf;
}

bool abilityToxicBoostAttack(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.toxicBoostAttack;
}

bool abilityFlareBoostSpAttack(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.flareBoostSpAttack;
}

bool abilityHarvestRecyclesBerry(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.harvestRecyclesBerry;
}

bool abilityOvercoatPowderWeather(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.overcoatPowderWeather;
}

bool abilityPoisonTouchContact(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.poisonTouchContact;
}

bool abilityBigPecksPreventDefDrop(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.bigPecksPreventDefDrop;
}

bool abilityWonderSkinReducedAccuracy(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.wonderSkinReducedAccuracy;
}

bool abilityAnalyticMoveLastBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.analyticMoveLastBoost;
}

bool abilityIllusionDisguise(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.illusionDisguise;
}

bool abilityJustifiedDarkBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.justifiedDarkBoost;
}

bool abilityRattledSpeedBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.rattledSpeedBoost;
}

bool abilitySandForceBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.sandForceBoost;
}

bool abilityVictoryStarAccuracy(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.victoryStarAccuracy;
}

bool abilityPlusMinusSpAtk(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.plusMinusSpAtk;
}

bool abilityForecastWeather(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.forecastWeather;
}

bool abilityFlowerGiftBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.flowerGiftBoost;
}

bool abilityBadDreamsDamage(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.badDreamsDamage;
}

bool abilityMoodyRandomBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.moodyRandomBoost;
}

bool abilityImposterTransform(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.imposterTransform;
}

bool abilityMoldBreakerLike(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.moldBreakerLike;
}

bool abilityAromaVeilProtection(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.aromaVeilProtection;
}

bool abilityFurCoatDefense(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.furCoatDefense;
}

bool abilityMagicianSteal(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.magicianSteal;
}

bool abilityRefrigerateNormal(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.refrigerateNormal;
}

bool abilitySweetVeilPreventSleep(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.sweetVeilPreventSleep;
}

bool abilityGaleWingsPriority(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.galeWingsPriority;
}

bool abilityMegaLauncherBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.megaLauncherBoost;
}

bool abilityPixilateNormal(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.pixilateNormal;
}

bool abilityGooeySlow(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.gooeySlow;
}

bool abilityAerilateNormal(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.aerilateNormal;
}

bool abilityParentalBond(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.parentalBond;
}

bool abilityStaminaDefBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.staminaDefBoost;
}

bool abilityMercilessAutoCrit(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.mercilessAutoCrit;
}

bool abilityBerserkSpAtkBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.berserkSpAtkBoost;
}

bool abilityLongReachNoContact(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.longReachNoContact;
}

bool abilityLiquidVoiceWater(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.liquidVoiceWater;
}

bool abilityGalvanizeElectric(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.galvanizeElectric;
}

bool abilityQueenlyMajestyPriority(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.queenlyMajestyPriority;
}

bool abilityDancerDanceCopy(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.dancerDanceCopy;
}

bool abilityBatteryAllySpAtk(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.batteryAllySpAtk;
}

bool abilityDarkAuraBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.darkAuraBoost;
}

bool abilityFairyAuraBoost(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.fairyAuraBoost;
}

bool abilityAuraBreakInvert(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.auraBreakInvert;
}

bool abilityPrimordialSea(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.primordialSea;
}

bool abilityDesolateLand(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.desolateLand;
}

bool abilityDeltaStream(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.deltaStream;
}

bool abilityHealerAllyStatus(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.healerAllyStatus;
}

bool abilityFriendGuardReduce(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.friendGuardReduce;
}

bool abilityTelepathyAvoidAlly(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.telepathyAvoidAlly;
}

bool abilityGrassPeltDefense(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.grassPeltDefense;
}

bool abilitySymbiosisPass(AbilityType abilityType) {
    return GameRegistry::instance().getAbility(abilityType).passive.symbiosisPass;
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
    if (abilityType == AbilityType::IronFist && move.getCategory() != Category::Status && isPunchingMove(move)) {
        modifiedPower = static_cast<int>(std::lround(modifiedPower * 1.2f));
    }
    if (abilityType == AbilityType::StrongJaw && move.getCategory() != Category::Status && isBitingMove(move)) {
        modifiedPower = static_cast<int>(std::lround(modifiedPower * 1.5f));
    }
    if (abilityType == AbilityType::ToughClaws && move.getCategory() != Category::Status) {
        modifiedPower = static_cast<int>(std::lround(modifiedPower * 1.3f));
    }
    if (abilityType == AbilityType::Reckless && move.getCategory() != Category::Status && hasRecoilMove(move)) {
        modifiedPower = static_cast<int>(std::lround(modifiedPower * 1.2f));
    }
    if (abilityType == AbilityType::Hustle && move.getCategory() == Category::Physical) {
        modifiedPower = static_cast<int>(std::lround(modifiedPower * 1.5f));
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

bool isSoundMove(const Move& move) {
    const std::string key = normalizeToken(move.getName());
    return key == "boomburst" || key == "bugbuzz" || key == "chatter"
        || key == "clangoroussoul" || key == "clangoroussoulblaze" || key == "confide"
        || key == "disarmingvoice" || key == "echoedvoice" || key == "fairywind"
        || key == "grasswhistle" || key == "growl" || key == "healbell"
        || key == "howl" || key == "hypervoice" || key == "metalvoice"
        || key == "nobleroar" || key == "overdrive" || key == "partingshot"
        || key == "perishsong" || key == "psychicnoise" || key == "relicsong"
        || key == "roar" || key == "round" || key == "screech"
        || key == "shadowpanis" || key == "sing" || key == "snarl"
        || key == "snore" || key == "sparklingaria" || key == "supersonic"
        || key == "torchsong" || key == "uproar" || key == "yawn"
        || key == "throatchop";
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

float abilityWeatherSpeedMultiplier(AbilityType abilityType, WeatherType weatherType) {
    switch (abilityType) {
        case AbilityType::SwiftSwim:
            return (weatherType == WeatherType::Rain) ? 2.0f : 1.0f;
        case AbilityType::Chlorophyll:
            return (weatherType == WeatherType::Sun) ? 2.0f : 1.0f;
        case AbilityType::SandRush:
            return (weatherType == WeatherType::Sandstorm) ? 2.0f : 1.0f;
        case AbilityType::SlushRush:
            return (weatherType == WeatherType::Hail || weatherType == WeatherType::Snow) ? 2.0f : 1.0f;
        case AbilityType::SurgeSurfer:
            // Electric Terrain is a field effect, not weather - speed is doubled independently
            return 2.0f;
        default:
            return 1.0f;
    }
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
    regPassive(AbilityType::SwiftSwim, [](auto& p) {});
    regPassive(AbilityType::Chlorophyll, [](auto& p) {});
    regPassive(AbilityType::SandRush, [](auto& p) {});
    regPassive(AbilityType::SlushRush, [](auto& p) {});
    regPassive(AbilityType::SurgeSurfer, [](auto& p) {});
    regPassive(AbilityType::Sturdy, [](auto& p) { p.sturdyEndure = true; });
    regPassive(AbilityType::Oblivious, [](auto& p) { p.preventsTaunt = true; p.preventsInfatuation = true; });
    regPassive(AbilityType::SereneGrace, [](auto& p) {});
    regPassive(AbilityType::IronFist, [](auto& p) {});
    regPassive(AbilityType::Reckless, [](auto& p) {});
    regPassive(AbilityType::StrongJaw, [](auto& p) {});
    regPassive(AbilityType::ToughClaws, [](auto& p) {});
    regPassive(AbilityType::BattleArmor, [](auto& p) { p.blocksCriticalHits = true; });
    regPassive(AbilityType::ShellArmor, [](auto& p) { p.blocksCriticalHits = true; });
    regPassive(AbilityType::MagicBounce, [](auto& p) { p.reflectsStatusMoves = true; });

    // Shed Skin: 30% chance to cure status at end of turn
    registry.registerAbilityBuilder(AbilityType::ShedSkin,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnTurnEnd] = [](Pokemon* self, Pokemon*, void*) {
                if (self && PRNG::nextInt(0, 100) < 30) {
                    self->removeStatus(StatusType::Burn);
                    self->removeStatus(StatusType::Freeze);
                    self->removeStatus(StatusType::Paralysis);
                    self->removeStatus(StatusType::Poison);
                    self->removeStatus(StatusType::ToxicPoison);
                    self->removeStatus(StatusType::Sleep);
                }
            };
        });

    // Speed Boost: +1 Speed at end of each turn
    registry.registerAbilityBuilder(AbilityType::SpeedBoost,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.effects[Trigger::OnTurnEnd] = [](Pokemon* self, Pokemon*, void*) {
                if (self) self->changeStatStage(StatIndex::Speed, 1);
            };
        });

    // Limber: immune to paralysis
    registry.registerAbilityBuilder(AbilityType::Limber,
        [](Ability&, AddTypeImmunity, AddStatusImmunity addS) {
            addS(StatusType::Paralysis);
        });

    // Own Tempo: immune to confusion
    registry.registerAbilityBuilder(AbilityType::OwnTempo,
        [](Ability&, AddTypeImmunity, AddStatusImmunity addS) {
            addS(StatusType::Confusion);
        });

    // Abilities that need runtime context (damage multipliers keyed on move/HP/state). (damage multipliers keyed on move/HP/state).
    // These abilities are self-describing via damageModifier + passive flags;
    // the remaining runtime checks live in Battle's calculateDamage helpers.
    regPassive(AbilityType::Sharpness, [](auto& p) {});
    regPassive(AbilityType::Stakeout, [](auto& p) {});
    regPassive(AbilityType::SupremeOverlord, [](auto& p) {});
    regPassive(AbilityType::Steelworker, [](auto& p) {});
    regPassive(AbilityType::Bulletproof, [](auto& p) {});

    // Pressure: opponent loses 1 extra PP when targeting this Pokemon
    regPassive(AbilityType::Pressure, [](auto& p) { p.drainsOpponentPP = true; });

    // Wonder Guard: only hit by super-effective moves (checked in damage calc)
    regPassive(AbilityType::WonderGuard, [](auto& p) { p.wonderGuard = true; });

    // Shadow Tag: prevents opponent from switching out
    registry.registerAbilityBuilder(AbilityType::ShadowTag,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.passive.trapsOpponent = true;
        });

    // Lightning Rod: redirects Electric moves, boosts SpAtk
    registry.registerAbilityBuilder(AbilityType::LightningRod,
        [](Ability& a, AddTypeImmunity addT, AddStatusImmunity) {
            a.passive.redirectsElectricMoves = true;
            addT(Type::Electric, false, 0);
        });

    // Soundproof: immune to sound-based moves
    registry.registerAbilityBuilder(AbilityType::Soundproof,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.passive.blocksSoundMoves = true;
        });

    // Trace: copies opponent's ability on entry
    registry.registerAbilityBuilder(AbilityType::Trace,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.passive.copiesOpponentAbility = true;
        });

    // Pure Power: doubles Attack stat
    regPassive(AbilityType::PurePower, [](auto& p) { p.doublesAttack = true; });

    // Compound Eyes: 30% accuracy boost
    regPassive(AbilityType::CompoundEyes, [](auto& p) {});

    // Rock Head: prevents recoil damage
    regPassive(AbilityType::RockHead, [](auto& p) { p.preventsRecoil = true; });

    // Shield Dust: blocks move secondary effects
    regPassive(AbilityType::ShieldDust, [](auto& p) { p.blocksMoveSecondaryEffects = true; });

    // Simple: doubles stat changes (passive, handled in stat change logic)
    regPassive(AbilityType::Simple, [](auto& p) {});

    // Synchronize: mirrors status conditions
    regPassive(AbilityType::Synchronize, [](auto& p) { p.mirrorsStatus = true; });

    // Magnet Pull: traps Steel-type opponents
    regPassive(AbilityType::MagnetPull, [](auto& p) { p.trapsSteelTypes = true; });

    // Arena Trap: traps grounded opponents
    regPassive(AbilityType::ArenaTrap, [](auto& p) { p.trapsGrounded = true; });

    // Rain Dish: heals 1/16 max HP in rain
    regPassive(AbilityType::RainDish, [](auto& p) { p.healsInRain = true; });

    // Sticky Hold: prevents item loss
    regPassive(AbilityType::StickyHold, [](auto& p) { p.preventsItemLoss = true; });

    // Damp: prevents SelfDestruct and Explosion
    regPassive(AbilityType::Damp, [](auto& p) { p.preventsExplosion = true; });

    // Early Bird: halves sleep duration
    regPassive(AbilityType::EarlyBird, [](auto& p) { p.halvesSleepTurns = true; });

    // Unburden: doubles Speed when item is consumed/lost
    regPassive(AbilityType::Unburden, [](auto& p) { p.speedDoubledWithoutItem = true; });

    // Anger Point: maxes Attack when hit by a critical hit
    regPassive(AbilityType::AngerPoint, [](auto& p) { p.attackMaxedOnCrit = true; });

    // Gluttony: consumes pinch berries at 1/2 HP instead of 1/4
    regPassive(AbilityType::Gluttony, [](auto& p) { p.earlyBerryConsumption = true; });

    // Effect Spore: 30% to inflict random status on contact
    registry.registerAbilityBuilder(AbilityType::EffectSpore,
        [](Ability& a, AddTypeImmunity, AddStatusImmunity) {
            a.passive.inflictsRandomContactStatus = true;
        });

    // Water Veil: prevents burn
    registry.registerAbilityBuilder(AbilityType::WaterVeil,
        [](Ability&, AddTypeImmunity, AddStatusImmunity addS) {
            addS(StatusType::Burn);
        });

    // Magma Armor: prevents freeze
    registry.registerAbilityBuilder(AbilityType::MagmaArmor,
        [](Ability&, AddTypeImmunity, AddStatusImmunity addS) {
            addS(StatusType::Freeze);
        });

    // Liquid Ooze: inverts draining moves (damage instead of heal)
    regPassive(AbilityType::LiquidOoze, [](auto& p) { p.invertsDrainingHeal = true; });

    // Sand Veil: evasion boosted in sandstorm
    regPassive(AbilityType::SandVeil, [](auto& p) { p.boostsEvasionInSand = true; });

    // Stench: 10% flinch chance on damaging moves
    regPassive(AbilityType::Stench, [](auto& p) { p.flinchOnHit = true; });

    // Cute Charm: 30% infatuation on contact
    regPassive(AbilityType::CuteCharm, [](auto& p) { p.infatuatesOnContact = true; });

    // Steadfast: +1 Speed when flinched
    regPassive(AbilityType::Steadfast, [](auto& p) { p.speedBoostWhenFlinched = true; });

    // Tangled Feet: evasion doubled when confused
    regPassive(AbilityType::TangledFeet, [](auto& p) { p.evasionDoubleWhenConfused = true; });

    // Rivalry: +/-25% damage based on gender
    regPassive(AbilityType::Rivalry, [](auto& p) { p.rivalryDamageModifier = true; });

    // Suction Cups: prevents forced switch
    regPassive(AbilityType::SuctionCups, [](auto& p) { p.preventsForcedSwitch = true; });

    // Color Change: changes type to match last hit
    regPassive(AbilityType::ColorChange, [](auto& p) { p.colorChangeOnHit = true; });

    // Heatproof: halves Fire damage
    regPassive(AbilityType::Heatproof, [](auto& p) { p.fireResistance = true; });

    // Air Lock: negates weather effects
    regPassive(AbilityType::AirLock, [](auto& p) { p.negatesWeather = true; });

    // Snow Cloak: evasion boosted in hail/snow
    regPassive(AbilityType::SnowCloak, [](auto& p) { p.evasionInSnow = true; });

    // Sniper: critical hits deal x2.25 instead of x1.5
    regPassive(AbilityType::Sniper, [](auto& p) { p.sniperCritBoost = true; });

    // No Guard: all moves always hit (both sides)
    regPassive(AbilityType::NoGuard, [](auto& p) { p.noGuardAlwaysHit = true; });

    // Skill Link: multi-hit moves always hit max times
    regPassive(AbilityType::SkillLink, [](auto& p) { p.skillLinkMaxHits = true; });

    // Hydration: heals status conditions in rain
    regPassive(AbilityType::Hydration, [](auto& p) { p.hydrationHealsStatus = true; });

    // Poison Heal: heals 1/8 max HP per turn when poisoned
    regPassive(AbilityType::PoisonHeal, [](auto& p) { p.poisonHealRecovery = true; });

    // Download: +1 Atk or SpAtk on entry based on foe's defenses
    regPassive(AbilityType::Download, [](auto& p) { p.downloadStatBoost = true; });

    // Normalize: all moves become Normal type
    regPassive(AbilityType::Normalize, [](auto& p) { p.normalizeAllNormal = true; });

    // Tinted Lens: not-very-effective moves deal x2 damage
    regPassive(AbilityType::TintedLens, [](auto& p) { p.tintedLensBoost = true; });

    // Klutz: held item has no effect
    regPassive(AbilityType::Klutz, [](auto& p) { p.klutzNoItem = true; });

    // Slow Start: Atk and Speed halved for first 5 turns
    regPassive(AbilityType::SlowStart, [](auto& p) { p.slowStartHalved = true; });

    // Swarm: Bug moves x1.5 at <= 1/3 HP
    regPassive(AbilityType::Swarm, [](auto& p) { p.swarmBugBoost = true; });

    // Dry Skin: healed by Water, damaged by Fire, hurt in sun
    regPassive(AbilityType::DrySkin, [](auto& p) { p.drySkinEffects = true; });

    // Solar Power: SpAtk x1.5 in sun, loses 1/8 HP per turn
    regPassive(AbilityType::SolarPower, [](auto& p) { p.solarPowerBoost = true; });

    // Quick Feet: Speed x1.5 when statused
    regPassive(AbilityType::QuickFeet, [](auto& p) { p.quickFeetSpeedBoost = true; });

    // Stall: always moves last
    regPassive(AbilityType::Stall, [](auto& p) { p.alwaysMovesLast = true; });

    // Leaf Guard: prevents status in sun
    regPassive(AbilityType::LeafGuard, [](auto& p) { p.leafGuardSun = true; });

    // Super Luck: +1 crit stage
    regPassive(AbilityType::SuperLuck, [](auto& p) { p.superLuckCrit = true; });

    // Anticipation: shudders if foe has super-effective or OHKO move
    regPassive(AbilityType::Anticipation, [](auto& p) { p.anticipationShudder = true; });

    // Forewarn: reveals highest-BP move on entry
    regPassive(AbilityType::Forewarn, [](auto& p) { p.forewarnReveal = true; });

    // Ice Body: heals 1/16 max HP in hail
    regPassive(AbilityType::IceBody, [](auto& p) { p.iceBodyHailHeal = true; });

    // Frisk: reveals opponent's held item on entry
    regPassive(AbilityType::Frisk, [](auto& p) { p.friskRevealsItem = true; });

    // Pickpocket: steals item on contact
    regPassive(AbilityType::Pickpocket, [](auto& p) { p.pickpocketStealsItem = true; });

    // Defeatist: Atk/SpAtk halved at <= 1/2 HP
    regPassive(AbilityType::Defeatist, [](auto& p) { p.defeatistDebuff = true; });

    // Cursed Body: 30% to disable move on hit
    regPassive(AbilityType::CursedBody, [](auto& p) { p.cursedBodyDisable = true; });

    // Weak Armor: -1 Def +2 Speed on physical hit
    regPassive(AbilityType::WeakArmor, [](auto& p) { p.weakArmorStatShift = true; });

    // Heavy Metal: weight doubled
    regPassive(AbilityType::HeavyMetal, [](auto& p) { p.heavyMetalWeightDouble = true; });

    // Light Metal: weight halved
    regPassive(AbilityType::LightMetal, [](auto& p) { p.lightMetalWeightHalf = true; });

    // Toxic Boost: Atk x1.5 when poisoned
    regPassive(AbilityType::ToxicBoost, [](auto& p) { p.toxicBoostAttack = true; });

    // Flare Boost: SpAtk x1.5 when burned
    regPassive(AbilityType::FlareBoost, [](auto& p) { p.flareBoostSpAttack = true; });

    // Harvest: 50% chance to recycle consumed berry in sun
    regPassive(AbilityType::Harvest, [](auto& p) { p.harvestRecyclesBerry = true; });

    // Overcoat: immune to powder and weather damage
    regPassive(AbilityType::Overcoat, [](auto& p) { p.overcoatPowderWeather = true; });

    // Poison Touch: 30% to poison on contact
    regPassive(AbilityType::PoisonTouch, [](auto& p) { p.poisonTouchContact = true; });

    // Big Pecks: prevents Defense drops
    regPassive(AbilityType::BigPecks, [](auto& p) { p.bigPecksPreventDefDrop = true; });

    // Wonder Skin: status moves have 50% accuracy
    regPassive(AbilityType::WonderSkin, [](auto& p) { p.wonderSkinReducedAccuracy = true; });

    // Analytic: x1.3 damage if moving last
    regPassive(AbilityType::Analytic, [](auto& p) { p.analyticMoveLastBoost = true; });

    // Illusion: disguised as last party member
    regPassive(AbilityType::Illusion, [](auto& p) { p.illusionDisguise = true; });

    // Justified: +1 Atk when hit by Dark move
    regPassive(AbilityType::Justified, [](auto& p) { p.justifiedDarkBoost = true; });

    // Rattled: +1 Speed when hit by Bug/Ghost/Dark
    regPassive(AbilityType::Rattled, [](auto& p) { p.rattledSpeedBoost = true; });

    // Sand Force: Rock/Ground/Steel x1.3 in sandstorm
    regPassive(AbilityType::SandForce, [](auto& p) { p.sandForceBoost = true; });

    // Victory Star: ally accuracy x1.1
    regPassive(AbilityType::VictoryStar, [](auto& p) { p.victoryStarAccuracy = true; });

    // Plus: SpAtk x1.5 if ally has Plus/Minus
    regPassive(AbilityType::Plus, [](auto& p) { p.plusMinusSpAtk = true; });

    // Minus: SpAtk x1.5 if ally has Plus/Minus
    regPassive(AbilityType::Minus, [](auto& p) { p.plusMinusSpAtk = true; });

    // Forecast: changes type with weather
    regPassive(AbilityType::Forecast, [](auto& p) { p.forecastWeather = true; });

    // Flower Gift: boosts ally Atk/SpDef in sun
    regPassive(AbilityType::FlowerGift, [](auto& p) { p.flowerGiftBoost = true; });

    // Bad Dreams: sleeping foes lose 1/8 HP per turn
    regPassive(AbilityType::BadDreams, [](auto& p) { p.badDreamsDamage = true; });

    // Moody: +2 one stat, -1 another at end of turn
    regPassive(AbilityType::Moody, [](auto& p) { p.moodyRandomBoost = true; });

    // Imposter: transforms into opponent on entry
    regPassive(AbilityType::Imposter, [](auto& p) { p.imposterTransform = true; });

    // Turboblaze: ignores opponent abilities
    regPassive(AbilityType::Turboblaze, [](auto& p) { p.moldBreakerLike = true; });

    // Teravolt: ignores opponent abilities
    regPassive(AbilityType::Teravolt, [](auto& p) { p.moldBreakerLike = true; });

    // Aroma Veil: protects from Taunt/Torment/Encore etc
    regPassive(AbilityType::AromaVeil, [](auto& p) { p.aromaVeilProtection = true; });

    // Fur Coat: doubles Defense
    regPassive(AbilityType::FurCoat, [](auto& p) { p.furCoatDefense = true; });

    // Magician: steals opponent's item on contact
    regPassive(AbilityType::Magician, [](auto& p) { p.magicianSteal = true; });

    // Refrigerate: Normal moves become Ice, x1.2
    regPassive(AbilityType::Refrigerate, [](auto& p) { p.refrigerateNormal = true; });

    // Sweet Veil: prevents sleep
    regPassive(AbilityType::SweetVeil, [](auto& p) { p.sweetVeilPreventSleep = true; });

    // Gale Wings: +1 priority to Flying moves at full HP
    regPassive(AbilityType::GaleWings, [](auto& p) { p.galeWingsPriority = true; });

    // Mega Launcher: pulse/aura moves x1.5
    regPassive(AbilityType::MegaLauncher, [](auto& p) { p.megaLauncherBoost = true; });

    // Pixilate: Normal moves become Fairy, x1.2
    regPassive(AbilityType::Pixilate, [](auto& p) { p.pixilateNormal = true; });

    // Gooey: -1 Speed on contact
    regPassive(AbilityType::Gooey, [](auto& p) { p.gooeySlow = true; });

    // Aerilate: Normal moves become Flying, x1.2
    regPassive(AbilityType::Aerilate, [](auto& p) { p.aerilateNormal = true; });

    // Parental Bond: damaging moves hit twice (second at 25%)
    regPassive(AbilityType::ParentalBond, [](auto& p) { p.parentalBond = true; });

    // Stamina: +1 Def when hit
    regPassive(AbilityType::Stamina, [](auto& p) { p.staminaDefBoost = true; });

    // Merciless: auto-crit on poisoned targets
    regPassive(AbilityType::Merciless, [](auto& p) { p.mercilessAutoCrit = true; });

    // Berserk: +1 SpAtk at half HP
    regPassive(AbilityType::Berserk, [](auto& p) { p.berserkSpAtkBoost = true; });

    // Long Reach: moves don't make contact
    regPassive(AbilityType::LongReach, [](auto& p) { p.longReachNoContact = true; });

    // Liquid Voice: sound moves become Water
    regPassive(AbilityType::LiquidVoice, [](auto& p) { p.liquidVoiceWater = true; });

    // Galvanize: Normal moves become Electric
    regPassive(AbilityType::Galvanize, [](auto& p) { p.galvanizeElectric = true; });

    // Queenly Majesty: blocks priority moves
    regPassive(AbilityType::QueenlyMajesty, [](auto& p) { p.queenlyMajestyPriority = true; });

    // Dancer: copies dance moves
    regPassive(AbilityType::Dancer, [](auto& p) { p.dancerDanceCopy = true; });

    // Battery: boosts ally SpAtk
    regPassive(AbilityType::Battery, [](auto& p) { p.batteryAllySpAtk = true; });

    // Dark Aura: Dark moves x1.33
    regPassive(AbilityType::DarkAura, [](auto& p) { p.darkAuraBoost = true; });

    // Fairy Aura: Fairy moves x1.33
    regPassive(AbilityType::FairyAura, [](auto& p) { p.fairyAuraBoost = true; });

    // Aura Break: inverts Dark/Fairy Aura
    regPassive(AbilityType::AuraBreak, [](auto& p) { p.auraBreakInvert = true; });

    // Primordial Sea: heavy rain, Fire immune
    regPassive(AbilityType::PrimordialSea, [](auto& p) { p.primordialSea = true; });

    // Desolate Land: harsh sun, Water immune
    regPassive(AbilityType::DesolateLand, [](auto& p) { p.desolateLand = true; });

    // Delta Stream: strong winds, Flying immune
    regPassive(AbilityType::DeltaStream, [](auto& p) { p.deltaStream = true; });

    // Healer: 30% to cure ally status
    regPassive(AbilityType::Healer, [](auto& p) { p.healerAllyStatus = true; });

    // Friend Guard: reduces ally damage
    regPassive(AbilityType::FriendGuard, [](auto& p) { p.friendGuardReduce = true; });

    // Telepathy: avoids ally moves
    regPassive(AbilityType::Telepathy, [](auto& p) { p.telepathyAvoidAlly = true; });

    // Grass Pelt: Def x1.5 in Grassy Terrain
    regPassive(AbilityType::GrassPelt, [](auto& p) { p.grassPeltDefense = true; });

    // Symbiosis: passes item to ally
    regPassive(AbilityType::Symbiosis, [](auto& p) { p.symbiosisPass = true; });
}

std::vector<Ability> getAbilitiesForPokemon(AbilityType type) {
    // For now, each Pokemon has one ability
    std::vector<Ability> abilities;
    abilities.push_back(getAbility(type));
    return abilities;
}