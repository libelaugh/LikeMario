/*==============================================================================

　　  ステージ01管理[stage01_manage.cpp]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/
#include "stage01_manage.h"
#include "stage01_make.h"

#include "cube_.h"
#include "texture.h"
#include "direct3d.h"
#include"stage_cube.h"
#include"stage_map.h"
#include <vector>
#include <cfloat> // FLT_MAX
#include <fstream>
#include <string>
#include <iterator>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include <cstring>



using namespace DirectX;

namespace
{
    std::vector<StageBlock> g_blocks;

    struct StageRuntimeOffset
    {
        XMFLOAT3 position{ 0,0,0 };
        XMFLOAT3 size{ 0,0,0 };
        XMFLOAT3 rotation{ 0,0,0 };
    };
    
    std::vector<StageRuntimeOffset> g_offsets;

    /*=====================================*/
    //テクスチャ追加するときは４箇所いじる
    enum TexSlot : int
    {
        TEX_BRICK = 0,
        TEX_RED = 1,
        TEX_WHITE = 2,

        TEX_STONE0 = 10,TEX_STONE1 = 11,TEX_STONE2 = 12, TEX_STONE3 = 13,
         TEX_STONE4 = 14, TEX_STONE5 = 15, TEX_STONE6 = 16, TEX_STONE7 = 17,
        TEX_STONE8 = 18, TEX_STONE9 = 19,

        TEX_WOOD0 = 20, TEX_WOOD1 =21, TEX_WOOD2 =22, TEX_WOOD3 =23,

        TEX_V0 = 30, TEX_V1 = 31, TEX_V2 = 32, TEX_V3 = 33,
        TEX_V4 = 34, TEX_V5 = 35, TEX_V6 = 36, TEX_V7 = 37,

        TEX_CHECK0 = 40, TEX_CHECK1 = 41,
        TEX_MAX
    };

    int g_tex[TEX_MAX];//TexSlotの個数

    char g_stageJsonPath[260] = "stage01.json";


    const XMFLOAT3 kCorners[8] =
    {
        {-0.5f,-0.5f,-0.5f}, {+0.5f,-0.5f,-0.5f},
        {-0.5f,+0.5f,-0.5f}, {+0.5f,+0.5f,-0.5f},
        {-0.5f,-0.5f,+0.5f}, {+0.5f,-0.5f,+0.5f},
        {-0.5f,+0.5f,+0.5f}, {+0.5f,+0.5f,+0.5f},
    };

    void Bake(StageBlock& b, const StageRuntimeOffset& offset)
    {
        const XMFLOAT3 size{
        b.size.x + b.sizeOffset.x + offset.size.x,
        b.size.y + b.sizeOffset.y + offset.size.y,
        b.size.z + b.sizeOffset.z + offset.size.z
        };
        const XMFLOAT3 rot{
        b.rotation.x + b.rotationOffset.x + offset.rotation.x,
        b.rotation.y + b.rotationOffset.y + offset.rotation.y,
        b.rotation.z + b.rotationOffset.z + offset.rotation.z
         };
        const XMFLOAT3 pos{
        b.position.x + b.positionOffset.x + offset.position.x,
        b.position.y + b.positionOffset.y + offset.position.y,
        b.position.z + b.positionOffset.z + offset.position.z
         };
        
        XMMATRIX S = XMMatrixScaling(size.x, size.y, size.z);
        XMMATRIX R = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
        XMMATRIX T = XMMatrixTranslation(pos.x, pos.y, pos.z);
        XMMATRIX W = S * R * T;

        XMStoreFloat4x4(&b.world, W);

        XMFLOAT3 mn{ +FLT_MAX, +FLT_MAX, +FLT_MAX };
        XMFLOAT3 mx{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

        for (auto& c : kCorners)
        {
            XMVECTOR p = XMVector3TransformCoord(XMLoadFloat3(&c), W);
            XMFLOAT3 wp{};
            XMStoreFloat3(&wp, p);

            mn.x = (wp.x < mn.x) ? wp.x : mn.x;
            mn.y = (wp.y < mn.y) ? wp.y : mn.y;
            mn.z = (wp.z < mn.z) ? wp.z : mn.z;

            mx.x = (wp.x > mx.x) ? wp.x : mx.x;
            mx.y = (wp.y > mx.y) ? wp.y : mx.y;
            mx.z = (wp.z > mx.z) ? wp.z : mx.z;
        }

        b.aabb.min = mn;
        b.aabb.max = mx;
    }
}

