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

/*cbuffer VS_CONSTANT_BUFFER3 : register(b3)
{
    float4 ambient_color;
};

cbuffer VS_CONSTANT_BUFFER4 : register(b4)
{
    float4 directional_world_vector;
    float4 directional_color;
};*/

cbuffer VS_CONSTANT_BUFFER5 : register(b5)
{
    float4x4 light_view_proj;
};

struct VS_IN
{
    float3 posL : POSITION0;
    float3 normalL : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct VS_OUT
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
    float4 posLightWVP : POSITION1;
    float4 color : COLOR0;
    float4 normalW : NORMAL0;
    //float4 directional : COLOR1;
    //float4 ambient : COLOR2;
    float2 uv : TEXCOORD0;
};

VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    float4 posW = mul(float4(vi.posL, 1.0f), world);

    vo.posW = posW;
    vo.posH = mul(mul(posW, view), projection);
    vo.posLightWVP = mul(posW, light_view_proj);

    float3 nW = mul(float4(vi.normalL, 0.0f), world).xyz;
    vo.normalW = float4(normalize(nW), 0.0f);

    vo.color = vi.color;
    vo.uv = vi.uv;
    vo.color = float4(1, 0, 0, 1);
    return vo;
}