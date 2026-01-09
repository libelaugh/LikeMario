/*==============================================================================

   テクスチャーの管理[texture.h]
														 Author : Youhei Sato
														 Date   : 2025/06/13
--------------------------------------------------------------------------------

==============================================================================*/


#ifndef TEXTURE_H
#define TEXTURE_H


#include<d3d11.h>


void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Texture_Finalize(void);

// テクスチャ画像の読み込み
//
// 戻り値：管理番号。読み込めなかった場合-1。
//
int Texture_Load(const wchar_t* pFilename);

void Texture_AllRelease();

void Texture_SetTexture(int texid, int slot = 0);
//テクスチャーの幅高さ
unsigned int Texture_Width(int texid);
unsigned int Texture_Height(int texid);

/*Texture_Initialize(...)：テクスチャ管理の初期化。デバイス保存。

Texture_Finalize()：すべてのテクスチャを解放。

Texture_Load(...)：ファイルから画像を読み込んでテクスチャ化。

Texture_SetTexture(...)：描画に使うテクスチャをセット。

TextureWidth() / TextureHeight()：読み込んだテクスチャのサイズ取得。

Texture_AllRelease()：全解放処理。*/



#endif//TEXTURE_H
