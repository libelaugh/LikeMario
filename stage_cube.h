/*==============================================================================

　　  キューブのオブジェクト管理[stage_cube.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_CUBE__H
#define STAGE_CUBE__H

#include "collision.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <array>

struct Vertex3d
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normalVector;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 texcoord;
};

constexpr int CUBE_FACE_COUNT = 6;
constexpr int CUBE_VERTS_PER_FACE = 4;
constexpr int CUBE_VERTEX_COUNT = CUBE_FACE_COUNT * CUBE_VERTS_PER_FACE;

// ===== Kind presets =====
constexpr int CUBE_KIND_LEGACY = 0;   // 既存互換
constexpr int CUBE_KIND_FULL_UV = 1;   // 全面UV 0-1 / 白
constexpr int CUBE_KIND_SOLID_RED = 2;   // 全面赤（デバッグ用）

constexpr int CUBE_KIND_RENGA = 10;  // レンガ
constexpr int CUBE_KIND_REGO = 11;  // レゴっぽい（色味）

enum CubeFace
{
    CUBE_FACE_FRONT = 0,
    CUBE_FACE_BACK,
    CUBE_FACE_LEFT,
    CUBE_FACE_RIGHT,
    CUBE_FACE_TOP,
    CUBE_FACE_BOTTOM,
};

struct CubeFaceDesc
{
    std::array<DirectX::XMFLOAT3, CUBE_VERTS_PER_FACE> pos{};

    DirectX::XMFLOAT3 normal{ 0,0,0 };

    std::array<DirectX::XMFLOAT4, CUBE_VERTS_PER_FACE> color{};
    std::array<DirectX::XMFLOAT2, CUBE_VERTS_PER_FACE> uv{};
};

struct CubeTemplate
{
    std::array<CubeFaceDesc, CUBE_FACE_COUNT> face{};
};



CubeTemplate CubeTemplate_Unit();

void CubeTemplate_SetAllFaceUV(CubeTemplate& t, const DirectX::XMFLOAT2& uvMin, const DirectX::XMFLOAT2& uvMax);

void CubeTemplate_SetFaceUV(CubeTemplate& t, CubeFace face, const DirectX::XMFLOAT2& uvMin, const DirectX::XMFLOAT2& uvMax);

void CubeTemplate_SetAllFaceColor(CubeTemplate& t, const DirectX::XMFLOAT4& color);

void CubeTemplate_SetFaceColor(CubeTemplate& t, CubeFace face, const DirectX::XMFLOAT4& color);



void Cube_RegisterKind(int kind, const CubeTemplate& tpl);

void Cube_UpdateKind(int kind, const CubeTemplate& tpl);

bool Cube_TryGetKindTemplate(int kind, CubeTemplate& outTpl);

// outKinds=nullptr なら総数だけ返す
int  Cube_GetKindList(int* outKinds, int maxKinds);

struct CubeBlock
{
    int kind = 0;
    int texId = -1;

    DirectX::XMFLOAT3 position{ 0,0,0 };
    DirectX::XMFLOAT3 size{ 1,1,1 };
    DirectX::XMFLOAT3 rotation{ 0,0,0 }; 

    DirectX::XMFLOAT4X4 world{};
    AABB aabb{};
};

void Cube_BakeBlock(CubeBlock& block);

void Cube_DrawBlock(const CubeBlock& block);

void Cube_DepthDrawBlock(const CubeBlock& block);

void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Cube_Finalize();
void Cube_Update(double elapsedTime);
void Cube_Draw(DirectX::XMMATRIX& mtrWorld);
void Cube_Draw02(int texId, const DirectX::XMMATRIX& mtrWorld);
void Cube_DepthDraw(const DirectX::XMMATRIX& mtrWorld);
AABB Cube_GetAABB(const DirectX::XMFLOAT3& position);

#endif//STAGE_CUBE_H
