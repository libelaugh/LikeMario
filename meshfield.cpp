/*==============================================================================

　　　メッシュフィールド（地面）作成[meshfield.h]
														 Author : Kouki Tanaka
														 Date   : 2025/09/09
--------------------------------------------------------------------------------

==============================================================================*/

#include "meshfield.h"
#include"direct3d.h"
#include"texture.h"
#include"shader_field.h"
#include"camera.h"
#include<DirectXMath.h>

using namespace DirectX;

static constexpr float FIELD_MESH_SIZE = 1.0f;//メッシュフィールドの幅
static constexpr int FIELD_MESH_W_COUNT = 50;//マス数
static constexpr int FIELD_MESH_H_COUNT = 50;//マス数
static constexpr int NUM_VERTEX_W = FIELD_MESH_W_COUNT + 1;// 頂点数　（マス数＋１）＊（マス数＋１）
static constexpr int NUM_VERTEX_H = FIELD_MESH_H_COUNT + 1;
static constexpr int NUM_VERTEX = NUM_VERTEX_W * NUM_VERTEX_H;
static constexpr int NUM_INDEX = 3 * 2 * FIELD_MESH_W_COUNT * FIELD_MESH_H_COUNT;

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr; // インデックスバッファ

static ID3D11ShaderResourceView* g_pTexture = nullptr; //テクスチャ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static int g_meshFieldTexId1 = -1;
static int g_meshFieldTexId2 = -1;

struct Vertex3d {
	XMFLOAT3 position; // 頂点座標
	XMFLOAT3 normalVector;//法線
	XMFLOAT4  color;
	XMFLOAT2 texcoord;//uv
};

//こんなバカでかいのはローカルの領域（１MBしかない)に置かない
static Vertex3d g_meshFieldVertex[NUM_VERTEX]{
	    {{-0.5f,  0.5f, -0.5f}, { 0.0f,1.0f,0.0f, },{1,1,1,1}, {0,0}},//TL  //0 
		{{ 0.5f,  0.5f, -0.5f}, { 0.0f,1.0f,0.0f, },{1,1,1,1}, {0.25f,0}},//TR //1
		{{ 0.5f, -0.5f, -0.5f}, { 0.0f,1.0f,0.0f, },{1,1,1,1}, {0.25,0.25}},//BR //2
		{{-0.5f, -0.5f, -0.5f}, { 0.0f,1.0f,0.0f, },{1,1,1,1}, {0,0.25f}},//BL //3
};

static unsigned short g_meshFieldIndex[NUM_INDEX];

void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;//DEFAULTでCPUでは書き換え不可
	bd.ByteWidth = sizeof(Vertex3d) * NUM_VERTEX;//sizeof(g_CubeVertex);でもいい
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;//CPUはアクセスできない

	//頂点情報を配列に作る
	for (int z = 0; z < NUM_VERTEX_H; z++) {
		for (int x = 0; x < NUM_VERTEX_W; x++) {
			//横＋横の最大数＊縦 １次元から２次元への変換インデックスってやつ
			int index = x + NUM_VERTEX_H * z;
			g_meshFieldVertex[index].position = { x * FIELD_MESH_SIZE,0.0f,z * FIELD_MESH_SIZE };
			g_meshFieldVertex[index].normalVector = { 0.0f,1.0f,0.0f, };
			g_meshFieldVertex[index].color = { 0.0f,1.0f,0.0f,1.0f };
			g_meshFieldVertex[index].texcoord = { x * 1.0f,z * 1.0f };
		}
	}

	//頂点カラー変えると１回のドローコールでオープンワールドに道っぽい線を描ける
	for (int z = 0; z < NUM_VERTEX_H; z++) {
		int index = 26 + FIELD_MESH_H_COUNT * z;
		g_meshFieldVertex[index].color = { 1.0f,0.0f,1.0f,1.0f };
	}


	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = g_meshFieldVertex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

	//インデックスバッファ作成
	bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	//インデックス情報を配列に作る //図を書くと法則性分かる
	int index = 0;
	for (int w = 0; w < FIELD_MESH_W_COUNT; w++) {
		for (int h = 0; h < FIELD_MESH_H_COUNT; h++) {
			g_meshFieldIndex[index + 0] = h + (w + 0) * NUM_VERTEX_W;      //0 1  5
			g_meshFieldIndex[index + 1] = h + (w + 1) * NUM_VERTEX_W + 1;  //5 6  10
			g_meshFieldIndex[index + 2] = g_meshFieldIndex[index + 0] + 1; //1 2  6
			g_meshFieldIndex[index + 3] = g_meshFieldIndex[index + 0];     //0 1  5
			g_meshFieldIndex[index + 4] = g_meshFieldIndex[index + 1] - 1; //4 5  9
			g_meshFieldIndex[index + 5] = g_meshFieldIndex[index + 1];     //5 6  10って感じのインデックスになる
			index += 6;
		}
	}

	sd.pSysMem = g_meshFieldIndex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);

	g_meshFieldTexId1 = Texture_Load(L"magma.jpg");
	g_meshFieldTexId2 = Texture_Load(L"magma.jpg");
	//g_meshFieldTexId2 = Texture_Load(L"Knight.png");

	ShaderField_Initialize(pDevice, pContext);
}

void MeshField_Finalize()
{
	SAFE_RELEASE(g_pVertexBuffer);
	SAFE_RELEASE(g_pIndexBuffer);
	Shader_field_Finalize();
}

void MeshField_Draw(DirectX::XMMATRIX& mtrWorld)
{
	// シェーダーを描画パイプラインに設定
	Shader_field_Begin();

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// インデックスバッファを描画パイプラインに設定
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);//unsigned shortはR16、unsigned intはR32


	Shader_field_SetWorldMatrix(mtrWorld);


	//テクスチャ設定
	Texture_SetTexture(g_meshFieldTexId1, 0);
	Texture_SetTexture(g_meshFieldTexId2,1);

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ポリゴン描画命令発行
				/*============面の数(増やすたびに6頂点ずつ必ず書き換える)==============*/
	//g_pContext->Draw(NUM_VERTEX, 0);
	g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

float MeshField_GetHalf()
{
	return FIELD_MESH_SIZE * FIELD_MESH_W_COUNT * 0.5f;
}
