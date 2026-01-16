/*==============================================================================

　　  カメラ制御[camera.cpp]
														 Author : Kouki Tanaka
														 Date   : 2025/09/11
--------------------------------------------------------------------------------

==============================================================================*/

#include "camera.h"
#include"direct3d.h"
#include"key_logger.h"
#include "mouse.h"
#include"debug_text.h"
#include "shader_billboard.h"
#include "billboard.h"
#include <windows.h>
#include<sstream>

using namespace DirectX;

//カメラをクラス化すると複数設置できる

//キー入力カメラ移動用
static XMFLOAT3 g_cameraPos{0.0f,0.0f,-0.5f};
static XMFLOAT3 g_cameraFront{ 0.0f,0.0f,1.0f };//前ベクトル
static XMFLOAT3 g_cameraUp{ 0.0f,1.0f,0.0f };//上ベクトル
static XMFLOAT3 g_cameraRight{ 1.0f,0.0f,-0.5f };//右ベクトル
static float CAMERA_MOVE_SPEED = 4.0f;
static float CAMERA_ROTATION_SPEED = XMConvertToRadians(30);
static XMFLOAT4X4 g_cameraMatrix;
static XMFLOAT4X4 g_perspectiveMatrix;

static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; // 定数バッファb1: view
static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr; // 定数バッファb2: proj


//マウス対応カメラ
// ====== 操作感パラメータ ======
static float s_orbitSense = 0.005f;   // Alt+LMB: 回転
static float s_panSpeed = 0.0025f;  // MMB: パン
static float s_dollySpeed = 0.01f;    // Alt+RMB: ドリー
static float s_wheelZoom = 0.0015f;  // ホイール

// ====== カメラ状態 ======
static XMFLOAT3 s_pivot = { 0.0f, 0.0f, 0.0f };//注視点
static float    s_yaw = 0.0f;//ヨー角、水平方向の回転角度
static float    s_pitch = 0.2f;//ピッチ角、水平方向の回転角度
static float    s_distance = 6.0f;//注視点からの距離

static DirectX::XMMATRIX s_view;
static DirectX::XMMATRIX s_proj;

// マウス状態
static POINT s_prevCursor = { 0, 0 };
static bool  s_initedCursor = false;
static float s_wheelAccum = 0.0f; // WndProcから加算

// ---- helpers ----
static float clampf(float v, float lo, float hi) {
	return (v < lo) ? lo : (v > hi) ? hi : v;
}
static POINT GetClientCursorPos() {
	POINT p;
	GetCursorPos(&p);
	HWND hwnd = GetActiveWindow();
	if (hwnd) ScreenToClient(hwnd, &p);
	return p;
}



static hal::DebugText* g_pDT = nullptr;

void Camera_Initialize()
{

    //キー入力カメラ移動
    g_cameraPos = { 0.0f,0.0f,0.0f };
    g_cameraFront = { 0.0f,0.0f,1.0f };
    g_cameraUp = { 0.0f,1.0f,0.0f };
    g_cameraRight = { 1.0f,0.0f,0.5f };

    XMStoreFloat4x4(&g_cameraMatrix, XMMatrixIdentity());


    //マウス入力カメラ移動
    //カメラの初期化
    s_pivot = { 0.0f, 0.0f, 0.0f };
    s_yaw = XMConvertToRadians(45.0f);
    s_pitch = -0.35f;   // 見下ろすならマイナス
    s_distance = 8.0f;

	s_initedCursor = false;
	s_wheelAccum = 0.0f;

    // 頂点シェーダー用定数バッファの作成
    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ
    Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer2); // proj
    Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1); // view
	
#if defined(DEBUG)||defined(_DEBUG)//リリースモードのときデバック用collisionを商品化で見えないようにする
    g_pDT = new hal::DebugText(Direct3D_GetDevice(), Direct3D_GetContext(), L"consolab_ascii_512.png",
        Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight(),
        0.0f, 28.0f,//ここからデバッグテキストの左上の位置を決める
        0, 0,
        0.0f, 16.0f);
#endif
}

