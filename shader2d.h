/*==============================================================================

   シェーダー2D用　[shader2d.h]
														 Author : Youhei Sato
														 Date   : 2025/09/19
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADER2D_H
#define	SHADER2D_H

#include <d3d11.h>
#include <DirectXMath.h>

bool Shader2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader2D_Finalize();

void Shader2D_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader2D_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);

void Shader2D_Begin();



#endif//SHADER2D_H