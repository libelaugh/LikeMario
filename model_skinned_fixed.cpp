/*==============================================================================

    Skinned Model (CPU Skinning) [model_skinned.cpp]
                                                            Author : Tanaka Kouki
                                                            Date   : 2026/01/01
--------------------------------------------------------------------------------

    シェーダを増やさず、既存の shader3d で描画できるように
    CPU 側でスキニングして VertexBuffer を毎フレーム更新する簡易実装。

==============================================================================*/

#include "model_skinned_fixed.h"
// --- DEFENSIVE INCLUDES (ヘッダの include guard 衝突や include 順の問題を避ける) ---
#include "collision.h"
#include <unordered_map>
#include <string>
#include "direct3d.h"
#include "texture.h"
#include "shader3d.h"
#include "WICTextureLoader11.h"
#include "shader_depth.h"
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cmath>

using namespace DirectX;

// 既存の shader3d の InputLayout に合わせるためのフォーマット
struct SkinnedVertex3d
{
    XMFLOAT3 position;
    XMFLOAT3 normalVector;
    XMFLOAT4 color;
    XMFLOAT2 texcoord;
};


struct BaseVertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 uv;
};

struct Influence4
{
    uint16_t idx[4] = { 0,0,0,0 };
    float    w[4] = { 0,0,0,0 };

    void Add(uint16_t boneIndex, float weight)
    {
        // 空きがあるなら入れる
        for (int i = 0; i < 4; ++i)
        {
            if (w[i] == 0.0f)
            {
                idx[i] = boneIndex;
                w[i] = weight;
                return;
            }
        }
        // 一番小さい重みより大きければ交換
        int minI = 0;
        for (int i = 1; i < 4; ++i)
        {
            if (w[i] < w[minI]) minI = i;
        }
        if (weight > w[minI])
        {
            idx[minI] = boneIndex;
            w[minI] = weight;
        }
    }

    void Normalize()
    {
        float sum = w[0] + w[1] + w[2] + w[3];
        if (sum <= 0.0f)
        {
            // 骨情報がない頂点は、そのまま（全0）にしておく
            // → Update() 側で「バインドポーズのまま」にフォールバックする
            return;
        }
        float inv = 1.0f / sum;
        w[0] *= inv; w[1] *= inv; w[2] *= inv; w[3] *= inv;
    }
};

struct SKINNED_MESH
{
    ID3D11Buffer* vb = nullptr; // dynamic
    ID3D11Buffer* ib = nullptr; // static

    std::vector<BaseVertex> baseVerts;
    std::vector<Influence4> influences;

    uint32_t numIndices = 0;
    uint32_t materialIndex = 0;

    // CPU skinning 用の一時作業領域
    std::vector<SkinnedVertex3d> skinnedVerts;
};
struct SKINNED_MODEL
{
    const aiScene* scene = nullptr;

    std::vector<SKINNED_MESH> meshes;
    std::unordered_map<std::string, ID3D11ShaderResourceView*> textures;

    // bone
    std::unordered_map<std::string, uint32_t> boneMap; // name->index
    std::vector<XMMATRIX> boneOffset;                  // aiBone::mOffsetMatrix
    std::vector<XMMATRIX> boneFinal;                   // 最終行列（CPU skinning で使う）

    XMMATRIX globalInverse = XMMatrixIdentity();

    // 簡易キャッシュ
    int lastAnimIndex = -1;
    double lastAnimTime = -1.0;

    // collision
    AABB local_aabb{};

    float importScale = 1.0f;
};

static int g_TextureWhite = -1;

