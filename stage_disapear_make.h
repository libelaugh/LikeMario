/*==============================================================================

　　  ステージdisapearステージ作り[stage_simple_make.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/11
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_DISAPEAR_MAKE_H
#define STAGE_DISAPEAR_MAKE_H

#include <DirectXMath.h>

void StageDisapear_Initialize();
void StageDisapear_Finalize();
void StageDisapear_Update(double elapsedTime);
bool StageDisapear_SetPlayerPositionAndLoadJson(const DirectX::XMFLOAT3& position, const char* jsonPath);

#endif//STAGE_DISAPEAR_MAKE_H