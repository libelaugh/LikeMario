/*==============================================================================

Å@Å@  3DÉÇÉfÉãÇÃì«Ç›çûÇ›[model.h]
														 Author : Kouki Tanaka
														 Date   : 2025/10/22
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef MODEL_H
#define MODEL_H


#include "Assimp/assimp/scene.h"
#include "Assimp/assimp/cimport.h"
#include "Assimp/assimp/postprocess.h"
#include "Assimp/assimp/matrix4x4.h"
#pragma comment (lib, "assimp-vc143-mt.lib")
#include <unordered_map>

#include"collision.h"
#include<d3d11.h>
#include<DirectXMath.h>



struct MODEL
{
	const aiScene* AiScene = nullptr;

	ID3D11Buffer** VertexBuffer;
	ID3D11Buffer** IndexBuffer;

	std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;

	AABB local_aabb{};
};


MODEL* ModelLoad(const char* FileName, float scale, bool isBrender=false);
void ModelRelease(MODEL* model);
void ModelDraw(MODEL* model ,const DirectX::XMMATRIX& mtxWorld);
void ModelDepthDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld);
void ModelUnlitDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld);

AABB Model_GetAABB(MODEL* model, const DirectX::XMFLOAT3& position);

#endif//MODEL_H