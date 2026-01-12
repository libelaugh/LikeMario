/*==============================================================================

@@  ‹ó‚Ì•\Ž¦[sky.cpp]
														 Author : Tanaka Kouki
														 Date   : 2025/11/21
--------------------------------------------------------------------------------

==============================================================================*/

#include "sky.h"
#include "direct3d.h"
#include "model.h"
#include "shader3d_unlit.h"

using namespace DirectX;

static MODEL* g_pModelSky{ nullptr };
static ID3D11RasterizerState* g_pRasterizerStateCullNone{ nullptr };
static XMFLOAT3 g_position{};


void Sky_Initialize()
{

	g_pModelSky = ModelLoad("sky.fbx", 100.0f, true);

	ID3D11Device* device = Direct3D_GetDevice();
	if (device) {
		D3D11_RASTERIZER_DESC rd{};
		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_NONE;
		rd.DepthClipEnable = TRUE;
		device->CreateRasterizerState(&rd, &g_pRasterizerStateCullNone);
	}
}

void Sky_Finalize()
{
	ModelRelease(g_pModelSky);
	SAFE_RELEASE(g_pRasterizerStateCullNone);
}

void Sky_Draw()
{
	Shader3DUnlit_Begin();

	//g_position.y = 0.0f;

	ID3D11DeviceContext* ctx = Direct3D_GetContext();
	ID3D11RasterizerState* prevState = nullptr;
	if (ctx && g_pRasterizerStateCullNone) {
		ctx->RSGetState(&prevState);
		ctx->RSSetState(g_pRasterizerStateCullNone);
	}

	ModelUnlitDraw(g_pModelSky, XMMatrixTranslationFromVector(XMLoadFloat3(&g_position)));

	if (ctx && g_pRasterizerStateCullNone) {
		ctx->RSSetState(prevState);
	}
	SAFE_RELEASE(prevState);
}

void Sky_SetPosition(const DirectX::XMFLOAT3& position)
{
	g_position = position;
}
