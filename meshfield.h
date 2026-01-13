/*==============================================================================

　　　メッシュフィールド（地面）作成[meshfield.h]
														 Author : Kouki Tanaka
														 Date   : 2025/09/09
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef MESHFIELD__H
#define MESHFIELD__H

#include<d3d11.h>
#include<DirectXMath.h>

void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void MeshField_Finalize();
void MeshField_Draw(DirectX::XMMATRIX& mtrWorld);

float MeshField_GetHalf();

#endif//MESHFIELD