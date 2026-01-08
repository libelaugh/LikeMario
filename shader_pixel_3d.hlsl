/*==============================================================================

   3D描画用ピクセルシェーダー [shader_pixel_3d.hlsl]
														 Author : Tanaka Kouki
														 Date   : 2025/09/11
--------------------------------------------------------------------------------

==============================================================================*/
//=============================================================================
// 3D描画用ピクセルシェーダー
//=============================================================================

//定数バッファ
cbuffer PS_CONSTANT_DIFFUSE : register(b0)
{
float4 diffuse_color;
};

cbuffer PS_CONSTANT_AMBIENT : register(b1)
{
    float4 ambient_color;
};

cbuffer PS_CONSTANT_DIRECTIONAL : register(b2)
{
    float4 directional_world_vector;
    float4 directional_color = { 1.0f, 1.0f, 1.0f, 1.0f };
}
    
cbuffer PS_CONSTANT_SPECULAR : register(b3)
{
    float3 eye_posW;
    float specular_power=30.0f;
    float4 specular_color = { 0.2f, 0.2f, 0.2f ,1.0f};
};

struct PointLight
{
    float3 pointlight_posW;
    float pointlight_range;
    float4 color;
};
cbuffer PS_CONSTANT_POINTLIGHT : register(b4)
{
    PointLight point_light[4];
    int point_light_count;
    float3 point_light_dummy;
};

struct PS_IN
{
    float4 posH : SV_POSITION; //データ型　変数名　：　入力セマンティクス
    float4 posW : POSITION0;
    float4 posLightWVP : POSITION1;
    float3 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

Texture2D tex : register(t0);//テクスチャ
Texture2D tex2 : register(t2);//深度テクスチャ
SamplerState samp : register(s0);
//SamplerState samp1 : register(s1);

float4 main(PS_IN pi) : SV_TARGET
{
    float3 material_color = tex.Sample(samp, pi.uv).rgb * pi.color.rgb * diffuse_color.rgb; //材質の色
    
    //ライト計算
    //平行光源(ディフューズライト・ディレクショナルライト)
    float3 normalW = normalize(pi.normalW.xyz);
    //float dl = max(0, dot(-directional_world_vector, normalW)); //内積(色の値は0～1だからmaxで<0は0にする）
    float dl = (dot(-directional_world_vector.xyz, normalW) + 1.0f) * 0.5f;//HalfLambert　現実では内積９０°以上の部分も明るいからこうする
    //色の見え方はライトの色×材質の色の乗算で出る（テクスチャ×頂点カラー×ライトの色）
    float3 diffuse = material_color * directional_color.rgb * dl;
    
    //環境洸（アンビエントライト）
    float3 ambient = material_color.rgb * ambient_color.rgb;
    
     //スペキュラーライト,Phongライト計算
    //頂点シェーダでやるとラスタライズ(線形補間)段階で角ばってしまうから、現代は必ずピクセルシェーダでやる。処理は重くなるけど大したことない
    float3 toEye = normalize(eye_posW - pi.posW.xyz); //視線ベクトル
    float3 r = reflect(directional_world_vector.xyz, pi.normalW); //反射ベクトル
    float t = pow(max(dot(r, toEye), 0.0f), specular_power); //スペキュラーライトの強さ
    float3 specular = specular_color.rgb * t;
   
    float alpha = tex.Sample(samp, pi.uv).a * pi.color.a * diffuse_color.a;
    float3 color = ambient + diffuse + specular;//ここは加算//これが最終的に目に届く色
    
    float lim = max(dot(normalW.xyz, toEye), 0.0f);
    lim = pow(lim, 2.0f);
    //color += float3(lim, lim, lim);
    
    //点光源（ポイントライト）のサンプルコード
    for (int i = 0; i<point_light_count; i++)
    {
        //点光源から面（ピクセル）
        float3 lightToPixel = pi.posW.xyz - point_light[i].pointlight_posW;
    //面（ピクセル）とライトとの距離を測る
        float D = length(pi.posW.xyz - point_light[i].pointlight_posW);
    
    //影響範囲の計算
        float influence = pow(max(1.0f - 1.0f / point_light[i].pointlight_range * D, 0.0f), 2.0f);
    //pow(A * A)
    // range = 400 length =0    ... A=1      A*A = 1
    //                    =100  ... A=0.75       =0.5625
    //                    =200  ... A=0.5        =0.25
    //                    =300  ... A=0.25       =0.0625
    //                    =400  ... A=0          =0
        
        float dl = max(0.0f, dot(-normalize(lightToPixel), normalW.xyz));
    //点光源の影響を加算する
        color += material_color * point_light[i].color.rgb * influence * dl;
        
        //点光源のスぺキュラ
        float3 r = reflect(normalize(lightToPixel), normalW.xyz);
        float t = pow(max(dot(r, toEye), 0.0f), specular_power);
        
        
        //点光源のスぺキュラを
        color += point_light[i].color.rgb * t;

    }
    
    //影の計算
    float2 shadowmap_uv = pi.posLightWVP.xy / pi.posLightWVP.w;//正規化。数学的に必要らしい
    shadowmap_uv = shadowmap_uv * float2(0.5f, -0.5f) + 0.5f;
    
    float depthmap_z = tex2.Sample(samp, shadowmap_uv).r;
    
    float shadowmap_z = pi.posLightWVP.z / pi.posLightWVP.w;
    
    if (shadowmap_z > depthmap_z)
    {
        color *= 0.5f;
    }
    
        return float4(color, alpha);;
    
    //float4 texColor = tex.Sample(samp, pi.uv);
    //return texColor * pi.color;
}
