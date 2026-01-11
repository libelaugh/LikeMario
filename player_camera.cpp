/*==============================================================================

	プレイヤー制御[player_camera.cpp]
														 Author : Tanaka Kouki
														 Date   : 2025/10/31
--------------------------------------------------------------------------------

==============================================================================*/

#include "player_camera.h"
#include"direct3d.h"
#include"shader3d.h"
#include"key_logger.h"
#include"gamepad.h"
#include "mouse.h"
#include"debug_text.h"
#include"player.h"
#include"shader_field.h"
#include"shader_billboard.h"
#include <windows.h>
#include<sstream>
#include <cmath>

using namespace DirectX;

static XMFLOAT3 g_cameraFront{ 0.0f,0.0f,1.0f };
static XMFLOAT3 g_cameraPos{ 0.0f,0.0f,0.0f };
static XMFLOAT3 g_cameraUp{ 0.0f,1.0f,0.0f };
static XMFLOAT3 g_cameraRight{ 1.0f,0.0f,0.0f };
static XMFLOAT4X4 g_CameraMatrix{};
static XMFLOAT4X4 g_CameraPerspectiveMatrix{};

static float CAMERA_MOVE_SPEED = 4.0f;
static float CAMERA_ROTATION_SPEED = XMConvertToRadians(30.0f);
static bool g_enableKeyCamera = false;
static bool g_prevToggleKey = false;
static float g_normalCameraYaw = 0.0f;

static const float NORMAL_CAMERA_TARGET_OFFSET_Y = 1.25f;
static const float NORMAL_CAMERA_STICK_DEADZONE = 0.2f;
static const float NORMAL_CAMERA_STICK_YAW_SENSITIVITY = XMConvertToRadians(90.0f);

void PlayerCamera_Initialize()
{

}

void PlayerCamera_Finalize()
{

}

void PlayerCamera_Update(float elapsedTime)
{
    const bool toggleKey = KeyLogger_IsPressed(KK_L);
    const bool togglePressed = toggleKey && !g_prevToggleKey;
    bool toggledOn = false;
    bool toggledOff = false;
    if (togglePressed) {
        g_enableKeyCamera = !g_enableKeyCamera;
        toggledOn = g_enableKeyCamera;
        toggledOff = !g_enableKeyCamera;
    }

    g_prevToggleKey = toggleKey;

    if (toggledOn) {
        XMVECTOR front = XMLoadFloat3(&g_cameraFront);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, front));
        up = XMVector3Normalize(XMVector3Cross(front, right));
        XMStoreFloat3(&g_cameraUp, up);
        XMStoreFloat3(&g_cameraRight, right);
    }
    else if (toggledOff) {
        XMVECTOR playerPos = XMLoadFloat3(&Player_GetPosition());
        XMVECTOR cameraPos = XMLoadFloat3(&g_cameraPos);
        XMVECTOR offsetVec = cameraPos - playerPos;
        XMFLOAT3 offset{};
        XMStoreFloat3(&offset, offsetVec);
        g_normalCameraYaw = std::atan2(-offset.x, -offset.z);
    }

    XMVECTOR position{};
    XMVECTOR front{};
    XMVECTOR up{};
    XMVECTOR right{};
    XMVECTOR target{};

    if (g_enableKeyCamera) {
        position = XMLoadFloat3(&g_cameraPos);
        front = XMLoadFloat3(&g_cameraFront);
        up = XMLoadFloat3(&g_cameraUp);
        right = XMLoadFloat3(&g_cameraRight);
    }
    else {
        GamepadState pad{};
        float stickX = 0.0f;
        if (Gamepad_GetState(0, &pad) && pad.connected) {
            stickX = pad.rx;
        }

        if (std::abs(stickX) < NORMAL_CAMERA_STICK_DEADZONE) {
            stickX = 0.0f;
        }

        float yawSpeed = stickX * NORMAL_CAMERA_STICK_YAW_SENSITIVITY;
        g_normalCameraYaw += yawSpeed * elapsedTime;

        XMVECTOR playerPos = XMLoadFloat3(&Player_GetPosition());
        XMVECTOR baseOffset = { 0.0f, 4.0f, -5.0f };//カメラ位置決め
        XMMATRIX yawRot = XMMatrixRotationY(g_normalCameraYaw);
        XMVECTOR rotatedOffset = XMVector3TransformCoord(baseOffset, yawRot);
        position = playerPos + rotatedOffset;

        XMVECTOR lookTarget = playerPos + XMVECTOR{ 0.0f, NORMAL_CAMERA_TARGET_OFFSET_Y, 0.0f };
        front = XMVector3Normalize(lookTarget - position);
        up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        right = XMVector3Normalize(XMVector3Cross(up, front));
        target = lookTarget;
    }

    //g_enableKeyCamera = true;
    if (g_enableKeyCamera) {
        // Rotate
        if (KeyLogger_IsPressed(KK_DOWN)) {
            XMMATRIX rot = XMMatrixRotationAxis(right, CAMERA_ROTATION_SPEED * elapsedTime);
            front = XMVector3Normalize(XMVector3TransformNormal(front, rot));
            up = XMVector3Normalize(XMVector3TransformNormal(up, rot));
            right = XMVector3Normalize(XMVector3Cross(up, front));
        }
        if (KeyLogger_IsPressed(KK_UP)) {
            XMMATRIX rot = XMMatrixRotationAxis(right, -CAMERA_ROTATION_SPEED * elapsedTime);
            front = XMVector3Normalize(XMVector3TransformNormal(front, rot));
            up = XMVector3Normalize(XMVector3TransformNormal(up, rot));
            right = XMVector3Normalize(XMVector3Cross(up, front));
        }
        if (KeyLogger_IsPressed(KK_LEFT)) {
            XMMATRIX rot = XMMatrixRotationAxis(up, -CAMERA_ROTATION_SPEED * elapsedTime);
            front = XMVector3Normalize(XMVector3TransformNormal(front, rot));
            right = XMVector3Normalize(XMVector3TransformNormal(right, rot));
            up = XMVector3Normalize(XMVector3Cross(front, right));
        }
        if (KeyLogger_IsPressed(KK_RIGHT)) {
            XMMATRIX rot = XMMatrixRotationAxis(up, CAMERA_ROTATION_SPEED * elapsedTime);
            front = XMVector3Normalize(XMVector3TransformNormal(front, rot));
            right = XMVector3Normalize(XMVector3TransformNormal(right, rot));
            up = XMVector3Normalize(XMVector3Cross(front, right));
        }

        // Move
        if (KeyLogger_IsPressed(KK_W)) {
            position += XMVector3Normalize(front * XMVECTOR{ 1.0f,0.0f,1.0f }) * CAMERA_MOVE_SPEED * elapsedTime;
        }
        if (KeyLogger_IsPressed(KK_S)) {
            position += XMVector3Normalize(-front * XMVECTOR{ 1.0f,0.0f,1.0f }) * CAMERA_MOVE_SPEED * elapsedTime;
        }
        if (KeyLogger_IsPressed(KK_A)) {
            position += -right * CAMERA_MOVE_SPEED * elapsedTime;
        }
        if (KeyLogger_IsPressed(KK_D)) {
            position += right * CAMERA_MOVE_SPEED * elapsedTime;
        }
        if (KeyLogger_IsPressed(KK_Q)) {
            position += XMVECTOR{ 0.0f,1.0f,0.0f }*CAMERA_MOVE_SPEED * elapsedTime;
        }
        if (KeyLogger_IsPressed(KK_E)) {
            position += XMVECTOR{ 0.0f,-1.0f,0.0f }*CAMERA_MOVE_SPEED * elapsedTime;
        }
        target = position + front;
    }

    XMStoreFloat3(&g_cameraPos, position);
    XMStoreFloat3(&g_cameraFront, front);
    XMStoreFloat3(&g_cameraUp, up);
    XMStoreFloat3(&g_cameraRight, right);


    // View matrix
    // LH = LeftHand  //引数：カメラ位置、注視点、カメラの上ベクトル（回転対応のため）
    XMMATRIX mtrView = XMMatrixLookAtLH(
        position,
        target,
        up);

    //パースペクティブ行列の作成
    //引数：画角度の半分rad、(float)アスペクト比=幅÷高さ,視錐台近平面、,視錐台遠平面
    constexpr float fovAngleY = XMConvertToRadians(60.0f);
    float aspectiveRatio = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
    float nearz = 0.1f;
    float farz = 1000.0f;
    XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(1.0f, aspectiveRatio, nearz, farz);

    //パースペクティブ行列を保存//カメラ行列を保存
    XMStoreFloat4x4(&g_CameraMatrix, mtrView);
    XMStoreFloat4x4(&g_CameraPerspectiveMatrix, mtxPerspective);
}