//------------------------------------------------------------------------------
// Utility
//------------------------------------------------------------------------------
static XMMATRIX AiToXM(const aiMatrix4x4& m)
{
    // aiMatrix4x4 は row-major の集まり (a1..d4)
    return XMMATRIX(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}

static XMMATRIX CalcNodeLocalTransform(const aiNode* node)
{
    return AiToXM(node->mTransformation);
}

static const aiNodeAnim* FindNodeAnim(const aiAnimation* anim, const std::string& nodeName)
{
    for (unsigned int i = 0; i < anim->mNumChannels; ++i)
    {
        const aiNodeAnim* channel = anim->mChannels[i];
        if (nodeName == channel->mNodeName.C_Str())
            return channel;
    }
    return nullptr;
}

static unsigned int FindKeyIndex(double animTime, const aiVectorKey* keys, unsigned int numKeys)
{
    for (unsigned int i = 0; i + 1 < numKeys; ++i)
    {
        if (animTime < keys[i + 1].mTime)
            return i;
    }
    return (std::max)(0u, numKeys - 1);

}

static unsigned int FindKeyIndex(double animTime, const aiQuatKey* keys, unsigned int numKeys)
{
    for (unsigned int i = 0; i + 1 < numKeys; ++i)
    {
        if (animTime < keys[i + 1].mTime)
            return i;
    }
    return (std::max)(0u, numKeys - 1);

}

static XMMATRIX InterpolatePosition(double animTime, const aiNodeAnim* channel)
{
    if (!channel || channel->mNumPositionKeys == 0)
        return XMMatrixIdentity();

    if (channel->mNumPositionKeys == 1)
    {
        const aiVector3D& v = channel->mPositionKeys[0].mValue;
        return XMMatrixTranslation(v.x, v.y, v.z);
    }

    unsigned int idx = FindKeyIndex(animTime, channel->mPositionKeys, channel->mNumPositionKeys);
    unsigned int next = idx + 1;

    double t1 = channel->mPositionKeys[idx].mTime;
    double t2 = channel->mPositionKeys[next].mTime;
    float factor = (t2 > t1) ? (float)((animTime - t1) / (t2 - t1)) : 0.0f;

    const aiVector3D& a = channel->mPositionKeys[idx].mValue;
    const aiVector3D& b = channel->mPositionKeys[next].mValue;

    aiVector3D out = a + (b - a) * factor;
    return XMMatrixTranslation(out.x, out.y, out.z);
}

static XMMATRIX InterpolateScaling(double animTime, const aiNodeAnim* channel)
{
    if (!channel || channel->mNumScalingKeys == 0)
        return XMMatrixIdentity();

    if (channel->mNumScalingKeys == 1)
    {
        const aiVector3D& v = channel->mScalingKeys[0].mValue;
        return XMMatrixScaling(v.x, v.y, v.z);
    }

    unsigned int idx = FindKeyIndex(animTime, channel->mScalingKeys, channel->mNumScalingKeys);
    unsigned int next = idx + 1;

    double t1 = channel->mScalingKeys[idx].mTime;
    double t2 = channel->mScalingKeys[next].mTime;
    float factor = (t2 > t1) ? (float)((animTime - t1) / (t2 - t1)) : 0.0f;

    const aiVector3D& a = channel->mScalingKeys[idx].mValue;
    const aiVector3D& b = channel->mScalingKeys[next].mValue;

    aiVector3D out = a + (b - a) * factor;
    return XMMatrixScaling(out.x, out.y, out.z);
}

static XMMATRIX InterpolateRotation(double animTime, const aiNodeAnim* channel)
{
    if (!channel || channel->mNumRotationKeys == 0)
        return XMMatrixIdentity();

    if (channel->mNumRotationKeys == 1)
    {
        const aiQuaternion& q = channel->mRotationKeys[0].mValue;
        XMVECTOR rot = XMVectorSet(q.x, q.y, q.z, q.w);
        return XMMatrixRotationQuaternion(rot);
    }

    unsigned int idx = FindKeyIndex(animTime, channel->mRotationKeys, channel->mNumRotationKeys);
    unsigned int next = idx + 1;

    double t1 = channel->mRotationKeys[idx].mTime;
    double t2 = channel->mRotationKeys[next].mTime;
    float factor = (t2 > t1) ? (float)((animTime - t1) / (t2 - t1)) : 0.0f;

    const aiQuaternion& a = channel->mRotationKeys[idx].mValue;
    const aiQuaternion& b = channel->mRotationKeys[next].mValue;

    aiQuaternion out;
    aiQuaternion::Interpolate(out, a, b, factor);
    out.Normalize();

    XMVECTOR rot = XMVectorSet(out.x, out.y, out.z, out.w);
    return XMMatrixRotationQuaternion(rot);
}

static void ReadNodeHierarchy(
    double animTime,
    const aiNode* node,
    const XMMATRIX& parentTransform,
    const aiAnimation* anim,
    const std::unordered_map<std::string, uint32_t>& boneMap,
    const std::vector<XMMATRIX>& boneOffset,
    const XMMATRIX& globalInverse,
    std::vector<XMMATRIX>& outFinal
)
{
    std::string nodeName = node->mName.C_Str();

    XMMATRIX nodeTransform = CalcNodeLocalTransform(node);

    const aiNodeAnim* channel = FindNodeAnim(anim, nodeName);
    if (channel)
    {
        // row-vector で「pos * S * R * T」になるようにする
        XMMATRIX S = InterpolateScaling(animTime, channel);
        XMMATRIX R = InterpolateRotation(animTime, channel);
        XMMATRIX T = InterpolatePosition(animTime, channel);
        nodeTransform = S * R * T;
    }

    // row-vector慣例：子のローカルを先にかけて、親の変換を後にかける
    XMMATRIX globalTransform = nodeTransform * parentTransform;

    auto it = boneMap.find(nodeName);
    if (it != boneMap.end())
    {
        uint32_t boneIndex = it->second;
        // Assimp定番を row-vector に直した形
        outFinal[boneIndex] = boneOffset[boneIndex] * globalTransform * globalInverse;
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ReadNodeHierarchy(animTime, node->mChildren[i], globalTransform, anim, boneMap, boneOffset, globalInverse, outFinal);
    }
}

static void ReadNodeHierarchyBindPose(
    const aiNode* node,
    const DirectX::XMMATRIX& parentTransform,
    const std::unordered_map<std::string, uint32_t>& boneMap,
    const std::vector<DirectX::XMMATRIX>& boneOffset,
    const DirectX::XMMATRIX& globalInverse,
    std::vector<DirectX::XMMATRIX>& outFinal
)
{
    using namespace DirectX;

    std::string nodeName = node->mName.C_Str();

    // 読み込み時（bind/rest）のローカル行列
    XMMATRIX nodeTransform = CalcNodeLocalTransform(node);

    // row-vector 系なので「子 * 親」
    XMMATRIX globalTransform = nodeTransform * parentTransform;

    auto it = boneMap.find(nodeName);
    if (it != boneMap.end())
    {
        uint32_t boneIndex = it->second;
        if (boneIndex < outFinal.size())
        {
            outFinal[boneIndex] = boneOffset[boneIndex] * globalTransform * globalInverse;
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ReadNodeHierarchyBindPose(
            node->mChildren[i],
            globalTransform,
            boneMap,
            boneOffset,
            globalInverse,
            outFinal
        );
    }
}


//------------------------------------------------------------------------------
// Texture helper (model.cpp とほぼ同じ)
//------------------------------------------------------------------------------
static void LoadEmbeddedTextures(SKINNED_MODEL* model)
{
    for (unsigned int i = 0; i < model->scene->mNumTextures; ++i)
    {
        aiTexture* aitexture = model->scene->mTextures[i];

        ID3D11ShaderResourceView* texture = nullptr;
        ID3D11Resource* resource = nullptr;

        CreateWICTextureFromMemory(
            Direct3D_GetDevice(),
            Direct3D_GetContext(),
            (const uint8_t*)aitexture->pcData,
            (size_t)aitexture->mWidth,
            &resource,
            &texture);

        assert(texture);
        if (resource) resource->Release();

        model->textures[aitexture->mFilename.data] = texture;
    }
}

static void LoadExternalTextures(SKINNED_MODEL* model, const char* fileName)
{
    const std::string modelPath(fileName);
    size_t pos = modelPath.find_last_of("/\\");
    std::string directory = (pos != std::string::npos) ? modelPath.substr(0, pos) : "";

    for (unsigned int m = 0; m < model->scene->mNumMeshes; ++m)
    {
        aiMesh* mesh = model->scene->mMeshes[m];
        aiMaterial* mat = model->scene->mMaterials[mesh->mMaterialIndex];

        aiString filename;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &filename);
        if (filename.length == 0) continue;
        if (model->textures.count(filename.C_Str())) continue;

        std::string texfilename = directory + "/" + filename.C_Str();

        int len = MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, nullptr, 0);
        std::vector<wchar_t> wpath((size_t)len);
        MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, wpath.data(), len);

        ID3D11ShaderResourceView* texture = nullptr;
        ID3D11Resource* resource = nullptr;

        CreateWICTextureFromFile(
            Direct3D_GetDevice(),
            Direct3D_GetContext(),
            wpath.data(),
            &resource,
            &texture);

        if (!texture)
            continue;

        if (resource) resource->Release();
        model->textures[filename.C_Str()] = texture;
    }
}

