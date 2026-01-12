/*==============================================================================

　　　ビルボードシェーダ[shader_billboard.h]
														 Author : Tanaka Kouki
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/

#include "shader_billboard.h"
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
static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; // b1: view
static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr; // b2: projection
static ID3D11Buffer* g_pVSConstantBuffer3 = nullptr; // 定数バッファb3
static ID3D11Buffer* g_pPSConstantBuffer0 = nullptr; // 定数バッファb0
static ID3D11PixelShader* g_pPixelShader = nullptr;


bool ShaderBillboard_Initialize()
{
	HRESULT hr; // 戻り値格納用


	// 事前コンパイル済み頂点シェーダーの読み込み
	//メモリにcsoファイル読み込む(GPUはまだHLSL文を知らない）
	//csoファイルはビルド時に作られる
	std::ifstream ifs_vs("shader_vertex_billboard.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_billboard.cso", "エラー", MB_OK);
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
	hr = Direct3D_GetDevice()->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

	if (FAILED(hr)) {
		hal::dout << "ShaderBillboard_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
		delete[] vsbinary_pointer; // メモリリークしないようにバイナリデータのバッファを解放
		return false;
	}


	// 頂点レイアウトの定義
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT num_elements = ARRAYSIZE(layout); // 配列の要素数を取得

	// 頂点レイアウトの作成
	hr = Direct3D_GetDevice()->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInputLayout);

	delete[] vsbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "ShaderBillboard_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}
	// 頂点シェーダー用定数バッファの作成
	// VS: world/view/proj (float4x4)
	D3D11_BUFFER_DESC mtx_desc{};
	mtx_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mtx_desc.Usage = D3D11_USAGE_DEFAULT;
	mtx_desc.ByteWidth = sizeof(DirectX::XMFLOAT4X4);

	Direct3D_GetDevice()->CreateBuffer(&mtx_desc, nullptr, &g_pVSConstantBuffer0); // b0 world
	Direct3D_GetDevice()->CreateBuffer(&mtx_desc, nullptr, &g_pVSConstantBuffer1); // b1 view
	Direct3D_GetDevice()->CreateBuffer(&mtx_desc, nullptr, &g_pVSConstantBuffer2); // b2 projection

	// VS: UVParameter (float2 + float2 = 16 bytes)
	D3D11_BUFFER_DESC uv_desc{};
	uv_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	uv_desc.Usage = D3D11_USAGE_DEFAULT;
	uv_desc.ByteWidth = sizeof(UVParameter); // 16 bytes（OK）

	Direct3D_GetDevice()->CreateBuffer(&uv_desc, nullptr, &g_pVSConstantBuffer3); // b3 uv



	// 事前コンパイル済みピクセルシェーダーの読み込み
	std::ifstream ifs_ps("shader_pixel_billboard.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_billboard.cso", "エラー", MB_OK);
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
		hal::dout << "ShaderBillboard_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
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
	Direct3D_GetDevice()->CreateBuffer(&ps_buffer_desc, nullptr, &g_pPSConstantBuffer0);


	/*==サンプラーステイト設定はsampler.cpp/hに移した*/
	Sampler_SetFilterAnisotropic();

	return true;
}

void ShaderBillboard_Finalize()
{
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pVSConstantBuffer0);
	SAFE_RELEASE(g_pVSConstantBuffer1);
	SAFE_RELEASE(g_pVSConstantBuffer2);
	SAFE_RELEASE(g_pVSConstantBuffer3);
	SAFE_RELEASE(g_pPSConstantBuffer0);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);
}

void ShaderBillboard_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	//===================UpdateSubresourceはデータをGPUに渡す関数=====================
	// 定数バッファに行列をセット
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void ShaderBillboard_SetViewMatrix(const DirectX::XMMATRIX& matrix)
{
	DirectX::XMFLOAT4X4 t;
	DirectX::XMStoreFloat4x4(&t, DirectX::XMMatrixTranspose(matrix));
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &t, 0, 0);
}

void ShaderBillboard_SetProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
	DirectX::XMFLOAT4X4 t;
	DirectX::XMStoreFloat4x4(&t, DirectX::XMMatrixTranspose(matrix));
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &t, 0, 0);
}


void ShaderBillboard_SetColor(const DirectX::XMFLOAT4& color)
{
	// 定数バッファに行列をセット
	Direct3D_GetContext()->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void ShaderBillboard_SetUVParameter(const UVParameter& parameter)
{
	// 定数バッファに行列をセット
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer3, 0, nullptr, &parameter, 0, 0);
}

void ShaderBillboard_Begin()
{
	Direct3D_GetContext()->VSSetShader(g_pVertexShader, nullptr, 0);
	Direct3D_GetContext()->PSSetShader(g_pPixelShader, nullptr, 0);
	Direct3D_GetContext()->IASetInputLayout(g_pInputLayout);

	Direct3D_GetContext()->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0); // b0 world
	Direct3D_GetContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1); // b1 view
	Direct3D_GetContext()->VSSetConstantBuffers(2, 1, &g_pVSConstantBuffer2); // b2 projection
	Direct3D_GetContext()->VSSetConstantBuffers(3, 1, &g_pVSConstantBuffer3); // b3 uv
}

