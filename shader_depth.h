/*==============================================================================

　　  ライトの設定[light.h]
														 Author : Kouki Tanaka
														 Date   : 2025/09/30
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SHADER_DEPTH_H
#define SHADER_DEPTH_H

#include<d3d11.h>
#include<DirectXMath.h>

bool ShaderDepth_Initialize();
void ShaderDepth_Finalize();

void ShaderDepth_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void ShaderDepth_SetViewMatrix(const DirectX::XMMATRIX& matrix);
void ShaderDepth_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);
void ShaderDepth_SetColor(const DirectX::XMFLOAT4& color);
void ShaderDepth_Begin();

#endif//SHADER_DEPTH_H

