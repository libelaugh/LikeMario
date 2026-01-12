/*==============================================================================

   シェーダー3D用(ライトなし)　[shader3d_unlit.h]
														 Author : Tanaka Kouki
														 Date   : 2025/11/21
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADER3D_UNLIT_H
#define	SHADER3D_UNLIT_H

#include <d3d11.h>
#include <DirectXMath.h>

bool Shader3DUnlit_Initialize();
void Shader3DUnlit_Finalize();

void Shader3DUnlit_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader3DUnlit_SetColor(const DirectX::XMFLOAT4& color);

void Shader3DUnlit_Begin();

#endif // SHADER3D_UNLIT_H
