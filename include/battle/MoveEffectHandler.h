#pragma once

class BattleContext;
class Pokemon;
class Move;

namespace MoveEffectHandlers {
void applyStandardMoveEffect(BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move& move);
}