//------------------------------------------------------------------------------
// Load
//------------------------------------------------------------------------------
SKINNED_MODEL* SkinnedModel_Load(const char* fileName, float scale, bool isBrender)
{
    SKINNED_MODEL* model = new SKINNED_MODEL;

    model->importScale = scale;

    model->scene = aiImportFile(fileName, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);
    assert(model->scene);

    // グローバル逆行列
    XMMATRIX root = AiToXM(model->scene->mRootNode->mTransformation);
    model->globalInverse = XMMatrixInverse(nullptr, root);

    // テクスチャ
    if (g_TextureWhite < 0) g_TextureWhite = Texture_Load(L"white.png");
    LoadEmbeddedTextures(model);
    LoadExternalTextures(model, fileName);

    // メッシュ
    model->meshes.resize(model->scene->mNumMeshes);

    bool aabbInit = false;

    for (unsigned int m = 0; m < model->scene->mNumMeshes; ++m)
    {
        aiMesh* mesh = model->scene->mMeshes[m];
        SKINNED_MESH& out = model->meshes[m];
        out.materialIndex = mesh->mMaterialIndex;

        out.baseVerts.resize(mesh->mNumVertices);
        out.influences.resize(mesh->mNumVertices);
        out.skinnedVerts.resize(mesh->mNumVertices);

        // base vertices
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
        {
            BaseVertex bv{};

            if (isBrender)
            {
                bv.position = XMFLOAT3(mesh->mVertices[v].x, mesh->mVertices[v].z, -mesh->mVertices[v].y);
                //bv.position = XMFLOAT3(mesh->mVertices[v].x * scale, mesh->mVertices[v].z * scale, -mesh->mVertices[v].y * scale);
                bv.normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].z, -mesh->mNormals[v].y);
            }
            else
            {
                bv.position = XMFLOAT3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
                //bv.position = XMFLOAT3(mesh->mVertices[v].x * scale, mesh->mVertices[v].y * scale, mesh->mVertices[v].z * scale);
                bv.normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
            }

            if (mesh->mTextureCoords[0])
                bv.uv = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
            else
                bv.uv = XMFLOAT2(0, 0);

            out.baseVerts[v] = bv;

            // local AABB (bind pose)
            if (!aabbInit)
            {
                model->local_aabb.min = bv.position;
                model->local_aabb.max = bv.position;
                aabbInit = true;
            }
            else
            {
                model->local_aabb.min.x = (std::min)(model->local_aabb.min.x, bv.position.x);
                model->local_aabb.min.y = (std::min)(model->local_aabb.min.y, bv.position.y);
                model->local_aabb.min.z = (std::min)(model->local_aabb.min.z, bv.position.z);
                model->local_aabb.max.x = (std::max)(model->local_aabb.max.x, bv.position.x);
                model->local_aabb.max.y = (std::max)(model->local_aabb.max.y, bv.position.y);
                model->local_aabb.max.z = (std::max)(model->local_aabb.max.z, bv.position.z);
            }

            // 初期はバインドポーズをそのまま入れておく
            out.skinnedVerts[v].position = bv.position;
            out.skinnedVerts[v].normalVector = bv.normal;
            out.skinnedVerts[v].texcoord = bv.uv;
            out.skinnedVerts[v].color = XMFLOAT4(1, 1, 1, 1);
        }

        // bones → 頂点影響のみを記録
        for (unsigned int b = 0; b < mesh->mNumBones; ++b)
        {
            aiBone* bone = mesh->mBones[b];
            std::string boneName = bone->mName.C_Str();

            uint32_t boneIndex = 0;
            auto it = model->boneMap.find(boneName);
            if (it == model->boneMap.end())
            {
                boneIndex = (uint32_t)model->boneMap.size();
                model->boneMap[boneName] = boneIndex;

                model->boneOffset.push_back(AiToXM(bone->mOffsetMatrix));
                model->boneFinal.push_back(XMMatrixIdentity());
            }
            else
            {
                boneIndex = it->second;
            }

            for (unsigned int w = 0; w < bone->mNumWeights; ++w)
            {
                const aiVertexWeight& vw = bone->mWeights[w];
                out.influences[vw.mVertexId].Add((uint16_t)boneIndex, vw.mWeight);
            }
        }

        for (auto& inf : out.influences)
            inf.Normalize();

        // index buffer
        std::vector<uint32_t> indices;
        indices.reserve(mesh->mNumFaces * 3);

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            assert(face.mNumIndices == 3);
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        out.numIndices = (uint32_t)indices.size();

        // create VB (dynamic) / IB (default)
        {
            D3D11_BUFFER_DESC bd{};
            bd.Usage = D3D11_USAGE_DYNAMIC;
            bd.ByteWidth = (UINT)(sizeof(SkinnedVertex3d) * out.skinnedVerts.size());
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            D3D11_SUBRESOURCE_DATA sd{};
            sd.pSysMem = out.skinnedVerts.data();

            HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &out.vb);
            assert(SUCCEEDED(hr));
        }

        {
            D3D11_BUFFER_DESC bd{};
            bd.Usage = D3D11_USAGE_DEFAULT;
            bd.ByteWidth = (UINT)(sizeof(uint32_t) * indices.size());
            bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA sd{};
            sd.pSysMem = indices.data();

            HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &out.ib);
            assert(SUCCEEDED(hr));
        }
    }

    return model;
}

