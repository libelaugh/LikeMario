/*==============================================================================

   ビルボード描画用頂点シェーダー [shader_vertex_billboard.hlsl]
														 Author : Tanaka Kouki
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/

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
    float2 scale;
    float2 translation;
};


struct VS_IN
{
    float4 posL : POSITION0; //データ型　変数名　：　入力セマンティクス（セマンティクスとはGPUの引数には変数に対してどんなデータが入っているかを設定する必要があるため、それを設定すること）
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct VS_OUT//ラスタライズ（線形補間）されてピクセルシェーダに送られるデータ
{
    float4 posH : SV_POSITION; //データ型　変数名　：　出力セマンティクス
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};


    VS_OUT main
    (VS_IN vi)
{
        VS_OUT vo;
    
    

    // モデル → ワールド変換
        float4 posW = mul(vi.posL, world);
    // ワールド → ビュー → プロジェクション変換
        float4 posV = mul(posW, view);
        vo.posH = mul(posV, projection);
    
    
        vo.color = vi.color;
        vo.uv = vi.uv * scale + translation;

        return vo;
    }