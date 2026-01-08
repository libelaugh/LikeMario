/*==============================================================================

Å@Å@  ImguiÇÃUIçÏÇË[editor_ui.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/04
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef EDITOR_UI_H
#define EDITOR_UI_H

#include <string>

void PlayerUI_Draw();
bool DragFloat3Rebuild(const char* label, float* v, float speed = 0.1f);
void EditorUI_Draw(double);

void BuildStage01Cpp(std::string& out);


#endif//EDITOR_UI_H