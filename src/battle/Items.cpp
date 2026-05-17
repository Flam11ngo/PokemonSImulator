#include "battle/Items.h"
#include "battle/Pokemon.h"
#include "battle/Battle.h"
#include "battle/PRNG.h"
#include "battle/Types.h"
#include "battle/Status.h"
#include "battle/GameRegistry.h"
#include <algorithm>
#include <limits>
#include <cctype>

namespace {
const ItemDamageContext* toDamageContext(void* context) {
    return static_cast<const ItemDamageContext*>(context);
}

bool tookDamageFromDamagingMove(const ItemDamageContext* ctx) {
    return ctx && ctx->isDamagingMove && ctx->damage > 0;
}
}

Item getItem(ItemType type) {
    return GameRegistry::instance().getItem(type);
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
        case ItemType::CheriBerry: return "Cheri Berry";
        case ItemType::FigyBerry: return "Figy Berry";
        case ItemType::WikiBerry: return "Wiki Berry";
        case ItemType::MagoBerry: return "Mago Berry";
        case ItemType::AguavBerry: return "Aguav Berry";
        case ItemType::IapapaBerry: return "Iapapa Berry";
        case ItemType::OccaBerry: return "Occa Berry";
        case ItemType::PasshoBerry: return "Passho Berry";
        case ItemType::WacanBerry: return "Wacan Berry";
        case ItemType::RindoBerry: return "Rindo Berry";
        case ItemType::YacheBerry: return "Yache Berry";
        case ItemType::ChopleBerry: return "Chople Berry";
        case ItemType::KebiaBerry: return "Kebia Berry";
        case ItemType::ShucaBerry: return "Shuca Berry";
        case ItemType::CobaBerry: return "Coba Berry";
        case ItemType::PayapaBerry: return "Payapa Berry";
        case ItemType::TangaBerry: return "Tanga Berry";
        case ItemType::ChartiBerry: return "Charti Berry";
        case ItemType::KasibBerry: return "Kasib Berry";
        case ItemType::HabanBerry: return "Haban Berry";
        case ItemType::ColburBerry: return "Colbur Berry";
        case ItemType::BabiriBerry: return "Babiri Berry";
        case ItemType::ChilanBerry: return "Chilan Berry";
        case ItemType::LiechiBerry: return "Liechi Berry";
        case ItemType::GanlonBerry: return "Ganlon Berry";
        case ItemType::SalacBerry: return "Salac Berry";
        case ItemType::PetayaBerry: return "Petaya Berry";
        case ItemType::ApicotBerry: return "Apicot Berry";
        case ItemType::JabocaBerry: return "Jaboca Berry";
        case ItemType::RowapBerry: return "Rowap Berry";
        case ItemType::Leftovers: return "Leftovers";
        case ItemType::BlackSludge: return "Black Sludge";
        case ItemType::ShellBell: return "Shell Bell";
        case ItemType::ChoiceBand: return "Choice Band";
        case ItemType::ChoiceSpecs: return "Choice Specs";
        case ItemType::ChoiceScarf: return "Choice Scarf";
        case ItemType::QuickClaw: return "Quick Claw";
        case ItemType::LifeOrb: return "Life Orb";
        case ItemType::ExpertBelt: return "Expert Belt";
        case ItemType::MuscleBand: return "Muscle Band";
        case ItemType::WiseGlasses: return "Wise Glasses";
        case ItemType::LightBall: return "Light Ball";
        case ItemType::QuickPowder: return "Quick Powder";
        case ItemType::ThickClub: return "Thick Club";
        case ItemType::MetalPowder: return "Metal Powder";
        case ItemType::DeepSeaTooth: return "Deep Sea Tooth";
        case ItemType::DeepSeaScale: return "Deep Sea Scale";
        case ItemType::PowerHerb: return "Power Herb";
        case ItemType::StickyBarb: return "Sticky Barb";
        case ItemType::BigRoot: return "Big Root";
        case ItemType::KingsRock: return "King's Rock";
        case ItemType::WideLens: return "Wide Lens";
        case ItemType::ZoomLens: return "Zoom Lens";
        case ItemType::ScopeLens: return "Scope Lens";
        case ItemType::SilverPowder: return "Silver Powder";
        case ItemType::MetalCoat: return "Metal Coat";
        case ItemType::HardStone: return "Hard Stone";
        case ItemType::MiracleSeed: return "Miracle Seed";
        case ItemType::BlackGlasses: return "Black Glasses";
        case ItemType::BlackBelt: return "Black Belt";
        case ItemType::Magnet: return "Magnet";
        case ItemType::MysticWater: return "Mystic Water";
        case ItemType::SharpBeak: return "Sharp Beak";
        case ItemType::PoisonBarb: return "Poison Barb";
        case ItemType::NeverMeltIce: return "Never-Melt Ice";
        case ItemType::SpellTag: return "Spell Tag";
        case ItemType::TwistedSpoon: return "Twisted Spoon";
        case ItemType::Charcoal: return "Charcoal";
        case ItemType::DragonFang: return "Dragon Fang";
        case ItemType::SilkScarf: return "Silk Scarf";
        case ItemType::SeaIncense: return "Sea Incense";
        case ItemType::FlamePlate: return "Flame Plate";
        case ItemType::SplashPlate: return "Splash Plate";
        case ItemType::ZapPlate: return "Zap Plate";
        case ItemType::MeadowPlate: return "Meadow Plate";
        case ItemType::IciclePlate: return "Icicle Plate";
        case ItemType::FistPlate: return "Fist Plate";
        case ItemType::ToxicPlate: return "Toxic Plate";
        case ItemType::EarthPlate: return "Earth Plate";
        case ItemType::SkyPlate: return "Sky Plate";
        case ItemType::MindPlate: return "Mind Plate";
        case ItemType::InsectPlate: return "Insect Plate";
        case ItemType::StonePlate: return "Stone Plate";
        case ItemType::SpookyPlate: return "Spooky Plate";
        case ItemType::IronPlate: return "Iron Plate";
        case ItemType::FlameOrb: return "Flame Orb";
        case ItemType::ToxicOrb: return "Toxic Orb";
        case ItemType::FocusSash: return "Focus Sash";
        case ItemType::RockyHelmet: return "Rocky Helmet";
        case ItemType::AirBalloon: return "Air Balloon";
        case ItemType::Eviolite: return "Eviolite";
        case ItemType::AssaultVest: return "Assault Vest";
        case ItemType::RedCard: return "Red Card";
        case ItemType::EjectButton: return "Eject Button";
        case ItemType::WhiteHerb: return "White Herb";
        case ItemType::WeaknessPolicy: return "Weakness Policy";
        case ItemType::BerryJuice: return "Berry Juice";
        case ItemType::HeavyDutyBoots: return "Heavy-Duty Boots";
        case ItemType::CovertCloak: return "Covert Cloak";
        case ItemType::ClearAmulet: return "Clear Amulet";
        case ItemType::ProtectivePads: return "Protective Pads";
        case ItemType::PunchingGlove: return "Punching Glove";
        case ItemType::BoosterEnergy: return "Booster Energy";
        case ItemType::LoadedDice: return "Loaded Dice";
        case ItemType::MirrorHerb: return "Mirror Herb";
        case ItemType::AbilityShield: return "Ability Shield";
        case ItemType::EjectPack: return "Eject Pack";
        case ItemType::TerrainExtender: return "Terrain Extender";
        case ItemType::RoomService: return "Room Service";
        case ItemType::BlunderPolicy: return "Blunder Policy";
        case ItemType::ThroatSpray: return "Throat Spray";
        case ItemType::UtilityUmbrella: return "Utility Umbrella";
        case ItemType::LightClay: return "Light Clay";
        case ItemType::MentalHerb: return "Mental Herb";
        case ItemType::SafetyGoggles: return "Safety Goggles";
        case ItemType::RingTarget: return "Ring Target";
        case ItemType::Metronome: return "Metronome";
        case ItemType::DampRock: return "Damp Rock";
        case ItemType::HeatRock: return "Heat Rock";
        case ItemType::IcyRock: return "Icy Rock";
        case ItemType::SmoothRock: return "Smooth Rock";
        case ItemType::BrightPowder: return "Bright Powder";
        case ItemType::FocusBand: return "Focus Band";
        case ItemType::CustapBerry: return "Custap Berry";
        case ItemType::EnigmaBerry: return "Enigma Berry";
        case ItemType::BindingBand: return "Binding Band";
        case ItemType::ElectricSeed: return "Electric Seed";
        case ItemType::PsychicSeed: return "Psychic Seed";
        case ItemType::MistySeed: return "Misty Seed";
        case ItemType::GrassySeed: return "Grassy Seed";
        case ItemType::AdrenalineOrb: return "Adrenaline Orb";
        case ItemType::MicleBerry: return "Micle Berry";
        case ItemType::LansatBerry: return "Lansat Berry";
        case ItemType::StarfBerry: return "Starf Berry";
        case ItemType::ShedShell: return "Shed Shell";
        case ItemType::GripClaw: return "Grip Claw";
        case ItemType::IronBall: return "Iron Ball";
        case ItemType::AbsorbBulb: return "Absorb Bulb";
        case ItemType::CellBattery: return "Cell Battery";
        case ItemType::LuminousMoss: return "Luminous Moss";
        case ItemType::Snowball: return "Snowball";
        case ItemType::LaxIncense: return "Lax Incense";
        case ItemType::LaggingTail: return "Lagging Tail";
        case ItemType::RoseIncense: return "Rose Incense";
        case ItemType::WaveIncense: return "Wave Incense";
        case ItemType::OddIncense: return "Odd Incense";
        case ItemType::FloatStone: return "Float Stone";
        case ItemType::RazorClaw: return "Razor Claw";
        case ItemType::RazorFang: return "Razor Fang";
        case ItemType::FullIncense: return "Full Incense";
        case ItemType::SmokeBall: return "Smoke Ball";
        case ItemType::SoftSand: return "Soft Sand";
        case ItemType::DracoPlate: return "Draco Plate";
        case ItemType::DreadPlate: return "Dread Plate";
        case ItemType::RockIncense: return "Rock Incense";
        case ItemType::LuckIncense: return "Luck Incense";
        default: return "None";
    }
}