namespace
{
    /*=============================================*/
    // 既存の enum TexSlot と g_tex[] はそのまま使う
    const char* g_texSlotNames[TEX_MAX] =
    {
        /*  0 */ "Brick",
        /*  1 */ "Red",
        /*  2 */ "White",
        /*  3 */ "Unused",
        /*  4 */ "Unused",
        /*  5 */ "Unused",
        /*  6 */ "Unused",
        /*  7 */ "Unused",
        /*  8 */ "Unused",
        /*  9 */ "Unused",
        /* 10 */ "Stone0",
        /* 11 */ "Stone1",
        /* 12 */ "Stone2",
        /* 13 */ "Stone3",
        /* 14 */ "Stone4",
        /* 15 */ "Stone5",
        /* 16 */ "Stone6",
        /* 17 */ "Stone7",
        /* 18 */ "Stone8",
        /* 19 */ "Stone9",
        /* 20 */ "Wood0",
        /* 21 */ "Wood1",
        /* 22 */ "Wood2",
        /* 23 */ "Wood3",
        /* 24 */ "Unused",
        /* 25 */ "Unused",
        /* 26 */ "Unused",
        /* 27 */ "Unused",
        /* 28 */ "Unused",
        /* 29 */ "Unused",
        /* 30 */ "V0",
        /* 31 */ "V1",
        /* 32 */ "V2",
        /* 33 */ "V3",
        /* 34 */ "V4",
        /* 35 */ "V5",
        /* 36 */ "V6",
        /* 37 */ "V7",
        /* 38 */ "Unused",
        /* 39 */ "Unused",
        /* 40 */ "Check0",
        /* 41 */ "Check1",
    };

    void ApplyTex(StageBlock& b)
    {
        // slot -> 実際の textureId に変換（描画用）
        if (b.texSlot >= 0 && b.texSlot < TEX_MAX)
            b.texId = g_tex[b.texSlot];
        else
            b.texId = g_tex[TEX_BRICK];
    }
}

int Stage01_GetTexSlotCount() { return TEX_MAX; }

const char* Stage01_GetTexSlotName(int slot)
{
    if (slot < 0 || slot >= TEX_MAX) return "Invalid";
    const char* s = g_texSlotNames[slot];
    return (s && s[0]) ? s : "Unused";
}

void Stage01_SetCurrentJsonPath(const char* filepath)
{
    if (!filepath || !filepath[0]) return;

    // 末尾は自動で '\0' になる。長すぎたら切り詰める。
    strncpy_s(g_stageJsonPath, sizeof(g_stageJsonPath), filepath, _TRUNCATE);
}

const char* Stage01_GetCurrentJsonPath()
{
    return g_stageJsonPath;
}

void Stage01_Initialize()
{
    Stage01_Initialize(Stage01_GetCurrentJsonPath());
    /*
    Cube_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Map_Initialize();


    g_blocks.clear();
    g_blocks.reserve(4096);

    g_tex[TEX_BRICK] = Texture_Load(L"texture/rengaBlock.png");
    g_tex[TEX_RED] = Texture_Load(L"red.png");

    if (Stage01_LoadJson("stage01.json"))
    {
        return;
    }


    const StageRow* rows = nullptr;
    int count = 0;
    Stage01_MakeTable(&rows, &count);

    for (int i = 0; i < count; ++i)
    {
        const StageRow& r = rows[i];

        StageBlock b{};
        //b.texId = (r.texSlot >= 0 && r.texSlot < TEX_MAX) ? g_tex[r.texSlot] : g_tex[TEX_BRICK];
        b.kind = r.kind;
        b.texSlot = r.texSlot;
        ApplyTex(b);

        b.position = { r.px, r.py, r.pz };
        b.size = { r.sx, r.sy, r.sz };
        b.rotation = { r.rx, r.ry, r.rz };

        Bake(b);
        g_blocks.push_back(b);
    }*/
}