void Camera_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front, const DirectX::XMFLOAT3& up, const DirectX::XMFLOAT3& right)
{
    Camera_Initialize();

    g_cameraPos = position;
    g_cameraFront = front;
    g_cameraUp = up;
    g_cameraRight = right;
}

void Camera_Finalize()
{
    SAFE_RELEASE(g_pVSConstantBuffer1);
    SAFE_RELEASE(g_pVSConstantBuffer2);
    delete g_pDT;
}

void Camera_Update(float elapsedTime)
{
    XMVECTOR position = XMLoadFloat3(&g_cameraPos);
    XMVECTOR front = XMLoadFloat3(&g_cameraFront);
    XMVECTOR up = XMLoadFloat3(&g_cameraUp);
    XMVECTOR right = XMLoadFloat3(&g_cameraRight);

    /*同時押しするとエラーでた
    if (KeyLogger_IsPressed(KK_SPACE)) {
        CAMERA_MOVE_SPEED *= 2;
        CAMERA_ROTATION_SPEED *= 2;
    }*/

    /*=====キー入力カメラ回転===========*/
    // 下向く
    if (KeyLogger_IsPressed(KK_DOWN)) {     //回転軸、回転スピード
        XMMATRIX rot = XMMatrixRotationAxis(right, CAMERA_ROTATION_SPEED * elapsedTime);
        //TransformNormalで回転行列掛ける、Normalizeでfloatの誤差無くす
        front = XMVector3Normalize(XMVector3TransformNormal(front, rot));
        up = XMVector3Normalize(XMVector3TransformNormal(up, rot));
        // 直交基底を再構成（左手系：right = up × front）
        right = XMVector3Normalize(XMVector3Cross(up, front));//「外積」で下向ける
    }
    // 上向く
    if (KeyLogger_IsPressed(KK_UP)) {
        XMMATRIX rot = XMMatrixRotationAxis(right, -CAMERA_ROTATION_SPEED * elapsedTime);
        front = XMVector3Normalize(XMVector3TransformNormal(front, rot));
        up = XMVector3Normalize(XMVector3TransformNormal(up, rot));
        right = XMVector3Normalize(XMVector3Cross(up, front));//「外積」で上向ける
    }
    // 左向く
    if (KeyLogger_IsPressed(KK_LEFT)) {     //回転軸、回転スピード
        XMMATRIX rot = XMMatrixRotationAxis(up, -CAMERA_ROTATION_SPEED * elapsedTime);//カメラupベクトル垂直回転
        //XMMATRIX rot = XMMatrixRotationY(-CAMERA_ROTATION_SPEED * elapsedTime);//Y軸垂直回転
        front = XMVector3Normalize(XMVector3TransformNormal(front, rot));
        right = XMVector3Normalize(XMVector3TransformNormal(right, rot));
        up = XMVector3Normalize(XMVector3Cross(front, right));//「外積」で左向ける
    }
    // 右向く
    if (KeyLogger_IsPressed(KK_RIGHT)) {     //回転軸、回転スピード
        XMMATRIX rot = XMMatrixRotationAxis(up, CAMERA_ROTATION_SPEED * elapsedTime);
        //XMMATRIX rot = XMMatrixRotationY(CAMERA_ROTATION_SPEED * elapsedTime);
        front = XMVector3Normalize(XMVector3TransformNormal(front, rot));
        right = XMVector3Normalize(XMVector3TransformNormal(right, rot));
        up = XMVector3Normalize(XMVector3Cross(front, right));//「外積」で右向ける
    }

    /*========キー入力カメラ移動=========*/
    if (KeyLogger_IsPressed(KK_W)) {//前
        //position += front * CAMERA_MOVE_SPEED * elapsedTime;
        //XZ平面と平行Ver
        position += XMVector3Normalize(front * XMVECTOR{ 1.0f,0.0f,1.0f }) * CAMERA_MOVE_SPEED * elapsedTime;

    }
    if (KeyLogger_IsPressed(KK_S)) {//後ろ
        //position += -front * CAMERA_MOVE_SPEED * elapsedTime;
        position += XMVector3Normalize(-front * XMVECTOR{ 1.0f,0.0f,1.0f }) * CAMERA_MOVE_SPEED * elapsedTime;
    }
    if (KeyLogger_IsPressed(KK_A)) {//左
        position += -right * CAMERA_MOVE_SPEED * elapsedTime;
    }
    if (KeyLogger_IsPressed(KK_D)) {//右
        position += right * CAMERA_MOVE_SPEED * elapsedTime;
    }
    if (KeyLogger_IsPressed(KK_Q)) {//上
        //position += up * CAMERA_MOVE_SPEED * elapsedTime;//回転してると斜め上なる
        position += XMVECTOR{ 0.0f,1.0f,0.0f }*CAMERA_MOVE_SPEED * elapsedTime;//どこでもY軸真上
    }
    if (KeyLogger_IsPressed(KK_E)) {//下
        //position += -up * CAMERA_MOVE_SPEED * elapsedTime;
        position += XMVECTOR{ 0.0f,-1.0f,0.0f }*CAMERA_MOVE_SPEED * elapsedTime;
    }
    //各所更新結果を保存 XMStoreFloat3
    XMStoreFloat3(&g_cameraPos, position);
    XMStoreFloat3(&g_cameraFront, front);
    XMStoreFloat3(&g_cameraUp, up);
    XMStoreFloat3(&g_cameraRight, right);

    //ビュー座標変換行列の作成　
    // LH = LeftHand  //引数：カメラ位置、注視点、カメラの上ベクトル（回転対応のため）
    XMMATRIX mtrView = XMMatrixLookAtLH(
        position,//カメラ位置
        position + front, //注視点
        up);//カメラの上の部分を定める
    //ビュー変換行列を保存
    XMStoreFloat4x4(&g_cameraMatrix, mtrView);

    // 頂点シェーダーに変換行列を設定
//パースペクティブ行列の作成
//引数：画角度の半分rad、(float)アスペクト比=幅÷高さ,視錐台近平面、,視錐台遠平面
    constexpr float fovAngleY = XMConvertToRadians(60.0f);
    float aspective = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
    float nearz = 0.1f;
    float farz = 100.0f;
    XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(fovAngleY, aspective, nearz, farz);
    //パースペクティブ行列を保存
    XMStoreFloat4x4(&g_perspectiveMatrix, mtxPerspective);
}

