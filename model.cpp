/*==============================================================================

　　  3Dモデルの読み込み[model.h]
														 Author : Kouki Tanaka
														 Date   : 2025/10/22
--------------------------------------------------------------------------------

==============================================================================*/

#include "direct3d.h"
#include "texture.h"
#include "model.h"
#include"shader3d.h"
#include "WICTextureLoader11.h"
#include"shader3d_unlit.h"
#include"shader_depth.h"
#include<assert.h>
#include<algorithm>
#include<DirectXMath.h>

using namespace DirectX;

// 3D頂点構造体
struct Vertex3d
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT3 normalVector;//法線
	XMFLOAT4  color;
	XMFLOAT2 texcoord;//uv
};
static int g_TextureWhite = -1;

MODEL* ModelLoad( const char *FileName ,float scale, bool isBrender)
{
	MODEL* model = new MODEL;

	model->AiScene = aiImportFile(FileName, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);
	assert(model->AiScene);

	model->VertexBuffer = new ID3D11Buffer*[model->AiScene->mNumMeshes];
	model->IndexBuffer = new ID3D11Buffer*[model->AiScene->mNumMeshes];

	model->local_aabb.min;


	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// 頂点バッファ生成
		{
			Vertex3d* vertex = new Vertex3d[mesh->mNumVertices];

			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				if (isBrender)//Brender座標系
				{
					vertex[v].position = XMFLOAT3(mesh->mVertices[v].x*scale, mesh->mVertices[v].z*scale, -mesh->mVertices[v].y*scale);
					vertex[v].normalVector = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].z, -mesh->mNormals[v].y);

				}
				else if(!isBrender) //MAYA座標系
				{
					vertex[v].position = XMFLOAT3(mesh->mVertices[v].x * scale, mesh->mVertices[v].y * scale, mesh->mVertices[v].z * scale);
					vertex[v].normalVector = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
				}

				vertex[v].texcoord = XMFLOAT2( mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
				vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);


				if (v == 0 && m == 0) {
					model->local_aabb.min = vertex[v].position;
					model->local_aabb.max = vertex[v].position;
				}
				else {
					model->local_aabb.min.x = std::min(model->local_aabb.min.x, vertex[v].position.x);
					model->local_aabb.min.y = std::min(model->local_aabb.min.y, vertex[v].position.y);
					model->local_aabb.min.z = std::min(model->local_aabb.min.z, vertex[v].position.z);
					model->local_aabb.max.x = std::max(model->local_aabb.max.x, vertex[v].position.x);
					model->local_aabb.max.y = std::max(model->local_aabb.max.y, vertex[v].position.y);
					model->local_aabb.max.z = std::max(model->local_aabb.max.z, vertex[v].position.z);
				}
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(Vertex3d) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = vertex;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

			delete[] vertex;
		}


		// インデックスバッファ生成
		{
			unsigned int* index = new unsigned int[mesh->mNumFaces * 3];

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];

				assert(face->mNumIndices == 3);

				index[f * 3 + 0] = face->mIndices[0];
				index[f * 3 + 1] = face->mIndices[1];
				index[f * 3 + 2] = face->mIndices[2];
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * mesh->mNumFaces * 3;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = index;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);

			delete[] index;
		}

	}

		g_TextureWhite = Texture_Load(L"white.png");

	//=====テクスチャ読み込み========
		//FBXにテクスチャが内包されてる場合
	for(unsigned int i = 0; i < model->AiScene->mNumTextures; i++)
	{
		aiTexture* aitexture = model->AiScene->mTextures[i];

		ID3D11ShaderResourceView* texture;
		ID3D11Resource* resource;

		CreateWICTextureFromMemory(
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			(const uint8_t*)aitexture->pcData,
			(size_t)aitexture->mWidth,
			&resource, // release!!!!!
			&texture);

		assert(texture);

		resource->Release();//!!!!!!!!!!!!

		model->Texture[aitexture->mFilename.data] = texture;

	}


	// fbxのファイルパスだけ取得
	const std::string modelPath(FileName);
// 最後の'/' または '\' の位置を探す (Windows対応)
	size_t pos = modelPath.find_last_of("/\\");
	std::string directory;

	if (pos != std::string::npos) {
		directory = modelPath.substr(0, pos);   // ファイル名を除いた部分
	}
	else {
		directory = "";   // パスに区切りがない場合（ファイル名のみ）
	}

	// テクスチャがFBXとは別に用意されている場合
	//FBXファイルに書かれている “テクスチャ画像名” を元に、同じフォルダでその画像を探し、DirectXのGPUリソースに登録する処理
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiString filename;
		aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &filename);

		if (filename.length == 0) {
			continue;
		}

		if (model->Texture.count(filename.C_Str())) {
			continue;
		}

		ID3D11ShaderResourceView* texture;
		ID3D11Resource* resource;

		std::string texfilename = directory + "/" + filename.C_Str();

		int len = MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, nullptr, 0);
		wchar_t* pWideFilename = new wchar_t[len];
		MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, pWideFilename, len);

		CreateWICTextureFromFile(
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			pWideFilename,
			&resource,
			&texture);

		delete[] pWideFilename;

		assert(texture);

		resource->Release(); // !!!!!!!!!!

		model->Texture[filename.C_Str()] = texture;
	}

	return model;
}




void ModelRelease(MODEL* model)
{
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		model->VertexBuffer[m]->Release();
		model->IndexBuffer[m]->Release();
	}

	delete[] model->VertexBuffer;
	delete[] model->IndexBuffer;


	for (std::pair<const std::string, ID3D11ShaderResourceView*> pair : model->Texture)
	{
		pair.second->Release();
	}


	aiReleaseImport(model->AiScene);


	delete model;
}

