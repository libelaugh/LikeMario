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
#include "mouse.h"
#include"debug_text.h"
#include"player.h"
#include"shader_field.h"
#include"shader_billboard.h"
#include <windows.h>
#include<sstream>

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

void PlayerCamera_Initialize()
{

}

void PlayerCamera_Finalize()
{

}

void PlayerCamera_Update(float elapsedTime)
{
    if (KeyLogger_IsTrigger(KK_L)) {
        g_enableKeyCamera = !g_enableKeyCamera;
        if (g_enableKeyCamera) {
            g_cameraUp = { 0.0f, 1.0f, 0.0f };
            g_cameraRight = { 1.0f, 0.0f, 0.0f };
        }
    }

    XMVECTOR position{};
    XMVECTOR front{};
    XMVECTOR up{};
    XMVECTOR right{};

    if (g_enableKeyCamera) {
        position = XMLoadFloat3(&g_cameraPos);
        front = XMLoadFloat3(&g_cameraFront);
        up = XMLoadFloat3(&g_cameraUp);
        right = XMLoadFloat3(&g_cameraRight);
    }
    else {
        position = XMLoadFloat3(&Player_GetPosition());
        XMVECTOR offset = { 0.0f, 0.5f, -3.5f };
        position += offset;
        XMFLOAT3 dir = { 0.0f, 0.0f, 1.0f };
        front = XMLoadFloat3(&dir);
        up = XMLoadFloat3(&g_cameraUp);
        right = XMLoadFloat3(&g_cameraRight);
    }

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
    }
    XMVECTOR target = position + front;
    // position += {0.0f, 10.0f, -7.0f};//カメラ位置

    XMStoreFloat3(&g_cameraPos, position);
    XMStoreFloat3(&g_cameraFront, front);
    XMStoreFloat3(&g_cameraUp, up);
    XMStoreFloat3(&g_cameraRight, right);

    //ビュー座標変換行列の作成　
    // LH = LeftHand  //引数：カメラ位置、注視点、カメラの上ベクトル（回転対応のため）
    // View matrix
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