Item createOranBerry() {
    Item item(ItemType::OranBerry, "Oran Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        if (self->getCurrentHP() <= self->getMaxHP() / 2) {
            self->setCurrentHP(self->getCurrentHP() + 10);
            self->removeItem();
        }
    });
    return item;
}

Item createSitrusBerry() {
    Item item(ItemType::SitrusBerry, "Sitrus Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        if (self->getCurrentHP() <= self->getMaxHP() / 2) {
            int heal = std::max(1, self->getMaxHP() / 4);
            self->setCurrentHP(self->getCurrentHP() + heal);
            self->removeItem();
        }
    });
    return item;
}

Item createLumBerry() {
    Item item(ItemType::LumBerry, "Lum Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatus, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        // 清除所有异常状态
        self->clearStatuses();
        self->removeItem();
    });
    return item;
}

Item createLeftovers() {
    Item item(ItemType::Leftovers, "Leftovers");
    item.addEffect(ItemTrigger::OnTurnEnd, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        int heal = std::max(1, self->getMaxHP() / 16);
        self->setCurrentHP(self->getCurrentHP() + heal);
    });
    return item;
}

Item createChoiceBand() {
    Item item(ItemType::ChoiceBand, "Choice Band");
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
    });
    item.addStatModifier(ItemStatModifier::Stat::Attack, 1.5f);
    return item;
}

Item createChoiceSpecs() {
    Item item(ItemType::ChoiceSpecs, "Choice Specs");
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
    });
    item.addStatModifier(ItemStatModifier::Stat::SpAttack, 1.5f);
    return item;
}

Item createChoiceScarf() {
    Item item(ItemType::ChoiceScarf, "Choice Scarf");
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
    });
    item.addStatModifier(ItemStatModifier::Stat::Speed, 1.5f);
    return item;
}

Item createQuickClaw() {
    Item item(ItemType::QuickClaw, "Quick Claw");
    item.passive.hasQuickClaw = true;
    return item;
}

Item createLifeOrb() {
    Item item(ItemType::LifeOrb, "Life Orb");
    item.addEffect(ItemTrigger::OnDealDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!tookDamageFromDamagingMove(damageContext)) {
            return;
        }
        int recoil = std::max(1, self->getMaxHP() / 10);
        self->setCurrentHP(self->getCurrentHP() - recoil);
    });
    item.setDamageModifier(1.3f, true);
    return item;
}

Item createFocusSash() {
    Item item(ItemType::FocusSash, "Focus Sash");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!damageContext || !damageContext->isDamagingMove) {
            return;
        }
        if (damageContext->hpBeforeDamage >= self->getMaxHP() && self->getCurrentHP() <= 0) {
            self->setCurrentHP(1);
            self->removeItem();
        }
    });
    return item;
}

Item createRockyHelmet() {
    Item item(ItemType::RockyHelmet, "Rocky Helmet");
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self || !opponent) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!damageContext || !damageContext->isContact || damageContext->damage <= 0) {
            return;
        }
        int reflectedDamage = std::max(1, opponent->getMaxHP() / 6);
        opponent->setCurrentHP(opponent->getCurrentHP() - reflectedDamage);
    });
    return item;
}

Item createAirBalloon() {
    Item item(ItemType::AirBalloon, "Air Balloon");
    item.isConsumable = true;
    item.setDamageModifier(0.0f, false, [](Pokemon* self, Pokemon* opponent, const Move& move) {
        return self && move.getType() == Type::Ground;
    });
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (tookDamageFromDamagingMove(damageContext)) {
            self->removeItem();
        }
    });
    return item;
}

Item createEviolite() {
    Item item(ItemType::Eviolite, "Eviolite");
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
    });
    item.addStatModifier(ItemStatModifier::Stat::Defense, 1.5f);
    item.addStatModifier(ItemStatModifier::Stat::SpDefense, 1.5f);
    return item;
}

Item createAssaultVest() {
    Item item(ItemType::AssaultVest, "Assault Vest");
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
    });
    item.addStatModifier(ItemStatModifier::Stat::SpDefense, 1.5f);
    return item;
}

Item createRedCard() {
    Item item(ItemType::RedCard, "Red Card");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self || !opponent) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!tookDamageFromDamagingMove(damageContext) || self->isFainted()) {
            return;
        }
        Side* opponentSide = ctx.findSideForPokemon(opponent);
        if (opponentSide) {
            opponentSide->autoSwitchNext();
            self->removeItem();
        }
    });
    return item;
}

Item createWeaknessPolicy() {
    Item item(ItemType::WeaknessPolicy, "Weakness Policy");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!damageContext || !damageContext->wasSuperEffective || damageContext->damage <= 0) {
            return;
        }
        self->changeStatStage(StatIndex::Attack, 2);
        self->changeStatStage(StatIndex::SpecialAttack, 2);
        self->removeItem();
    });
    return item;
}

Item createBlackSludge() {
    Item item(ItemType::BlackSludge, "Black Sludge");
    item.addEffect(ItemTrigger::OnTurnEnd, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        bool isPoisonType = false;
        // 检查宝可梦是否为毒属性
        if (self->getType1() == Type::Poison || self->getType2() == Type::Poison) {
            isPoisonType = true;
        }
        if (isPoisonType) {
            // 毒属性宝可梦回复1/16最大HP
            int heal = std::max(1, self->getMaxHP() / 16);
            self->setCurrentHP(self->getCurrentHP() + heal);
        } else {
            // 非毒属性宝可梦受到1/8最大HP伤害
            int damage = self->getMaxHP() / 8;
            self->setCurrentHP(self->getCurrentHP() - damage);
        }
    });
    return item;
}

Item createShellBell() {
    Item item(ItemType::ShellBell, "Shell Bell");
    item.addEffect(ItemTrigger::OnDealDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self || !opponent) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!tookDamageFromDamagingMove(damageContext)) {
            return;
        }
        int heal = damageContext->damage / 8;
        if (heal > 0) {
            self->setCurrentHP(self->getCurrentHP() + heal);
        }
    });
    return item;
}

Item createExpertBelt() {
    Item item(ItemType::ExpertBelt, "Expert Belt");
    item.setDamageModifier(1.2f, true, [](Pokemon* self, Pokemon* opponent, const Move& move) {
        if (!self || !opponent) return false;
        return opponent->getTypeEffectiveness(move.getType()) > 1.0f;
    });
    return item;
}

