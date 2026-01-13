/*==============================================================================

　　  ステージsimple管理[stage_simple_manager.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/12
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_SIMPLE_MANAGER_H
#define STAGE_SIMPLE_MANAGER_H

#include "stage_registry.h"
#include <DirectXMath.h>

//void StageSimpleManager_Initialize();
void StageSimpleManager_Initialize(const StageInfo& info);
void StageSimpleManager_ChangeStage(const StageInfo& info);
void StageSimpleManager_Finalize();
void StageSimpleManager_Update(double elapsedTime);
void StageSimpleManager_Draw();

DirectX::XMFLOAT3 StageSimpleManager_GetSpawnPosition();
const char* StageSimpleManager_GetStageJsonPath();

#endif//STAGE_SIMPLE_MANAGER_H