void Stage01_Initialize(const char* jsonPath)
{
    if (jsonPath && jsonPath[0])
        Stage01_SetCurrentJsonPath(jsonPath);

    Cube_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Map_Initialize();

    g_blocks.clear();
    g_offsets.clear();
    g_blocks.reserve(4096);
    g_offsets.reserve(4096);

    std::fill(std::begin(g_tex), std::end(g_tex), -1);


/*==============================================*/
    g_tex[TEX_BRICK] = Texture_Load(L"texture/rengaBlock.png");
    g_tex[TEX_RED] = Texture_Load(L"texture/red.png");
    g_tex[TEX_WHITE] = Texture_Load(L"white.png");

    g_tex[TEX_STONE0] = Texture_Load(L"texture/stone0.png"); g_tex[TEX_STONE1] = Texture_Load(L"texture/stone1.png");
    g_tex[TEX_STONE2] = Texture_Load(L"texture/stone2.png"); g_tex[TEX_STONE3] = Texture_Load(L"texture/stone3.png");
    g_tex[TEX_STONE4] = Texture_Load(L"texture/stone4.png"); g_tex[TEX_STONE5] = Texture_Load(L"texture/stone5.png");
    g_tex[TEX_STONE6] = Texture_Load(L"texture/stone6.png"); g_tex[TEX_STONE7] = Texture_Load(L"texture/stone7.png");
    g_tex[TEX_STONE8] = Texture_Load(L"texture/stone8.jpg"); g_tex[TEX_STONE9] = Texture_Load(L"texture/stone9.jpg");

    g_tex[TEX_WOOD0] = Texture_Load(L"texture/wood0.png"); g_tex[TEX_WOOD1] = Texture_Load(L"texture/wood1.png");
    g_tex[TEX_WOOD2] = Texture_Load(L"texture/wood2.png"); g_tex[TEX_WOOD3] = Texture_Load(L"texture/wood3.png");

    g_tex[TEX_V0] = Texture_Load(L"texture/v0.png"); g_tex[TEX_V1] = Texture_Load(L"texture/v1.png");
    g_tex[TEX_V2] = Texture_Load(L"texture/v2.png"); g_tex[TEX_V3] = Texture_Load(L"texture/v3.png");
    g_tex[TEX_V4] = Texture_Load(L"texture/v4.png"); g_tex[TEX_V5] = Texture_Load(L"texture/v5.png");
    g_tex[TEX_V6] = Texture_Load(L"texture/v6.jpg"); g_tex[TEX_V7] = Texture_Load(L"texture/v7.png");

    g_tex[TEX_CHECK0] = Texture_Load(L"texture/check0.png"); g_tex[TEX_CHECK1] = Texture_Load(L"texture/check1.jpg");

    // まずは指定 json を読む（例: stage02.json）
    if (Stage01_LoadJson(Stage01_GetCurrentJsonPath()))
        return;

    // stage01.json が無いときだけ、従来の初期配置を入れる
    if (std::strcmp(Stage01_GetCurrentJsonPath(), "stage01.json") != 0)
        return;

    const StageRow* rows = nullptr;
    int count = 0;
    Stage01_MakeTable(&rows, &count);

    for (int i = 0; i < count; ++i)
    {
        const StageRow& r = rows[i];

        StageBlock b{};
        b.kind = r.kind;
        b.texSlot = r.texSlot;
        ApplyTex(b);

        b.position = { r.px, r.py, r.pz };
        b.size = { r.sx, r.sy, r.sz };
        b.rotation = { r.rx, r.ry, r.rz };

        Stage01_Add(b, true);
    }
}

void Stage01_Finalize()
{
    Cube_Finalize();
    Map_Finalize();

    g_blocks.clear();
    g_offsets.clear();
}

void Stage01_Update(double elapsedTime)
{
    Cube_Update(elapsedTime);
}

void Stage01_Draw()
{
    for (const auto& b : g_blocks)
    {
        CubeBlock cb{};
        cb.kind = b.kind;
        cb.texId = b.texId;
        cb.world = b.world;// Bake済みのworldをそのまま使う
        Cube_DrawBlock(cb);
    }
    /*
    for (const auto& b : g_blocks)
    {
        XMMATRIX W = XMLoadFloat4x4(&b.world);
        Cube_Draw02(b.texId, W);
    }*/
}

