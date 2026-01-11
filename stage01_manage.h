/*==============================================================================

　　  ステージ01管理[stage01_manage.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE01_MANAGE_H
#define STAGE01_MANAGE_H

#include "collision.h"
#include <DirectXMath.h>


// ImGuiで直接いじる“編集対象”
// ※ world と aabb は「焼いた結果」なので、編集後に Rebuild で更新する
struct StageBlock
{
    int kind = 0;     // キューブ種類（UV/色/法線のテンプレ）※今は0でOK
    int texSlot = 0;
    int texId = -1;


    DirectX::XMFLOAT3 position{ 0,0,0 }; // 中心
    DirectX::XMFLOAT3 size{ 1,1,1 };     // スケール
    DirectX::XMFLOAT3 rotation{ 0,0,0 }; // pitch,yaw,roll (ラジアン)
    DirectX::XMFLOAT3 positionOffset{ 0,0,0 }; // 一時の座標変化(ImGuiの編集値は保持)
    DirectX::XMFLOAT3 sizeOffset{ 0,0,0 };
    DirectX::XMFLOAT3 rotationOffset{ 0,0,0 };

    DirectX::XMFLOAT4X4 world{};
    AABB aabb{};
};
int Stage01_GetTexSlotCount();
const char* Stage01_GetTexSlotName(int slot);

// ===== Stage JSON file (Stage02, Stage03...) =====
// Current stage file path used by Stage01_Initialize/Save/Load.
void        Stage01_SetCurrentJsonPath(const char* filepath);
const char* Stage01_GetCurrentJsonPath();


void Stage01_Initialize();
void Stage01_Initialize(const char* jsonPath);
void Stage01_Finalize();
void Stage01_Update(double elapsedTime);
void Stage01_Draw();
void Stage01_DepthDraw(); // 影用（使うなら）

// ===== ImGuiが使うための最低限 =====
int  Stage01_GetCount();
const StageBlock* Stage01_Get(int i);
StageBlock* Stage01_GetMutable(int i);

void Stage01_RebuildObject(int i);   // 編集後に呼ぶ
void Stage01_RebuildAll();           // まとめて焼き直し

int  Stage01_Add(const StageBlock& b, bool bake = true);
void Stage01_Remove(int i);
void Stage01_Clear();

bool Stage01_AddObjectTransform(int index,
    const DirectX::XMFLOAT3& positionDelta,
    const DirectX::XMFLOAT3& sizeDelta,
    const DirectX::XMFLOAT3& rotationDelta);

bool Stage01_SaveJson(const char* filepath);
bool Stage01_LoadJson(const char* filepath);

enum StageSwitchResult
{
    STAGE_SWITCH_LOADED,        // json をロードして切替できた
    STAGE_SWITCH_CREATED_EMPTY, // json が無くて空ステージにした（新規作成用）
    STAGE_SWITCH_FAILED         // 引数不正など
};

StageSwitchResult Stage01_SwitchStage(const char* jsonPath, bool createEmptyIfMissing = true);


#endif//STAGE01_MANAGE_H