const DirectX::XMFLOAT4X4& Camera_GetMatrix()
{
    return g_cameraMatrix;
}

const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix()
{
    return g_perspectiveMatrix;
}

const DirectX::XMFLOAT3& Camera_GetPosition()
{
    return g_cameraPos;
}

const DirectX::XMFLOAT3& Camera_GetFront()
{
    return g_cameraFront;
}

void Camera_SetMatrix(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection)
{
    //定数バッファへビュー変換行列とプロジェクション変換行列を設定する
    XMFLOAT4X4 v, p;
    XMStoreFloat4x4(&v, XMMatrixTranspose(view));
    XMStoreFloat4x4(&p, XMMatrixTranspose(projection));

    Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &v, 0, 0);
    Direct3D_GetContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);
    Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &p, 0, 0);
    Direct3D_GetContext()->VSSetConstantBuffers(2, 1, &g_pVSConstantBuffer2);

    // ---- Billboard でも同じ view/proj を使う ----
    ShaderBillboard_SetViewMatrix(view);
    ShaderBillboard_SetProjectionMatrix(projection);

    // ビルボードの「常にカメラ正面」用（平行移動は Billboard_SetViewMatrix 側で消す）
    DirectX::XMFLOAT4X4 viewF{};
    DirectX::XMStoreFloat4x4(&viewF, view);
    Billboard_SetViewMatrix(viewF);
}

void Camera_OnMouseWheel(short wheelDelta) {
    // 通常 120 単位。前進を正に（好みで逆にしてOK）
    s_wheelAccum += (float)wheelDelta * s_wheelZoom;
}