void Stage01_DepthDraw()
{
    for (const auto& b : g_blocks)
    {
        CubeBlock cb{};
        cb.kind = b.kind;
        cb.texId = b.texId;
        cb.world = b.world;
        Cube_DepthDrawBlock(cb);
    }
    /*
    for (const auto& b : g_blocks)
    {
        XMMATRIX W = XMLoadFloat4x4(&b.world);
        Cube_DepthDraw(W);
    }*/
}

// ===== ImGui向け（今は未使用でもOK）=====
int Stage01_GetCount() { return (int)g_blocks.size(); }

const StageBlock* Stage01_Get(int i)
{
    if (i < 0 || i >= (int)g_blocks.size()) return nullptr;
    return &g_blocks[i];
}

StageBlock* Stage01_GetMutable(int i)
{
    if (i < 0 || i >= (int)g_blocks.size()) return nullptr;
    return &g_blocks[i];
}

void Stage01_RebuildObject(int i)
{
    if (i < 0 || i >= (int)g_blocks.size()) return;
    ApplyTex(g_blocks[i]);
    Bake(g_blocks[i], g_offsets[i]);
}

void Stage01_RebuildAll()
{
    for (size_t i = 0; i < g_blocks.size(); ++i) {
        Bake(g_blocks[i], g_offsets[i]);
        ApplyTex(g_blocks[i]);
    }
}

int Stage01_Add(const StageBlock& b, bool bake)
{
    g_blocks.push_back(b);
    g_offsets.emplace_back();
    if (bake) {
        ApplyTex(g_blocks.back());
        Bake(g_blocks.back(), g_offsets.back());
    }
    return (int)g_blocks.size() - 1;
}

void Stage01_Remove(int i)
{
    if (i < 0 || i >= (int)g_blocks.size()) return;
    g_blocks.erase(g_blocks.begin() + i);
    g_offsets.erase(g_offsets.begin() + i);
}

void Stage01_Clear()
{
    g_blocks.clear();
    g_offsets.clear();
}

bool Stage01_AddObjectTransform(int index,
    const DirectX::XMFLOAT3& positionDelta,
    const DirectX::XMFLOAT3& sizeDelta,
    const DirectX::XMFLOAT3& rotationDelta)
{
    if (index < 0 || index >= (int)g_offsets.size()) return false;
    StageRuntimeOffset & offset = g_offsets[index];
    
    offset.position.x += positionDelta.x;
    offset.position.y += positionDelta.y;
    offset.position.z += positionDelta.z;
    offset.size.x += sizeDelta.x;
    offset.size.y += sizeDelta.y;
    offset.size.z += sizeDelta.z;
    offset.rotation.x += rotationDelta.x;
    offset.rotation.y += rotationDelta.y;
    offset.rotation.z += rotationDelta.z;
    Stage01_RebuildObject(index);
    return true;
}


// ===== JSON Save/Load =====
namespace
{
    static size_t FindMatchingBracket(const std::string& s, size_t openPos)
    {
        // openPos は '[' の位置
        int depth = 0;
        for (size_t i = openPos; i < s.size(); ++i)
        {
            const char c = s[i];
            if (c == '[') ++depth;
            else if (c == ']')
            {
                --depth;
                if (depth == 0) return i; // 対応する ']' を発見
            }
        }
        return std::string::npos;
    }

    static size_t FindMatchingBrace(const std::string& s, size_t openPos)
    {
        // openPos は '{' の位置
        int depth = 0;
        for (size_t i = openPos; i < s.size(); ++i)
        {
            const char c = s[i];
            if (c == '{') ++depth;
            else if (c == '}')
            {
                --depth;
                if (depth == 0) return i; // 対応する '}' を発見
            }
        }
        return std::string::npos;
    }

    static bool ExtractVec2(const std::string& s, const char* key, DirectX::XMFLOAT2& out)
    {
        const std::string k = std::string("\"") + key + "\"";
        size_t pos = s.find(k);
        if (pos == std::string::npos) return false;
        pos = s.find('[', pos);
        if (pos == std::string::npos) return false;

        const char* p = s.c_str() + pos + 1;
        char* end = nullptr;

        double x = std::strtod(p, &end);
        if (end == p) return false;
        p = end; while (*p && (*p == ',' || std::isspace((unsigned char)*p))) ++p;

        double y = std::strtod(p, &end);
        if (end == p) return false;

        out.x = (float)x; out.y = (float)y;
        return true;
    }

