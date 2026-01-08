/*==============================================================================

　　  ライトの設定[light.h]
														 Author : Kouki Tanaka
														 Date   : 2025/09/30
--------------------------------------------------------------------------------

==============================================================================*/

#include "light.h"
#include"direct3d.h"

using namespace DirectX;

static ID3D11Buffer* g_pPSConstantBuffer1 = nullptr;//定数バッファｂ1 //ambient
static ID3D11Buffer* g_pPSConstantBuffer2 = nullptr;//定数バッファｂ2 //法線ベクトル
static ID3D11Buffer* g_pPSConstantBuffer3 = nullptr;//定数バッファｂ3
static ID3D11Buffer* g_pPSConstantBuffer4 = nullptr;//定数バッファｂ4

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

//平行光源(拡散反射光）
struct DirectionalLight {//配列にすれば複数作れる
	XMFLOAT4 directional;
	XMFLOAT4 color;
};

//鏡面反射光
struct SpecularLight {
	XMFLOAT3 camraraPosition;
	float power;
	XMFLOAT4 color;
};

//点光源（ポイントライト）
struct PointLight {
	XMFLOAT3 position;
	float range;
	XMFLOAT4 color;
	//float specular_power;
	//XMFLOAT3 dummy;
};
struct PointLightList {
	PointLight light[4];
	int count;
	XMFLOAT3 dummy;
};
static PointLightList g_PointLights{};

void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ

	buffer_desc.ByteWidth = sizeof(XMFLOAT4); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer1);//ambient


	buffer_desc.ByteWidth = sizeof(DirectionalLight); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer2);//directional

	buffer_desc.ByteWidth = sizeof(SpecularLight); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer3);//specular

	buffer_desc.ByteWidth = sizeof(PointLightList); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer4);//point

	/*PointLightList list{
		{
	       { { 0.0f,2.0f,0.0f},    6.0f, {1.0f,1.0f,1.0f,1.0f} },
	       { { 0.0f, 2.0f, 0.0f }, 6.0f, { 0.0f,0.0f,1.0f,1.0f } },
	       { { 0.0f, 2.0f, 0.0f }, 6.0f, { 1.0f,1.0f,1.0f,1.0f } },
		   { { 0.0f, 2.0f, 0.0f }, 6.0f, { 1.0f,1.0f,1.0f,1.0f } },
	    },
		2
	};
	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &list, 0, 0);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);*/
}

void Light_Finalize()
{
	SAFE_RELEASE(g_pPSConstantBuffer1);
	SAFE_RELEASE(g_pPSConstantBuffer2);
	SAFE_RELEASE(g_pPSConstantBuffer3);
	SAFE_RELEASE(g_pPSConstantBuffer4);
}

void Light_SetAmbient(const DirectX::XMFLOAT3& color)
{
	XMFLOAT4 ambient = { color.x, color.y, color.z, 1.0f };
	// 定数バッファにアンビエントをセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer1, 0, nullptr, &ambient, 0, 0);
	g_pContext->PSSetConstantBuffers(1, 1, &g_pPSConstantBuffer1);
}


void Light_SetDirectionalWorld(const DirectX::XMFLOAT4& worldDirectional, const DirectX::XMFLOAT4& color)
{
	DirectionalLight dlight{
		worldDirectional,
		color
	};
	g_pContext->UpdateSubresource(g_pPSConstantBuffer2, 0, nullptr, &dlight, 0, 0);
	g_pContext->PSSetConstantBuffers(2, 1, &g_pPSConstantBuffer2);
}

void Light_SetSpecularWorld(const DirectX::XMFLOAT3& cameraPosition, float power, const DirectX::XMFLOAT4& color)
{
	SpecularLight slight{
		cameraPosition,power, color
	};

	g_pContext->UpdateSubresource(g_pPSConstantBuffer3, 0, nullptr, &slight, 0, 0);
	g_pContext->PSSetConstantBuffers(3, 1, &g_pPSConstantBuffer3);
}

void Light_SetPointLightCount(int count)
{
	g_PointLights.count = count;

	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_PointLights, 0, 0);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);
}

void Light_SetPointLight(int n, const DirectX::XMFLOAT3& position, float range, const DirectX::XMFLOAT3& color)
{
	g_PointLights.light[n].position = position;
	g_PointLights.light[n].range = range;
	g_PointLights.light[n].color = { color.x,color.y,color.z,1.0f };

	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_PointLights, 0, 0);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);
}
