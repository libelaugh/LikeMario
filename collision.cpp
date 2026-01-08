/*==============================================================================

　　　衝突の制御[collision.h]
														 Author : Tanaka Kouki
														 Date   : 2025/11/05
--------------------------------------------------------------------------------

==============================================================================*/

#include"collision.h"
#include"direct3d.h"
#include"texture.h"
#include"shader2d.h"
#include<algorithm>

using namespace DirectX;

static constexpr int NUM_VERTEX = 5000; // 頂点数
static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static int g_WhiteTexId = -1;

// 頂点構造体
struct Vertex
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4  color;
	XMFLOAT2 uv;//テクスチャ座標
	XMFLOAT2 texcoord;
};

bool Collision_IsOverlapSphere(const Sphere& a, const Sphere& b)
{
	XMVECTOR ac = XMLoadFloat3(&a.center);
	XMVECTOR bc = XMLoadFloat3(&b.center);
	XMVECTOR lsq = XMVector3LengthSq(bc - ac);

	return (a.radius * a.radius) * (a.radius * a.radius) > XMVectorGetX(lsq);
}

bool Collision_IsOverlapSphere(const Sphere& a, const DirectX::XMFLOAT3& point)
{
	XMVECTOR ac = XMLoadFloat3(&a.center);
	XMVECTOR bc = XMLoadFloat3(&point);
	XMVECTOR lsq = XMVector3LengthSq(bc - ac);

	return a.radius * a.radius > XMVectorGetX(lsq);
}

bool Collision_IsOverlapCircle(const Circle& a, const Circle& b)
{
	/*
	float x1 = a.center.x - b.center.x;
	float y1 = a.center.y - b.center.y;

	//衝突判定基準　ルートより２乗の方が処理が軽いからセオリーこれ　半径の和＞中心間の距離
	return (a.radius + b.radius) * (a.radius + b.radius) > (x1 * x1 + y1 * y1);*/

	//上のより処理早いかものやつ
	/*SIMD（並列計算命令）対応なので、高パフォーマンス。
下の XMVECTOR バージョンは 正しくて速い、安全な方法。
DirectXMath環境なら、下のベクトル版を使うべきです。特にゲームではミス防止・最適化の面で有利です。*/
	XMVECTOR ac = XMLoadFloat2(&a.center);
	XMVECTOR bc = XMLoadFloat2(&b.center);
	XMVECTOR lsq = XMVector2LengthSq(bc - ac);

	return(a.radius + b.radius) * (a.radius + b.radius) > XMVectorGetX(lsq);
};

bool Collision_IsOverlapBox(const Box& a, const Box& b)
{
	float at = a.center.y - a.half_height;
	float ab = a.center.y + a.half_height;
	float al = a.center.x - a.half_width;
	float ar = a.center.x + a.half_width;
	float bt = b.center.y - b.half_height;
	float bb = b.center.y + b.half_height;
	float br = b.center.x + b.half_width;
	float bl = b.center.x - b.half_width;

	return al<br && ar >bl && at<bb && ab>bt;
}

bool Collision_IsOverlapAABB(const AABB& a, const AABB& b)
{
	return a.min.x < b.max.x
		&& a.max.x > b.min.x
		&& a.min.y < b.max.y
		&& a.max.y > b.min.y
		&& a.min.z < b.max.z
		&& a.max.z > b.min.z;
}