    static bool ExtractVec4(const std::string& s, const char* key, DirectX::XMFLOAT4& out)
    {
        const std::string k = std::string("\"") + key + "\"";
        size_t pos = s.find(k);
        if (pos == std::string::npos) return false;
        pos = s.find('[', pos);
        if (pos == std::string::npos) return false;

        const char* p = s.c_str() + pos + 1;
        char* end = nullptr;

        double x = std::strtod(p, &end);
        if (end == p) return false;
        p = end; while (*p && (*p == ',' || std::isspace((unsigned char)*p))) ++p;

        double y = std::strtod(p, &end);
        if (end == p) return false;
        p = end; while (*p && (*p == ',' || std::isspace((unsigned char)*p))) ++p;

        double z = std::strtod(p, &end);
        if (end == p) return false;
        p = end; while (*p && (*p == ',' || std::isspace((unsigned char)*p))) ++p;

        double w = std::strtod(p, &end);
        if (end == p) return false;

        out.x = (float)x; out.y = (float)y; out.z = (float)z; out.w = (float)w;
        return true;
    }

    static bool ExtractInt(const std::string& s, const char* key, int& out)
    {
        const std::string k = std::string("\"") + key + "\"";
        size_t pos = s.find(k);
        if (pos == std::string::npos) return false;
        pos = s.find(':', pos);
        if (pos == std::string::npos) return false;

        const char* p = s.c_str() + pos + 1;
        while (*p && std::isspace((unsigned char)*p)) ++p;

        char* end = nullptr;
        long v = std::strtol(p, &end, 10);
        if (end == p) return false;

        out = (int)v;
        return true;
    }

    static bool ExtractVec3(const std::string& s, const char* key, DirectX::XMFLOAT3& out)
    {
        const std::string k = std::string("\"") + key + "\"";
        size_t pos = s.find(k);
        if (pos == std::string::npos) return false;
        pos = s.find('[', pos);
        if (pos == std::string::npos) return false;

        const char* p = s.c_str() + pos + 1;
        char* end = nullptr;

        double x = std::strtod(p, &end);
        if (end == p) return false;
        p = end; while (*p && (*p == ',' || std::isspace((unsigned char)*p))) ++p;

        double y = std::strtod(p, &end);
        if (end == p) return false;
        p = end; while (*p && (*p == ',' || std::isspace((unsigned char)*p))) ++p;

        double z = std::strtod(p, &end);
        if (end == p) return false;

        out.x = (float)x; out.y = (float)y; out.z = (float)z;
        return true;
    }
}

static void GetFaceUvMinMax(const CubeFaceDesc& fd, DirectX::XMFLOAT2& outUvMin, DirectX::XMFLOAT2& outUvMax)
{
    outUvMin.x = outUvMin.y = +FLT_MAX;
    outUvMax.x = outUvMax.y = -FLT_MAX;

    for (int i = 0; i < CUBE_VERTS_PER_FACE; ++i)
    {
        const float u = fd.uv[i].x;
        const float v = fd.uv[i].y;
        outUvMin.x = (u < outUvMin.x) ? u : outUvMin.x;
        outUvMin.y = (v < outUvMin.y) ? v : outUvMin.y;
        outUvMax.x = (u > outUvMax.x) ? u : outUvMax.x;
        outUvMax.y = (v > outUvMax.y) ? v : outUvMax.y;
    }
}

