/*==============================================================================

　　  スピンアクションAABB[player_spin.h]
														 Author : Kouki Tanaka
														 Date   : 2026/01/10
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef PLAYER_SPIN_H
#define PLAYER_SPIN_H

#include "player.h"

// Spin AABB: expands only in XZ when spinning.
inline AABB Player_GetSpinAABB(float expandXZ = 0.4f)
{
	AABB aabb = Player_GetAABB();
	const PlayerActionState* act = Player_GetActionState();
	if (act && act->id == PlayerActionId::Spin)
	{
		aabb.min.x -= expandXZ;
		aabb.max.x += expandXZ;
		aabb.min.z -= expandXZ;
		aabb.max.z += expandXZ;
	}
	return aabb;
}

#endif // PLAYER_SPIN_H