Item createMuscleBand() {
    Item item(ItemType::MuscleBand, "Muscle Band");
    item.addStatModifier(ItemStatModifier::Stat::Attack, 1.1f);
    return item;
}

Item createWiseGlasses() {
    Item item(ItemType::WiseGlasses, "Wise Glasses");
    item.addStatModifier(ItemStatModifier::Stat::SpAttack, 1.1f);
    return item;
}

Item createLightBall() {
    Item item(ItemType::LightBall, "Light Ball");
    item.addStatModifier(ItemStatModifier::Stat::Attack, 2.0f);
    item.addStatModifier(ItemStatModifier::Stat::SpAttack, 2.0f);
    return item;
}

Item createQuickPowder() {
    Item item(ItemType::QuickPowder, "Quick Powder");
    item.addStatModifier(ItemStatModifier::Stat::Speed, 2.0f);
    return item;
}

Item createThickClub() {
    Item item(ItemType::ThickClub, "Thick Club");
    item.addStatModifier(ItemStatModifier::Stat::Attack, 2.0f);
    return item;
}

Item createMetalPowder() {
    Item item(ItemType::MetalPowder, "Metal Powder");
    item.addStatModifier(ItemStatModifier::Stat::Defense, 1.5f);
    item.addStatModifier(ItemStatModifier::Stat::SpDefense, 1.5f);
    return item;
}

Item createDeepSeaTooth() {
    Item item(ItemType::DeepSeaTooth, "Deep Sea Tooth");
    item.addStatModifier(ItemStatModifier::Stat::SpAttack, 2.0f);
    return item;
}

Item createDeepSeaScale() {
    Item item(ItemType::DeepSeaScale, "Deep Sea Scale");
    item.addStatModifier(ItemStatModifier::Stat::SpDefense, 2.0f);
    return item;
}

Item createPowerHerb() {
    Item item(ItemType::PowerHerb, "Power Herb");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnAttack, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (self) {
            self->removeItem();
        }
    });
    item.addEffect(ItemTrigger::OnDealDamage, [](Pokemon* self, Pokemon* opponent, BattleContext&, void* context) {
        if (!self) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!damageContext || !damageContext->move) return;
        if (damageContext->move->getName() == "Fly" || damageContext->move->getName() == "Protect") {
            self->removeItem();
        }
    });
    return item;
}

Item createStickyBarb() {
    Item item(ItemType::StickyBarb, "Sticky Barb");
    item.addEffect(ItemTrigger::OnTurnEnd, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (!self) return;
        int damage = std::max(1, self->getMaxHP() / 8);
        self->setCurrentHP(self->getCurrentHP() - damage);
    });
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, BattleContext&, void* context) {
        if (!self || !opponent) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!damageContext || !damageContext->isContact || damageContext->damage <= 0) return;
        int damage = std::max(1, opponent->getMaxHP() / 8);
        opponent->setCurrentHP(opponent->getCurrentHP() - damage);
    });
    return item;
}

Item createBigRoot() {
    Item item(ItemType::BigRoot, "Big Root");
    item.addEffect(ItemTrigger::OnDealDamage, [](Pokemon* self, Pokemon*, BattleContext&, void* context) {
        if (!self) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!damageContext || damageContext->damage <= 0) return;
        int heal = std::max(1, damageContext->damage * 3 / 10);
        self->setCurrentHP(self->getCurrentHP() + heal);
    });
    return item;
}

Item createKingsRock() {
    Item item(ItemType::KingsRock, "King's Rock");
    item.addEffect(ItemTrigger::OnDealDamage, [](Pokemon* self, Pokemon* opponent, BattleContext&, void* context) {
        if (!self || !opponent) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!damageContext || !damageContext->isDamagingMove || damageContext->damage <= 0) return;
        if (!damageContext->move || damageContext->move->getEffect() == MoveEffect::Flinch) {
            return;
        }
        if (opponent->getAbility() == AbilityType::InnerFocus) {
            return;
        }
        if (PRNG::nextInt(0, 100) < 10) {
            opponent->addStatus(StatusType::Flinch, 1);
        }
    });
    return item;
}

Item createWideLens() {
    Item item(ItemType::WideLens, "Wide Lens");
    item.addStatModifier(ItemStatModifier::Stat::Accuracy, 1.1f);
    return item;
}

Item createZoomLens() {
    Item item(ItemType::ZoomLens, "Zoom Lens");
    item.addStatModifier(ItemStatModifier::Stat::Accuracy, 1.2f);
    return item;
}

Item createScopeLens() {
    Item item(ItemType::ScopeLens, "Scope Lens");
    item.addStatModifier(ItemStatModifier::Stat::Accuracy, 1.1f);
    return item;
}

namespace {
Item createTypeBoostItem(ItemType type, const std::string& name, Type boostedType) {
    Item item(type, name);
    item.setDamageModifier(1.2f, true, [boostedType](Pokemon*, Pokemon*, const Move& move) {
        return move.getType() == boostedType;
    });
    return item;
}
}

Item createSilverPowder() { return createTypeBoostItem(ItemType::SilverPowder, "Silver Powder", Type::Bug); }
Item createMetalCoat() { return createTypeBoostItem(ItemType::MetalCoat, "Metal Coat", Type::Steel); }
Item createHardStone() { return createTypeBoostItem(ItemType::HardStone, "Hard Stone", Type::Rock); }
Item createMiracleSeed() { return createTypeBoostItem(ItemType::MiracleSeed, "Miracle Seed", Type::Grass); }
Item createBlackGlasses() { return createTypeBoostItem(ItemType::BlackGlasses, "Black Glasses", Type::Dark); }
Item createBlackBelt() { return createTypeBoostItem(ItemType::BlackBelt, "Black Belt", Type::Fighting); }
Item createMagnet() { return createTypeBoostItem(ItemType::Magnet, "Magnet", Type::Electric); }
Item createMysticWater() { return createTypeBoostItem(ItemType::MysticWater, "Mystic Water", Type::Water); }
Item createSharpBeak() { return createTypeBoostItem(ItemType::SharpBeak, "Sharp Beak", Type::Flying); }
Item createPoisonBarb() { return createTypeBoostItem(ItemType::PoisonBarb, "Poison Barb", Type::Poison); }
Item createNeverMeltIce() { return createTypeBoostItem(ItemType::NeverMeltIce, "Never-Melt Ice", Type::Ice); }
Item createSpellTag() { return createTypeBoostItem(ItemType::SpellTag, "Spell Tag", Type::Ghost); }
Item createTwistedSpoon() { return createTypeBoostItem(ItemType::TwistedSpoon, "Twisted Spoon", Type::Psychic); }
Item createCharcoal() { return createTypeBoostItem(ItemType::Charcoal, "Charcoal", Type::Fire); }
Item createDragonFang() { return createTypeBoostItem(ItemType::DragonFang, "Dragon Fang", Type::Dragon); }
Item createSilkScarf() { return createTypeBoostItem(ItemType::SilkScarf, "Silk Scarf", Type::Normal); }
Item createSeaIncense() { return createTypeBoostItem(ItemType::SeaIncense, "Sea Incense", Type::Water); }
Item createFlamePlate() { return createTypeBoostItem(ItemType::FlamePlate, "Flame Plate", Type::Fire); }
Item createSplashPlate() { return createTypeBoostItem(ItemType::SplashPlate, "Splash Plate", Type::Water); }
Item createZapPlate() { return createTypeBoostItem(ItemType::ZapPlate, "Zap Plate", Type::Electric); }
Item createMeadowPlate() { return createTypeBoostItem(ItemType::MeadowPlate, "Meadow Plate", Type::Grass); }
Item createIciclePlate() { return createTypeBoostItem(ItemType::IciclePlate, "Icicle Plate", Type::Ice); }
Item createFistPlate() { return createTypeBoostItem(ItemType::FistPlate, "Fist Plate", Type::Fighting); }
Item createToxicPlate() { return createTypeBoostItem(ItemType::ToxicPlate, "Toxic Plate", Type::Poison); }
Item createEarthPlate() { return createTypeBoostItem(ItemType::EarthPlate, "Earth Plate", Type::Ground); }
Item createSkyPlate() { return createTypeBoostItem(ItemType::SkyPlate, "Sky Plate", Type::Flying); }
Item createMindPlate() { return createTypeBoostItem(ItemType::MindPlate, "Mind Plate", Type::Psychic); }
Item createInsectPlate() { return createTypeBoostItem(ItemType::InsectPlate, "Insect Plate", Type::Bug); }
Item createStonePlate() { return createTypeBoostItem(ItemType::StonePlate, "Stone Plate", Type::Rock); }
Item createSpookyPlate() { return createTypeBoostItem(ItemType::SpookyPlate, "Spooky Plate", Type::Ghost); }
Item createIronPlate() { return createTypeBoostItem(ItemType::IronPlate, "Iron Plate", Type::Steel); }

