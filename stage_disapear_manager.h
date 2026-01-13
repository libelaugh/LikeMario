/*==============================================================================

　　  ステージdisapear管理[stage_disapear_manager.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_DISAPEAR_MANAGER_H
#define STAGE_DISAPEAR_MANAGER_H

#include "stage_registry.h"
#include <DirectXMath.h>

void StageDisapearManager_Initialize(const StageInfo& info);
void StageDisapearManager_ChangeStage(const StageInfo& info);
void StageDisapearManager_Finalize();
void StageDisapearManager_Update(double elapsedTime);
void StageDisapearManager_Draw();

DirectX::XMFLOAT3 StageDisapearManager_GetSpawnPosition();
const char* StageDisapearManager_GetStageJsonPath();


#endif//STAGE_DISAPEAR_MANAGER_H