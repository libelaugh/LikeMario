/*==============================================================================

　　  ゲームパッド入出力[gamepad.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/04
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef GAMEPAD_H
#define GAMEPAD_H

struct GamepadState
{
    bool connected = false;

    // left stick (-1..1)
    float lx = 0.0f;
    float ly = 0.0f;

    // buttons
    bool a = false;
    bool b = false;
    bool x = false;
    bool y = false;

    bool start = false;
    bool back = false;

    bool lb = false;
    bool rb = false;

    // triggers (0..1)
    float lt = 0.0f;
    float rt = 0.0f;
};

// Call once per frame.
void Gamepad_Update();

// idx: 0..3. Returns false if not connected.
bool Gamepad_GetState(int idx, GamepadState* out);

#endif//GAMEPAD_H