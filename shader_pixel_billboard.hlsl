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

float4 main(PS_IN pi) : SV_TARGET
{
    //direct3d.cppのbd.AlphaToCoverageEnable = FALSE; をコードで書いてみた
    float4 color = tex.Sample(samp, pi.uv) * pi.color;
    clip(sin(pi.uv.x * 500));//「clip」テクスチャを一定間隔で切り取る表現
    if (color.a < 0.1f)
    {
        discard;//そのピクセルは描画されない
        //clip(-1);
    }
    
        return color;
}