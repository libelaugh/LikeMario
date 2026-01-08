/*==============================================================================

   Direct3Dの初期化関連 [direct3d.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/12
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef DIRECT3D_H
#define DIRECT3D_H


#include <Windows.h>
#include<d3d11.h>
#include<DirectXMath.h>



// セーフリリースマクロ
/*リソースは使ったらメモリ開けるためにリリースする。リソースを安全にリリースするためのショートカット。
リソースとはGPUが絵を描くために必要な、データのまとまり（かたまり）です。
バッファ・テクスチャ画像・3Dモデル画像も、座標も、色も、全部 GPU に渡して使う「材料・道具」です。
メモリやGPUのリソースをちゃんと解放して、メモリリーク（使い終わったメモリを放置するバグ）を防げる
NULL にするのはポインタが使い終わった道具を指すのをやめさせて、
間違ってまた使おうとして起こるバグ（変な動き）を防ぐため。*/
#define SAFE_RELEASE(o) if (o) { (o)->Release(); o = NULL; }

/*hWnd は「どのウィンドウに描画するか」を表すハンドル（ウィンドウの住所みたいなもの）。
HWNDはウィンドウハンドルの型名。成功すれば true、失敗すれば false を返す。
中では、Direct3D の本体（IDirect3D9）や、描画に使う「デバイス（IDirect3DDevice9）」
「デバイスコンテキスト(ID3D11DeviceContext)」「スワップチェーン」を作る処理をします。
本体（IDirect3D9）とはこのオブジェクトを作ることで、Direct3D の機能が使える。Direct3D という世界に入るための入り口、電源ボタン
デバイス（IDirect3DDevice9）はこれは 実際に画面に絵を描く司令官、職人。全ての描画命令はこの人に指示する
shader.cpp や sprite.cppはリソースを使って実際に絵を描く機能を実装しているコードの場所。
「どんなふうに絵を描くか」を書いている部分 。絵の指示書。*/
bool Direct3D_Initialize(HWND hWnd); // Direct3Dの初期化(準備）

/*Direct3D、アプリを閉じる前に必ず呼ぶべき関数です。
初期化で作ったものを、SAFE_RELEASE マクロで順番に片付けます（リリースする）。*/
void Direct3D_Finalize(); // Direct3Dの終了処理（片付け）




/*前のフレームの絵が残っていたら,次の絵と重ならないように
まず最初にウィンドウを「まっさらなキャンバス」にする(黒板消し)*/
//void Direct3D_Clear(); // バックバッファ(下書き用の画面)のクリア

/*Clear() して → バックバッファにいろいろ描いて → 最後に それを画面(フロントバッファ（実際の画面))に映す。
「下書き用の画面（バックバッファ）」に描いた絵を、本番用の画面(フロントバッファ）に映す作業*/
void Direct3D_Present(); // フロントバッファ（実際の画面)にバックバッファ(下書き用の画面)の表示する





//バックバッファ(下書き用の画面)の大きさの取得（pxピクセル単位で）
unsigned int Direct3D_GetBackBufferWidth();//幅
unsigned int Direct3D_GetBackBufferHeight();//高さ





/*デバイス(ID3D11Device)はGPUに使わせるリソースを作って渡す装置
戻り値の型 関数名(引数リスト)、ID3D11Device*はポインタの型名、*まで含めてポインタの型名*/
//Direct3Dデバイス(ID3D11Device)を取得する関数
ID3D11Device* Direct3D_GetDevice();

/* GPU に描画命令を出す「司令塔」
「このバッファを使って描け」「このシェーダーで処理しろ」「画面をクリアしろ」*/
//Direct3Dデバイスコンテキスト(ID3D11DeviceContext)を取得する関数
ID3D11DeviceContext* Direct3D_GetContext();


//深度バッファの設定
void Direct3D_SetDepthEnable(bool enable);
void Direct3D_SetDepthDepthWriteDisable();


//ビューポート行列の作成
DirectX::XMMATRIX Direct3D_MatrixViewport();

//スクリーン座標　→　3D座標変換
DirectX::XMFLOAT3 Direct3D_ScreenToWorld(int x, int y, float depth, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

//3D座標変換　→　スクリーン座標
//DirectX::XMFLOAT3 Direct3D_WorldToScreen();

//バックバッフアのクリア
void Direct3D_ClearBackBuffer();//void Direct3D_Clear();

//バックバッフアへのレンダリングに切り替える
void Direct3D_SetBackBuffer();

//バックバッフアのクリア
void Direct3D_ClearOffscreen();

//テクスチャへのレンダリングに切り替える
void Direct3D_SetOffscreen();

//オフスクリーンレンダリングテクスチャの設定
void Direct3D_SetOffscreenTexture(int slot);

//深度情報バッファのクリア
void Direct3D_ClearShadowDepth();

//深度情報バッファのレンダリングに切り替える
void Direct3D_SetShadowDepth();

//深度情報バッフアレンダリングテクスチャの設定
void Direct3D_SetDepthShadowTexture(int slot);

//ライトビュープロジェクション行列の定数バッファへの登録と設定
void Direct3D_SetLightViewProjectionMatrix(const DirectX::XMMATRIX & matrix);

#endif // DIRECT3D_H