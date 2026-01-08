/*==============================================================================

　　  壁・崖掴みセンサー[player_sensors.cpp]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/07
--------------------------------------------------------------------------------

==============================================================================*/

#include "player_sensors.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
    inline XMFLOAT3 Center(const AABB& a)
    {
        return { (a.min.x + a.max.x) * 0.5f, (a.min.y + a.max.y) * 0.5f, (a.min.z + a.max.z) * 0.5f };
    }

    inline XMFLOAT3 Half(const AABB& a)
    {
        return { (a.max.x - a.min.x) * 0.5f, (a.max.y - a.min.y) * 0.5f, (a.max.z - a.min.z) * 0.5f };
    }

    inline float Height(const AABB& a) { return (a.max.y - a.min.y); }

    inline AABB Expand(const AABB& a, float e)
    {
        AABB r = a;
        r.min.x -= e; r.min.y -= e; r.min.z -= e;
        r.max.x += e; r.max.y += e; r.max.z += e;
        return r;
    }

    // Determine outward normal axis from block->player using centers (robust against normal sign confusion)
    inline XMFLOAT3 AxisNormalFromCenters(const AABB& playerAabb, const AABB& blockAabb)
    {
        const XMFLOAT3 pc = Center(playerAabb);
        const XMFLOAT3 bc = Center(blockAabb);
        const float dx = pc.x - bc.x;
        const float dz = pc.z - bc.z;

        if (std::fabs(dx) >= std::fabs(dz))
            return { (dx >= 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f };
        else
            return { 0.0f, 0.0f, (dz >= 0.0f) ? 1.0f : -1.0f };
    }

    inline bool IsWallNormal(const XMFLOAT3& n)
    {
        return (std::fabs(n.y) < 0.5f) && (std::fabs(n.x) > 0.5f || std::fabs(n.z) > 0.5f);
    }

    inline AABB MakeClearanceProbe(const XMFLOAT3& hangCenter,
        const XMFLOAT3& playerHalf,
        float topY,
        const PlayerLedgeTuning& t,
        float playerHeight)
    {
        const float probeHalfX = playerHalf.x * t.clearanceXZFrac;
        const float probeHalfZ = playerHalf.z * t.clearanceXZFrac;

        const float minY = topY + t.clearanceMinY;
        const float maxY = minY + playerHeight * t.clearanceHeightFrac;

        AABB probe{};
        probe.min = { hangCenter.x - probeHalfX, minY, hangCenter.z - probeHalfZ };
        probe.max = { hangCenter.x + probeHalfX, maxY, hangCenter.z + probeHalfZ };
        return probe;
    }

    inline bool OverlapsAnyStage(const AABB& aabb)
    {
        const int n = Stage01_GetCount();
        for (int i = 0; i < n; ++i)
        {
            const StageBlock* b = Stage01_Get(i);
            if (!b) continue;
            if (Collision_IsOverlapAABB(aabb, b->aabb)) return true;
        }
        return false;
    }

    inline XMFLOAT3 ComputeHangCenter(const AABB& playerAabb,
        const AABB& wallAabb,
        const XMFLOAT3& outwardAxis,
        float wallGap)
    {
        const XMFLOAT3 pc = Center(playerAabb);
        const XMFLOAT3 ph = Half(playerAabb);

        XMFLOAT3 hang = pc;

        // Decide which side based on player vs wall positions (center-based)
        if (std::fabs(outwardAxis.x) > 0.5f)
        {
            // player is on left side of wall -> hang to the left, else right
            const float wallMinX = wallAabb.min.x;
            const float wallMaxX = wallAabb.max.x;
            hang.x = (outwardAxis.x < 0.0f) ? (wallMinX - ph.x - wallGap)
                : (wallMaxX + ph.x + wallGap);
        }
        else
        {
            const float wallMinZ = wallAabb.min.z;
            const float wallMaxZ = wallAabb.max.z;
            hang.z = (outwardAxis.z < 0.0f) ? (wallMinZ - ph.z - wallGap)
                : (wallMaxZ + ph.z + wallGap);
        }
        return hang;
    }
}

void PlayerSensors_BuildWallLedge(const AABB& playerAabb,
    float velY,
    bool onGround,
    const PlayerLedgeTuning& tuning,
    PlayerWallLedgeSensors& out)
{
    out = PlayerWallLedgeSensors{};

    const float e = std::max(0.0f, tuning.contactEps);
    const AABB touchAabb = Expand(playerAabb, e);

    // 1) Wall touch search (after collision resolution we use expanded AABB)
    bool foundWall = false;
    StageBlock const* bestWall = nullptr;
    XMFLOAT3 bestNormal{ 0,0,0 };

    const int count = Stage01_GetCount();
    for (int i = 0; i < count; ++i)
    {
        const StageBlock* b = Stage01_Get(i);
        if (!b) continue;

        if (!Collision_IsOverlapAABB(touchAabb, b->aabb))
            continue;

        // Use MTV normal from overlap, but compute outward axis from centers for stability
        Hit hit = Collision_IsHitAABB(touchAabb, b->aabb);
        if (!hit.isHit) continue;

        XMFLOAT3 outward = AxisNormalFromCenters(playerAabb, b->aabb);
        if (!IsWallNormal(outward)) continue;

        // Avoid treating ground contact as wall contact
        if (onGround)
        {
            // If player is grounded, still allow wall touch but only if the wall's top is above player's min.y
            // (prevents floor block from being picked as wall)
        }

        foundWall = true;
        bestWall = b;
        bestNormal = outward;
        break; // first is enough for now (stable)
    }

    if (foundWall && bestWall)
    {
        out.wallTouch = true;
        out.wallNormal = bestNormal;
    }
    else
    {
        // No wall => no ledge
        return;
    }

    // 2) Ledge availability (optional)
    // Conditions:
    // - not grounded
    // - falling
    // - wall touch
    if (onGround) return;
    if (velY > tuning.minFallingVelY) return;

    const float playerH = Height(playerAabb);
    const XMFLOAT3 ph = Half(playerAabb);

    const float topY = bestWall->aabb.max.y;

    // Player should be near the ledge height
    if (playerAabb.max.y < topY - tuning.ledgeTopBelowPlayerTopMax) return;
    if (topY < playerAabb.min.y + tuning.ledgeTopAbovePlayerMinYMin) return;

    // Compute hang center on the outward side of the wall
    XMFLOAT3 hangCenter = ComputeHangCenter(playerAabb, bestWall->aabb, bestNormal, tuning.hangWallGap);

    // Place player a bit below the ledge top
    hangCenter.y = topY - ph.y - tuning.hangDown;

    // Clearance probe above the ledge (head space)
    const AABB probe = MakeClearanceProbe(hangCenter, ph, topY, tuning, playerH);

    // If anything blocks the space above the ledge, no grab
    if (OverlapsAnyStage(probe)) return;

    // Also ensure the hang position itself isn't embedded into geometry
    {
        AABB hangAabb = playerAabb;
        const XMFLOAT3 pc = Center(playerAabb);
        const XMFLOAT3 delta{ hangCenter.x - pc.x, hangCenter.y - pc.y, hangCenter.z - pc.z };
        hangAabb.min.x += delta.x; hangAabb.max.x += delta.x;
        hangAabb.min.y += delta.y; hangAabb.max.y += delta.y;
        hangAabb.min.z += delta.z; hangAabb.max.z += delta.z;

        if (OverlapsAnyStage(hangAabb)) return;
    }

    out.ledgeAvailable = true;
    out.ledgeHangPos = hangCenter;
    out.ledgeNormal = bestNormal;
}