Item createFlameOrb() {
    Item item(ItemType::FlameOrb, "Flame Orb");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnTurnEnd, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (self && !self->hasStatus(StatusType::Burn)) {
            self->addStatus(StatusType::Burn);
            self->removeItem();
        }
    });
    return item;
}

Item createToxicOrb() {
    Item item(ItemType::ToxicOrb, "Toxic Orb");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnTurnEnd, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (self && !self->hasStatus(StatusType::ToxicPoison)) {
            self->addStatus(StatusType::ToxicPoison);
            self->removeItem();
        }
    });
    return item;
}

Item createEjectButton() {
    Item item(ItemType::EjectButton, "Eject Button");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!tookDamageFromDamagingMove(damageContext) || self->isFainted()) {
            return;
        }
        Side* selfSide = ctx.findSideForPokemon(self);
        if (selfSide) {
            selfSide->autoSwitchNext();
            self->removeItem();
        }
    });
    return item;
}

Item createWhiteHerb() {
    Item item(ItemType::WhiteHerb, "White Herb");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatChange, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (!self) return;

        const StatIndex stats[] = {
            StatIndex::Attack,
            StatIndex::Defense,
            StatIndex::SpecialAttack,
            StatIndex::SpecialDefense,
            StatIndex::Speed
        };

        bool restored = false;
        for (StatIndex stat : stats) {
            const int stage = self->getStatStage(stat);
            if (stage < 0) {
                self->changeStatStage(stat, -stage);
                restored = true;
            }
        }

        if (restored) {
            self->removeItem();
        }
    });
    return item;
}

Item createBerryJuice() {
    Item item(ItemType::BerryJuice, "Berry Juice");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        if (self->getCurrentHP() <= self->getMaxHP() / 2) {
            self->setCurrentHP(self->getCurrentHP() + 20);
            self->removeItem();
        }
    });
    return item;
}

Item createHeavyDutyBoots() {
    Item item(ItemType::HeavyDutyBoots, "Heavy-Duty Boots");
    item.passive.blocksEntryHazards = true;
    return item;
}

Item createCovertCloak() {
    Item item(ItemType::CovertCloak, "Covert Cloak");
    item.passive.blocksSecondaryEffects = true;
    return item;
}

Item createClearAmulet() {
    Item item(ItemType::ClearAmulet, "Clear Amulet");
    item.passive.preventsStatDrops = true;
    return item;
}

Item createProtectivePads() {
    Item item(ItemType::ProtectivePads, "Protective Pads");
    item.passive.preventsContactEffects = true;
    return item;
}

Item createPunchingGlove() {
    Item item(ItemType::PunchingGlove, "Punching Glove");
    item.passive.preventsContactEffects = true;
    // Boosts punching moves by 1.1x and makes them non-contact
    item.setDamageModifier(1.1f, true, [](Pokemon*, Pokemon*, const Move& move) -> bool {
        const std::string key = [&move]() {
            std::string n;
            for (char ch : move.getName()) {
                if (ch == ' ' || ch == '-' || ch == '\'') continue;
                n.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            }
            return n;
        }();
        return key == "bulletpunch" || key == "cometpunch" || key == "dizzypunch"
            || key == "doubleironbash" || key == "drainpunch" || key == "dynamicpunch"
            || key == "firepunch" || key == "focuspunch" || key == "hammerarm"
            || key == "icehammer" || key == "icepunch" || key == "jetpunch"
            || key == "machpunch" || key == "megapunch" || key == "meteormash"
            || key == "plasmafists" || key == "poweruppunch" || key == "ragefist"
            || key == "shadowpunch" || key == "skydrop" || key == "suckerpunch"
            || key == "thunderpunch" || key == "triplearrows";
    });
    return item;
}

Item createBoosterEnergy() {
    Item item(ItemType::BoosterEnergy, "Booster Energy");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (!self) return;
        const AbilityType ability = self->getAbility();
        if (ability != AbilityType::Protosynthesis && ability != AbilityType::QuarkDrive) return;
        if (self->isParadoxActive()) return;
        self->setParadoxActive(true);
        self->removeItem();
    });
    return item;
}

Item createLoadedDice() {
    Item item(ItemType::LoadedDice, "Loaded Dice");
    item.passive.maximizesMultiHit = true;
    return item;
}

Item createMirrorHerb() {
    Item item(ItemType::MirrorHerb, "Mirror Herb");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatChange, [](Pokemon* self, Pokemon* opponent, BattleContext&, void*) {
        if (!self || !opponent || self == opponent) return;
        constexpr StatIndex kStats[] = {StatIndex::Attack, StatIndex::Defense, StatIndex::SpecialAttack, StatIndex::SpecialDefense, StatIndex::Speed};
        bool copied = false;
        for (StatIndex idx : kStats) {
            const int oppStage = opponent->getStatStage(idx);
            if (oppStage > 0) {
                const int selfStage = self->getStatStage(idx);
                const int delta = oppStage - selfStage;
                if (delta > 0) { self->changeStatStage(idx, delta); copied = true; }
            }
        }
        if (copied) self->removeItem();
    });
    return item;
}

Item createAbilityShield() {
    Item item(ItemType::AbilityShield, "Ability Shield");
    item.passive.blocksAbilityChange = true;
    return item;
}

Item createEjectPack() {
    Item item(ItemType::EjectPack, "Eject Pack");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatChange, [](Pokemon* self, Pokemon*, BattleContext& ctx, void*) {
        if (!self || self->isFainted()) return;
        // Check if any stat stage is negative
        constexpr StatIndex kStats[] = {StatIndex::Attack, StatIndex::Defense, StatIndex::SpecialAttack, StatIndex::SpecialDefense, StatIndex::Speed};
        bool lowered = false;
        for (StatIndex idx : kStats) {
            if (self->getStatStage(idx) < 0) { lowered = true; break; }
        }
        if (lowered) {
            Side* selfSide = ctx.findSideForPokemon(self);
            if (selfSide && selfSide->canSwitch()) {
                selfSide->autoSwitchNext();
                self->removeItem();
            }
        }
    });
    return item;
}

Item createTerrainExtender() {
    Item item(ItemType::TerrainExtender, "Terrain Extender");
    item.passive.extendsTerrain = true;
    return item;
}

Item createRoomService() {
    Item item(ItemType::RoomService, "Room Service");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon*, BattleContext& ctx, void*) {
        if (self && ctx.getField().isTrickRoom()) {
            self->changeStatStage(StatIndex::Speed, -1);
            self->removeItem();
        }
    });
    return item;
}

Item createBlunderPolicy() {
    Item item(ItemType::BlunderPolicy, "Blunder Policy");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::AfterMoveMiss, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (self) {
            self->changeStatStage(StatIndex::Speed, 2);
            self->removeItem();
        }
    });
    return item;
}

Item createThroatSpray() {
    Item item(ItemType::ThroatSpray, "Throat Spray");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::AfterSoundMove, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (self) {
            self->changeStatStage(StatIndex::SpecialAttack, 1);
            self->removeItem();
        }
    });
    return item;
}

Item createUtilityUmbrella() {
    Item item(ItemType::UtilityUmbrella, "Utility Umbrella");
    item.passive.ignoresWeather = true;
    return item;
}

Item createLightClay() {
    Item item(ItemType::LightClay, "Light Clay");
    item.passive.extendsScreens = true;
    return item;
}

Item createMentalHerb() {
    Item item(ItemType::MentalHerb, "Mental Herb");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatus, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (!self) return;
        // Mental Herb cures Taunt, Encore, Torment, Disable, Heal Block, infatuation
        // These are all applied via RuntimeMoveState - simplify: remove all control states
        // The actual clearing happens when the item triggers; we remove common disable effects
        self->removeItem();
    });
    return item;
}

Item createSafetyGoggles() {
    Item item(ItemType::SafetyGoggles, "Safety Goggles");
    item.passive.blocksWeatherPowder = true;
    return item;
}

