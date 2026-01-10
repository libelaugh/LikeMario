/*==============================================================================

   キーボード入力の記録[key_logger.h]
														 Author : Youhei Sato
														 Date   : 2025/06/27
--------------------------------------------------------------------------------

==============================================================================*/

/*キーボードを１回一瞬押すとき一瞬でも３fps進んでしまうから、
				キートリガーやキーフレームを作る、１fpsにする*/
#include "key_logger.h"
#include"keyboard.h"


static Keyboard_State g_PrevState{};
static Keyboard_State g_TriggerState{};
static Keyboard_State g_ReleaseState{};




void KeyLogger_Initialize()
{
	Keyboard_Initialize();
}

//1番大事ゲームプログラマー必須
void KeyLogger_Update()
{
	const Keyboard_State* pState = Keyboard_GetState();
	//unsigned charのポインタ型LPBYTE 1バイトずつ情報を取得できる
	LPBYTE pn = (LPBYTE)pState;//now
	LPBYTE pp = (LPBYTE)&g_PrevState;//preview
	//0 0 ->0
	//0 1 ->1
	//1 0 ->0
	//1 1 ->0
	LPBYTE pt = (LPBYTE)&g_TriggerState;//押した瞬間だけ取得　1fps前false押してない次fpsの今押したときtrue
	//0 0 ->0
	//0 1 ->0
	//1 0 ->1
	//1 1 ->0
	LPBYTE pr = (LPBYTE)&g_ReleaseState;

	//丸暗記するぐらい重要
	for (int i = 0;i < sizeof(Keyboard_State);i++) {
		pt[i] = (pp[i] ^ pn[i]) & pn[i];//0 1->1のときだけ１
		pr[i] = (pp[i] ^ pn[i]) & pp[i];//1 0->1のときだけ１
	}

	g_PrevState = *pState;
}

//IsKeyDownと同じ長押しver
bool KeyLogger_IsPressed(Keyboard_Keys key)
{
	return Keyboard_IsKeyDown(key);
}

bool KeyLogger_IsTrigger(Keyboard_Keys key)
{
	return Keyboard_IsKeyDown(key,&g_TriggerState);
}

bool KeyLogger_IsRelease(Keyboard_Keys key)
{
	return Keyboard_IsKeyDown(key,&g_ReleaseState);
}