void SkinnedModel_Release(SKINNED_MODEL* model)
{
    if (!model) return;

    for (auto& mesh : model->meshes)
    {
        if (mesh.vb) mesh.vb->Release();
        if (mesh.ib) mesh.ib->Release();
        mesh.vb = nullptr;
        mesh.ib = nullptr;
    }

    for (auto& kv : model->textures)
    {
        if (kv.second) kv.second->Release();
    }
    model->textures.clear();

    if (model->scene)
        aiReleaseImport(model->scene);

    delete model;
}

//------------------------------------------------------------------------------
// Update (CPU skinning)
//------------------------------------------------------------------------------
static const aiAnimation* SkinnedModel_GetAnimation(SKINNED_MODEL* model, int& animationIndex)
{
    if (!model || !model->scene) return nullptr;
    if (model->scene->mNumAnimations == 0) return nullptr;

    animationIndex = (std::max)(0, (std::min)(animationIndex, (int)model->scene->mNumAnimations - 1));
    return model->scene->mAnimations[animationIndex];
}

static void SkinnedModel_ApplyAnimation(SKINNED_MODEL* model,
    const aiAnimation* anim,
    int animationIndex,
    double animTime)
{
    if (!model || !model->scene || !anim) return;

    if (model->lastAnimIndex == animationIndex && model->lastAnimTime == animTime)
        return;
    model->lastAnimIndex = animationIndex;
    model->lastAnimTime = animTime;

    // boneFinal XV
    for (auto& mtx : model->boneFinal)
    {
        mtx = XMMatrixIdentity();
    }

    ReadNodeHierarchy(
        animTime,
        model->scene->mRootNode,
        XMMatrixIdentity(),
        anim,
        model->boneMap,
        model->boneOffset,
        model->globalInverse,
        model->boneFinal);

    
    ID3D11DeviceContext* ctx = Direct3D_GetContext();

    for (unsigned int m = 0; m < model->meshes.size(); ++m)
    {
        SKINNED_MESH& mesh = model->meshes[m];

        // CPU skinning
        for (size_t v = 0; v < mesh.baseVerts.size(); ++v)
        {
            const BaseVertex& bv = mesh.baseVerts[v];
            const Influence4& inf = mesh.influences[v];

            XMVECTOR p = XMLoadFloat3(&bv.position);
            XMVECTOR n = XMLoadFloat3(&bv.normal);

            XMVECTOR pOut = XMVectorZero();
            XMVECTOR nOut = XMVectorZero();
            float sumW = 0.0f;

            for (int i = 0; i < 4; ++i)
            {
                float w = inf.w[i];
                if (w <= 0.0f) continue;

                sumW += w;

                uint32_t bi = inf.idx[i];
                if (bi >= model->boneFinal.size()) continue;

                XMMATRIX M = model->boneFinal[bi];

                XMVECTOR tp = XMVector3TransformCoord(p, M);
                XMVECTOR tn = XMVector3TransformNormal(n, M);

                pOut += tp * w;
                nOut += tn * w;
            }


            if (sumW <= 0.0f)
            {
                pOut = p;
                nOut = n;
            }

            nOut = XMVector3Normalize(nOut);

            XMStoreFloat3(&mesh.skinnedVerts[v].position, pOut);
            XMStoreFloat3(&mesh.skinnedVerts[v].normalVector, nOut);
            // uv/color は変わらない
        }

        // Map/Unmap
        D3D11_MAPPED_SUBRESOURCE mapped{};
        HRESULT hr = ctx->Map(mesh.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            memcpy(mapped.pData, mesh.skinnedVerts.data(), sizeof(SkinnedVertex3d) * mesh.skinnedVerts.size());
            ctx->Unmap(mesh.vb, 0);
        }
    }
}