void Camera_UpdateInput() {
    const bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    const bool lDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    const bool rDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    const bool mDown = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

    POINT cur = GetClientCursorPos();
    if (!s_initedCursor) {
        s_prevCursor = cur;
        s_initedCursor = true;
    }
    int dx = cur.x - s_prevCursor.x;
    int dy = cur.y - s_prevCursor.y;
    s_prevCursor = cur;

    // いまの姿勢で基底ベクトル
    float cp = cosf(s_pitch), sp = sinf(s_pitch);
    float cy = cosf(s_yaw), sy = sinf(s_yaw);
    XMVECTOR forward = XMVectorSet(sy * cp, sp, cy * cp, 0.0f);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward));
    XMVECTOR up = XMVector3Normalize(XMVector3Cross(forward, right));

    // --- Wheel Dolly via Mouse module (ポーリング) ---
    {
        Mouse_State ms{};
        Mouse_GetState(&ms);
        if (ms.scrollWheelValue != 0) {
            // Windowsのホイールは通常 120 単位。+が前進（逆が良ければ - に）
            s_wheelAccum += (float)ms.scrollWheelValue * s_wheelZoom;
            Mouse_ResetScrollWheelValue(); // 次フレームに持ち越さない
        }
    }

    // --- Orbit: Alt + LMB ---
    if (altDown && lDown) {
        s_yaw += dx * s_orbitSense;
        s_pitch += dy * s_orbitSense;
        s_pitch = clampf(s_pitch, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);
    }

    // --- Pan: MMB ---
    if (mDown) {
        // 距離に下限を入れて、近距離で遅くなりすぎるのを防ぐ
        const float kPanMinDist = 2.0f; // ←好みで調整（1.0～3.0あたり）
        float dist = (s_distance < kPanMinDist) ? kPanMinDist : s_distance;

        float scale = s_panSpeed * dist;
        XMVECTOR dp = XMVectorAdd(
            XMVectorScale(right, -dx * scale),
            XMVectorScale(up, dy * scale)
        );
        XMVECTOR pv = XMLoadFloat3(&s_pivot);
        pv = XMVectorAdd(pv, dp);
        XMStoreFloat3(&s_pivot, pv);
    }

    // --- Dolly: Alt + RMB ---
    if (altDown && rDown) {
        s_distance *= (1.0f + dy * s_dollySpeed);
    }

    // --- Wheel Dolly (累積) ---
    if (s_wheelAccum != 0.0f) {
        s_distance *= (1.0f - s_wheelAccum); // 小さくなるほど前進
        s_wheelAccum = 0.0f;
    }

    s_distance = clampf(s_distance, 0.05f, 1000.0f);
}

void Camera_UpdateMatrices(float aspect) {
    // 最新 yaw/pitch から forward/right/up 再算出
    float cp = cosf(s_pitch), sp = sinf(s_pitch);
    float cy = cosf(s_yaw), sy = sinf(s_yaw);
    XMVECTOR forward = XMVectorSet(sy * cp, sp, cy * cp, 0.0f);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward));
    XMVECTOR up = XMVector3Normalize(XMVector3Cross(forward, right));

    XMVECTOR pivotV = XMLoadFloat3(&s_pivot);
    XMVECTOR camPos = XMVectorSubtract(pivotV, XMVectorScale(forward, s_distance));

    s_view = XMMatrixLookAtLH(camPos, pivotV, up);
    //Shader3D_SetViewMatrix(s_view);

    constexpr float fovY = XMConvertToRadians(60.0f);
    float nearz = 0.1f, farz = 500.0f;
    s_proj = XMMatrixPerspectiveFovLH(fovY, aspect, nearz, farz);
   // Shader3D_SetProjectionMatrix(s_proj);

    XMStoreFloat3(&g_cameraPos, camPos);
    XMStoreFloat3(&g_cameraFront, forward);
    XMStoreFloat3(&g_cameraUp, up);
    XMStoreFloat3(&g_cameraRight, right);
}

