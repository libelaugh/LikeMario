/*==============================================================================

   スプライトアニメーション描画[sprite_anim.h]
														 Author : Youhei Sato
														 Date   : 2025/06/17
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SPRITE_ANIM_H
#define SPRITE_ANIM_H

#include<DirectXMath.h>

void SpriteAnim_Initialize();
void SpriteAnim_Finalize();

void SpriteAnim_Update(double elapsed_time);
void SpriteAnim_Draw(int playid,float dx,float dy,float dw,float dh);

void BillboardAnim_Draw(int playid, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot = { 0.0f,0.0f });

int SpriteAnim_RegisterPattern(
	int texid ,// 使用するテクスチャのID（画像の管理番号）
	int pattern_max, // アニメーションの最大コマ数（全部で何枚使うか）
	int h_pattern_max,// 横に並んでいるコマ数（列数）horizonのh
	double m_seconds_per_pattern,// 1枚の画像（コマ）を表示する時間（秒）
	const DirectX::XMUINT2&pattern_size,// 1コマのサイズ（幅×高さ）
	const DirectX::XMUINT2& start_position, // スプライトシートの開始位置（左上の座標）
	bool is_looped = true); // アニメーションをループするかどうか

int SpriteAnim_CreatePlayer(int anim_pattern_id);
bool SpriteAnim_IsStopped(int index);
void SpriteAnim_DestroyPlayer(int index);


#endif//SPRITE_ANIM_H
