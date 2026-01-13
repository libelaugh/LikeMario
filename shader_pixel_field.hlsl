/*==============================================================================

   フィールド描画用ピクセルシェーダー [shader_pixel_field.hlsl]
														 Author : Tanaka Kouki
														 Date   : 2025/09/26
--------------------------------------------------------------------------------

==============================================================================*/
//=============================================================================
// 3D描画用ピクセルシェーダー
//=============================================================================
cbuffer PS_CONSTANT_AMBIENT : register(b1)
{
    float4 ambient_color;
};

cbuffer PS_CONSTANT_DIRECTIONAL : register(b2)
{
    float4 directional_world_vector;
    float4 directional_color;
};

Texture2D tex0 : register(t0);
Texture2D tex2 : register(t2); // shadow depth (R32_FLOAT)
SamplerState samp : register(s0);

struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
    float4 posLightWVP : POSITION1;
    float4 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 main(PS_IN pi) : SV_TARGET
{
    // ベース色（とりあえず tex0 のみ）
    float4 base = tex0.Sample(samp, pi.uv);

    // ライティング（3Dと同じ b1/b2 を使う）
    float3 n = normalize(pi.normalW.xyz);
    float dl = max(0.0f, dot(-directional_world_vector.xyz, n));
    float3 light = ambient_color.rgb + directional_color.rgb * dl;

    // 影（範囲外は影なし、bias付き）
    float shadow = 1.0f;

    if (pi.posLightWVP.w > 0.0001f)
    {
        float2 suv = pi.posLightWVP.xy / pi.posLightWVP.w;
        suv = suv * float2(0.5f, -0.5f) + 0.5f;

        if (suv.x >= 0.0f && suv.x <= 1.0f && suv.y >= 0.0f && suv.y <= 1.0f)
        {
            float depthmap_z = tex2.Sample(samp, suv).r;
            float shadowmap_z = (pi.posLightWVP.z / pi.posLightWVP.w);

            float bias = 0.0008f;
            if (shadowmap_z - bias > depthmap_z)
                shadow = 0.5f;
        }
    }

    float3 rgb = base.rgb * light * shadow;
    //return pi.color;//バグなう
    return float4(rgb, base.a); //これは正常に動く
    //return float4(rgb, base.a) * pi.color; // *pi.color頂点カラーで色付け
}

