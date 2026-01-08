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
static XMFLOAT4X4 g_CameraMatrix{};
static XMFLOAT4X4 g_CameraPerspectiveMatrix{};

void PlayerCamera_Initialize()
{

}

void PlayerCamera_Finalize()
{

}

void PlayerCamera_Update(float elapsedTime)
{
    //XMVECTOR position = XMLoadFloat3(&Player_GetPosition()) - XMLoadFloat3(&Player_GetFront()) * 22.0f;
    XMVECTOR position = XMLoadFloat3(&Player_GetPosition());
    position *= {1.0f, 0.0f, 1.0f};

    XMVECTOR target = position;

    position += {0.0f, 3.0f, -5.0f};//カメラ位置

    XMVECTOR front = XMVector3Normalize(target - position);
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
}


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