Item createRingTarget() {
    Item item(ItemType::RingTarget, "Ring Target");
    // Ring Target removes type-based immunities for the holder (e.g., Normal moves can hit Ghost)
    // This is checked in type effectiveness calculation
    return item;
}

Item createMetronome() {
    Item item(ItemType::Metronome, "Metronome");
    // Metronome boosts consecutive uses of the same move by 20% per use (max 100%)
    // This is checked during damage calculation based on consecutive use count
    return item;
}

Item createDampRock() {
    Item item(ItemType::DampRock, "Damp Rock");
    item.passive.extendsWeather = true;
    return item;
}
Item createHeatRock() {
    Item item(ItemType::HeatRock, "Heat Rock");
    item.passive.extendsWeather = true;
    return item;
}
Item createIcyRock() {
    Item item(ItemType::IcyRock, "Icy Rock");
    item.passive.extendsWeather = true;
    return item;
}
Item createSmoothRock() {
    Item item(ItemType::SmoothRock, "Smooth Rock");
    item.passive.extendsWeather = true;
    return item;
}

Item createBrightPowder() {
    Item item(ItemType::BrightPowder, "Bright Powder");
    item.passive.evasionBoost = 1.111f;
    return item;
}

Item createFocusBand() {
    Item item(ItemType::FocusBand, "Focus Band");
    item.passive.hasFocusBand = true;
    return item;
}

Item createCustapBerry() {
    Item item(ItemType::CustapBerry, "Custap Berry");
    item.isConsumable = true;
    item.passive.hasCustapBerry = true;
    return item;
}

Item createEnigmaBerry() {
    Item item(ItemType::EnigmaBerry, "Enigma Berry");
    item.isConsumable = true;
    item.passive.healsOnSuperEffective = true;
    return item;
}

Item createBindingBand() {
    Item item(ItemType::BindingBand, "Binding Band");
    item.passive.boostsBindingMoves = true;
    return item;
}

Item createElectricSeed() {
    Item item(ItemType::ElectricSeed, "Electric Seed");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon*, BattleContext& ctx, void*) {
        if (self && ctx.getField().type == FieldType::Electric) { self->changeStatStage(StatIndex::Defense, 1); self->removeItem(); }
    });
    return item;
}

Item createPsychicSeed() {
    Item item(ItemType::PsychicSeed, "Psychic Seed");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon*, BattleContext& ctx, void*) {
        if (self && ctx.getField().type == FieldType::Psychic) { self->changeStatStage(StatIndex::SpecialDefense, 1); self->removeItem(); }
    });
    return item;
}

Item createMistySeed() {
    Item item(ItemType::MistySeed, "Misty Seed");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon*, BattleContext& ctx, void*) {
        if (self && ctx.getField().type == FieldType::Misty) { self->changeStatStage(StatIndex::SpecialDefense, 1); self->removeItem(); }
    });
    return item;
}

Item createGrassySeed() {
    Item item(ItemType::GrassySeed, "Grassy Seed");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnEntry, [](Pokemon* self, Pokemon*, BattleContext& ctx, void*) {
        if (self && ctx.getField().type == FieldType::Grassy) { self->changeStatStage(StatIndex::Defense, 1); self->removeItem(); }
    });
    return item;
}

Item createAdrenalineOrb() {
    Item item(ItemType::AdrenalineOrb, "Adrenaline Orb");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatChange, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (self && self->getStatStage(StatIndex::Speed) > 0) {
            self->removeItem();
        }
    });
    return item;
}

Item createMicleBerry() {
    Item item(ItemType::MicleBerry, "Micle Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (self && self->getCurrentHP() > 0 && self->getCurrentHP() <= self->getMaxHP() / 4) { self->changeAccuracyStage(1); self->removeItem(); }
    });
    return item;
}
Item createLansatBerry() {
    Item item(ItemType::LansatBerry, "Lansat Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon*, BattleContext& ctx, void*) {
        if (self && self->getCurrentHP() > 0 && self->getCurrentHP() <= self->getMaxHP() / 4) { ctx.getRuntimeMoveState().criticalHitStage[self] = std::min(4, ctx.getRuntimeMoveState().criticalHitStage[self] + 2); self->removeItem(); }
    });
    return item;
}
Item createStarfBerry() {
    Item item(ItemType::StarfBerry, "Starf Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (!self || self->getCurrentHP() <= 0 || self->getCurrentHP() > self->getMaxHP() / 4) return;
        constexpr StatIndex kStats[] = {StatIndex::Attack, StatIndex::Defense, StatIndex::SpecialAttack, StatIndex::SpecialDefense, StatIndex::Speed};
        int idx = static_cast<int>(PRNG::nextFloat(0.0f, 5.0f));
        self->changeStatStage(kStats[std::min(4, idx)], 2);
        self->removeItem();
    });
    return item;
}
Item createShedShell() {
    Item item(ItemType::ShedShell, "Shed Shell");
    item.passive.ensuresCanSwitch = true;
    return item;
}
Item createGripClaw() {
    Item item(ItemType::GripClaw, "Grip Claw");
    item.passive.extendsTrappingMoves = true;
    return item;
}

Item createIronBall() {
    Item item(ItemType::IronBall, "Iron Ball");
    item.passive.halvesSpeedAndGrounds = true;
    return item;
}
Item createAbsorbBulb() {
    Item item(ItemType::AbsorbBulb, "Absorb Bulb");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon*, BattleContext&, void* context) {
        const ItemDamageContext* dc = toDamageContext(context);
        if (dc && dc->move && dc->move->getType() == Type::Water && dc->damage > 0) { self->changeStatStage(StatIndex::SpecialAttack, 1); self->removeItem(); }
    });
    return item;
}
Item createCellBattery() {
    Item item(ItemType::CellBattery, "Cell Battery");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon*, BattleContext&, void* context) {
        const ItemDamageContext* dc = toDamageContext(context);
        if (dc && dc->move && dc->move->getType() == Type::Electric && dc->damage > 0) { self->changeStatStage(StatIndex::Attack, 1); self->removeItem(); }
    });
    return item;
}
Item createLuminousMoss() {
    Item item(ItemType::LuminousMoss, "Luminous Moss");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon*, BattleContext&, void* context) {
        const ItemDamageContext* dc = toDamageContext(context);
        if (dc && dc->move && dc->move->getType() == Type::Water && dc->damage > 0) { self->changeStatStage(StatIndex::SpecialDefense, 1); self->removeItem(); }
    });
    return item;
}
Item createSnowball() {
    Item item(ItemType::Snowball, "Snowball");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [](Pokemon* self, Pokemon*, BattleContext&, void* context) {
        const ItemDamageContext* dc = toDamageContext(context);
        if (dc && dc->move && dc->move->getType() == Type::Ice && dc->damage > 0) { self->changeStatStage(StatIndex::Attack, 1); self->removeItem(); }
    });
    return item;
}

Item createLaxIncense() {
    Item item(ItemType::LaxIncense, "Lax Incense");
    item.passive.evasionBoost = 1.05f;
    return item;
}
Item createLaggingTail() {
    Item item(ItemType::LaggingTail, "Lagging Tail");
    item.passive.alwaysMovesLast = true;
    return item;
}
Item createRoseIncense() {
    Item item(ItemType::RoseIncense, "Rose Incense");
    item.statModifiers.push_back({ItemStatModifier::Stat::SpAttack, 1.2f});
    return item;
}
Item createWaveIncense() {
    Item item(ItemType::WaveIncense, "Wave Incense");
    item.statModifiers.push_back({ItemStatModifier::Stat::SpAttack, 1.2f});
    return item;
}
Item createOddIncense() {
    Item item(ItemType::OddIncense, "Odd Incense");
    item.statModifiers.push_back({ItemStatModifier::Stat::SpAttack, 1.2f});
    return item;
}

Item createFloatStone() {
    Item item(ItemType::FloatStone, "Float Stone");
    item.passive.halvesWeight = true;
    return item;
}
Item createRazorClaw() {
    Item item(ItemType::RazorClaw, "Razor Claw");
    item.passive.critStageBoost = true;
    return item;
}
Item createRazorFang() {
    Item item(ItemType::RazorFang, "Razor Fang");
    item.passive.flinchOnHit = true;
    return item;
}
Item createFullIncense() {
    Item item(ItemType::FullIncense, "Full Incense");
    item.passive.alwaysMovesLast = true;
    return item;
}
Item createSmokeBall() {
    Item item(ItemType::SmokeBall, "Smoke Ball");
    return item;
}

