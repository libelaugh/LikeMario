/*==============================================================================

   スプライトアニメーション描画[sprite_anim.cpp]
														 Author : Youhei Sato
														 Date   : 2025/06/17
--------------------------------------------------------------------------------

==============================================================================*/
#include "sprite_anim.h"
#include"sprite.h"
#include"texture.h"
#include"billboard.h"
#include<DirectXMath.h>
using namespace DirectX;

struct AnimPatternData
{
	int m_TextureId = -1;//テクスチャID
	int m_PatternMax = 0;//アニメーションのパターン数
	int m_HPatternMax = 0;//横のパターン最大数(horizon)
	XMUINT2 m_StartPosition{ 0,0 };//アニメーションのスタート座標
	XMUINT2 m_PatternSize = { 0,0 };//1パターンの幅,高さ
	//int m_PatternHeight = 0;//1パターンの高さ
	double m_seconds_per_pattern = 0.1;
	bool m_IsLooped = true;//ループするか
};

struct AnimPlayData
{
	int m_PatternId = -1;//アニメーションパターンID
    int m_PatternNum = 0;//現在再生中のパターン番号
    double m_AccumulatedTime = 0.0f;//累積番号
	bool m_IsStopped = false;
};

static constexpr int ANIM_PATTERN_MAX = 128;
static AnimPatternData g_AnimPattern[ANIM_PATTERN_MAX];
static constexpr int ANIM_PLAY_MAX = 256;
static AnimPlayData g_AnimPlay[ANIM_PLAY_MAX];


void SpriteAnim_Initialize()
{
	//アニメーションパターン管理情報を初期化（全て利用していない)状況にする
	for (AnimPatternData& data : g_AnimPattern) {
		data.m_TextureId = -1;//-1ならテクスチャ使ってない
	}

	for (AnimPlayData& data : g_AnimPlay) {
		data.m_PatternId = -1;
		data.m_IsStopped = false;
	}

	/*g_AnimPattern[0].m_TextureId = Texture_Load(L"kokosozai.png");
	g_AnimPattern[0].m_PatternMax=8;
	g_AnimPattern[0].m_PatternSize = { 32,32 };
	g_AnimPattern[0].m_StartPosition = { 0,96 };
	g_AnimPlay[0].m_PatternId = 0;

	g_AnimPattern[1].m_TextureId = Texture_Load(L"kokosozai.png");
	g_AnimPattern[1].m_PatternMax = 13;
	g_AnimPattern[1].m_PatternSize = { 32, 32 };
	g_AnimPattern[1].m_StartPosition = { 0, 32 };
	g_AnimPlay[1].m_PatternId = 1;

	g_AnimPattern[2].m_TextureId = Texture_Load(L"kokosozai.png");
	g_AnimPattern[2].m_PatternMax = 4;
	g_AnimPattern[2].m_PatternSize = { 32, 32 };
	g_AnimPattern[2].m_StartPosition = { 32 * 2, 32 * 5 };
	g_AnimPattern[2].m_IsLooped = false;
	g_AnimPlay[2].m_PatternId = 2;
	*/

	/*g_AnimPlay[0].m_PatternId = 0;
	g_AnimPlay[1].m_PatternId = 1;
	g_AnimPlay[2].m_PatternId = 2;*/

}

void SpriteAnim_Finalize()
{
}

void SpriteAnim_Update(double elapsed_time)
{
	for (int i = 0; i < ANIM_PLAY_MAX; i++) {

		if (g_AnimPlay[i].m_PatternId < 0)continue;

        AnimPatternData* pAnimPatternData = &g_AnimPattern[g_AnimPlay[i].m_PatternId];

		if (g_AnimPlay[i].m_AccumulatedTime >= pAnimPatternData->m_seconds_per_pattern) {
			g_AnimPlay[i].m_PatternNum++;

			

			if (g_AnimPlay[i].m_PatternNum >= pAnimPatternData->m_PatternMax) {
				if (pAnimPatternData->m_IsLooped) {
					g_AnimPlay[i].m_PatternNum = 0;
				}
				else {
					g_AnimPlay[i].m_PatternNum = pAnimPatternData->m_PatternMax - 1;
				}
			}

			g_AnimPlay[i].m_AccumulatedTime -= pAnimPatternData->m_seconds_per_pattern;
		}

		g_AnimPlay[i].m_AccumulatedTime += elapsed_time;
	}

}


