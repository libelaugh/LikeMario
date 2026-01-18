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


using namespace DirectX;

static constexpr int NUM_VERTEX = 4 * 6;// 頂点数//意味ある式
static constexpr float HALF_LENGTH = 0.5f;

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr; // インデックスバッファ

static XMFLOAT4X4 g_mtxView{};//ビュー行列の平行移動成分をカットした行列

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

	static Vertex3d Vertex[24]{
        {{-0.5f,  0.5f, 0.0f}, {1,1,1,1}, {0,0}},
		{{ 0.5f,  0.5f, 0.0f}, {1,1,1,1}, {1.0f,0}},
		{{ 0.5f, -0.5f, 0.0f}, {1,1,1,1}, {0.0f,1.0f}},
		{{ 0.5f, -0.5f, 0.0f}, {1,1,1,1}, {0,1.0f}},
	};

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;//DEFAULTでCPUでは書き換え不可
	bd.ByteWidth = sizeof(Vertex3d) * NUM_VERTEX;//sizeof(g_CubeVertex);でもいい
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;//CPUはアクセスできない

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = Vertex;

	Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &g_pVertexBuffer);
}

void Billboard_Finalize()
{
	ShaderBillboard_Finalize();
	SAFE_RELEASE(g_pVertexBuffer);
}

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
}

void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT2& scale, const DirectX::XMUINT4& tex_cut, const DirectX::XMFLOAT4& color,
	const DirectX::XMFLOAT2& pivot )
{
	float uv_x = (float)tex_cut.x / Texture_Width(texId);
	float uv_y = (float)tex_cut.y / Texture_Height(texId);
	float uv_w = (float)tex_cut.z / Texture_Width(texId);
	float uv_h = (float)tex_cut.w / Texture_Height(texId);

	ShaderBillboard_SetUVParameter({ { 1.0f ,1.0f },{0.0f,0.0f} });

	ShaderBillboard_Begin();

	// シェーダーを描画パイプラインに設定
	Shader3D_Begin();

	//ピクセルシェーダに色を設定
	Shader3d_SetColor(color);

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

	XMMATRIX s = XMMatrixScaling(scale.x, scale.y, 1.0f);
	XMMATRIX t = XMMatrixTranslation(position.x, position.y, position.z);
	ShaderBillboard_SetWorldMatrix(s * pivotOffset * iv * t);

	// ポリゴン描画命令発行
	Direct3D_GetContext()->DrawIndexed(NUM_VERTEX, 0, 0);
}

void Billboard_SetViewMatrix(const DirectX::XMFLOAT4X4& view)
{
	//カメラ行列の平行移動成分をカット
	g_mtxView = view;
	g_mtxView._41 = g_mtxView._42 = g_mtxView._43 = 0.0f;//打消し
}