void Camera_UpdateKeys(float dt)
{
    // 現在の yaw/pitch から基底ベクトルを作成
    float cp = cosf(s_pitch), sp = sinf(s_pitch);
    float cy = cosf(s_yaw), sy = sinf(s_yaw);
    DirectX::XMVECTOR forward = DirectX::XMVectorSet(sy * cp, sp, cy * cp, 0.0f);
    DirectX::XMVECTOR right = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(DirectX::XMVectorSet(0, 1, 0, 0), forward));
    DirectX::XMVECTOR up = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(forward, right));

    float move = 4.0f * dt;          // 移動スピード（お好みで）
    float rot = 1.2f * dt;          // 回転スピード（お好みで）
    //if (KeyLogger_IsPressed(KK_SHIFT)) { move *= 3.0f; } // ダッシュ

    // 平行移動（Pivotを動かす）
    DirectX::XMVECTOR pivot = DirectX::XMLoadFloat3(&s_pivot);
    if (KeyLogger_IsPressed(KK_W)) pivot = DirectX::XMVectorAdd(pivot, DirectX::XMVectorScale(forward, move));
    if (KeyLogger_IsPressed(KK_S)) pivot = DirectX::XMVectorSubtract(pivot, DirectX::XMVectorScale(forward, move));
    if (KeyLogger_IsPressed(KK_D)) pivot = DirectX::XMVectorAdd(pivot, DirectX::XMVectorScale(right, move));
    if (KeyLogger_IsPressed(KK_A)) pivot = DirectX::XMVectorSubtract(pivot, DirectX::XMVectorScale(right, move));
    if (KeyLogger_IsPressed(KK_E)) pivot = DirectX::XMVectorAdd(pivot, DirectX::XMVectorScale(up, move));
    if (KeyLogger_IsPressed(KK_Q)) pivot = DirectX::XMVectorSubtract(pivot, DirectX::XMVectorScale(up, move));
    DirectX::XMStoreFloat3(&s_pivot, pivot);

    // 角度（矢印キーなどで向きを回す）
    if (KeyLogger_IsPressed(KK_LEFT))  s_yaw -= rot;
    if (KeyLogger_IsPressed(KK_RIGHT)) s_yaw += rot;
    if (KeyLogger_IsPressed(KK_UP))    s_pitch += rot;
    if (KeyLogger_IsPressed(KK_DOWN))  s_pitch -= rot;

    // ピッチ制限（真上真下を避ける）
    const float kPitchMin = -DirectX::XM_PIDIV2 + 0.01f;
    const float kPitchMax = DirectX::XM_PIDIV2 - 0.01f;
    if (s_pitch < kPitchMin) s_pitch = kPitchMin;
    if (s_pitch > kPitchMax) s_pitch = kPitchMax;
}

void Camera_DebugDraw()
{
    /*============この位置見てオブジェクト好きな位置にマウスクリックで配置できる============*/
#if defined(DEBUG)||defined(_DEBUG)
    std::stringstream ss;//coutと同じようにできるようにするやつ
    ss << "Camera Position: x=" << g_cameraPos.x,
                    ss << " y=" << g_cameraPos.y,
                    ss << " z=" << g_cameraPos.z << std::endl;
    ss << "Camera Front: x=" << g_cameraFront.x,
                 ss << " y=" << g_cameraFront.y,
                 ss << " z=" << g_cameraFront.z << std::endl;
    ss << "Camera Up: x=" << g_cameraUp.x,
              ss << " y=" << g_cameraUp.y,
              ss << " z=" << g_cameraUp.z << std::endl;
    ss << "Camera Right: x=" << g_cameraRight.x,
                 ss << " y=" << g_cameraRight.y,
                 ss << " z=" << g_cameraRight.z << std::endl;
    g_pDT->SetText(ss.str().c_str(), { 0.0f,1.0f,0.0f ,1.0f });
    g_pDT->Draw();
    g_pDT->Clear();
#endif
    // 描画後に t0 を明示的に外す（任意）
    //ID3D11ShaderResourceView* nullSRV = nullptr;
    //Direct3D_GetContext()->PSSetShaderResources(0, 1, &nullSRV);
}