/*==============================================================================

   ビルボード描画用ピクセルシェーダー [shader_pixel_billboard.hlsl]
														 Author : Tanaka Kouki
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/

struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANT_COLOR : register(b0)
{
    float4 mul_color; // 16byte
};

float4 main(PS_IN pi) : SV_TARGET
{
    float4 color = tex.Sample(samp, pi.uv) * pi.color;
    clip(color.a - 0.1f);
    
    return color;
}