/*==============================================================================

Å@Å@  ãÛÇÃï\é¶[sky.h]
														 Author : Tanaka Kouki
														 Date   : 2025/11/21
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SKY_H
#define SKY_H

#include<DirectXMath.h>

void Sky_Initialize();
void Sky_Finalize();
void Sky_Draw();

void Sky_SetPosition(const DirectX::XMFLOAT3& position);



#endif//SKY_H