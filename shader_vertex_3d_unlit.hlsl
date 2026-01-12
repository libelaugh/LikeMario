/*==============================================================================

   3D描画用頂点シェーダー [shader_vertex_3d.hlsl]
														 Author : Tanaka Kouki
														 Date   : 2025/09/11
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




struct VS_IN
{
    float4 posL : POSITION0;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct VS_OUT//ラスタライズ（線形補間）されてピクセルシェーダに送られるデータ
{
    float4 posH : POSITION0;
    float2 uv : TEXCOORD0;
};

VS_OUT main(VS_IN vi)
{
    VS_OUT vo;

    float4x4 mtxWV = mul(world, view);
    float4x4 mtxWVP = mul(mtxWV, projection);
    vo.posH = mul(vi.posL, mtxWVP);
    vo.uv = vi.uv;

    return vo;
}