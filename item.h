/*==============================================================================

	ÉAÉCÉeÉÄä«óù[item.h]
														 Author : Tanaka Kouki
														 Date   : 2026/01/05
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef ITEM_H
#define ITEM_H

#include <DirectXMath.h>

void Item_Initialize();
void Item_Finalize();
void Item_Update();
void Item_Draw();

int Item_LoadModel(const char* modelPath, float scale = 0.1f, bool isBrender = false);
int Item_Add(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotationDeg, int modelIndex);
int Item_GetHitCount();
void Item_ResetHitCount();

#endif // ITEM_H