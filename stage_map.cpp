/*==============================================================================

　　  ステージオブジェクト管理[stage_map.cpp]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#include"stage_map.h"
#include"stage_cube.h"
#include"texture.h"
#include"light.h"

#include<DirectXMath.h>
#include<vector>

using namespace DirectX;

static std::vector<CubeBlock> g_Blocks;

static int g_TexBrick   = -1;
static int g_TexDefault = -1;

static constexpr int KIND_FULL_UV   = 1;
static constexpr int KIND_SOLID_RED = 2;

static CubeBlock MakeBlock(
    int kind,
    int texId,
    const XMFLOAT3& pos,
    const XMFLOAT3& size,
    const XMFLOAT3& rotRad)
{
    CubeBlock b{};
    b.kind = kind;
    b.texId = texId;
    b.position = pos;
    b.size = size;
    b.rotation = rotRad;
    Cube_BakeBlock(b);
    return b;
}

bool Map_Initialize()
{
    g_Blocks.clear();
    g_Blocks.reserve(2048);

    g_TexBrick   = Texture_Load(L"rengaBlock.png");
    g_TexDefault = Texture_Load(L"cubeTexture1.png");

    {
        CubeTemplate t = CubeTemplate_Unit();
        CubeTemplate_SetAllFaceUV(t, {0,0}, {1,1});
        CubeTemplate_SetAllFaceColor(t, {1,1,1,1});
        Cube_RegisterKind(KIND_FULL_UV, t);
    }

    {
        CubeTemplate t = CubeTemplate_Unit();
        CubeTemplate_SetAllFaceUV(t, {0,0}, {1,1});
        CubeTemplate_SetAllFaceColor(t, {1,0,0,1});
        Cube_RegisterKind(KIND_SOLID_RED, t);
    }

    g_Blocks.push_back(MakeBlock(KIND_FULL_UV, g_TexBrick, {0.0f, -0.5f, 0.0f}, {40.0f, 1.0f, 40.0f}, {0,0,0}));

    g_Blocks.push_back(MakeBlock(KIND_FULL_UV, g_TexBrick, {0.0f, 1.0f, 0.0f}, {4.0f, 1.0f, 4.0f}, {0.0f, XM_PIDIV4, 0.0f}));
    g_Blocks.push_back(MakeBlock(KIND_FULL_UV, g_TexDefault, {6.0f, 2.0f, 0.0f}, {6.0f, 1.0f, 2.0f}, {0.0f, XM_PIDIV2, 0.0f}));
    g_Blocks.push_back(MakeBlock(KIND_SOLID_RED, g_TexDefault, {-6.0f, 1.0f, 0.0f}, {2.0f, 3.0f, 2.0f}, {0,0,0}));

    return true;
}

void Map_Finalize()
{
    g_Blocks.clear();
}

void Map_Update(double)
{
}

void Map_Draw()
{
    //Light_SetSpecularWorld({0.2f, 0.2f, 0.2f, 1.0f});

    for (const auto& b : g_Blocks)
    {
        Cube_DrawBlock(b);
    }
}

void Map_DepthDraw()
{
    for (const auto& b : g_Blocks)
    {
        Cube_DepthDrawBlock(b);
    }
}

int Map_GetObjectsCount()
{
    return static_cast<int>(g_Blocks.size());
}

const CubeBlock* Map_GetObject(int index)
{
    if (index < 0 || index >= static_cast<int>(g_Blocks.size())) return nullptr;
    return &g_Blocks[index];
}

CubeBlock* Map_GetObjectMutable(int index)
{
    if (index < 0 || index >= static_cast<int>(g_Blocks.size())) return nullptr;
    return &g_Blocks[index];
}

void Map_RebuildObject(int index)
{
    if (index < 0 || index >= static_cast<int>(g_Blocks.size())) return;
    Cube_BakeBlock(g_Blocks[index]);
}

int Map_AddBlock(const CubeBlock& block, bool bake)
{
    g_Blocks.push_back(block);
    if (bake)
    {
        Cube_BakeBlock(g_Blocks.back());
    }
    return static_cast<int>(g_Blocks.size()) - 1;
}

bool Map_AddObjectTransform(int index,
    const XMFLOAT3& positionDelta,
    const XMFLOAT3& sizeDelta,
    const XMFLOAT3& rotationDelta)
{
    if (index < 0 || index >= static_cast<int>(g_Blocks.size())) return false;
    CubeBlock& block = g_Blocks[index];
    block.position.x += positionDelta.x;
    block.position.y += positionDelta.y;
    block.position.z += positionDelta.z;
    block.size.x += sizeDelta.x;
    block.size.y += sizeDelta.y;
    block.size.z += sizeDelta.z;
    block.rotation.x += rotationDelta.x;
    block.rotation.y += rotationDelta.y;
    block.rotation.z += rotationDelta.z;
    Cube_BakeBlock(block);
    return true;
}

void Map_Clear()
{
    g_Blocks.clear();
}
