/*==============================================================================

　　  ステージmagmaステージ作り[stage_magma_make.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_MAGMA_MAKE_H
#define STAGE_MAGMA_MAKE_H

#include <DirectXMath.h>

void StageMagma_Initialize();
void StageMagma_Finalize();
void StageMagma_Update(double elapsedTime);
bool StageMagma_SetPlayerPositionAndLoadJson(const DirectX::XMFLOAT3& position, const char* jsonPath);

#endif//STAGE_MAGMA_MAKE_H