//------------------------------------------------------------------------------
// Update (CPU skinning)
//------------------------------------------------------------------------------
void SkinnedModel_Update(SKINNED_MODEL* model, float timeSec, int animationIndex)
{
    const aiAnimation* anim = SkinnedModel_GetAnimation(model, animationIndex);
    if (!anim) return;

    double ticksPerSecond = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 25.0;
    double timeInTicks = (double)timeSec * ticksPerSecond;
    double animTime = fmod(timeInTicks, anim->mDuration);

    SkinnedModel_ApplyAnimation(model, anim, animationIndex, animTime);
}

void SkinnedModel_UpdateAtTime(SKINNED_MODEL* model, float timeSec, int animationIndex)
{
    const aiAnimation* anim = SkinnedModel_GetAnimation(model, animationIndex);
    if (!anim) return;

    double ticksPerSecond = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 25.0;
    double animTime = (double)timeSec * ticksPerSecond;
    animTime = (std::max)(0.0, (std::min)(animTime, anim->mDuration));

    SkinnedModel_ApplyAnimation(model, anim, animationIndex, animTime);
}

void SkinnedModel_UpdateClip(SKINNED_MODEL* model,
    float timeSec,
    int animationIndex,
    float clipStartSec,
    float clipEndSec,
    bool holdLastFrame)
{
    const aiAnimation* anim = SkinnedModel_GetAnimation(model, animationIndex);
    if (!anim) return;

    double ticksPerSecond = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 25.0;
    double durationTicks = anim->mDuration;

    double clipStartTicks = (double)clipStartSec * ticksPerSecond;
    clipStartTicks = (std::max)(0.0, (std::min)(clipStartTicks, durationTicks));

    double clipEndTicks = durationTicks;
    if (clipEndSec > clipStartSec)
    {
        clipEndTicks = (double)clipEndSec * ticksPerSecond;
        clipEndTicks = (std::max)(clipStartTicks, (std::min)(clipEndTicks, durationTicks));
    }

    double clipLength = clipEndTicks - clipStartTicks;
    if (clipLength <= 0.0)
    {
        SkinnedModel_ApplyAnimation(model, anim, animationIndex, clipStartTicks);
        return;
    }

    double timeInTicks = (std::max)(0.0, (double)timeSec * ticksPerSecond);
    double animTime = clipStartTicks;
    if (holdLastFrame)
    {
        animTime = clipStartTicks + (std::min)(timeInTicks, clipLength);
    }
    else
    {
        animTime = clipStartTicks + fmod(timeInTicks, clipLength);
    }

    SkinnedModel_ApplyAnimation(model, anim, animationIndex, animTime);
}

