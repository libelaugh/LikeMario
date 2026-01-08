#include "shader_depth.h"
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
static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; // 定数バッファb1: view
static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr; // 定数バッファb2: proj
static ID3D11Buffer* g_pPSConstantBuffer0 = nullptr; // 定数バッファb0
static ID3D11PixelShader* g_pPixelShader = nullptr;

bool ShaderDepth_Initialize()
{
	HRESULT hr; // 戻り値格納用


	// 事前コンパイル済み頂点シェーダーの読み込み
	//メモリにcsoファイル読み込む(GPUはまだHLSL文を知らない）
	//csoファイルはビルド時に作られる
	std::ifstream ifs_vs("shader_vertex_depth.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_depth.cso", "エラー", MB_OK);
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



	/*======ビルド後の実行時にCreateVertexShader()で.csoファイルに圧縮されたHLSLコードをGPUに渡す*/
	// 頂点シェーダーの作成
	hr = Direct3D_GetDevice()->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

	if (FAILED(hr)) {
		hal::dout << "ShaderDepth_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
		delete[] vsbinary_pointer; // メモリリークしないようにバイナリデータのバッファを解放
		return false;
	}


	// 頂点レイアウトの定義
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	UINT num_elements = ARRAYSIZE(layout); // 配列の要素数を取得

	// 頂点レイアウトの作成
	hr = Direct3D_GetDevice()->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInputLayout);

	delete[] vsbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "ShaderDepth_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}
	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0); // world
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1); // b1 view
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer2); // b2 proj


	// 事前コンパイル済みピクセルシェーダーの読み込み
	std::ifstream ifs_ps("shader_pixel_depth.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_depth.cso", "エラー", MB_OK);
		return false;
	}

	ifs_ps.seekg(0, std::ios::end);
	filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);

	unsigned char* psbinary_pointer = new unsigned char[filesize];
	ifs_ps.read((char*)psbinary_pointer, filesize);
	ifs_ps.close();

	// ピクセルシェーダーの作成
	hr = Direct3D_GetDevice()->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pPixelShader);

	delete[] psbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "ShaderDepth_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
		return false;
	}

	D3D11_BUFFER_DESC ps_buffer_desc{};
	ps_buffer_desc.ByteWidth = sizeof(XMFLOAT4);
	ps_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Direct3D_GetDevice()->CreateBuffer(&ps_buffer_desc, nullptr, &g_pPSConstantBuffer0);

	/*==サンプラーステイト設定はsampler.cpp/hに移した*/
	Sampler_SetFilterAnisotropic();

	return true;
}

void ShaderDepth_Finalize()
{
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pPSConstantBuffer0);
	SAFE_RELEASE(g_pVSConstantBuffer0);
	SAFE_RELEASE(g_pVSConstantBuffer1);
	SAFE_RELEASE(g_pVSConstantBuffer2);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);
}

void ShaderDepth_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	//===================UpdateSubresourceはデータをGPUに渡す関数=====================
	// 定数バッファに行列をセット
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void ShaderDepth_SetViewMatrix(const DirectX::XMMATRIX& matrix)
{
	XMFLOAT4X4 t;
	XMStoreFloat4x4(&t, XMMatrixTranspose(matrix));
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &t, 0, 0);
}

void ShaderDepth_SetProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
	XMFLOAT4X4 t;
	XMStoreFloat4x4(&t, XMMatrixTranspose(matrix));
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &t, 0, 0);
}


void ShaderDepth_SetColor(const DirectX::XMFLOAT4& color)
{
	if (!g_pPSConstantBuffer0) {
		// ログ出したり、アサートしたり
		hal::dout << "ShaderDepth_SetColor : g_pPSConstantBuffer0 が null\n";
		return;
	}

	// 定数バッファに行列をセット
	Direct3D_GetContext()->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void ShaderDepth_Begin()
{
	//=======VSSetShader() や VSSetConstantBuffers()がGPUへUpdateSubresource()で送ったデータを元にした描画を命令する関数====
	// 
	// 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	Direct3D_GetContext()->VSSetShader(g_pVertexShader, nullptr, 0);
	Direct3D_GetContext()->PSSetShader(g_pPixelShader, nullptr, 0);

	// 頂点レイアウトを描画パイプラインに設定
	Direct3D_GetContext()->IASetInputLayout(g_pInputLayout);

	// 定数バッファ(VS)を描画パイプラインに設定
	//Direct3D_GetContext()->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0); // world
	ID3D11Buffer* vsCBs[] = { g_pVSConstantBuffer0, g_pVSConstantBuffer1, g_pVSConstantBuffer2 };
	Direct3D_GetContext()->VSSetConstantBuffers(0, 3, vsCBs);

	// 定数バッファ（PS）を設定（色用）
	Direct3D_GetContext()->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);
}
