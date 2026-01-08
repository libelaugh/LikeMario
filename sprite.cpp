/*==============================================================================

  　スプライト [suprite.cpp]
														 Author : Youhei Sato
														 Date   : 2025/06/12
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "shader3d.h"
#include "debug_ostream.h" 
#include "sprite.h"
#include"texture.h"



static constexpr int NUM_VERTEX = 4; // 頂点数

/*頂点バッファとは？
頂点（position・色・UVなど）データ、つまりポリゴンをまとめて保管してGPUに送るためのメモリの箱です。

DirectXでの頂点バッファの使い方の流れ
1.バッファを作る（最初に一度だけ）：
ID3D11Buffer* g_pVertexBuffer; として作る

2.ロックして書き込み（Map）
g_pContext->Map(g_pVertexBuffer, ...);

3.ポリゴンの頂点データを記入
v[0].position = ...;
v[0].uv = ...;

4.アンロックしてGPUに戻す
g_pContext->Unmap(g_pVertexBuffer, 0);

5.描画時に「これ使って！」と指定
g_pContext->IASetVertexBuffers(..., &g_pVertexBuffer, ...);*/
static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ（上行に説明）
static ID3D11ShaderResourceView* g_pTexture = nullptr; //テクスチャ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


// 頂点構造体
//Vertex構造体	各頂点の情報（座標・UV・色など）をまとめたもの
struct Vertex
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4  color;
	XMFLOAT2 uv;//テクスチャ座標
};




void Sprite_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Sprite_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

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


	
}


void Sprite_Finalize(void)
{
	SAFE_RELEASE(g_pTexture);
	SAFE_RELEASE(g_pVertexBuffer);
}

void Sprite_Begin()
{
	// 頂点シェーダーに変換行列を設定
	// 頂点情報を書き込み
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();


	//Shader3D_SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));
}

//指定した位置にテクスチャを貼った四角形（スプライト）を描画する関数です。
/*色や大きさは自動でテクスチャのサイズに合わせて設定され、シェーダー・頂点バッファ・テクスチャなど
描画に必要な処理もすべてこの関数内で完結しています。*/
void Sprite_Draw(int texid, float dx, float dy, const DirectX::XMFLOAT4& color)
{
	// シェーダーを描画パイプラインに設定
	/*描画用シェーダーの準備
→ これを呼ばないと、GPUが「どう描画するか」をわかりません。*/
	Shader3D_Begin();

	// 頂点バッファをロックする
	/* GPUのメモリにある「頂点バッファ」を一時的にCPU側から書き換えられるようにします
Map()：ロックして書き込み開始
v：頂点データを書き込むためのポインタ*/
	
	D3D11_MAPPED_SUBRESOURCE msr;//GPUのバッファにアクセスするための情報（構造体）**です
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;


	// 画面の左上から右下に向かう線分を描画する

	/*// 頂点データの初期化用変数
	constexpr float w = 512.0f;
	constexpr float h = 512.0f;*/

	/*指定されたテクスチャの幅 (dw) と高さ (dh) を取得します
→ 頂点の座標に加算してスプライトのサイズが自動的に画像サイズになるようにします。*/
	unsigned int dw = Texture_Width(texid);
	unsigned int dh = Texture_Height(texid);
	//4つの頂点の「座標」を設定//画像のサイズ調整//四角形のポリゴン作る
	/*ポリゴン（四角形＝スプライト）を作るための頂点の座標を定義しています。
	 どうやってポリゴンになるの？
描画モードがこれ：
g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); 
描画方式が「TRIANGLESTRIP（三角形ストリップ）」になってるので、
順番に並べることで自動的にポリゴン（四角形）になります。
[0]------[1]
 |     /  |
 |   /    |
[2]------[3]
このように
三角形① → 頂点 0 → 1 → 2
三角形② → 頂点 2 → 1 → 3
という2つの三角形で**1枚の四角形ポリゴン（スプライト）**を作っています。
(頂点３つにすれば三角形のポリゴン作れる）*/
	v[0].position = { dx,        dy,        0.0f }; // 左上
	v[1].position = { dx+dw ,   dy,        0.0f }; // 右上
	v[2].position = { dx,        dy+dh ,   0.0f }; // 左下
	v[3].position = { dx +dw,   dy+dh ,   0.0f }; // 右下

	/*全ての頂点に同じ色を設定
→ 白にすればそのままの画像、赤にすれば赤く染まった画像になります。*/
	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;


	/*画像のどの部分を貼り付けるかを設定
→ (0.0〜1.0) の範囲で「画像の左上〜右下」を指定。*/
	v[0].uv = { 0.0f, 0.0f, };//左上
	v[1].uv = { 1.0f, 0.0f, };//右上
	v[2].uv = { 0.0f, 1.0f, };//左下
	v[3].uv = { 1.0f, 1.0f, };//右下




	// 頂点バッファのロックを解除
	// 書き込んだ頂点データをGPUに戻す（ロック解除）
	g_pContext->Unmap(g_pVertexBuffer, 0);


	//world変換行列を設定
	//XMMatrixIdentity単位行列を作る　かけても変わらんやつ１と同じ
	Shader3D_SetWorldMatrix(XMMatrixIdentity());

	// 頂点バッファを描画パイプラインに設定
	//どの頂点バッファを使うかをGPUに伝える処理
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	


	// プリミティブトポロジ設定（描画方式）
	/*描画方式を「三角形ストリップ」に設定(変更可能）
→ 4頂点で2枚の三角形をつないで、四角形を1枚描きます。*/
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//テクスチャの設定
	//これで指定したテクスチャ（texid番の画像）を GPU に渡します。
	Texture_SetTexture(texid);

	// ポリゴン描画命令発行
	/*これで、positionで作ったポリゴンにuvで切り取ったテクスチャを貼り付けて、
	テクスチャが貼られた四角形（スプライト）が画面に描画されます。*/
	g_pContext->Draw(NUM_VERTEX, 0);
}



