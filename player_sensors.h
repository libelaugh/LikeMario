/*==============================================================================

　　  壁・崖掴みセンサー[player_sensors.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/07
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef PLAYER_SENSORS_H
#define PLAYER_SENSORS_H

#include <DirectXMath.h>
#include "collision.h"        // AABB / Hit / Collision_IsOverlapAABB / Collision_IsHitAABB
#include "stage01_manage.h"   // Stage01_GetCount / Stage01_Get (StageBlock::aabb)

struct PlayerWallLedgeSensors
{
    bool wallTouch = false;
    DirectX::XMFLOAT3 wallNormal{ 0,0,0 }; // wall -> player (axis)

    bool ledgeAvailable = false;
    DirectX::XMFLOAT3 ledgeHangPos{ 0,0,0 }; // snap position (center)
    DirectX::XMFLOAT3 ledgeNormal{ 0,0,0 };  // wall -> player (axis)
};

// Tuning values for ledge logic (make these ImGui parameters later).
struct PlayerLedgeTuning
{
    // Touch detection epsilon (expand player AABB)
    float contactEps = 0.06f;

    // Ledge check is only valid when falling (velY <= this)
    float minFallingVelY = -0.05f;

    // Ledge height constraints relative to player height
    float ledgeTopBelowPlayerTopMax = 0.35f; // player.max.y must be >= topY - this
    float ledgeTopAbovePlayerMinYMin = 0.20f; // topY must be >= player.min.y + this

    // Hang placement
    float hangWallGap = 0.06f; // offset away from wall
    float hangDown = 0.20f;    // place player a bit below top

    // Clearance probe (to ensure head-space is free above the ledge)
    float clearanceMinY = 0.05f;         // probe starts at (topY + clearanceMinY)
    float clearanceHeightFrac = 0.65f;   // probe height = playerHeight*frac
    float clearanceXZFrac = 0.80f;       // probe XZ halfsize scale
};

// Build sensors.
// - playerAabb: current player AABB.
// - velY: current vertical velocity.
// - onGround: grounded flag.
void PlayerSensors_BuildWallLedge(const AABB& playerAabb,
    float velY,
    bool onGround,
    const PlayerLedgeTuning& tuning,
    PlayerWallLedgeSensors& out);


#endif//PLAYER_SENSORS_H