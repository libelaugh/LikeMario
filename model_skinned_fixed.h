/*==============================================================================

    Skinned Model (CPU Skinning) [model_skinned.h]
                                                            Author : Tanaka Kouki
                                                            Date   : 2026/01/01
--------------------------------------------------------------------------------

    シェーダを増やさず、既存の shader3d で描画できるように
    CPU 側でスキニングして「POSITION/NORMAL/UV/Color」のみを GPU に流し込む簡易実装。
ポイント
      - Assimp で glTF/FBX そのまま読めます
      - 骨とアニメを計算して、毎フレーム VertexBuffer を Update(Map)
      - モデルが大きいと CPU 負荷は上がるけど、動かし始めはこれが一番楽

==============================================================================*/

#ifndef MODEL_SKINNED_FIXED_H
#define MODEL_SKINNED_FIXED_H

#include <d3d11.h>
#include <DirectXMath.h>
#include <unordered_map>
#include <vector>
#include <string>

#include "Assimp/assimp/scene.h"
#include "Assimp/assimp/cimport.h"
#include "Assimp/assimp/postprocess.h"
#include "Assimp/assimp/matrix4x4.h"

#include "collision.h"

#pragma comment (lib, "assimp-vc143-mt.lib")


struct SKINNED_MODEL;

// 読み込み
SKINNED_MODEL* SkinnedModel_Load(const char* fileName, float scale, bool isBrender = false);
void SkinnedModel_Release(SKINNED_MODEL* model);

// アニメ更新（timeSec：秒）
void SkinnedModel_Update(SKINNED_MODEL* model, float timeSec, int animationIndex = 0);//ループ再生
void SkinnedModel_UpdateAtTime(SKINNED_MODEL* model, float timeSec, int animationIndex = 0);//切り抜き静止画
void SkinnedModel_UpdateClip(SKINNED_MODEL* model,//切り抜き再生(ループON/OFF有り)
    float timeSec,
    int animationIndex,
    float clipStartSec,
    float clipEndSec,
    bool holdLastFrame = true);

void SkinnedModel_ResetPose(SKINNED_MODEL* model);

// 描画（既存の Shader3D で描画）
void SkinnedModel_Draw(SKINNED_MODEL* model, const DirectX::XMMATRIX& mtxWorld);

void SkinnedModel_DepthDraw(SKINNED_MODEL* model, const DirectX::XMMATRIX& mtxWorld);

// AABB
AABB SkinnedModel_GetAABB(SKINNED_MODEL* model, const DirectX::XMFLOAT3& position);

#endif//MODLE_SKINNED_FIXED_H
