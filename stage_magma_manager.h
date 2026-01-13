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

void StageMagmaManager_Initialize();
void StageMagmaManager_Finalize();
void StageMagmaManager_Update(double elapsedTime);
void StageMagmaManager_Draw();

DirectX::XMFLOAT3 StageMagmaManager_GetSpawnPosition();
const char* StageMagmaManager_GetStageJsonPath();

#endif//STAGE_MAGMA_MANAGER_H