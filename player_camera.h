/*==============================================================================

	ÉvÉåÉCÉÑÅ[êßå‰[player_camera.h]
														 Author : Tanaka Kouki
														 Date   : 2025/10/31
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef PLAYER_CAMERA_H
#define PLAYER_CAMERA_H

#include<DirectXMath.h>

void PlayerCamera_Initialize();
void PlayerCamera_Finalize();
void PlayerCamera_Update(float elapsedTime);

const DirectX::XMFLOAT3& PlayerCamera_GetPosition();
const DirectX::XMFLOAT3& PlayerCamera_GetFront();

const DirectX::XMFLOAT4X4& PlayerCamera_GetViewMatrix();
const DirectX::XMFLOAT4X4& PlayerCamera_GetPerspectiveMatrix();

#endif//PLAYER_CAMERA_H