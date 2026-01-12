/*==============================================================================

　　  ステージsimpleステージ作り[stage_simple_make.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/11
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_SIMPLE_MAKE_H
#define STAGE_SIMPLE_MAKE_H

#include <DirectXMath.h>

void StageSimple_Initialize();
void StageSimple_Finalize();
void StageSimple_Update(double elapsedTime);

bool StageSimple_SetPlayerPositionAndLoadJson(const DirectX::XMFLOAT3& position, const char* jsonPath);

#endif//STAGE_SIMPLE_MAKE_H