/*==============================================================================

　　　音を付ける[audio.h]
														 Author : Youhei Sato
														 Date   : 2025/07/09
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef AUDIO_H
#define AUDIO_H



void InitAudio();
void UninitAudio();


int LoadAudio(const char* FileName);
void UnloadAudio(int Index);
void PlayAudio(int Index, bool Loop = false);

#endif//AUDIO_H