static void WriteKindsJson(std::ofstream& ofs)
{
    int kinds[512];
    int total = Cube_GetKindList(kinds, 512);
    if (total > 1) std::sort(kinds, kinds + total);

    // ちゃんと「書くkind」だけ数える（コンマ崩れ防止）
    std::vector<int> valid;
    valid.reserve((size_t)total);

    for (int i = 0; i < total; ++i)
    {
        CubeTemplate tpl{};
        if (Cube_TryGetKindTemplate(kinds[i], tpl))
            valid.push_back(kinds[i]);
    }

    ofs << "  \"kinds\": [\n";

    for (int i = 0; i < (int)valid.size(); ++i)
    {
        const int kind = valid[i];
        CubeTemplate tpl{};
        Cube_TryGetKindTemplate(kind, tpl);

        ofs << "    {\"kind\":" << kind << ",\"faces\":[\n";

        for (int f = 0; f < CUBE_FACE_COUNT; ++f)
        {
            const CubeFaceDesc& fd = tpl.face[f];

            DirectX::XMFLOAT2 uvMin{}, uvMax{};
            GetFaceUvMinMax(fd, uvMin, uvMax);

            const DirectX::XMFLOAT4 c = fd.color[0]; // 現状UIは面単色運用なので先頭だけでOK
            const DirectX::XMFLOAT3 n = fd.normal;

            ofs << "      {"
                << "\"uvMin\":[" << uvMin.x << "," << uvMin.y << "],"
                << "\"uvMax\":[" << uvMax.x << "," << uvMax.y << "],"
                << "\"color\":[" << c.x << "," << c.y << "," << c.z << "," << c.w << "],"
                << "\"normal\":[" << n.x << "," << n.y << "," << n.z << "]"
                << "}";

            if (f != CUBE_FACE_COUNT - 1) ofs << ",";
            ofs << "\n";
        }

        ofs << "    ]}";
        if (i != (int)valid.size() - 1) ofs << ",";
        ofs << "\n";
    }

    ofs << "  ],\n";
}

static bool ParseKindsJson(const std::string& txt)
{
    // kinds は任意（古いjson互換）
    size_t pKinds = txt.find("\"kinds\"");
    if (pKinds == std::string::npos) return true;

    size_t lb = txt.find('[', pKinds);
    if (lb == std::string::npos) return false;

    size_t rb = FindMatchingBracket(txt, lb);
    if (rb == std::string::npos) return false;

    size_t cur = lb + 1;

    while (true)
    {
        size_t ob = txt.find('{', cur);
        if (ob == std::string::npos || ob > rb) break;

        size_t cb = FindMatchingBrace(txt, ob);
        if (cb == std::string::npos || cb > rb) return false;

        std::string kindObj = txt.substr(ob, cb - ob + 1);

        int kind = 0;
        if (!ExtractInt(kindObj, "kind", kind))
        {
            cur = cb + 1;
            continue;
        }

        CubeTemplate tpl = CubeTemplate_Unit(); // pos は unit 前提（今のUI仕様に一致）

        // faces
        size_t pFaces = kindObj.find("\"faces\"");
        if (pFaces != std::string::npos)
        {
            size_t flb = kindObj.find('[', pFaces);
            size_t frb = (flb != std::string::npos) ? FindMatchingBracket(kindObj, flb) : std::string::npos;
            if (flb == std::string::npos || frb == std::string::npos) return false;

            int faceIndex = 0;
            size_t fcur = flb + 1;

            while (faceIndex < CUBE_FACE_COUNT)
            {
                size_t fob = kindObj.find('{', fcur);
                if (fob == std::string::npos || fob > frb) break;

                size_t fcb = FindMatchingBrace(kindObj, fob);
                if (fcb == std::string::npos || fcb > frb) return false;

                std::string faceObj = kindObj.substr(fob, fcb - fob + 1);

                DirectX::XMFLOAT2 uvMin{}, uvMax{};
                DirectX::XMFLOAT4 col{};
                DirectX::XMFLOAT3 nor{};

                bool hasUvMin = ExtractVec2(faceObj, "uvMin", uvMin);
                bool hasUvMax = ExtractVec2(faceObj, "uvMax", uvMax);
                bool hasCol = ExtractVec4(faceObj, "color", col);
                bool hasNor = ExtractVec3(faceObj, "normal", nor);

                if (hasUvMin && hasUvMax)
                    CubeTemplate_SetFaceUV(tpl, (CubeFace)faceIndex, uvMin, uvMax);
                if (hasCol)
                    CubeTemplate_SetFaceColor(tpl, (CubeFace)faceIndex, col);
                if (hasNor)
                    tpl.face[faceIndex].normal = nor;

                ++faceIndex;
                fcur = fcb + 1;
            }
        }

        // 反映（既存kindは Update、なければ Register）
        CubeTemplate dummy{};
        if (Cube_TryGetKindTemplate(kind, dummy))
            Cube_UpdateKind(kind, tpl);
        else
            Cube_RegisterKind(kind, tpl);

        cur = cb + 1;
    }

    return true;
}

