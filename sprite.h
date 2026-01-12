/*==============================================================================

   スプライト表示 [sprite.h]
														 Author : Tanaka Kouki
														 Date   : 2025/06/12
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SPRITE_H
#define SPRITE_H


#include<d3d11.h>
#include <DirectXMath.h>

void Sprite_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Sprite_Finalize(void);

void Sprite_Begin();



//Sprite_Draw４つの共通コード部分の説明は１つめの関数にあり
/*シェーダー・頂点バッファ・テクスチャなど
描画に必要な処理もすべてこの関数内で完結しています。*/

//テクスチャ全表示
/*dx,dyが左上の座標*/
void Sprite_Draw(int texid, float dx, float dy,
	             const DirectX::XMFLOAT4&color ={ 1.0f, 1.0f, 1.0f,1.0f });


//テクスチャ全表示(表示のサイズ変更できるver)
/*dx,dyで左上の座標設定、残り３点をdw,dhを加算してスプライトの表示サイズを調整する*/
void Sprite_Draw(int texid,float dx, float dy, float dw, float dh,
	             const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f,1.0f });

//テクスチャの一部分（切り抜き）を、画面に描画する
//UVカット(テクスチャの切り取り）               //pが切り取るテクスチャのピクセル座標//pw,phで残り３つ調整
void Sprite_Draw(int texid, float dx, float dy, int px,int py,int pw,int ph,
	             const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f,1.0f });

//UVカット(表示のサイズ変更できるver)           //pが切り取るテクスチャのピクセル座標//pw,phで残り３つ調整
void Sprite_Draw04(int texid, float dx, float dy, float dw, float dh,int px, int py, int pw, int ph, 
	             const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f,1.0f });


//UVカット(表示のサイズ変更できるver)回転ver           //pが切り取るテクスチャのピクセル座標//pw,phで残り３つ調整
void Sprite_Draw(int texid, float dx, float dy, float dw, float dh, int px, int py, int pw, int ph,
	float angle,
	const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f,1.0f });


//テクスチャ全表示(表示のサイズ変更できるver)
/*dx,dyで左上の座標設定、残り３点をdw,dhを加算してスプライトの表示サイズを調整する*/
void Sprite_Draw(float dx, float dy, float dw, float dh,
	const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f,1.0f });

#endif//SPRITE_H