void Sprite_Draw(int texid, float dx, float dy, float dw, float dh, const DirectX::XMFLOAT4& color)
{
	// シェーダーを描画パイプラインに設定
	Shader3D_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	

	// 画面の左上から右下に向かう線分を描画する

	/*// 頂点データの初期化用変数
	constexpr float w = 512.0f;
	constexpr float h = 512.0f;*/

	/*dx,dyで左上の座標設定、残り３点にdw,dhを加算してスプライトの表示サイズを調整する*/
	v[0].position = { dx,        dy,        0.0f }; // 左上
	v[1].position = { dx + dw,   dy,        0.0f }; // 右上
	v[2].position = { dx,        dy + dh,   0.0f }; // 左下
	v[3].position = { dx + dw,   dy + dh,   0.0f }; // 右下


	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;


	v[0].uv = { 0.0f, 0.0f, };//左上
	v[1].uv = { 1.0f, 0.0f, };//右上
	v[2].uv = { 0.0f, 1.0f, };//左下
	v[3].uv = { 1.0f, 1.0f, };//右下




	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//world変換行列を設定
	//XMMatrixIdentity単位行列を作る　かけても変わらんやつ１と同じ
	Shader3D_SetWorldMatrix(XMMatrixIdentity());

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);




	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//テクスチャの設定
	Texture_SetTexture(texid);

	// ポリゴン描画命令発行
	g_pContext->Draw(NUM_VERTEX, 0);
}




void Sprite_Draw(int texid, float dx, float dy, int px, int py, int pw, int ph, const DirectX::XMFLOAT4& color)
{
	// シェーダーを描画パイプラインに設定
	Shader3D_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;



	// 画面の左上から右下に向かう線分を描画する

	/*// 頂点データの初期化用変数
	constexpr float w = 512.0f;
	constexpr float h = 512.0f;*/

	v[0].position = { dx,        dy,        0.0f }; // 左上
	v[1].position = { dx + pw,   dy,        0.0f }; // 右上
	v[2].position = { dx,        dy + ph,   0.0f }; // 左下
	v[3].position = { dx + pw,   dy + ph,   0.0f }; // 右下


	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;

	//テクスチャの幅、高さ（単位はピクセル）
	float tw = (float)Texture_Width(texid);
	float th = (float)Texture_Height(texid);

	/*ピクセル単位の座標をUV（0.0〜1.0）に変換しています。%計算と同じ
→ テクスチャ画像の中で、どこからどこまでを使うかをGPUに伝えるための処理です。*/
    float u0 = px / tw;
    float v0 = py / th;
    float u1 = (px + pw) / tw;
    float v1 = (py + ph) / th;

	v[0].uv = { u0,v0 };//左上
	v[1].uv = { u1,v0 };//右上
	v[2].uv = { u0,v1 };//左下
	v[3].uv = { u1,v1 };//右下




	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);


	//world変換行列を設定
	//XMMatrixIdentity単位行列を作る　かけても変わらんやつ１と同じ
	Shader3D_SetWorldMatrix(XMMatrixIdentity());

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);




	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//テクスチャの設定
	Texture_SetTexture(texid);

	// ポリゴン描画命令発行
	g_pContext->Draw(NUM_VERTEX, 0);
}




