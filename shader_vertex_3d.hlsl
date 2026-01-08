/*==============================================================================

   3D描画用頂点シェーダー [shader_vertex_3d.hlsl]
														 Author : Tanaka Kouki
														 Date   : 2025/09/11
--------------------------------------------------------------------------------

==============================================================================*/
//定数バッファ
/*定数バッファとは？
定数バッファ（Constant Buffer）は*回転行列・色・時間・光の位置などの定数（≒変化はするけど、1つの描画中は一定）
を入れる箱で、GPU（グラフィックカード）に少量のデータ（定数）を高速に渡すための仕組みです。

どんなとき使うの？
毎フレーム変わるような行列（ワールド行列、ビュー行列、プロジェクション行列など）
ライトの情報（光の位置、色）
ゲーム中の時間や状態などの値
などをシェーダーに渡すために使います。*/

/*DirectX（特に定数バッファ）では「16バイト単位（16バイト境界）」が重要です。
なぜ16バイト？
GPU側（HLSLのシェーダー）では、定数バッファの中身は16バイト（128ビット）単位で読み込む設計になっています。
つまり：「構造体の各メンバーは16バイト単位で配置しないとダメ」
これを守らないと、正しく動かない or 無駄なメモリが使われることがあります。
ex)OKな定数バッファ
{
    DirectX::XMMATRIX world;     // 16バイト × 4 = 64バイト
    DirectX::XMFLOAT4 color;     // 16バイト
};
XMMATRIX は 4×4 行列で 64バイト
XMFLOAT4 は 4つの float で 16バイト
→ 合計 80バイト → OK（16の倍数）*/
//=============================================================================
// 3D描画用頂点シェーダー
//=============================================================================
cbuffer VS_CONSTANT_BUFFER0 : register(b0) 
{
    float4x4 world;
};

cbuffer VS_CONSTANT_BUFFER1 : register(b1)
{
    float4x4 view;
};

cbuffer VS_CONSTANT_BUFFER2 : register(b2) 
{
    float4x4 projection;
};

cbuffer VS_CONSTANT_BUFFER3 : register(b3)
{
    float4x4 light_view_proj;
};

struct VS_IN
{
    float4 posL : POSITION0; //データ型　変数名　：　入力セマンティクス（セマンティクスとはGPUの引数には変数に対してどんなデータが入っているかを設定する必要があるため、それを設定すること）
    float4 normalL : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct VS_OUT//ラスタライズ（線形補間）されてピクセルシェーダに送られるデータ
{
    float4 posH : SV_POSITION; //データ型　変数名　：　出力セマンティクス
    float4 posW : POSITION0;
    float4 posLightWVP : POSITION1;
    float3 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    
    // モデル → ワールド変換
    float4 posW = mul(vi.posL, world);
    // ワールド → ビュー → プロジェクション変換
    float4 posV = mul(posW, view);
    vo.posH = mul(posV, projection);
    
    //ライトビュープロジェクション空間へ座標変換
    vo.posLightWVP = mul(vi.posL, mul(world, light_view_proj));
    
    //ライト計算
    //普通のワールド変換行列はダメ×
    //ワールド変換行列の転置逆行列OK〇
    float4 normalW = mul(float4(vi.normalL.xyz, 0.0f), world); //内積前にloclaからworld座標系に//0.0fで平行移動成分消せるらしい
    vo.normalW = normalize(normalW.xyz);
    vo.posW = mul(vi.posL, world); //vo.posW = posW;
    
    
    vo.color = vi.color;
    vo.uv = vi.uv;

    return vo;
}