Hit Collision_IsHitAABB(const AABB& a, const AABB& b)
{
	Hit hit{};

	//重なったか？
	hit.isHit = Collision_IsOverlapAABB(a, b);

	if (!hit.isHit) {
		return hit;
	}

	/*=================進入深度（MTVver)====================*/ //現代ゲーム開発でもよく使われる
	//各軸の深度を調べる
	const float xdepth = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
	const float ydepth = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
	const float zdepth = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

	//重なってる軸の内、最も深度が浅い軸＝「当たってる面」なるらしい
	// 一番浅い軸を面法線にする
	XMFLOAT3 n{ 0,0,0 };
	if (xdepth <= ydepth && xdepth <= zdepth) {
		// Aの中心とBの中心で符号を決める
		const float ax = (a.min.x + a.max.x) * 0.5f;
		const float bx = (b.min.x + b.max.x) * 0.5f;
		n.x = (ax < bx) ? 1.0f : -1.0f;
	}
	else if (ydepth <= xdepth && ydepth <= zdepth) {
		const float ay = (a.min.y + a.max.y) * 0.5f;
		const float by = (b.min.y + b.max.y) * 0.5f;
		n.y = (ay < by) ? 1.0f : -1.0f;
	}
	else {
		const float az = (a.min.z + a.max.z) * 0.5f;
		const float bz = (b.min.z + b.max.z) * 0.5f;
		n.z = (az < bz) ? 1.0f : -1.0f;
	}

	hit.normal = n;
	return hit;
	/*bool isShallowX = false;
	bool isShallowY = false;
	bool isShallowZ = false;

	if (xdepth > ydepth) {
		if (ydepth > zdepth) {
			//zの面
			isShallowZ = true;
		}
		else {
			//yの面
			isShallowZ = true;
		}
	}
	else {
		if (zdepth > xdepth) {
			//xの面
			isShallowZ = true;
		}
		else {
			//zの面
			isShallowZ = true;
		}
	}

	//+-どっちから当たったか
	XMFLOAT3 a_center = a.GetCenter(); XMFLOAT3 b_center = b.GetCenter();
	XMVECTOR normal = XMLoadFloat3(&a_center) - XMLoadFloat3(&b_center);

	if (isShallowX) {
		normal = XMVector3Normalize(normal * XMVECTOR{ 1.0f, 0.0f, 0.0f });
	}
	else if (isShallowX) {
		normal = XMVector3Normalize(normal * XMVECTOR{ 1.0f, 0.0f, 0.0f });
	}
	else if (isShallowX) {
		normal = XMVector3Normalize(normal * XMVECTOR{ 1.0f, 0.0f, 0.0f });
	}

	XMStoreFloat3(&hit.normal, normal);


		return hit;*/
}




void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	
	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	g_WhiteTexId = Texture_Load(L"white.png");
}

void Collision_DebugFinalize()
{
	SAFE_RELEASE(g_pVertexBuffer);//頂点バッファの後片付け
}


//衝突判定範囲を視覚化
void Collision_DebugDraw(const Circle& circle, const DirectX::XMFLOAT4& color)
{
	//点の数を算出
  int numVertex = (int)(circle.radius * 2.0f * XM_PI);//円周の長さ=点の数

  // シェーダーを描画パイプラインに設定
  Shader2D_Begin();

  Shader2D_SetWorldMatrix(XMMatrixIdentity());

  // 頂点バッファをロックする
  D3D11_MAPPED_SUBRESOURCE msr;
  g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

  // 頂点バッファへの仮想ポインタを取得
  Vertex* v = (Vertex*)msr.pData;

  // 頂点情報を書き込み
  const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
  const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

  const float rad = XM_2PI / numVertex;//1radをPIで表した

  for (int i = 0;i < numVertex;i++) {
	  v[i].position.x = cosf(rad * i) * circle.radius + circle.center.x;//x=cosΘ*半径+中心座標
	  v[i].position.y = sinf(rad * i) * circle.radius + circle.center.y;
	  v[i].position.z = 0;
	  v[i].color = color;
	  v[i].texcoord = { 0.0f,0.0f };
  }

  // 頂点バッファのロックを解除
  g_pContext->Unmap(g_pVertexBuffer, 0);

  // 頂点バッファを描画パイプラインに設定
  UINT stride = sizeof(Vertex);
  UINT offset = 0;
  g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

  // 頂点シェーダーに変換行列を設定
  Shader2D_SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

  // プリミティブトポロジ設定
  g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

  //テクスチャ設定
  //g_pContext->PSSetShaderResources(0, 1, &g_pTexture);

  // ポリゴン描画命令発行
  g_pContext->Draw(numVertex, 0);
}



void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4& color)
{

	// シェーダーを描画パイプラインに設定
	Shader2D_Begin();

	Shader2D_SetWorldMatrix(XMMatrixIdentity());

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	// 頂点情報を書き込み
	 // 頂点情報を書き込み
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

	v[0].position = { box.center.x - box.half_width, box.center.y - box.half_height,0.0f };
	v[1].position = { box.center.x - box.half_width, box.center.y - box.half_height,0.0f };
	v[2].position = { box.center.x - box.half_width, box.center.y - box.half_height,0.0f };
	v[3].position = { box.center.x - box.half_width, box.center.y - box.half_height,0.0f };
	v[4].position = { box.center.x - box.half_width, box.center.y - box.half_height,0.0f };

	for (int i = 0;i < 5;i++) {
		v[i].color = color;
		v[i].uv = { 0.0f,0.0f };
	}

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// 頂点シェーダーに変換行列を設定
	Shader2D_SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	//テクスチャ設定
	//g_pContext->PSSetShaderResources(0, 1, &g_pTexture);

	// ポリゴン描画命令発行
	g_pContext->Draw(5, 0);
};