//アニメ再生中の現在のコマを、スプライトとして指定位置に描画する関数です
/*【関数の目的】
SpriteAnim_Draw(int playid, float dx, float dy, float dw, float dh)
指定された再生プレイヤー（playid）に対応するアニメーションの**現在のコマ（パターン）を、
dx, dy の位置に、サイズ dw, dh で画面に表示する関数です。

指定されたアニメ再生データから「今のコマの位置」を計算して、スプライトシートからそのコマを切り出して、
指定位置に描画している。*/
void SpriteAnim_Draw(int playid, float dx, float dy, float dw, float dh)
{
	/*アニメパターンIDの取得
	playid で指定されたアニメ再生スロットから、どのアニメパターンを再生中かを取り出します。
    m_PatternId は、登録されたアニメーションの管理番号です。*/
	int anim_pattern_id = g_AnimPlay[playid].m_PatternId;
	/*アニメパターンデータのポインタを取得
	g_AnimPattern 配列から、そのパターンIDに対応するアニメ情報を取得します。*/
	AnimPatternData* pAnimPatternData = &g_AnimPattern[anim_pattern_id];

	/* 描画処理
	m_TextureId：アニメに使われているテクスチャIDを使って、スプライトを描画。
dx, dy：描画位置（画面上のX・Y座標）。
dw, dh：描画するサイズ（幅・高さ）。*/
	Sprite_Draw04(pAnimPatternData->m_TextureId,
		dx, dy, dw, dh,
		/*現在のコマの位置（切り出し位置）を計算
アニメ画像はスプライトシート（1枚に複数コマが並んだ画像）。
m_PatternNum：現在のコマ番号（0から増えてループ）。
% m_HPatternMax：横方向に何コマ並んでいるかで割った余り ⇒ 今の横位置
m_PatternSize.x：1コマの横サイズ
m_StartPosition.x：スプライトシートの開始位置（X座標）
つまりこれは、**「今表示すべきコマがスプライトシートのどこにあるか」**を計算してる。*/
		pAnimPatternData->m_StartPosition.x
		+ pAnimPatternData->m_PatternSize.x
		*(g_AnimPlay[playid].m_PatternNum % pAnimPatternData->m_HPatternMax),
		/*縦方向のコマ位置も同様に計算
		縦方向は%を/にしないとバグ起きるかもらしい
		g_AnimPlay[playid].m_PatternNumのみだとm_PatternNumは0～９を繰り返してる?*/
		pAnimPatternData->m_StartPosition.y
		+ pAnimPatternData->m_PatternSize.y
		* (g_AnimPlay[playid].m_PatternNum / pAnimPatternData->m_HPatternMax),
		/*コマのサイズを指定
		切り出すコマの幅と高さ（1コマ分のサイズ）を指定*/
		pAnimPatternData->m_PatternSize.x,
		pAnimPatternData->m_PatternSize.y
	);
}

void BillboardAnim_Draw(int playid, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot)
{
	int anim_pattern_id = g_AnimPlay[playid].m_PatternId;
	AnimPatternData* pAnimPatternData = &g_AnimPattern[anim_pattern_id];

	Billboard_Draw(pAnimPatternData->m_TextureId,
		position, scale,
		{
			pAnimPatternData->m_StartPosition.x
			+ pAnimPatternData->m_PatternSize.x
			* (g_AnimPlay[playid].m_PatternNum % pAnimPatternData->m_HPatternMax),

			pAnimPatternData->m_StartPosition.y
			+ pAnimPatternData->m_PatternSize.y
			* (g_AnimPlay[playid].m_PatternNum / pAnimPatternData->m_HPatternMax),

			pAnimPatternData->m_PatternSize.x,
			pAnimPatternData->m_PatternSize.y,
		},
		{ 1.0f,1.0f,1.0f,1.0f },
		pivot
	);
}

