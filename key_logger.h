/*==============================================================================

   キーボード入力の記録[key_logger.h]
														 Author : Youhei Sato
														 Date   : 2025/06/27
--------------------------------------------------------------------------------

==============================================================================*/

/*キーボードを１回一瞬押すとき一瞬でも３fps進んでしまうから、
				キートリガーやキーフレームを作る、１fpsにする*/
#ifndef KEY_LOGGER_H
#define KEY_LOGGER_H

#include"keyboard.h"

void KeyLogger_Initialize();

void KeyLogger_Update();

bool KeyLogger_IsPressed(Keyboard_Keys key);//IsKeyDownと同じ長押しver
bool KeyLogger_IsTrigger(Keyboard_Keys key);
bool KeyLogger_IsRelease(Keyboard_Keys key);//任天堂入れてたから一応入れた


#endif//KEY_LOGGER_H
