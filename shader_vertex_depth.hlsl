/*==============================================================================

   2D描画用頂点シェーダー [shader_vertex_depth.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/12/10
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4x4 world;
};
 
cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 view;
};

cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4x4 proj;
};
 
struct VS_IN
{
    float4 posL : POSITION0;
};
 
struct VS_0UT
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
};
 
//=============================================================================
// 頂点シェーダ
//=============================================================================
VS_0UT main(VS_IN vi)
{
    VS_0UT vo;
    
    vo.posW = mul(vi.posL, world);
    float4x4 mtxVP = mul(view, proj);
    vo.posH = mul(vo.posW, mtxVP);
    
    return vo;
}