/*1つのアニメーションパターン（スプライトの分割情報）を登録して、管理番号を返す関数*/
/*関数の目的
スプライトシート（キャラ画像がたくさん並んでいる1枚絵）から、
1つのアニメーションパターンを登録して、再生できるようにする関数です。
*/
//引数の意味は宣言に解説あり
int SpriteAnim_RegisterPattern(int texid, int pattern_max,int h_pattern_max, double m_seconds_per_pattern,
	const DirectX::XMUINT2& pattern_size, const DirectX::XMUINT2& start_position, bool is_looped)
{
	// g_AnimPattern という配列（アニメーションパターンを管理するリスト）を、先頭から順番に探します。
	for (int i = 0;i < ANIM_PATTERN_MAX;i++) {

		/*m_TextureId が -1 でない場所は すでに使われている のでスキップ。
　まだ使われていない（空き）スロットを探しています。*/
		if (g_AnimPattern[i].m_TextureId >= 0)continue;

		/*空いてる場所を見つけたら、そこに設定をしていきます。
		以下のように「どの画像」「何コマ」「どのサイズ・位置・速度・ループか」を保存*/
		g_AnimPattern[i].m_TextureId = texid;
		g_AnimPattern[i].m_PatternMax = pattern_max;
		g_AnimPattern[i].m_HPatternMax = h_pattern_max;
		g_AnimPattern[i].m_seconds_per_pattern = m_seconds_per_pattern;
		g_AnimPattern[i].m_PatternSize = pattern_size;
		g_AnimPattern[i].m_StartPosition = start_position;
		g_AnimPattern[i].m_IsLooped = is_looped;

		/*登録したアニメーションの ID（インデックス番号）を返す。
　これが後でアニメ再生に使われる「管理番号」です。*/
		return i;
	}
	/*空きがなかった場合。登録に失敗したことを示します
	*/
	return -1;
}




/*未使用のスロットを探して、複数のアニメを同時に再生できるようにする
アニメーションを再生するための「再生プレイヤー」を1つ確保し、そのIDを返す関数です。
（つまり、「このアニメを再生したい！」と頼んで、空いてる再生枠を用意してもらう関数)*/
/*再生プレイヤー（SpriteAnim_CreatePlayer が作るもの）について
CPU側のメモリ上にある「アニメーションの再生状態を管理する構造体（データ）」**です。
「今どのコマを表示しているか」「再生時間はどれくらい経ったか」などの情報を持ちます。
いわば「アニメの再生機械のインスタンス」です。

アニメの状態管理（再生時間、現在コマなど）はCPU側が持つ情報を元に、
GPUに「どのテクスチャのどの部分を描くか」を指示します。*/
//アニメ再生用の空きを探して確保する関数
int SpriteAnim_CreatePlayer(int anim_pattern_id)
{
	/* g_AnimPlay という配列を先頭から順番に見て、使っていない（空いている）再生スロットを探す
	*/
	for (int i = 0;i < ANIM_PLAY_MAX;i++) {
		/*m_PatternId が 0以上 のときは すでに使用中 なのでスキップ
　（未使用のときは -1 にしてある前提）*/
		if (g_AnimPlay[i].m_PatternId >= 0)continue;

		g_AnimPlay[i].m_PatternId = anim_pattern_id;//このスロットで どのアニメパターンを再生するかを設定
		/*→ 再生の初期状態を設定：
AccumulatedTime：経過時間 → 0にして最初から再生
PatternNum：今どのコマか（何番目の画像か） → 0にして最初のフレームから*/
		g_AnimPlay[i].m_AccumulatedTime = 0.0;//0フレーム目から再生
		g_AnimPlay[i].m_PatternNum = 0;//時間をリセット
		g_AnimPlay[i].m_IsStopped = false;

		/*確保した再生スロットのID（管理番号）を返す
→ これを使って、後で SpriteDraw のようにアニメを再生・更新・停止などに使える*/
		return i;//管理用IDを返す（呼び出し元が使う）
	}
	return -1;
}

bool SpriteAnim_IsStopped(int index)
{
	return g_AnimPlay[index].m_IsStopped;
}

void SpriteAnim_DestroyPlayer(int index)
{
	g_AnimPlay[index].m_PatternId = -1;
}