void SkinnedModel_ResetPose(SKINNED_MODEL* model)
{
    using namespace DirectX;

    if (!model || !model->scene) return;

    // キャッシュ無効化（同フレームで戻したい時に効く）
    model->lastAnimIndex = -1;
    model->lastAnimTime = -1.0;

    // boneFinal を一旦初期化
    for (auto& mtx : model->boneFinal)
        mtx = XMMatrixIdentity();

    // 読み込み時ポーズ（bind/rest）で boneFinal を作る
    ReadNodeHierarchyBindPose(
        model->scene->mRootNode,
        XMMatrixIdentity(),
        model->boneMap,
        model->boneOffset,
        model->globalInverse,
        model->boneFinal
    );

    // boneFinal で CPU スキニングして VB 更新（ApplyAnimation と同じ処理）
    ID3D11DeviceContext* ctx = Direct3D_GetContext();

    for (unsigned int m = 0; m < model->meshes.size(); ++m)
    {
        SKINNED_MESH& mesh = model->meshes[m];

        for (size_t v = 0; v < mesh.baseVerts.size(); ++v)
        {
            const BaseVertex& bv = mesh.baseVerts[v];
            const Influence4& inf = mesh.influences[v];

            XMVECTOR p = XMLoadFloat3(&bv.position);
            XMVECTOR n = XMLoadFloat3(&bv.normal);

            XMVECTOR pOut = XMVectorZero();
            XMVECTOR nOut = XMVectorZero();
            float sumW = 0.0f;

            for (int i = 0; i < 4; ++i)
            {
                float w = inf.w[i];
                if (w <= 0.0f) continue;

                uint32_t bi = inf.idx[i];
                if (bi >= model->boneFinal.size()) continue;

                sumW += w;

                XMMATRIX M = model->boneFinal[bi];
                pOut += XMVector3TransformCoord(p, M) * w;
                nOut += XMVector3TransformNormal(n, M) * w;
            }

            if (sumW <= 0.0f)
            {
                // ウェイトが無い頂点はそのまま
                pOut = p;
                nOut = n;
            }
            else
            {
                // 念のため正規化（inf.Normalize() 済みなら実質不要）
                float inv = 1.0f / sumW;
                pOut *= inv;
                nOut *= inv;
            }

            nOut = XMVector3Normalize(nOut);

            XMStoreFloat3(&mesh.skinnedVerts[v].position, pOut);
            XMStoreFloat3(&mesh.skinnedVerts[v].normalVector, nOut);
        }

        D3D11_MAPPED_SUBRESOURCE mapped{};
        HRESULT hr = ctx->Map(mesh.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            memcpy(mapped.pData, mesh.skinnedVerts.data(),
                sizeof(SkinnedVertex3d) * mesh.skinnedVerts.size());
            ctx->Unmap(mesh.vb, 0);
        }
    }
}



