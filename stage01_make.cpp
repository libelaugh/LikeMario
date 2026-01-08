/*==============================================================================

　　  ステージ01オブジェクト配置[stage01_make.cpp]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#include "stage01_make.h"
#include <DirectXMath.h>

static constexpr float s1 = 0.5f;
static constexpr float s2 = 1.0f;
static constexpr float s3x = 3.0f;
static constexpr float s3y = 0.8f;
static constexpr float s3z = 2.0f;
static constexpr float s4 = 1.0f;

//データはjsonファイルに移行した
static const StageRow kStage01[] =
{
    // kind tex   px    py    pz     sx  sy  sz     rx  ry  rz
    {   0,  0,   0,   0.5f,  0,      s1, s1, s1,    0,  0,  0 },
    {   0,  0,   1,   0.5f,  0,      s2, s2, s2,    0,  0, 0 },
    {   0,  0,   1,   2.5f,  0,      s2, s2, s2,    0,  0, 0 },
    {   0,  0,  -3,   s3y*0.5f,  0,      s3x,s3y,s3z,    0,  0, 0 },
    {   0,  0,   6,   0.5f,  0,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   6,   0.5f,  1,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   6,   0.5f,  2,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   6,   0.5f,  3,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   6,   0.5f,  4,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   6,   0.5f,  5,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   6,   0.5f,  6,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   6,   0.5f,  7,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   6,   0.5f,  8,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   7,   0.5f,  0,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   7,   0.5f,  1,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   7,   0.5f,  2,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   7,   0.5f,  3,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   7,   0.5f,  4,      s4, s4, s4,    0,  0, 0 },
    {   0,  0,   7,   0.5f,  7,      s4, s4, s4,    0,  0, 0 },

};

void Stage01_MakeTable(const StageRow** outRows, int* outCount)
{
    *outRows = kStage01;
    *outCount = (int)(sizeof(kStage01) / sizeof(kStage01[0]));
}
