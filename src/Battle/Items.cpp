#include "Battle/Items.h"
#include "Battle/Pokemon.h"
#include "Battle/Battle.h"
#include <algorithm>
#include<iostream>
Item getItem(ItemType type) {
    Item item;
    switch (type) {
        case ItemType::OranBerry:
            item = Item(ItemType::OranBerry, "Oran Berry");
            item.isConsumable = true;
            item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
                if (!self) return;
                self->setCurrentHP(self->getCurrentHP() + 10);
            });
            break;

        case ItemType::Leftovers:
            item = Item(ItemType::Leftovers, "Leftovers");
            item.addEffect(ItemTrigger::OnTurnEnd, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
                if (!self) return;
                int heal = std::max(1, self->getMaxHP() / 16);
                self->setCurrentHP(self->getCurrentHP() + heal);
            });
            break;

        case ItemType::FocusSash:
            item = Item(ItemType::FocusSash, "Focus Sash");
            item.isConsumable = true;
            item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
                if (!self) return;
                if (self->getCurrentHP() <= 0) {
                    self->setCurrentHP(1);
                }
            });
            break;

        case ItemType::RockyHelmet:
            item = Item(ItemType::RockyHelmet, "Rocky Helmet");
            item.addEffect(ItemTrigger::OnAttack, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
                if (!self || !opponent) return;
                opponent->setCurrentHP(opponent->getCurrentHP() - 10);
            });
            break;

        case ItemType::AirBalloon:
            item = Item(ItemType::AirBalloon, "Air Balloon");
            item.isConsumable = true;
            item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
                if (!self) return;
                // Air Balloon may prevent the first Ground-based effect; placeholder implementation.
            });
            break;

        case ItemType::RedCard:
            item = Item(ItemType::RedCard, "Red Card");
            item.isConsumable = true;
            item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
                if (!self || !opponent) return;
                // 强制对手交换宝可梦
                Side* opponentSide = Battle::findSideForPokemon(battle, opponent);
                if (opponentSide) {
                    opponentSide->autoSwitchNext();
                }
            });
            break;

        case ItemType::ChoiceBand:
            item = Item(ItemType::ChoiceBand, "Choice Band");
            item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
                if (!self) return;
                // Choice Band would boost physical attack. Actual stat modification is handled elsewhere.
            });
            item.addStatModifier(ItemStatModifier::Stat::Attack, 1.5f);
            break;

        default:
            item = Item(ItemType::None, "None");
            break;
    }
    return item;
}

std::string getItemName(ItemType type) {
    switch (type) {
        case ItemType::OranBerry: return "Oran Berry";
        case ItemType::SitrusBerry: return "Sitrus Berry";
        case ItemType::LumBerry: return "Lum Berry";
        case ItemType::ChestoBerry: return "Chesto Berry";
        case ItemType::PechaBerry: return "Pecha Berry";
        case ItemType::RawstBerry: return "Rawst Berry";
        case ItemType::AspearBerry: return "Aspear Berry";
        case ItemType::PersimBerry: return "Persim Berry";
        case ItemType::Leftovers: return "Leftovers";
        case ItemType::BlackSludge: return "Black Sludge";
        case ItemType::ShellBell: return "Shell Bell";
        case ItemType::ChoiceBand: return "Choice Band";
        case ItemType::ChoiceSpecs: return "Choice Specs";
        case ItemType::ChoiceScarf: return "Choice Scarf";
        case ItemType::LifeOrb: return "Life Orb";
        case ItemType::ExpertBelt: return "Expert Belt";
        case ItemType::MuscleBand: return "Muscle Band";
        case ItemType::WiseGlasses: return "Wise Glasses";
        case ItemType::FocusSash: return "Focus Sash";
        case ItemType::RockyHelmet: return "Rocky Helmet";
        case ItemType::AirBalloon: return "Air Balloon";
        case ItemType::Eviolite: return "Eviolite";
        case ItemType::AssaultVest: return "Assault Vest";
        case ItemType::RedCard: return "Red Card";
        case ItemType::EjectButton: return "Eject Button";
        case ItemType::WeaknessPolicy: return "Weakness Policy";
        case ItemType::BerryJuice: return "Berry Juice";
        default: return "None";
    }
}

Item createOranBerry() {
    Item item(ItemType::OranBerry, "Oran Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
        self->setCurrentHP(self->getCurrentHP() + 10);
    });
    return item;
}

Item createSitrusBerry() {
    Item item(ItemType::SitrusBerry, "Sitrus Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
        int heal = self->getMaxHP() / 4;
        self->setCurrentHP(self->getCurrentHP() + heal);
    });
    return item;
}

