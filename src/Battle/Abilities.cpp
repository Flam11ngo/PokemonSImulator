#include "Abilities.h"
#include "Pokemon.h" // For Pokemon class
#include <cctype>

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
        case AbilityType::Insomnia: return "Insomnia";
        case AbilityType::VitalSpirit: return "Vital Spirit";
        case AbilityType::Guts: return "Guts";
        case AbilityType::HugePower: return "Huge Power";
        case AbilityType::ThickFat: return "Thick Fat";
        case AbilityType::MarvelScale: return "Marvel Scale";
        case AbilityType::SapSipper: return "Sap Sipper";
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
    if (key == "insomnia") return AbilityType::Insomnia;
    if (key == "vitalspirit") return AbilityType::VitalSpirit;
    if (key == "guts") return AbilityType::Guts;
    if (key == "hugepower") return AbilityType::HugePower;
    if (key == "thickfat") return AbilityType::ThickFat;
    if (key == "marvelscale") return AbilityType::MarvelScale;
    if (key == "sapsipper") return AbilityType::SapSipper;
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
    return AbilityType::None;
}

Ability getAbility(AbilityType type) {
    AbilityData abilityData = getAbilityData(type);
    if (abilityData.type == AbilityType::None && type != AbilityType::None) {
        abilityData.type = type;
    }
    if (abilityData.name.empty() || abilityData.name == "None") {
        abilityData.name = getAbilityName(type);
    }

    Ability ability(abilityData);
    ability.damageModifier = {1.0f, true};

    switch (type) {
        case AbilityType::Intimidate:
            ability.effects[Trigger::OnEntry] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!opponent || !self) return;
                if (!opponent->isFainted()) {
                    const AbilityType opponentAbility = opponent->getAbility();
                    if (opponentAbility == AbilityType::ClearBody || opponentAbility == AbilityType::WhiteSmoke || opponentAbility == AbilityType::HyperCutter) {
                        return;
                    }
                    if (opponentAbility == AbilityType::MirrorArmor) {
                        self->changeStatStage(StatIndex::Attack, -1);
                        return;
                    }
                    const int beforeAttack = opponent->getStatStage(StatIndex::Attack);
                    opponent->changeStatStage(StatIndex::Attack, -1);
                    const int afterAttack = opponent->getStatStage(StatIndex::Attack);
                    if (afterAttack < beforeAttack) {
                        if (opponentAbility == AbilityType::Defiant) {
                            opponent->changeStatStage(StatIndex::Attack, 2);
                        } else if (opponentAbility == AbilityType::Competitive) {
                            opponent->changeStatStage(StatIndex::SpecialAttack, 2);
                        }
                    }
                }
            };
            break;
        case AbilityType::Overgrow:
            ability.effects[Trigger::OnDamage] = [](Pokemon*, Pokemon*, void*) {};
            break;
        case AbilityType::Blaze:
            ability.effects[Trigger::OnDamage] = [](Pokemon*, Pokemon*, void*) {};
            break;
        case AbilityType::Torrent:
            ability.effects[Trigger::OnDamage] = [](Pokemon*, Pokemon*, void*) {};
            break;
        case AbilityType::Multiscale:
            ability.effects[Trigger::OnDamage] = [](Pokemon*, Pokemon*, void*) {};
            break;
        case AbilityType::Levitate:
            break;
        case AbilityType::WaterAbsorb:
            break;
        case AbilityType::VoltAbsorb:
            break;
        case AbilityType::FlashFire:
            break;
        case AbilityType::Static:
            ability.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void*) {
                if (!self || !opponent) return;
                opponent->addStatus(StatusType::Paralysis);
            };
            break;
        case AbilityType::Insomnia:
        case AbilityType::VitalSpirit:
        case AbilityType::Immunity:
            break;
        case AbilityType::Guts:
        case AbilityType::HugePower:
        case AbilityType::ThickFat:
        case AbilityType::MarvelScale:
        case AbilityType::SapSipper:
        case AbilityType::StormDrain:
        case AbilityType::MotorDrive:
        case AbilityType::Technician:
        case AbilityType::Filter:
        case AbilityType::SolidRock:
        case AbilityType::Moxie:
        case AbilityType::InnerFocus:
        case AbilityType::Regenerator:
        case AbilityType::NaturalCure:
        case AbilityType::MagicGuard:
        case AbilityType::Unaware:
        case AbilityType::Prankster:
        case AbilityType::ClearBody:
        case AbilityType::Defiant:
        case AbilityType::Competitive:
        case AbilityType::WhiteSmoke:
        case AbilityType::MirrorArmor:
        case AbilityType::HyperCutter:
        case AbilityType::KeenEye:
        case AbilityType::Drizzle:
        case AbilityType::Drought:
        case AbilityType::SandStream:
        case AbilityType::SnowWarning:
        case AbilityType::CloudNine:
        case AbilityType::GrassySurge:
        case AbilityType::ElectricSurge:
        case AbilityType::PsychicSurge:
        case AbilityType::MistySurge:
        case AbilityType::HadronEngine:
        case AbilityType::Protean:
        case AbilityType::Libero:
        case AbilityType::Adaptability:
        case AbilityType::SheerForce:
        case AbilityType::Infiltrator:
        case AbilityType::BeadsOfRuin:
        case AbilityType::SwordOfRuin:
        case AbilityType::TabletsOfRuin:
        case AbilityType::VesselOfRuin:
        case AbilityType::Unnerve:
            break;
        default:
            break;
    }
    return ability;
}

std::vector<Ability> getAbilitiesForPokemon(AbilityType type) {
    // For now, each Pokemon has one ability
    std::vector<Ability> abilities;
    abilities.push_back(getAbility(type));
    return abilities;
}