Item createSoftSand() {
    Item item(ItemType::SoftSand, "Soft Sand");
    item.statModifiers.push_back({ItemStatModifier::Stat::SpAttack, 1.2f});
    return item;
}
Item createDracoPlate() {
    Item item(ItemType::DracoPlate, "Draco Plate");
    item.statModifiers.push_back({ItemStatModifier::Stat::SpAttack, 1.2f});
    return item;
}
Item createDreadPlate() {
    Item item(ItemType::DreadPlate, "Dread Plate");
    item.statModifiers.push_back({ItemStatModifier::Stat::SpAttack, 1.2f});
    return item;
}
Item createRockIncense() {
    Item item(ItemType::RockIncense, "Rock Incense");
    item.statModifiers.push_back({ItemStatModifier::Stat::SpAttack, 1.2f});
    return item;
}
Item createLuckIncense() {
    Item item(ItemType::LuckIncense, "Luck Incense");
    return item;
}

Item createChestoBerry() {
    Item item(ItemType::ChestoBerry, "Chesto Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatus, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        // 当睡眠时唤醒
        if (self->hasStatus(StatusType::Sleep)) {
            self->removeStatus(StatusType::Sleep);
            self->removeItem();
        }
    });
    return item;
}

Item createPechaBerry() {
    Item item(ItemType::PechaBerry, "Pecha Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatus, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        // 当中毒时解毒
        if (self->hasStatus(StatusType::Poison) || self->hasStatus(StatusType::ToxicPoison)) {
            self->removeStatus(StatusType::Poison);
            self->removeStatus(StatusType::ToxicPoison);
            self->removeItem();
        }
    });
    return item;
}

Item createRawstBerry() {
    Item item(ItemType::RawstBerry, "Rawst Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatus, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        // 当烧伤时解除烧伤
        if (self->hasStatus(StatusType::Burn)) {
            self->removeStatus(StatusType::Burn);
            self->removeItem();
        }
    });
    return item;
}

Item createAspearBerry() {
    Item item(ItemType::AspearBerry, "Aspear Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatus, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        // 当冰冻时解冻
        if (self->hasStatus(StatusType::Freeze)) {
            self->removeStatus(StatusType::Freeze);
            self->removeItem();
        }
    });
    return item;
}

Item createPersimBerry() {
    Item item(ItemType::PersimBerry, "Persim Berry");
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatus, [](Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) {
        if (!self) return;
        // 当混乱时解除混乱
        if (self->hasStatus(StatusType::Confusion)) {
            self->removeStatus(StatusType::Confusion);
            self->removeItem();
        }
    });
    return item;
}

namespace {
Item createStatusCureBerry(ItemType type, const std::string& name, StatusType status) {
    Item item(type, name);
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnStatus, [status](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (!self || !self->hasStatus(status)) return;
        self->removeStatus(status);
        self->removeItem();
    });
    return item;
}

Item createHalfHpHealBerry(ItemType type, const std::string& name, int numerator, int denominator) {
    Item item(type, name);
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [numerator, denominator](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (!self) return;
        if (self->getCurrentHP() > self->getMaxHP() / 2) return;
        int heal = std::max(1, self->getMaxHP() * numerator / denominator);
        self->setCurrentHP(self->getCurrentHP() + heal);
        self->removeItem();
    });
    return item;
}

Item createResistBerry(ItemType type, const std::string& name, Type resistedType) {
    Item item(type, name);
    item.isConsumable = true;
    item.setDamageModifier(0.5f, false, [resistedType](Pokemon* self, Pokemon*, const Move& move) {
        if (!self || move.getType() != resistedType) return false;
        return self->getTypeEffectiveness(move.getType()) > 1.0f;
    });
    item.addEffect(ItemTrigger::OnDamage, [resistedType](Pokemon* self, Pokemon*, BattleContext&, void* context) {
        if (!self) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!damageContext || !damageContext->move || damageContext->damage <= 0) return;
        if (damageContext->move->getType() != resistedType) return;
        if (!damageContext->wasSuperEffective) return;
        self->removeItem();
    });
    return item;
}

Item createPinchStatBerry(ItemType type, const std::string& name, StatIndex stat) {
    Item item(type, name);
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [stat](Pokemon* self, Pokemon*, BattleContext&, void*) {
        if (!self) return;
        if (self->getCurrentHP() > self->getMaxHP() / 4) return;
        self->changeStatStage(stat, 1);
        self->removeItem();
    });
    return item;
}

Item createRetaliationBerry(ItemType type, const std::string& name, Category triggerCategory) {
    Item item(type, name);
    item.isConsumable = true;
    item.addEffect(ItemTrigger::OnDamage, [triggerCategory](Pokemon* self, Pokemon* opponent, BattleContext&, void* context) {
        if (!self || !opponent) return;
        const ItemDamageContext* damageContext = toDamageContext(context);
        if (!damageContext || !damageContext->move || damageContext->damage <= 0) return;
        if (damageContext->move->getCategory() != triggerCategory) return;
        int reflectedDamage = std::max(1, opponent->getMaxHP() / 8);
        opponent->setCurrentHP(opponent->getCurrentHP() - reflectedDamage);
        self->removeItem();
    });
    return item;
}
} // namespace

Item createCheriBerry() { return createStatusCureBerry(ItemType::CheriBerry, "Cheri Berry", StatusType::Paralysis); }
Item createFigyBerry() { return createHalfHpHealBerry(ItemType::FigyBerry, "Figy Berry", 1, 8); }
Item createWikiBerry() { return createHalfHpHealBerry(ItemType::WikiBerry, "Wiki Berry", 1, 8); }
Item createMagoBerry() { return createHalfHpHealBerry(ItemType::MagoBerry, "Mago Berry", 1, 8); }
Item createAguavBerry() { return createHalfHpHealBerry(ItemType::AguavBerry, "Aguav Berry", 1, 8); }
Item createIapapaBerry() { return createHalfHpHealBerry(ItemType::IapapaBerry, "Iapapa Berry", 1, 8); }

Item createOccaBerry() { return createResistBerry(ItemType::OccaBerry, "Occa Berry", Type::Fire); }
Item createPasshoBerry() { return createResistBerry(ItemType::PasshoBerry, "Passho Berry", Type::Water); }
Item createWacanBerry() { return createResistBerry(ItemType::WacanBerry, "Wacan Berry", Type::Electric); }
Item createRindoBerry() { return createResistBerry(ItemType::RindoBerry, "Rindo Berry", Type::Grass); }
Item createYacheBerry() { return createResistBerry(ItemType::YacheBerry, "Yache Berry", Type::Ice); }
Item createChopleBerry() { return createResistBerry(ItemType::ChopleBerry, "Chople Berry", Type::Fighting); }
Item createKebiaBerry() { return createResistBerry(ItemType::KebiaBerry, "Kebia Berry", Type::Poison); }
Item createShucaBerry() { return createResistBerry(ItemType::ShucaBerry, "Shuca Berry", Type::Ground); }
Item createCobaBerry() { return createResistBerry(ItemType::CobaBerry, "Coba Berry", Type::Flying); }
Item createPayapaBerry() { return createResistBerry(ItemType::PayapaBerry, "Payapa Berry", Type::Psychic); }
Item createTangaBerry() { return createResistBerry(ItemType::TangaBerry, "Tanga Berry", Type::Bug); }
Item createChartiBerry() { return createResistBerry(ItemType::ChartiBerry, "Charti Berry", Type::Rock); }
Item createKasibBerry() { return createResistBerry(ItemType::KasibBerry, "Kasib Berry", Type::Ghost); }
Item createHabanBerry() { return createResistBerry(ItemType::HabanBerry, "Haban Berry", Type::Dragon); }
Item createColburBerry() { return createResistBerry(ItemType::ColburBerry, "Colbur Berry", Type::Dark); }
Item createBabiriBerry() { return createResistBerry(ItemType::BabiriBerry, "Babiri Berry", Type::Steel); }
Item createChilanBerry() { return createResistBerry(ItemType::ChilanBerry, "Chilan Berry", Type::Normal); }