//------------------------------------------------------------------------------
// Draw
//------------------------------------------------------------------------------
void SkinnedModel_Draw(SKINNED_MODEL* model, const XMMATRIX& mtxWorld)
{
    if (!model || !model->scene) return;

    XMMATRIX S = XMMatrixScaling(model->importScale, model->importScale, model->importScale);
    XMMATRIX world = S * mtxWorld;   // ← “モデルの拡大”を先に掛ける（位置は拡大されない）

    Shader3D_Begin();
    Shader3d_SetColor({ 1,1,1,1 });
    Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Shader3D_SetWorldMatrix(mtxWorld);

    ID3D11DeviceContext* ctx = Direct3D_GetContext();

    for (unsigned int m = 0; m < model->meshes.size(); ++m)
    {
        SKINNED_MESH& mesh = model->meshes[m];

        // texture
        aiMesh* aiMeshPtr = model->scene->mMeshes[m];
        aiMaterial* mat = model->scene->mMaterials[aiMeshPtr->mMaterialIndex];

        aiString tex;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex);

        if (tex.length != 0 && model->textures.count(tex.C_Str()))
        {
            ID3D11ShaderResourceView* srv = model->textures[tex.C_Str()];
            ctx->PSSetShaderResources(0, 1, &srv);
        }
        else
        {
            Texture_SetTexture(g_TextureWhite);
        }

        UINT stride = sizeof(SkinnedVertex3d);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
        ctx->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R32_UINT, 0);

        ctx->DrawIndexed(mesh.numIndices, 0, 0);
    }
}

void SkinnedModel_DepthDraw(SKINNED_MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
    if (!model || !model->scene) return;

    ShaderDepth_Begin();
    Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ShaderDepth_SetWorldMatrix(mtxWorld);

    ID3D11DeviceContext* ctx = Direct3D_GetContext();

    for (unsigned int m = 0; m < model->meshes.size(); ++m)
    {
        SKINNED_MESH& mesh = model->meshes[m];

        // texture（α抜きするdepthシェーダなら必要）
        aiMesh* aiMeshPtr = model->scene->mMeshes[m];
        aiMaterial* mat = model->scene->mMaterials[aiMeshPtr->mMaterialIndex];

        aiString tex;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex);

        if (tex.length != 0 && model->textures.count(tex.C_Str()))
        {
            ID3D11ShaderResourceView* srv = model->textures[tex.C_Str()];
            ctx->PSSetShaderResources(0, 1, &srv);
        }
        else
        {
            Texture_SetTexture(g_TextureWhite);
        }

        UINT stride = sizeof(SkinnedVertex3d);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
        ctx->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R32_UINT, 0);

        ctx->DrawIndexed(mesh.numIndices, 0, 0);
    }
}

AABB SkinnedModel_GetAABB(SKINNED_MODEL* model, const DirectX::XMFLOAT3& position)
{
    if (!model) return {};

    return {
        {position.x + model->local_aabb.min.x, position.y + model->local_aabb.min.y, position.z + model->local_aabb.min.z},
        {position.x + model->local_aabb.max.x, position.y + model->local_aabb.max.y, position.z + model->local_aabb.max.z}
    };
}