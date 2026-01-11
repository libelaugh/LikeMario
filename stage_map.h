/*==============================================================================

　　  ステージオブジェクト管理[stage_map.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_MAP_H
#define STAGE_MAP_H

#include "stage_cube.h"
#include <vector>

bool Map_Initialize();
void Map_Finalize();
void Map_Update(double elapsedTime);
void Map_Draw();
void Map_DepthDraw();

int Map_GetObjectsCount();
const CubeBlock* Map_GetObject(int index);
CubeBlock* Map_GetObjectMutable(int index);

void Map_RebuildObject(int index);

int Map_AddBlock(const CubeBlock& block, bool bake = true);

bool Map_AddObjectTransform(int index,
    const DirectX::XMFLOAT3& positionDelta,
    const DirectX::XMFLOAT3& sizeDelta,
    const DirectX::XMFLOAT3& rotationDelta);

void Map_Clear();

#endif//STAGE_MAP_H