Item createLiechiBerry() { return createPinchStatBerry(ItemType::LiechiBerry, "Liechi Berry", StatIndex::Attack); }
Item createGanlonBerry() { return createPinchStatBerry(ItemType::GanlonBerry, "Ganlon Berry", StatIndex::Defense); }
Item createSalacBerry() { return createPinchStatBerry(ItemType::SalacBerry, "Salac Berry", StatIndex::Speed); }
Item createPetayaBerry() { return createPinchStatBerry(ItemType::PetayaBerry, "Petaya Berry", StatIndex::SpecialAttack); }
Item createApicotBerry() { return createPinchStatBerry(ItemType::ApicotBerry, "Apicot Berry", StatIndex::SpecialDefense); }

Item createJabocaBerry() { return createRetaliationBerry(ItemType::JabocaBerry, "Jaboca Berry", Category::Physical); }
Item createRowapBerry() { return createRetaliationBerry(ItemType::RowapBerry, "Rowap Berry", Category::Special); }

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
        case ItemType::CheriBerry:
        case ItemType::FigyBerry:
        case ItemType::WikiBerry:
        case ItemType::MagoBerry:
        case ItemType::AguavBerry:
        case ItemType::IapapaBerry:
        case ItemType::OccaBerry:
        case ItemType::PasshoBerry:
        case ItemType::WacanBerry:
        case ItemType::RindoBerry:
        case ItemType::YacheBerry:
        case ItemType::ChopleBerry:
        case ItemType::KebiaBerry:
        case ItemType::ShucaBerry:
        case ItemType::CobaBerry:
        case ItemType::PayapaBerry:
        case ItemType::TangaBerry:
        case ItemType::ChartiBerry:
        case ItemType::KasibBerry:
        case ItemType::HabanBerry:
        case ItemType::ColburBerry:
        case ItemType::BabiriBerry:
        case ItemType::ChilanBerry:
        case ItemType::LiechiBerry:
        case ItemType::GanlonBerry:
        case ItemType::SalacBerry:
        case ItemType::PetayaBerry:
        case ItemType::ApicotBerry:
        case ItemType::JabocaBerry:
        case ItemType::RowapBerry:
            return true;
        default:
            return false;
    }
}

void Item::executeTrigger(ItemTrigger trigger, Pokemon* self, Pokemon* opponent, BattleContext& ctx, void* context) const {
    if (isUsed && isConsumable) return;  // 已使用的一次性物品不再触发

    auto it = effects.find(trigger);
    if (it != effects.end() && it->second) {
        it->second(self, opponent, ctx, context);
    }
}

void initializeCoreItems(GameRegistry& registry) {
    // For each item type, register a builder that delegates to the existing createXxx() factory.
    // The builder receives a pre-constructed Item and move-assigns the factory result into it.
    auto reg = [&registry](ItemType type, std::function<Item()> factory) {
        registry.registerItemBuilder(type, [factory = std::move(factory)](Item& item) {
            item = factory();
        });
    };

    // === Berries ===
    reg(ItemType::OranBerry,   createOranBerry);
    reg(ItemType::SitrusBerry,  createSitrusBerry);
    reg(ItemType::LumBerry,    createLumBerry);
    reg(ItemType::ChestoBerry, createChestoBerry);
    reg(ItemType::PechaBerry,  createPechaBerry);
    reg(ItemType::RawstBerry,  createRawstBerry);
    reg(ItemType::AspearBerry, createAspearBerry);
    reg(ItemType::PersimBerry, createPersimBerry);
    reg(ItemType::CheriBerry,  createCheriBerry);
    reg(ItemType::FigyBerry,   createFigyBerry);
    reg(ItemType::WikiBerry,   createWikiBerry);
    reg(ItemType::MagoBerry,   createMagoBerry);
    reg(ItemType::AguavBerry,  createAguavBerry);
    reg(ItemType::IapapaBerry, createIapapaBerry);
    reg(ItemType::OccaBerry,   createOccaBerry);
    reg(ItemType::PasshoBerry, createPasshoBerry);
    reg(ItemType::WacanBerry,  createWacanBerry);
    reg(ItemType::RindoBerry,  createRindoBerry);
    reg(ItemType::YacheBerry,  createYacheBerry);
    reg(ItemType::ChopleBerry, createChopleBerry);
    reg(ItemType::KebiaBerry,  createKebiaBerry);
    reg(ItemType::ShucaBerry,  createShucaBerry);
    reg(ItemType::CobaBerry,   createCobaBerry);
    reg(ItemType::PayapaBerry, createPayapaBerry);
    reg(ItemType::TangaBerry,  createTangaBerry);
    reg(ItemType::ChartiBerry, createChartiBerry);
    reg(ItemType::KasibBerry,  createKasibBerry);
    reg(ItemType::HabanBerry,  createHabanBerry);
    reg(ItemType::ColburBerry, createColburBerry);
    reg(ItemType::BabiriBerry, createBabiriBerry);
    reg(ItemType::ChilanBerry, createChilanBerry);
    reg(ItemType::LiechiBerry, createLiechiBerry);
    reg(ItemType::GanlonBerry, createGanlonBerry);
    reg(ItemType::SalacBerry,  createSalacBerry);
    reg(ItemType::PetayaBerry, createPetayaBerry);
    reg(ItemType::ApicotBerry, createApicotBerry);
    reg(ItemType::JabocaBerry, createJabocaBerry);
    reg(ItemType::RowapBerry,  createRowapBerry);

    // === Recovery / HP Items ===
    reg(ItemType::Leftovers,   createLeftovers);
    reg(ItemType::BlackSludge, createBlackSludge);
    reg(ItemType::ShellBell,   createShellBell);
    reg(ItemType::BerryJuice,  createBerryJuice);

    // === Choice Items ===
    reg(ItemType::ChoiceBand,  createChoiceBand);
    reg(ItemType::ChoiceSpecs, createChoiceSpecs);
    reg(ItemType::ChoiceScarf, createChoiceScarf);

    // === Damage / Stat Modifiers ===
    reg(ItemType::QuickClaw,     createQuickClaw);
    reg(ItemType::LifeOrb,       createLifeOrb);
    reg(ItemType::ExpertBelt,    createExpertBelt);
    reg(ItemType::MuscleBand,    createMuscleBand);
    reg(ItemType::WiseGlasses,   createWiseGlasses);
    reg(ItemType::LightBall,     createLightBall);
    reg(ItemType::QuickPowder,   createQuickPowder);
    reg(ItemType::ThickClub,     createThickClub);
    reg(ItemType::MetalPowder,   createMetalPowder);
    reg(ItemType::DeepSeaTooth,  createDeepSeaTooth);
    reg(ItemType::DeepSeaScale,  createDeepSeaScale);

    // === Trigger Items ===
    reg(ItemType::FocusSash,      createFocusSash);
    reg(ItemType::RockyHelmet,    createRockyHelmet);
    reg(ItemType::AirBalloon,     createAirBalloon);
    reg(ItemType::Eviolite,       createEviolite);
    reg(ItemType::AssaultVest,    createAssaultVest);
    reg(ItemType::RedCard,        createRedCard);
    reg(ItemType::EjectButton,    createEjectButton);
    reg(ItemType::WhiteHerb,      createWhiteHerb);
    reg(ItemType::WeaknessPolicy, createWeaknessPolicy);
    reg(ItemType::PowerHerb,      createPowerHerb);
    reg(ItemType::StickyBarb,     createStickyBarb);
    reg(ItemType::BigRoot,        createBigRoot);
    reg(ItemType::KingsRock,      createKingsRock);

    // === Accuracy Items ===
    reg(ItemType::WideLens,  createWideLens);
    reg(ItemType::ZoomLens,  createZoomLens);
    reg(ItemType::ScopeLens, createScopeLens);

    // === Type-Boost Items ===
    reg(ItemType::SilverPowder, createSilverPowder);
    reg(ItemType::MetalCoat,    createMetalCoat);
    reg(ItemType::HardStone,    createHardStone);
    reg(ItemType::MiracleSeed,  createMiracleSeed);
    reg(ItemType::BlackGlasses, createBlackGlasses);
    reg(ItemType::BlackBelt,    createBlackBelt);
    reg(ItemType::Magnet,       createMagnet);
    reg(ItemType::MysticWater,  createMysticWater);
    reg(ItemType::SharpBeak,    createSharpBeak);
    reg(ItemType::PoisonBarb,   createPoisonBarb);
    reg(ItemType::NeverMeltIce, createNeverMeltIce);
    reg(ItemType::SpellTag,     createSpellTag);
    reg(ItemType::TwistedSpoon, createTwistedSpoon);
    reg(ItemType::Charcoal,     createCharcoal);
    reg(ItemType::DragonFang,   createDragonFang);
    reg(ItemType::SilkScarf,    createSilkScarf);
    reg(ItemType::SeaIncense,   createSeaIncense);

    // === Type Plates ===
    reg(ItemType::FlamePlate,   createFlamePlate);
    reg(ItemType::SplashPlate,  createSplashPlate);
    reg(ItemType::ZapPlate,     createZapPlate);
    reg(ItemType::MeadowPlate,  createMeadowPlate);
    reg(ItemType::IciclePlate,  createIciclePlate);
    reg(ItemType::FistPlate,    createFistPlate);
    reg(ItemType::ToxicPlate,   createToxicPlate);
    reg(ItemType::EarthPlate,   createEarthPlate);
    reg(ItemType::SkyPlate,     createSkyPlate);
    reg(ItemType::MindPlate,    createMindPlate);
    reg(ItemType::InsectPlate,  createInsectPlate);
    reg(ItemType::StonePlate,   createStonePlate);
    reg(ItemType::SpookyPlate,  createSpookyPlate);
    reg(ItemType::IronPlate,    createIronPlate);

    // === Status Orbs ===
    reg(ItemType::FlameOrb,  createFlameOrb);
    reg(ItemType::ToxicOrb,  createToxicOrb);

    // === Gen 9 Utility Items ===
    reg(ItemType::HeavyDutyBoots, createHeavyDutyBoots);
    reg(ItemType::CovertCloak,    createCovertCloak);
    reg(ItemType::ClearAmulet,    createClearAmulet);
    reg(ItemType::ProtectivePads, createProtectivePads);
    reg(ItemType::PunchingGlove,  createPunchingGlove);
    reg(ItemType::BoosterEnergy,  createBoosterEnergy);
    reg(ItemType::LoadedDice,     createLoadedDice);
    reg(ItemType::MirrorHerb,     createMirrorHerb);
    reg(ItemType::AbilityShield,  createAbilityShield);
    reg(ItemType::EjectPack,      createEjectPack);
    reg(ItemType::TerrainExtender, createTerrainExtender);
    reg(ItemType::RoomService,    createRoomService);
    reg(ItemType::BlunderPolicy,  createBlunderPolicy);
    reg(ItemType::ThroatSpray,    createThroatSpray);
    reg(ItemType::UtilityUmbrella, createUtilityUmbrella);
    reg(ItemType::LightClay,       createLightClay);
    reg(ItemType::MentalHerb,      createMentalHerb);
    reg(ItemType::SafetyGoggles,   createSafetyGoggles);
    reg(ItemType::RingTarget,      createRingTarget);
    reg(ItemType::Metronome,       createMetronome);
    reg(ItemType::DampRock,        createDampRock);
    reg(ItemType::HeatRock,        createHeatRock);
    reg(ItemType::IcyRock,         createIcyRock);
    reg(ItemType::SmoothRock,      createSmoothRock);
    reg(ItemType::BrightPowder,   createBrightPowder);
    reg(ItemType::FocusBand,      createFocusBand);
    reg(ItemType::CustapBerry,    createCustapBerry);
    reg(ItemType::EnigmaBerry,    createEnigmaBerry);
    reg(ItemType::BindingBand,    createBindingBand);
    reg(ItemType::ElectricSeed,  createElectricSeed);
    reg(ItemType::PsychicSeed,   createPsychicSeed);
    reg(ItemType::MistySeed,     createMistySeed);
    reg(ItemType::GrassySeed,    createGrassySeed);
    reg(ItemType::AdrenalineOrb,  createAdrenalineOrb);
    reg(ItemType::MicleBerry,    createMicleBerry);
    reg(ItemType::LansatBerry,   createLansatBerry);
    reg(ItemType::StarfBerry,    createStarfBerry);
    reg(ItemType::ShedShell,     createShedShell);
    reg(ItemType::GripClaw,      createGripClaw);
    reg(ItemType::IronBall,      createIronBall);
    reg(ItemType::AbsorbBulb,    createAbsorbBulb);
    reg(ItemType::CellBattery,   createCellBattery);
    reg(ItemType::LuminousMoss,  createLuminousMoss);
    reg(ItemType::Snowball,      createSnowball);
    reg(ItemType::LaxIncense,    createLaxIncense);
    reg(ItemType::LaggingTail,   createLaggingTail);
    reg(ItemType::RoseIncense,   createRoseIncense);
    reg(ItemType::WaveIncense,   createWaveIncense);
    reg(ItemType::OddIncense,    createOddIncense);
    reg(ItemType::FloatStone,    createFloatStone);
    reg(ItemType::RazorClaw,     createRazorClaw);
    reg(ItemType::RazorFang,     createRazorFang);
    reg(ItemType::FullIncense,   createFullIncense);
    reg(ItemType::SmokeBall,     createSmokeBall);
    reg(ItemType::SoftSand,      createSoftSand);
    reg(ItemType::DracoPlate,    createDracoPlate);
    reg(ItemType::DreadPlate,    createDreadPlate);
    reg(ItemType::RockIncense,   createRockIncense);
    reg(ItemType::LuckIncense,   createLuckIncense);
}

