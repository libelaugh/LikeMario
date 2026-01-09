/*==============================================================================

　　　ビルボード描画[billboard.h]
														 Author : Tanaka Kouki
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/

#include "billboard.h"
#include"direct3d.h"
#include"shader3d.h"
#include"texture.h"
#include"shader_billboard.h"
#include"player_camera.h"
#include<DirectXMath.h>


using namespace DirectX;

static constexpr int NUM_VERTEX = 4 * 6;// 頂点数//意味ある式
static constexpr float HALF_LENGTH = 0.5f;

static constexpr UINT BB_VERTEX_COUNT = 4;
static constexpr UINT BB_INDEX_COUNT = 6;


static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr; // インデックスバッファ

static XMFLOAT4X4 g_mtxView{};//ビュー行列の平行移動成分をカットした行列

struct VertexBillboard
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT2 texcoord;
};

static const VertexBillboard s_vertices[BB_VERTEX_COUNT] =
{
	{{-0.5f,  0.5f, 0.0f}, {1,1,1,1}, {0,0}},
	{{ 0.5f,  0.5f, 0.0f}, {1,1,1,1}, {1,0}},
	{{ 0.5f, -0.5f, 0.0f}, {1,1,1,1}, {1,1}},
	{{-0.5f, -0.5f, 0.0f}, {1,1,1,1}, {0,1}},
};

static const uint16_t s_indices[BB_INDEX_COUNT] =
{
	0,1,2,
	0,2,3
};


// 3D頂点構造体
struct Vertex3d
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4  color;
	XMFLOAT2 texcoord;//uv
};

void Billboard_Initialize()
{
	ShaderBillboard_Initialize();

	// VB
	D3D11_BUFFER_DESC vbd{};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(VertexBillboard) * BB_VERTEX_COUNT;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vsd{};
	vsd.pSysMem = s_vertices;

	Direct3D_GetDevice()->CreateBuffer(&vbd, &vsd, &g_pVertexBuffer);

	// IB
	D3D11_BUFFER_DESC ibd{};
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(uint16_t) * BB_INDEX_COUNT;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA isd{};
	isd.pSysMem = s_indices;

	Direct3D_GetDevice()->CreateBuffer(&ibd, &isd, &g_pIndexBuffer);
}


void Billboard_Finalize()
{
	ShaderBillboard_Finalize();
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
}

/*
void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position, float scaleX, float scaleY, const XMFLOAT2& pivot)
{
	ShaderBillboard_SetUVParameter({ { 0.2f ,1.0f },{1.0f,1.0f} });

	ShaderBillboard_Begin();

	// シェーダーを描画パイプラインに設定
	Shader3D_Begin();

	//ピクセルシェーダに色を設定
	Shader3d_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	Direct3D_GetContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// インデックスバッファを描画パイプラインに設定
	Direct3D_GetContext()->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);//unsigned shortはR16、unsigned intはR32


	//テクスチャ設定
	Texture_SetTexture(texId);

	// プリミティブトポロジ設定
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	
	//カメラ行列の回転だけ逆行列を作る
	//XMMATRIX iv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&mtxCamera));  //※逆行列の生成はかなり重い処理
	//直行行列の逆行列は転地行列に等しい(逆行列生成より処理軽くできる)
	XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&g_mtxView));

	//回転軸までのオフセット行列
	XMMATRIX pivotOffset = XMMatrixTranslation(-pivot.x, -pivot.y, 1.0f);

	XMMATRIX s = XMMatrixScaling(scaleX, scaleY, 1.0f);
	XMMATRIX t = XMMatrixTranslation(position.x, position.y, position.z);
	ShaderBillboard_SetWorldMatrix(s * pivotOffset * iv * t);

	// ポリゴン描画命令発行
	Direct3D_GetContext()->DrawIndexed(NUM_VERTEX, 0, 0);
}*/

void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT2& scale, const DirectX::XMUINT4& tex_cut,
	const DirectX::XMFLOAT4& color, const DirectX::XMFLOAT2& pivot)
{
	// UV（あなたの仕組みに合わせてscale/translationを作る）
	/*ShaderBillboard_SetUVParameter(
		DirectX::XMFLOAT2{ 1.0f, 1.0f },
		DirectX::XMFLOAT2{ 0.0f, 1.0f }
	);*/



	ShaderBillboard_Begin();

	// ★ここを消す：Shader3D_Begin();
	// ★ここも消す：Shader3d_SetColor(...);
	ShaderBillboard_SetColor(color);

	UINT stride = sizeof(VertexBillboard);
	UINT offset = 0;
	Direct3D_GetContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	Direct3D_GetContext()->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Texture_SetTexture(texId);

	// View/Proj は ShaderBillboard 側に正しく渡す（後述）
	// world（ビルボード回転）
	using namespace DirectX;
	XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&g_mtxView)); // translationゼロ済み想定
	XMMATRIX pivotOffset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);
	XMMATRIX s = XMMatrixScaling(scale.x, scale.y, 1.0f);
	XMMATRIX t = XMMatrixTranslation(position.x, position.y, position.z);

	ShaderBillboard_SetWorldMatrix(s * pivotOffset * iv * t);

	Direct3D_GetContext()->DrawIndexed(BB_INDEX_COUNT, 0, 0);
}


void Billboard_SetViewMatrix(const DirectX::XMFLOAT4X4& view)
{
	//カメラ行列の平行移動成分をカット
	g_mtxView = view;
	g_mtxView._41 = g_mtxView._42 = g_mtxView._43 = 0.0f;//打消し
}
