/*==============================================================================

Å@Å@  ãÛÇÃï\é¶[sky.cpp]
														 Author : Tanaka Kouki
														 Date   : 2025/11/21
--------------------------------------------------------------------------------

==============================================================================*/

#include "sky.h"
#include"model.h"
#include"shader3d_unlit.h"

using namespace DirectX;

static MODEL* g_pModelSky{ nullptr };
static XMFLOAT3 g_position{};


void Sky_Initialize()
{

	g_pModelSky = ModelLoad("sky.fbx", 100.0f, true);
}

void Sky_Finalize()
{
	ModelRelease(g_pModelSky);
}

void Sky_Draw()
{
	Shader3DUnlit_Begin();

	g_position.y = 0.0f;

	ModelUnlitDraw(g_pModelSky, XMMatrixTranslationFromVector(XMLoadFloat3(&g_position)));
}

void Sky_SetPosition(const DirectX::XMFLOAT3& position)
{
	g_position = position;
}
