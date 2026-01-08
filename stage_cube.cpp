/*==============================================================================

　　  キューブのオブジェクト管理[stage_cube.cpp]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/
#include "stage_cube.h"

#include "direct3d.h"
#include "shader3d.h"
#include "shader_depth.h"
#include "texture.h"

#include <DirectXMath.h>
#include <cfloat>
#include <cstring>
#include <unordered_map>

using namespace DirectX;

static constexpr int NUM_INDEX = 3 * 2 * 6;
static constexpr float HALF = 0.5f;

static unsigned short g_CubeIndex[NUM_INDEX] = {
    0,1,2,0,2,3,
    4,5,6,4,6,7,
    8,9,10,8,10,11,
    12,13,14,12,14,15,
    16,17,18,16,18,19,
    20,21,22,20,22,23,
};

static ID3D11Buffer* g_pIndexBuffer = nullptr;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static int g_defaultTexId = -1;

struct KindGpu
{
    ID3D11Buffer* vb = nullptr;
    CubeTemplate tpl{};
    std::array<XMFLOAT3, CUBE_VERTEX_COUNT> localPos{};
};

static std::unordered_map<int, KindGpu> g_kinds;

static void buildVerticesFromTemplate(
    const CubeTemplate& tpl,
    std::array<Vertex3d, CUBE_VERTEX_COUNT>& outVerts,
    std::array<XMFLOAT3, CUBE_VERTEX_COUNT>& outPos)
{
    for (int f = 0; f < CUBE_FACE_COUNT; ++f)
    {
        const CubeFaceDesc& face = tpl.face[f];
        for (int v = 0; v < CUBE_VERTS_PER_FACE; ++v)
        {
            const int idx = f * CUBE_VERTS_PER_FACE + v;
            outVerts[idx].position = face.pos[v];
            outVerts[idx].normalVector = face.normal;
            outVerts[idx].color = face.color[v];
            outVerts[idx].texcoord = face.uv[v];
            outPos[idx] = face.pos[v];
        }
    }
}

static ID3D11Buffer* createDynamicVertexBuffer(const void* initialData, size_t byteSize)
{
    if (!g_pDevice) return nullptr;

    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = static_cast<UINT>(byteSize);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = initialData;

    ID3D11Buffer* vb = nullptr;
    g_pDevice->CreateBuffer(&bd, &sd, &vb);
    return vb;
}

static KindGpu* findKind(int kind)
{
    auto it = g_kinds.find(kind);
    if (it != g_kinds.end()) return &it->second;

    it = g_kinds.find(0);
    if (it != g_kinds.end()) return &it->second;

    return nullptr;
}

static void drawKindInternal(int kind, int texId, const XMMATRIX& world, bool depth)
{
    KindGpu* k = findKind(kind);
    if (!k || !k->vb || !g_pIndexBuffer) return;

    const UINT stride = sizeof(Vertex3d);
    const UINT offset = 0;

    g_pContext->IASetVertexBuffers(0, 1, &k->vb, &stride, &offset);
    g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    if (depth)
    {
        ShaderDepth_Begin();
        ShaderDepth_SetWorldMatrix(world);
        g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
        return;
    }

    Shader3D_Begin();
    Shader3d_SetColor({ 1,1,1,1 });
    Shader3D_SetWorldMatrix(world);

    if (texId < 0) texId = g_defaultTexId;
    Texture_SetTexture(texId);

    g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

CubeTemplate CubeTemplate_Unit()
{
    CubeTemplate t{};

    auto initFace = [&](CubeFace face, const std::array<XMFLOAT3,4>& pos, const XMFLOAT3& n)
    {
        CubeFaceDesc& f = t.face[static_cast<int>(face)];
        f.pos = pos;
        f.normal = n;
        for (int i = 0; i < 4; ++i)
        {
            f.color[i] = { 1,1,1,1 };
            f.uv[i] = { 0,0 };
        }
    };

    initFace(CUBE_FACE_FRONT,  { XMFLOAT3{-HALF, +HALF, -HALF}, XMFLOAT3{+HALF, +HALF, -HALF}, XMFLOAT3{+HALF, -HALF, -HALF}, XMFLOAT3{-HALF, -HALF, -HALF} }, XMFLOAT3{0,0,-1});
    initFace(CUBE_FACE_BACK,   { XMFLOAT3{+HALF, +HALF, +HALF}, XMFLOAT3{-HALF, +HALF, +HALF}, XMFLOAT3{-HALF, -HALF, +HALF}, XMFLOAT3{+HALF, -HALF, +HALF} }, XMFLOAT3{0,0,1});
    initFace(CUBE_FACE_LEFT,   { XMFLOAT3{-HALF, +HALF, +HALF}, XMFLOAT3{-HALF, +HALF, -HALF}, XMFLOAT3{-HALF, -HALF, -HALF}, XMFLOAT3{-HALF, -HALF, +HALF} }, XMFLOAT3{-1,0,0});
    initFace(CUBE_FACE_RIGHT,  { XMFLOAT3{+HALF, +HALF, -HALF}, XMFLOAT3{+HALF, +HALF, +HALF}, XMFLOAT3{+HALF, -HALF, +HALF}, XMFLOAT3{+HALF, -HALF, -HALF} }, XMFLOAT3{1,0,0});
    initFace(CUBE_FACE_TOP,    { XMFLOAT3{-HALF, +HALF, +HALF}, XMFLOAT3{+HALF, +HALF, +HALF}, XMFLOAT3{+HALF, +HALF, -HALF}, XMFLOAT3{-HALF, +HALF, -HALF} }, XMFLOAT3{0,1,0});
    initFace(CUBE_FACE_BOTTOM, { XMFLOAT3{-HALF, -HALF, -HALF}, XMFLOAT3{+HALF, -HALF, -HALF}, XMFLOAT3{+HALF, -HALF, +HALF}, XMFLOAT3{-HALF, -HALF, +HALF} }, XMFLOAT3{0,-1,0});

    return t;
}

void CubeTemplate_SetAllFaceUV(CubeTemplate& t, const XMFLOAT2& uvMin, const XMFLOAT2& uvMax)
{
    for (int f = 0; f < CUBE_FACE_COUNT; ++f)
    {
        CubeTemplate_SetFaceUV(t, static_cast<CubeFace>(f), uvMin, uvMax);
    }
}

void CubeTemplate_SetFaceUV(CubeTemplate& t, CubeFace face, const XMFLOAT2& uvMin, const XMFLOAT2& uvMax)
{
    CubeFaceDesc& f = t.face[static_cast<int>(face)];

    f.uv[0] = { uvMin.x, uvMin.y };
    f.uv[1] = { uvMax.x, uvMin.y };
    f.uv[2] = { uvMax.x, uvMax.y };
    f.uv[3] = { uvMin.x, uvMax.y };
}

void CubeTemplate_SetAllFaceColor(CubeTemplate& t, const XMFLOAT4& color)
{
    for (int f = 0; f < CUBE_FACE_COUNT; ++f)
    {
        CubeTemplate_SetFaceColor(t, static_cast<CubeFace>(f), color);
    }
}

void CubeTemplate_SetFaceColor(CubeTemplate& t, CubeFace face, const XMFLOAT4& color)
{
    CubeFaceDesc& f = t.face[static_cast<int>(face)];
    for (int i = 0; i < 4; ++i)
    {
        f.color[i] = color;
    }
}

void Cube_RegisterKind(int kind, const CubeTemplate& tpl)
{
    if (!g_pDevice || !g_pContext) return;

    std::array<Vertex3d, CUBE_VERTEX_COUNT> verts{};
    std::array<XMFLOAT3, CUBE_VERTEX_COUNT> pos{};
    buildVerticesFromTemplate(tpl, verts, pos);

    auto& k = g_kinds[kind];
    if (k.vb)
    {
        SAFE_RELEASE(k.vb);
    }

    k.vb = createDynamicVertexBuffer(verts.data(), sizeof(Vertex3d) * verts.size());
    k.tpl = tpl;
    k.localPos = pos;
}

void Cube_UpdateKind(int kind, const CubeTemplate& tpl)
{
    if (!g_pDevice || !g_pContext) return;

    auto it = g_kinds.find(kind);
    if (it == g_kinds.end() || !it->second.vb)
    {
        Cube_RegisterKind(kind, tpl);
        return;
    }

    KindGpu& k = it->second;

    std::array<Vertex3d, CUBE_VERTEX_COUNT> verts{};
    std::array<XMFLOAT3, CUBE_VERTEX_COUNT> pos{};
    buildVerticesFromTemplate(tpl, verts, pos);

    D3D11_MAPPED_SUBRESOURCE ms{};
    if (SUCCEEDED(g_pContext->Map(k.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
    {
        memcpy(ms.pData, verts.data(), sizeof(Vertex3d) * verts.size());
        g_pContext->Unmap(k.vb, 0);
    }

    k.tpl = tpl;
    k.localPos = pos;
}

bool Cube_TryGetKindTemplate(int kind, CubeTemplate& outTpl)
{
    auto it = g_kinds.find(kind);
    if (it == g_kinds.end()) return false;
    outTpl = it->second.tpl;
    return true;
}

int Cube_GetKindList(int* outKinds, int maxKinds)
{
    int n = 0;
    for (auto& kv : g_kinds)
    {
        if (outKinds && n < maxKinds) outKinds[n] = kv.first;
        ++n;
    }
    return n;
}

void Cube_BakeBlock(CubeBlock& block)
{
    // world = S * R * T
    const XMMATRIX mS = XMMatrixScaling(block.size.x, block.size.y, block.size.z);
    const XMMATRIX mR = XMMatrixRotationRollPitchYaw(block.rotation.x, block.rotation.y, block.rotation.z);
    const XMMATRIX mT = XMMatrixTranslation(block.position.x, block.position.y, block.position.z);
    const XMMATRIX world = mS * mR * mT;

    XMStoreFloat4x4(&block.world, world);

    KindGpu* k = findKind(block.kind);
    if (!k)
    {
        block.aabb = Cube_GetAABB(block.position);
        return;
    }

    XMFLOAT3 mn{ +FLT_MAX, +FLT_MAX, +FLT_MAX };
    XMFLOAT3 mx{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (const auto& p : k->localPos)
    {
        const XMVECTOR wp = XMVector3TransformCoord(XMLoadFloat3(&p), world);
        XMFLOAT3 wpf{};
        XMStoreFloat3(&wpf, wp);

        mn.x = (wpf.x < mn.x) ? wpf.x : mn.x;
        mn.y = (wpf.y < mn.y) ? wpf.y : mn.y;
        mn.z = (wpf.z < mn.z) ? wpf.z : mn.z;

        mx.x = (wpf.x > mx.x) ? wpf.x : mx.x;
        mx.y = (wpf.y > mx.y) ? wpf.y : mx.y;
        mx.z = (wpf.z > mx.z) ? wpf.z : mx.z;
    }

    block.aabb.min = mn;
    block.aabb.max = mx;
}

void Cube_DrawBlock(const CubeBlock& block)
{
    const XMMATRIX world = XMLoadFloat4x4(&block.world);
    drawKindInternal(block.kind, block.texId, world, false);
}

void Cube_DepthDrawBlock(const CubeBlock& block)
{
    const XMMATRIX world = XMLoadFloat4x4(&block.world);
    drawKindInternal(block.kind, block.texId, world, true);
}

static CubeTemplate makeLegacyTemplate()
{
    CubeTemplate t = CubeTemplate_Unit();

    auto setFaceUvRaw = [&](CubeFace face, const XMFLOAT2& uv0, const XMFLOAT2& uv1, const XMFLOAT2& uv2, const XMFLOAT2& uv3)
        {
            auto& f = t.face[static_cast<int>(face)];
            f.uv[0] = uv0;
            f.uv[1] = uv1;
            f.uv[2] = uv2;
            f.uv[3] = uv3;
        };

    // 全面に 0～1 を貼る
    setFaceUvRaw(CUBE_FACE_FRONT, { 0,0 }, { 1,0 }, { 1,1 }, { 0,1 });
    setFaceUvRaw(CUBE_FACE_BACK, { 0,0 }, { 1,0 }, { 1,1 }, { 0,1 });
    setFaceUvRaw(CUBE_FACE_LEFT, { 0,0 }, { 1,0 }, { 1,1 }, { 0,1 });
    setFaceUvRaw(CUBE_FACE_RIGHT, { 0,0 }, { 1,0 }, { 1,1 }, { 0,1 });
    setFaceUvRaw(CUBE_FACE_TOP, { 0,0 }, { 1,0 }, { 1,1 }, { 0,1 });
    setFaceUvRaw(CUBE_FACE_BOTTOM, { 0,0 }, { 1,0 }, { 1,1 }, { 0,1 });

    return t;
}

static CubeTemplate makeFullUvTemplate()
{
    CubeTemplate t = CubeTemplate_Unit();
    CubeTemplate_SetAllFaceUV(t, { 0,0 }, { 1,1 });
    CubeTemplate_SetAllFaceColor(t, { 1,1,1,1 });
    return t;
}

static CubeTemplate makeSolidColorTemplate(const XMFLOAT4& c)
{
    CubeTemplate t = CubeTemplate_Unit();
    CubeTemplate_SetAllFaceUV(t, { 0,0 }, { 1,1 });
    CubeTemplate_SetAllFaceColor(t, c);
    return t;
}

static CubeTemplate makeRengaTemplate()
{
    CubeTemplate t = CubeTemplate_Unit();
    CubeTemplate_SetAllFaceUV(t, { 0,0 }, { 1,1 });

    // テクスチャに軽く“陰影”を足す（色は乗算される想定）
    CubeTemplate_SetAllFaceColor(t, { 0.95f, 0.95f, 0.95f, 1.0f });              // 基本少し暗く
    CubeTemplate_SetFaceColor(t, CUBE_FACE_TOP, { 1.05f, 1.05f, 1.05f, 1.0f }); // 上面ちょい明るく
    CubeTemplate_SetFaceColor(t, CUBE_FACE_BOTTOM, { 0.80f, 0.80f, 0.80f, 1.0f }); // 底面ちょい暗く

    return t;
}

static CubeTemplate makeRegoTemplate()
{
    CubeTemplate t = CubeTemplate_Unit();
    CubeTemplate_SetAllFaceUV(t, { 0,0 }, { 1,1 });

    // レゴっぽい“発色”の感じ（ここは好みで後からUIでいじれる）
    CubeTemplate_SetAllFaceColor(t, { 1.10f, 1.05f, 0.95f, 1.0f }); // 少し暖色寄り・明るめ
    CubeTemplate_SetFaceColor(t, CUBE_FACE_BOTTOM, { 0.85f, 0.85f, 0.85f, 1.0f });

    return t;
}


void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;

    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = g_CubeIndex;

    g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);

    g_defaultTexId = Texture_Load(L"white.png");

    Cube_RegisterKind(CUBE_KIND_LEGACY, makeLegacyTemplate());
    Cube_RegisterKind(CUBE_KIND_FULL_UV, makeFullUvTemplate());
    Cube_RegisterKind(CUBE_KIND_SOLID_RED, makeSolidColorTemplate({ 1,0,0,1 }));

    Cube_RegisterKind(CUBE_KIND_RENGA, makeRengaTemplate());
    Cube_RegisterKind(CUBE_KIND_REGO, makeRegoTemplate());

}

void Cube_Finalize()
{
    for (auto& kv : g_kinds)
    {
        SAFE_RELEASE(kv.second.vb);
    }
    g_kinds.clear();

    SAFE_RELEASE(g_pIndexBuffer);
}

void Cube_Update(double)
{
}

void Cube_Draw(XMMATRIX& mtrWorld)
{
    drawKindInternal(0, g_defaultTexId, mtrWorld, false);
}

void Cube_Draw02(int texId, const XMMATRIX& mtrWorld)
{
    drawKindInternal(0, texId, mtrWorld, false);
}

void Cube_DepthDraw(const XMMATRIX& mtrWorld)
{
    drawKindInternal(0, g_defaultTexId, mtrWorld, true);
}

AABB Cube_GetAABB(const XMFLOAT3& position)
{
    AABB aabb{};
    aabb.min = { position.x - HALF, position.y - HALF, position.z - HALF };
    aabb.max = { position.x + HALF, position.y + HALF, position.z + HALF };
    return aabb;
}