void ModelDraw(MODEL* model, const XMMATRIX& mtxWorld)
{
	// シェーダーを描画パイプラインに設定
	Shader3D_Begin();

	// プリミティブトポロジ設定
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Shader3D_SetWorldMatrix(mtxWorld);

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)//mがメッシュ(モデルの部位)の番号
	{
		// テクスチャの設定
			aiString texture;
			aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
			aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texture);

			if (texture.length != 0) {
				//if (texture != aiString("")) {
				Direct3D_GetContext()->PSSetShaderResources(0, 1, &model->Texture[texture.data]);

				aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
			}
			else {
				Texture_SetTexture(g_TextureWhite);

				aiColor3D diffuse;
//＝＝＝＝＝＝＝＝＝現代ゲーム開発のモデル別、パーツ別、ピクセル別にレンダリング変える方法は「スペキュラーマップ」。「テクスチャマップ」の中の１種＝＝＝＝
				aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);//AI_MATKEY_COLOR_SPECULARとかパーツごとにレンダリング変えれるかも
				Shader3d_SetColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });//shader3d.h/cpp
			}

		//マテリアル設定 カービィの足をピンクから赤色に戻す処理fbxでは元々赤色
		//aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		/*aiColor3D diffuse;
		aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		Shader3d_SetColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });//shader3d.h/cpp*/


		// 頂点バッファを描画パイプラインに設定
		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);

		// インデックスバッファを描画パイプラインに設定
		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);//unsigned shortはR16、unsigned intはR32

		// ポリゴン描画命令発行
					/*============面の数(増やすたびに6頂点ずつ必ず書き換える)==============*/
		//g_pContext->Draw(NUM_VERTEX, 0);
		Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3 , 0, 0);
	}
}

void ModelDepthDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
	// シェーダーを描画パイプラインに設定
	ShaderDepth_Begin();

	// プリミティブトポロジ設定
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ShaderDepth_SetWorldMatrix(mtxWorld);

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)//mがメッシュ(モデルの部位)の番号
	{
		// テクスチャの設定
		aiString texture;
		aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texture);

		if (texture.length != 0) {
			//if (texture != aiString("")) {
			Direct3D_GetContext()->PSSetShaderResources(0, 1, &model->Texture[texture.data]);

			aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		}
		else {
			Texture_SetTexture(g_TextureWhite);

			aiColor3D diffuse;
			//＝＝＝＝＝＝＝＝＝現代ゲーム開発のモデル別、パーツ別、ピクセル別にレンダリング変える方法は「スペキュラーマップ」。「テクスチャマップ」の中の１種＝＝＝＝
			aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);//AI_MATKEY_COLOR_SPECULARとかパーツごとにレンダリング変えれるかも
			ShaderDepth_SetColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });//shader3d.h/cpp
		}

		//マテリアル設定 カービィの足をピンクから赤色に戻す処理fbxでは元々赤色
		//aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		/*aiColor3D diffuse;
		aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		Shader3d_SetColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });//shader3d.h/cpp*/


		// 頂点バッファを描画パイプラインに設定
		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);

		// インデックスバッファを描画パイプラインに設定
		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);//unsigned shortはR16、unsigned intはR32

		// ポリゴン描画命令発行
					/*============面の数(増やすたびに6頂点ずつ必ず書き換える)==============*/
		//g_pContext->Draw(NUM_VERTEX, 0);
		Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0);
	}
}

void ModelUnlitDraw(MODEL* model, const XMMATRIX& mtxWorld)
{
	// シェーダーを描画パイプラインに設定
	Shader3DUnlit_Begin();

	// プリミティブトポロジ設定
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Shader3DUnlit_SetWorldMatrix(mtxWorld);

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)//mがメッシュ(モデルの部位)の番号
	{
		Shader3DUnlit_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		// テクスチャの設定
		aiString texture;
		aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texture);

		if (texture.length != 0) {
			//if (texture != aiString("")) {
			Direct3D_GetContext()->PSSetShaderResources(0, 1, &model->Texture[texture.data]);

			aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		}
		else {
			Texture_SetTexture(g_TextureWhite);

			aiColor3D diffuse;
			//＝＝＝＝＝＝＝＝＝現代ゲーム開発のモデル別、パーツ別、ピクセル別にレンダリング変える方法は「スペキュラーマップ」。「テクスチャマップ」の中の１種＝＝＝＝
			aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);//AI_MATKEY_COLOR_SPECULARとかパーツごとにレンダリング変えれるかも
			Shader3DUnlit_SetColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });//shader3d.h/cpp
		}

		//マテリアル設定 カービィの足をピンクから赤色に戻す処理fbxでは元々赤色
		//aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		/*aiColor3D diffuse;
		aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		Shader3d_SetColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });//shader3d.h/cpp*/


		// 頂点バッファを描画パイプラインに設定
		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);

		// インデックスバッファを描画パイプラインに設定
		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);//unsigned shortはR16、unsigned intはR32

		// ポリゴン描画命令発行
					/*============面の数(増やすたびに6頂点ずつ必ず書き換える)==============*/
		//g_pContext->Draw(NUM_VERTEX, 0);
		Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0);
	}
}

AABB Model_GetAABB(MODEL* model, const DirectX::XMFLOAT3& position)
{
	return {
		{position.x + model->local_aabb.min.x, position.y + model->local_aabb.min.y, position.z + model->local_aabb.min.z},
		{position.x + model->local_aabb.max.x, position.y + model->local_aabb.max.y, position.z + model->local_aabb.max.z}

	};
}