bool Stage01_SaveJson(const char* filepath)
{
    if (!filepath || !filepath[0]) return false;

    Stage01_SetCurrentJsonPath(filepath);

    std::ofstream ofs(filepath, std::ios::binary);
    if (!ofs) return false;

    ofs << std::fixed << std::setprecision(3);

    ofs << "{\n";
    ofs << "  \"version\": 2,\n";

    // Kind(見た目テンプレ)も一緒に保存
    WriteKindsJson(ofs);

    ofs << "  \"blocks\": [\n";

    const int n = Stage01_GetCount();
    for (int i = 0; i < n; ++i)
    {
        const StageBlock* b = Stage01_Get(i);
        if (!b) continue;

        ofs << "    {"
            << "\"kind\":" << b->kind << ","
            << "\"texSlot\":" << b->texSlot << ","
            << "\"position\":[" << b->position.x << "," << b->position.y << "," << b->position.z << "],"
            << "\"size\":[" << b->size.x << "," << b->size.y << "," << b->size.z << "],"
            << "\"rotation\":[" << b->rotation.x << "," << b->rotation.y << "," << b->rotation.z << "]"
            << "}";

        if (i != n - 1) ofs << ",";
        ofs << "\n";
    }

    ofs << "  ]\n";
    ofs << "}\n";

    return true;
}

bool Stage01_LoadJson(const char* filepath)
{
    if (!filepath || !filepath[0]) return false;

    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs) return false;

    std::string txt((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    // kinds は任意。あれば先に反映してから blocks を読む
    if (!ParseKindsJson(txt))
        return false;

    size_t pBlocks = txt.find("\"blocks\"");
    if (pBlocks == std::string::npos) return false;

    size_t lb = txt.find('[', pBlocks);
    if (lb == std::string::npos) return false;

    size_t rb = FindMatchingBracket(txt, lb);
    if (rb == std::string::npos) return false;

    std::vector<StageBlock> temp; // 失敗時に消えないよう一旦 temp に読む
    temp.reserve(1024);

    size_t cur = lb + 1;
    while (true)
    {
        size_t ob = txt.find('{', cur);
        if (ob == std::string::npos || ob > rb) break;

        size_t cb = FindMatchingBrace(txt, ob);
        if (cb == std::string::npos || cb > rb) return false;

        std::string obj = txt.substr(ob, cb - ob + 1);

        StageBlock b{};
        int kind = 0, slot = 0;
        DirectX::XMFLOAT3 v{};

        if (ExtractInt(obj, "kind", kind)) b.kind = kind;
        if (ExtractInt(obj, "texSlot", slot)) b.texSlot = slot;

        if (ExtractVec3(obj, "position", v)) b.position = v;
        if (ExtractVec3(obj, "size", v))     b.size = v;
        if (ExtractVec3(obj, "rotation", v)) b.rotation = v;

        temp.push_back(b);
        cur = cb + 1;
    }

    Stage01_Clear();
    for (auto& b : temp)
        Stage01_Add(b, true); // ApplyTex + Bake

    Stage01_SetCurrentJsonPath(filepath);

    return true;
}

StageSwitchResult Stage01_SwitchStage(const char* jsonPath, bool createEmptyIfMissing)
{
    if (!jsonPath || !jsonPath[0])
        return STAGE_SWITCH_FAILED;

    // まずはロードを試す（成功したらそれで終わり）
    if (Stage01_LoadJson(jsonPath))
    {
        // LoadJson内で SetCurrentJsonPath してるなら不要だが、保険で呼んでOK
        Stage01_SetCurrentJsonPath(jsonPath);
        return STAGE_SWITCH_LOADED;
    }

    // 失敗したら、空ステージに切替（新規作成として扱える）
    if (createEmptyIfMissing)
    {
        Stage01_Clear();                 // 今のブロックを全消し
        Stage01_SetCurrentJsonPath(jsonPath);
        return STAGE_SWITCH_CREATED_EMPTY;
    }

    return STAGE_SWITCH_FAILED;
}