void Sprite_Draw04(int texid, float dx, float dy, float dw, float dh, int px, int py, int pw, int ph,
	const DirectX::XMFLOAT4& color)
{
	// シェーダーを描画パイプラインに設定
	Shader3D_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	

	// 画面の左上から右下に向かう線分を描画する

	/*// 頂点データの初期化用変数
	constexpr float w = 512.0f;
	constexpr float h = 512.0f;*/

	//dw,dhで画像のサイズ調整
	v[0].position = { dx,        dy,        0.0f }; // 左上
	v[1].position = { dx + dw,   dy,        0.0f }; // 右上
	v[2].position = { dx,        dy + dh,   0.0f }; // 左下
	v[3].position = { dx + dw,   dy + dh,   0.0f }; // 右下


	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;


	float tw = (float)Texture_Width(texid);
	float th = (float)Texture_Height(texid);

	//uvは０～１だから切り取りたい部分の座標/テクスチャ全体＝uとｖの値（割合）＝０～１（*10で％) ex)0.25とか
	float u0 = px / tw; //ここにstatic入ってて最長デバック時間なった
	float v0 = py / th;
	float u1 = (px + pw) / tw;
	float v1 = (py + ph) / th;

	v[0].uv = { u0,v0 };//左上
	v[1].uv = { u1,v0 };//右上
	v[2].uv = { u0,v1 };//左下
	v[3].uv = { u1,v1 };//右下




	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//world変換行列を設定
	//XMMatrixIdentity単位行列を作る　かけても変わらんやつ１と同じ
	Shader3D_SetWorldMatrix(XMMatrixIdentity());

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);




	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//テクスチャの設定
	Texture_SetTexture(texid);

	// ポリゴン描画命令発行
	g_pContext->Draw(NUM_VERTEX, 0);
}


void Sprite_Draw(int texid, float dx, float dy, float dw, float dh, int px, int py, int pw, int ph,
	float angle, const DirectX::XMFLOAT4& color)
{
	// シェーダーを描画パイプラインに設定
	Shader3D_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;



	// 画面の左上から右下に向かう線分を描画する

	/*// 頂点データの初期化用変数
	constexpr float w = 512.0f;
	constexpr float h = 512.0f;*/

	//dw,dhで画像のサイズ調整
	v[0].position = { -0.5f,    -0.5f,   0.0f }; // 左上
	v[1].position = { +0.5f ,   -0.5f,   0.0f }; // 右上
	v[2].position = { -0.5f,    +0.5f,   0.0f }; // 左下
	v[3].position = { +0.5f ,   +0.5f,   0.0f }; // 右下


	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;


	float tw = (float)Texture_Width(texid);
	float th = (float)Texture_Height(texid);

	//uvは０～１だから切り取りたい部分の座標/テクスチャ全体＝uとｖの値（割合）＝０～１（*10で％) ex)0.25とか
	float u0 = px / tw; //ここにstatic入ってて最長デバック時間なった
	float v0 = py / th;
	float u1 = (px + pw) / tw;
	float v1 = (py + ph) / th;

	v[0].uv = { u0,v0 };//左上
	v[1].uv = { u1,v0 };//右上
	v[2].uv = { u0,v1 };//左下
	v[3].uv = { u1,v1 };//右下




	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);


	//world変換行列を設定
	//XMMatrixIdentity単位行列を作る　かけても変わらんやつ１と同じ
	Shader3D_SetWorldMatrix(XMMatrixIdentity());

	//回転行列をシェーダーに設定
	//XMMatrix~関数色々種類あって色々できる
	//2次元アフィン変換s,r,tの順番
	/*v[0].position = { -0.5f,    -0.5f,   0.0f }; // 左上
	v[1].position = { +0.5f ,   -0.5f,   0.0f }; // 右上
	v[2].position = { -0.5f,    +0.5f,   0.0f }; // 左下
	v[3].position = { +0.5f ,   +0.5f,   0.0f }; // 右下
	これで左上に中心座標が(0,0)の１＊１のポリゴン作って、ここでＺ軸回転してる
	中心がピボットポイント
	
	それをｓ、ｒ、ｔするとテクスチャ自身が回転してるように見える
	本当はＺ軸回転してる*/
	XMMATRIX scale = XMMatrixScaling(dw, dh, 1.0f);//拡大縮小
    XMMATRIX rotation = XMMatrixRotationZ(angle);//これだけでZ軸回転できる単位はrad
	XMMATRIX translation = XMMatrixTranslation(dx, dy, 0.0f);//dx,dyに平行移動
	
	Shader3D_SetWorldMatrix(scale*rotation*translation);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);




	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//テクスチャの設定
	Texture_SetTexture(texid);

	// ポリゴン描画命令発行
	g_pContext->Draw(NUM_VERTEX, 0);
}

