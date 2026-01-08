/*==============================================================================

   シェーダー3D[shader.cpp]
														 Author : Youhei Sato
														 Date   : 2025/09/10
--------------------------------------------------------------------------------

==============================================================================*/
#include "shader3D.h"
#include "debug_ostream.h"
#include"direct3d.h"
#include"sampler.h"
#include <DirectXMath.h>
#include <d3d11.h>
#include <fstream>

using namespace DirectX;

static ID3D11VertexShader* g_pVertexShader = nullptr; //このポインタはCreateVertexShader()でHLSLのcsoファイルをGPUに渡した後にハンドルを貰う
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr; // 定数バッファb0: world
//static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; // 定数バッファb1: view
//static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr; // 定数バッファb2: proj
static ID3D11Buffer* g_pPSConstantBuffer0 = nullptr; // 定数バッファb0
static ID3D11PixelShader* g_pPixelShader = nullptr;

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

struct VSConst0 { DirectX::XMFLOAT4X4 view; DirectX::XMFLOAT4X4 proj; };
static VSConst0 g_vs0{};\


bool Shader3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr; // 戻り値格納用

	// デバイスとデバイスコンテキストがあるかチェック
	if (!pDevice || !pContext) {
		hal::dout << "Shader_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return false;
	}

	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 事前コンパイル済み頂点シェーダーの読み込み
	//メモリにcsoファイル読み込む(GPUはまだHLSL文を知らない）
	//csoファイルはビルド時に作られる
	std::ifstream ifs_vs("shader_vertex_3d.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_3d.cso", "エラー", MB_OK);
		return false;
	}

	// ファイルサイズを取得
	ifs_vs.seekg(0, std::ios::end); // ファイルポインタを末尾に移動
	std::streamsize filesize = ifs_vs.tellg(); // ファイルポインタの位置を取得（つまりファイルサイズ）
	ifs_vs.seekg(0, std::ios::beg); // ファイルポインタを先頭に戻す

	// バイナリデータを格納するためのバッファを確保
	unsigned char* vsbinary_pointer = new unsigned char[filesize];

	ifs_vs.read((char*)vsbinary_pointer, filesize); // バイナリデータを読み込む
	ifs_vs.close(); // ファイルを閉じる


	// 作成部（Initialize 内）
	/*D3D11_BUFFER_DESC bd0{};
	bd0.ByteWidth = sizeof(VSConst0);                 // ★ b0: view+proj の2行列
	bd0.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_pDevice->CreateBuffer(&bd0, nullptr, &g_pVSConstantBuffer0);

	D3D11_BUFFER_DESC bd1{};
	bd1.ByteWidth = sizeof(DirectX::XMFLOAT4X4);      // ★ b1: world 1行列
	bd1.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_pDevice->CreateBuffer(&bd1, nullptr, &g_pVSConstantBuffer1);*/

	// Initialize内
	/*D3D11_BUFFER_DESC bd{};
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
	g_pDevice->CreateBuffer(&bd, nullptr, &g_pVSConstantBuffer0); // world
	g_pDevice->CreateBuffer(&bd, nullptr, &g_pVSConstantBuffer1); // view
	g_pDevice->CreateBuffer(&bd, nullptr, &g_pVSConstantBuffer2); // proj*/


	/*======ビルド後の実行時にCreateVertexShader()で.csoファイルに圧縮されたHLSLコードをGPUに渡す*/
	// 頂点シェーダーの作成
	hr = g_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
		delete[] vsbinary_pointer; // メモリリークしないようにバイナリデータのバッファを解放
		return false;
	}


	// 頂点レイアウトの定義
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT num_elements = ARRAYSIZE(layout); // 配列の要素数を取得

	// 頂点レイアウトの作成
	hr = g_pDevice->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInputLayout);

	delete[] vsbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}
	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0); // world
	//g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1); // view
	//g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer2); // proj


	// 事前コンパイル済みピクセルシェーダーの読み込み
	std::ifstream ifs_ps("shader_pixel_3d.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_2d.cso", "エラー", MB_OK);
		return false;
	}

	ifs_ps.seekg(0, std::ios::end);
	filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);

	unsigned char* psbinary_pointer = new unsigned char[filesize];
	ifs_ps.read((char*)psbinary_pointer, filesize);
	ifs_ps.close();

	// ピクセルシェーダーの作成
	hr = g_pDevice->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pPixelShader);

	delete[] psbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
		return false;
	}

	// ピクセルシェーダー用定数バッファの作成
	/*D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer0); // world*/
	D3D11_BUFFER_DESC ps_buffer_desc{};
	ps_buffer_desc.ByteWidth = sizeof(XMFLOAT4X4);
	ps_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_pDevice->CreateBuffer(&ps_buffer_desc, nullptr, &g_pPSConstantBuffer0);


	/*==サンプラーステイト設定はsampler.cpp/hに移した*/
	Sampler_SetFilterAnisotropic();

	return true;
}




void Shader3D_Finalize()
{
	SAFE_RELEASE(g_pPSConstantBuffer0);
	SAFE_RELEASE(g_pVSConstantBuffer0);
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);
	g_pDevice = nullptr;
	g_pContext = nullptr;
}
void Shader3D_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	//===================UpdateSubresourceはデータをGPUに渡す関数=====================
	// 定数バッファに行列をセット
	g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

/*void Shader3D_SetViewMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定数バッファに行列をセット
	g_pContext->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &transpose, 0, 0);
	/*XMMATRIX mt = XMMatrixTranspose(matrix);
	XMStoreFloat4x4(&g_vs0.view, mt);
	// b0 は view と proj をまとめて送る設計なので、構造体ごと更新
	g_pContext->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &g_vs0, 0, 0);
}

void Shader3D_SetProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定数バッファに行列をセット
	g_pContext->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &transpose, 0, 0);
	/*XMMATRIX mt = XMMatrixTranspose(matrix);
	XMStoreFloat4x4(&g_vs0.proj, mt);                    // ★ proj だけ更新
	g_pContext->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &g_vs0, 0, 0); // ★ b0全体送る
}*/

void Shader3d_SetColor(const DirectX::XMFLOAT4& color)
{
	// 定数バッファに行列をセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void Shader3D_Begin()
{
	//=======VSSetShader() や VSSetConstantBuffers()がGPUへUpdateSubresource()で送ったデータを元にした描画を命令する関数====
	// 
	// 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

	// 頂点レイアウトを描画パイプラインに設定
	g_pContext->IASetInputLayout(g_pInputLayout);

	// 定数バッファ(VS)を描画パイプラインに設定
	g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0); // world

	// 定数バッファ（PS）を設定（色用）
	g_pContext->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

	//サンプラーステイトを描画パイプラインに設定
	//g_pContext->PSSetSamplers(0, 1, &g_pSamplerState);
	// ◎ 3Dは遠景の床などに効く異方性
	Sampler_SetFilterAnisotropic();
}