/*==============================================================================

   テクスチャーの管理[texture.h]
														 Author : Youhei Sato
														 Date   : 2025/06/13
--------------------------------------------------------------------------------

==============================================================================*/


#include "texture.h"
#include"d3d11.h"//Releaseを使うため
#include "direct3d.h"
#include"WICTextureLoader11.h"
#include<string>

using namespace DirectX;


//テクスチャー最大管理枚数
static constexpr int TEXTURE_MAX = 256;

struct Texture {
	std::wstring filename;
	ID3D11Resource* pTexture = nullptr;
	ID3D11ShaderResourceView* pTextureView;//シェーダーがアクセスできるリソース（テクスチャやバッファ）」を表す
	unsigned int width;
	unsigned int height;
};


static Texture g_Textures[TEXTURE_MAX]{};
static  int g_SetTextureIndex = -1;

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	for (Texture& t : g_Textures) {
		t.pTextureView = nullptr;
	};

	g_SetTextureIndex = -1;

	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

}

void Texture_Finalize(void)
{
	Texture_AllRelease();
}

/*同じテクスチャを重複して読み込まないようにしつつ、新しいテクスチャを読み込んで管理番号を返す関数
 
この Texture_Load 関数は、指定された画像ファイル（テクスチャ）を
このプロジェクトに読み込んで使えるようにする処理です。
もう少し具体的に言うと：
pFilename に指定された画像ファイル（例：L"enemy.png"）を

DirectX用のテクスチャ（GPUで使える形式）に変換して

プログラム内の g_Textures という配列に登録します

そのテクスチャがすでに読み込まれていたら、再読み込みせずにその管理番号（インデックス）を返すようになっています
（具体的には読み込んだ画像（テクスチャ）は GPU（グラフィックボード）のメモリにアップロードされます。）
*/
int Texture_Load(const wchar_t* pFilename)
{
	//すでに読み込んだファイルは読み込まない
	for (int i = 0;i < TEXTURE_MAX;i++) {
		if (g_Textures[i].filename == pFilename) {//iは管理番号
			return i;
		}
	}
	//空いてる管理領域を探す
	for (int i = 0;i < TEXTURE_MAX;i++) {

		if (g_Textures[i].pTextureView)continue;//使用中


		//テクスチャの読み込み
		HRESULT hr;

	    hr = CreateWICTextureFromFile(g_pDevice, g_pContext, pFilename, &g_Textures[i].pTexture, &g_Textures[i].pTextureView);
	
		/*このようにして、hr の中身が成功か失敗かをチェックしています。
		*/
		if (FAILED(hr)) {
			MessageBoxW(nullptr, L"テクスチャの読み込みに失敗しました", pFilename, MB_OK | MB_ICONERROR);
			return -1;
		}

		ID3D11Texture2D* pTexture = (ID3D11Texture2D*)g_Textures[i].pTexture;
		D3D11_TEXTURE2D_DESC t2desc;
		pTexture->GetDesc(&t2desc);
		g_Textures[i].width = t2desc.Width;
		g_Textures[i].height = t2desc.Height;
		

		g_Textures[i].filename = pFilename;


		return i;
	}
	return -1;
}

void Texture_AllRelease()
{
	for (Texture& t : g_Textures) {
		t.filename.clear();
		SAFE_RELEASE(t.pTexture);
		SAFE_RELEASE(t.pTextureView);
	}
}

//テクスチャ召喚
void Texture_SetTexture(int texid, int slot)
{
	if (texid < 0)return;

	g_SetTextureIndex = texid;

	//テクスチャ設定
	g_pContext->PSSetShaderResources(slot, 1, &g_Textures[texid].pTextureView);
}

unsigned int Texture_Width(int texid)
{
	if (texid < 0)return 0;
	return g_Textures[texid].width;
}

unsigned int Texture_Height(int texid)
{
	if (texid < 0)return 0;
	return g_Textures[texid].height;
}