/*void PlayerCamera_Update(float elapsedTime)
{
    XMVECTOR position = XMLoadFloat3(&Player_GetPosition());
    XMVECTOR offset = { 0.0f, 0.5f, -3.5f };
    position += offset;
    XMFLOAT3 dir = { 0.0f, 0.0f, 1.0f };
    XMVECTOR front = XMLoadFloat3(&dir);
    XMVECTOR target = position + front;
   // position += {0.0f, 10.0f, -7.0f};//カメラ位置

    XMStoreFloat3(&g_cameraPos, position);
    XMStoreFloat3(&g_cameraFront, front);

    //ビュー座標変換行列の作成　
    // LH = LeftHand  //引数：カメラ位置、注視点、カメラの上ベクトル（回転対応のため）
    XMMATRIX mtrView = XMMatrixLookAtLH(
        position,
        target,
        { 0.0f,1.0f,0.0f });


//パースペクティブ行列の作成
//引数：画角度の半分rad、(float)アスペクト比=幅÷高さ,視錐台近平面、,視錐台遠平面
    constexpr float fovAngleY = XMConvertToRadians(60.0f);
    float aspectiveRatio = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
    float nearz = 0.1f;
    float farz = 1000.0f;
    XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(1.0f, aspectiveRatio, nearz, farz);

    //パースペクティブ行列を保存//カメラ行列を保存
    XMStoreFloat4x4(&g_CameraMatrix, mtrView);
    XMStoreFloat4x4(&g_CameraPerspectiveMatrix, mtxPerspective);
}*/



const DirectX::XMFLOAT3& PlayerCamera_GetPosition()
{
    return g_cameraPos;
}

const DirectX::XMFLOAT3& PlayerCamera_GetFront()
{
    return g_cameraFront;
}

const DirectX::XMFLOAT4X4& PlayerCamera_GetViewMatrix()
{
    return g_CameraMatrix;
}

const DirectX::XMFLOAT4X4& PlayerCamera_GetPerspectiveMatrix()
{
    return g_CameraPerspectiveMatrix;
}