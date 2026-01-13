/*==============================================================================

　　  ステージmagma管理[stage_magma_manager.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/12
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_MAGMA_MANAGER_H
#define STAGE_MAGMA_MANAGER_H

#include "stage_registry.h"
#include <DirectXMath.h>

void StageMagmaManager_Initialize(const StageInfo& info);
void StageMagmaManager_ChangeStage(const StageInfo& info);
void StageMagmaManager_Finalize();
void StageMagmaManager_Update(double elapsedTime);
void StageMagmaManager_Draw();

DirectX::XMFLOAT3 StageMagmaManager_GetSpawnPosition();
const char* StageMagmaManager_GetStageJsonPath();

float StageMagmaManager_GetMagmaY();
void StageMagmaManager_AddMagmaY(float a);
void StageMagmaManager_SetMagmaY(float y);
float StageMagmaManager_GetMagmaBaseY();

#endif//STAGE_MAGMA_MANAGER_H