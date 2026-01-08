/*==============================================================================

　　  ステージ01オブジェクト配置[stage01_make.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE01_MAKE_H
#define STAGE01_MAKE_H

#include <DirectXMath.h>

// 1行 = 1ブロック（表）
struct StageRow
{
    int kind;
    int texSlot;  // 0=Brick, 1=Red ...

    float px, py, pz; // position
    float sx, sy, sz; // size
    float rx, ry, rz; // rotation (ラジアン)
};

void Stage01_MakeTable(const StageRow** outRows, int* outCount);

#endif//STAGE01_MAKE_H