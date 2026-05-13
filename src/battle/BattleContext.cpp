#include "battle/BattleContext.h"
#include "battle/Battle.h"
#include "battle/Pokemon.h"

BattleContext::BattleContext(Weather& weather, Field& field,
                             Side& sideA, Side& sideB,
                             EventSystem& eventSystem,
                             RuntimeMoveState& runtimeState,
                             int& turnNumber,
                             std::vector<nlohmann::json>& specialEvents)
    : weather_(weather), field_(field),
      sideA_(sideA), sideB_(sideB),
      eventSystem_(eventSystem),
      runtimeState_(runtimeState),
      turnNumber_(turnNumber),
      specialEvents_(specialEvents),
      battle_(nullptr) {}

Side& BattleContext::getOpponentSide(const Side& side) {
    return (&side == &sideA_) ? sideB_ : sideA_;
}

Side* BattleContext::findSideForPokemon(Pokemon* pokemon) {
    if (sideA_.getActivePokemon() == pokemon) return &sideA_;
    if (sideB_.getActivePokemon() == pokemon) return &sideB_;
    for (Pokemon* member : sideA_.getTeam()) {
        if (member == pokemon) return &sideA_;
    }
    for (Pokemon* member : sideB_.getTeam()) {
        if (member == pokemon) return &sideB_;
    }
    return nullptr;
}

const Side* BattleContext::findSideForPokemon(const Pokemon* pokemon) const {
    if (sideA_.getActivePokemon() == pokemon) return &sideA_;
    if (sideB_.getActivePokemon() == pokemon) return &sideB_;
    for (Pokemon* member : sideA_.getTeam()) {
        if (member == pokemon) return &sideA_;
    }
    for (Pokemon* member : sideB_.getTeam()) {
        if (member == pokemon) return &sideB_;
    }
    return nullptr;
}

Pokemon* BattleContext::getOpponentPokemon(Pokemon* self) const {
    if (!self) return nullptr;
    const Side* side = findSideForPokemon(self);
    if (!side) return nullptr;
    const Side& opponentSide = (&sideA_ == side) ? sideB_ : sideA_;
    return opponentSide.getActivePokemon();
}

void BattleContext::appendSpecialEvent(const std::string& eventType,
                                        const nlohmann::json& details) {
    nlohmann::json event;
    event["type"] = eventType;
    event["details"] = details;
    specialEvents_.push_back(std::move(event));
}

void BattleContext::setBattlePointer(Battle* battle) {
    battle_ = battle;
}

void BattleContext::triggerAbility(Pokemon* pokemon, Trigger trigger,
                                    Pokemon* opponent, void* context) {
    if (battle_) battle_->triggerAbility(pokemon, trigger, opponent, context);
}

void BattleContext::triggerItemEffect(Pokemon* pokemon, ItemTrigger trigger,
                                       Pokemon* opponent, void* context) {
    if (battle_) battle_->triggerItemEffect(pokemon, trigger, opponent, context);
}

bool BattleContext::switchPokemon(Side& side, int newIndex) {
    return battle_ ? battle_->switchPokemon(side, newIndex) : false;
}

bool BattleContext::canBeForcedToSwitch(Pokemon* defender) const {
    return battle_ ? battle_->canBeForcedToSwitch(defender) : false;
}

int BattleContext::calculateDamage(Pokemon* attacker, Pokemon* defender, const Move& move) const {
    return battle_ ? battle_->calculateDamage(attacker, defender, move) : 0;
}

void BattleContext::processMoveEffects(Pokemon* attacker, Pokemon* defender, const Move& move) {
    if (battle_) battle_->processMoveEffects(attacker, defender, move);
}

void BattleContext::applyDisableToTarget(Pokemon* target) {
    if (battle_) battle_->applyDisableToTarget(target);
}