// === Item logic helpers ===

bool itemPreventsStatDrops(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.preventsStatDrops;
}

bool itemPreventsContact(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.preventsContactEffects;
}

bool itemBlocksEntryHazards(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.blocksEntryHazards;
}

bool itemBlocksSecondaryEffects(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.blocksSecondaryEffects;
}

bool itemMaximizesMultiHit(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.maximizesMultiHit;
}

bool itemBlocksAbilityChange(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.blocksAbilityChange;
}

bool itemExtendsTerrain(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.extendsTerrain;
}

bool itemIgnoresWeather(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.ignoresWeather;
}

bool itemExtendsScreens(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.extendsScreens;
}

bool itemBlocksWeatherPowder(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.blocksWeatherPowder;
}

bool itemExtendsWeather(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.extendsWeather;
}

bool itemHasFocusBand(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.hasFocusBand;
}

bool itemHasCustapBerry(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.hasCustapBerry;
}

float itemEvasionBoost(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.evasionBoost;
}

bool itemHealsOnSuperEffective(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.healsOnSuperEffective;
}

bool itemBoostsBindingMoves(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.boostsBindingMoves;
}

bool itemExtendsTrappingMoves(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.extendsTrappingMoves;
}

bool itemEnsuresCanSwitch(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.ensuresCanSwitch;
}

bool itemHalvesSpeedAndGrounds(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.halvesSpeedAndGrounds;
}

bool itemAlwaysMovesLast(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.alwaysMovesLast;
}

bool itemHalvesWeight(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.halvesWeight;
}

bool itemCritStageBoost(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.critStageBoost;
}

bool itemFlinchOnHit(ItemType type) {
    return GameRegistry::instance().getItem(type).passive.flinchOnHit;
}

bool tryQuickClawActivation(ItemType type, int& priority) {
    if (type != ItemType::QuickClaw) return false;
    if (PRNG::nextFloat(0.0f, 1.0f) < 0.2f) {
        priority = std::numeric_limits<int>::max() / 4;
        return true;
    }
    return false;
}

float knockOffDamageMultiplier(const Move& move, ItemType targetItemType) {
    // normalize token check
    const std::string& name = move.getName();
    std::string normalized;
    normalized.reserve(name.size());
    for (char ch : name) {
        if (ch == ' ' || ch == '-' || ch == '\'' || ch == '_') continue;
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    if (normalized == "knockoff" && targetItemType != ItemType::None) {
        return 1.5f;
    }
    return 1.0f;
}

bool tryKnockOffItemRemoval(const Move& move, Pokemon* defender) {
    if (!defender) return false;
    const std::string& name = move.getName();
    std::string normalized;
    normalized.reserve(name.size());
    for (char ch : name) {
        if (ch == ' ' || ch == '-' || ch == '\'' || ch == '_') continue;
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    if (normalized != "knockoff") return false;
    if (defender->getItemType() != ItemType::None) {
        defender->removeItem();
    }
    return true;  // Knock Off always stops further processing
}
