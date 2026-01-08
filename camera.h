/*==============================================================================

　　  カメラ制御[camera.h]
														 Author : Kouki Tanaka
														 Date   : 2025/09/11
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef CAMERA__H
#define CAMERA__H

#include<DirectXMath.h>

void Camera_Initialize();
void Camera_Initialize(const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& front, const DirectX::XMFLOAT3& up, const DirectX::XMFLOAT3&right);
void Camera_Finalize();
void Camera_Update(float elapsedTime);

const DirectX::XMFLOAT4X4& Camera_GetMatrix();
const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix();//画面クリックでオブジェクト選択できるように
const DirectX::XMFLOAT3& Camera_GetPosition();
const DirectX::XMFLOAT3& Camera_GetFront();

void Camera_SetMatrix(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection);


// 毎フレーム呼ぶ
void Camera_UpdateInput();                 // ここで Alt/マウス入力に応じて s_yaw/pitch/pivot/distance を更新
void Camera_UpdateMatrices(float aspect);  // ここで View/Proj を計算してシェーダへ渡す
void Camera_UpdateKeys(float dt);//キー版
//マウスホイール対応したい場合はこれをWndProcから呼ぶ
void Camera_OnMouseWheel(short wheelDelta);

void Camera_DebugDraw();


#endif//CAMERA_H