void Sprite_Draw(float dx, float dy, float dw, float dh, const DirectX::XMFLOAT4& color)
{
	Shader3D_Begin();

	// 頂点バッファをロックする
	/* GPUのメモリにある「頂点バッファ」を一時的にCPU側から書き換えられるようにします
Map()：ロックして書き込み開始
v：頂点データを書き込むためのポインタ*/

	D3D11_MAPPED_SUBRESOURCE msr;//GPUのバッファにアクセスするための情報（構造体）**です
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;


	v[0].position = { dx,        dy,        0.0f }; // 左上
	v[1].position = { dx + dw ,   dy,        0.0f }; // 右上
	v[2].position = { dx,        dy + dh ,   0.0f }; // 左下
	v[3].position = { dx + dw,   dy + dh ,   0.0f }; // 右下

	/*全ての頂点に同じ色を設定
→ 白にすればそのままの画像、赤にすれば赤く染まった画像になります。*/
	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;


	/*画像のどの部分を貼り付けるかを設定
→ (0.0〜1.0) の範囲で「画像の左上〜右下」を指定。*/
	v[0].uv = { 0.0f, 0.0f, };//左上
	v[1].uv = { 1.0f, 0.0f, };//右上
	v[2].uv = { 0.0f, 1.0f, };//左下
	v[3].uv = { 1.0f, 1.0f, };//右下




	// 頂点バッファのロックを解除
	// 書き込んだ頂点データをGPUに戻す（ロック解除）
	g_pContext->Unmap(g_pVertexBuffer, 0);


	//world変換行列を設定
	//XMMatrixIdentity単位行列を作る　かけても変わらんやつ１と同じ
	Shader3D_SetWorldMatrix(XMMatrixIdentity());

	// 頂点バッファを描画パイプラインに設定
	//どの頂点バッファを使うかをGPUに伝える処理
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);




	// プリミティブトポロジ設定（描画方式）
	/*描画方式を「三角形ストリップ」に設定(変更可能）
→ 4頂点で2枚の三角形をつないで、四角形を1枚描きます。*/
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


	// ポリゴン描画命令発行
	/*これで、positionで作ったポリゴンにuvで切り取ったテクスチャを貼り付けて、
	テクスチャが貼られた四角形（スプライト）が画面に描画されます。*/
	g_pContext->Draw(NUM_VERTEX, 0);
}




/*void Sprite_Draw(float dx, float dy)
{
	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	

	// 画面の左上から右下に向かう線分を描画する
	// 頂点データの初期化用変数
	
	constexpr float w = 512.0f;
	constexpr float h = 512.0f;

	v[0].position = { dx ,    dy,     0.0f };//左上
	v[1].position = { dx + w, dy,     0.0f };//右上
	v[2].position = { dx,     dy + h, 0.0f };//左下
	v[3].position = { dx + w, dy + h, 0.0f };//右下

	v[0].color = { 1.0f, 1.0f, 1.0f,1.0f };
	v[1].color = { 1.0f, 1.0f, 1.0f,1.0f };
	v[2].color = { 1.0f, 1.0f, 1.0f,1.0f };
	v[3].color = { 1.0f, 1.0f, 1.0f,1.0f 

		//頂点情報を書き込み
	float tw = (float)Texture_Width(texid);
	float th = (float)Texture_Height(texid);
   
	static float u0 = px  / tw;
	static float v0 = py  / th;
	static float u1 = (px+pw)/tw;
	static float v1 = (py+ph)/th;

	v[0].uv = { u0,v0 };//左上
	v[1].uv = { u1,v0 };//右上
	v[2].uv = { u0,v1 };//左下
	v[3].uv = { u1,v1 };//右下

	テクスチャのトリミング
	static float u0 = 32.0f * 1.0f / 512.0f;
	static float v0 = 32.0f * 5.0f / 512.0f;
	static float u1 = 32.0f * 2.0f / 512.0f;
	static float v1 = 32.0f * 6.0f / 512.0f;

	v[0].uv = {u0,v0 };//左上
	v[1].uv = { u1,v0 };//右上
	v[2].uv = { u0,v1};//左下
	v[3].uv = { u1,v1 };//右下


	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// 頂点シェーダーに変換行列を設定
	// 頂点情報を書き込み
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();
	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));


	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);



	// ポリゴン描画命令発行
	g_pContext->Draw(NUM_VERTEX, 0);
}
*/