Item createLumBerry() {
    Item item(ItemType::LumBerry, "Lum Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatus, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
        // 清除所有异常状态
    });
    return item;
}

Item createLeftovers() {
    Item item(ItemType::Leftovers, "Leftovers");
    item.addEffect(ItemTrigger::OnTurnEnd, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
        int heal = std::max(1, self->getMaxHP() / 16);
        self->setCurrentHP(self->getCurrentHP() + heal);
    });
    return item;
}

Item createChoiceBand() {
    Item item(ItemType::ChoiceBand, "Choice Band");
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
    });
    item.addStatModifier(ItemStatModifier::Stat::Attack, 1.5f);
    return item;
}

Item createChoiceSpecs() {
    Item item(ItemType::ChoiceSpecs, "Choice Specs");
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
    });
    item.addStatModifier(ItemStatModifier::Stat::SpAttack, 1.5f);
    return item;
}

Item createChoiceScarf() {
    Item item(ItemType::ChoiceScarf, "Choice Scarf");
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
    });
    item.addStatModifier(ItemStatModifier::Stat::Speed, 1.5f);
    return item;
}

Item createLifeOrb() {
    Item item(ItemType::LifeOrb, "Life Orb");
    item.addEffect(ItemTrigger::OnDealDamage, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
        int recoil = self->getMaxHP() / 10;
        self->setCurrentHP(self->getCurrentHP() - recoil);
    });
    item.setDamageModifier(1.3f, true);
    return item;
}

Item createFocusSash() {
    Item item(ItemType::FocusSash, "Focus Sash");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
        if (self->getCurrentHP() <= 0) {
            self->setCurrentHP(1);
        }
    });
    return item;
}

Item createRockyHelmet() {
    Item item(ItemType::RockyHelmet, "Rocky Helmet");
    item.addEffect(ItemTrigger::OnAttack, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self || !opponent) return;
        opponent->setCurrentHP(opponent->getCurrentHP() - 10);
    });
    return item;
}

Item createAirBalloon() {
    Item item(ItemType::AirBalloon, "Air Balloon");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
        // Air Balloon may prevent the first Ground-based effect; placeholder implementation.
    });
    return item;
}

Item createEviolite() {
    Item item(ItemType::Eviolite, "Eviolite");
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
    });
    item.addStatModifier(ItemStatModifier::Stat::Defense, 1.5f);
    item.addStatModifier(ItemStatModifier::Stat::SpDefense, 1.5f);
    return item;
}

Item createAssaultVest() {
    Item item(ItemType::AssaultVest, "Assault Vest");
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
    });
    item.addStatModifier(ItemStatModifier::Stat::SpDefense, 1.5f);
    return item;
}

Item createRedCard() {
    Item item(ItemType::RedCard, "Red Card");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
                if (!self || !opponent) return;
                // 强制对手交换宝可梦
                Side* opponentSide = Battle::findSideForPokemon(battle, opponent);
                if (opponentSide) {
                    opponentSide->autoSwitchNext();
                }
            });
    return item;
}

Item createWeaknessPolicy() {
    Item item(ItemType::WeaknessPolicy, "Weakness Policy");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [&item](Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
        if (!self) return;
        // 当受到克制伤害时，攻击和特攻提升2级
        item.addStatChange(ItemStatModifier::Stat::Attack, 2);
        item.addStatChange(ItemStatModifier::Stat::SpAttack, 2);
    });
    return item;
}

bool isBerry(ItemType type) {
    switch (type) {
        case ItemType::OranBerry:
        case ItemType::SitrusBerry:
        case ItemType::LumBerry:
        case ItemType::ChestoBerry:
        case ItemType::PechaBerry:
        case ItemType::RawstBerry:
        case ItemType::AspearBerry:
        case ItemType::PersimBerry:
            return true;
        default:
            return false;
    }
}

void Item::executeTrigger(ItemTrigger trigger, Pokemon* self, Pokemon* opponent, Battle& battle, void* context) {
    if (isUsed && isConsumable) return;  // 已使用的一次性物品不再触发
    
    auto it = effects.find(trigger);
    if (it != effects.end() && it->second) {
        it->second(self, opponent, battle, context);
        
        // 如果是消耗品，标记为已使用
        if (isConsumable && trigger != ItemTrigger::OnEntry) {
            isUsed = true;
            // 可选：从宝可梦身上移除物品
            if (self) {
                self->removeItem();
            }
        }
    }
}
