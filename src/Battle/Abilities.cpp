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
                    opponent->changeStatStage(StatIndex::Attack, -1);
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