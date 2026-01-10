/*==============================================================================

	ÉAÉCÉeÉÄä«óù[item.h]
														 Author : Tanaka Kouki
														 Date   : 2026/01/05
--------------------------------------------------------------------------------

==============================================================================*/

#include "item.h"
#include "collision.h"
#include "model.h"
#include "player.h"

#include <vector>

using namespace DirectX;

namespace {
	struct ItemData {
		XMFLOAT3 position{};
		XMFLOAT3 rotation{};
		int modelIndex = -1;
		bool active = true;
	};

	std::vector<ItemData> g_items;
	std::vector<MODEL*> g_itemModels;
	int g_hitCount = 0;
}

void Item_Initialize()
{
	g_items.clear();
	g_itemModels.clear();
	g_hitCount = 0;
}

void Item_Finalize()
{
	for (auto* model : g_itemModels) {
		if (model) {
			ModelRelease(model);
		}
	}
	g_itemModels.clear();
	g_items.clear();
}

int Item_LoadModel(const char* modelPath, float scale, bool isBrender)
{
	MODEL* model = ModelLoad(modelPath, scale, isBrender);
	g_itemModels.push_back(model);
	return static_cast<int>(g_itemModels.size()) - 1;
}

int Item_Add(const XMFLOAT3& position, const XMFLOAT3& rotationDeg, int modelIndex)
{
	ItemData data{};
	data.position = position;
	data.rotation = {
		XMConvertToRadians(rotationDeg.x),
		XMConvertToRadians(rotationDeg.y),
		XMConvertToRadians(rotationDeg.z)
	};
	data.modelIndex = modelIndex;
	data.active = true;
	g_items.push_back(data);
	return static_cast<int>(g_items.size()) - 1;
}

void Item_Update()
{
	const AABB playerAabb = Player_GetAABB();
	for (auto& item : g_items) {
		if (!item.active) {
			continue;
		}

		if (item.modelIndex < 0 || item.modelIndex >= static_cast<int>(g_itemModels.size())) {
			continue;
		}

		MODEL* model = g_itemModels[item.modelIndex];
		if (!model) {
			continue;
		}

		const AABB itemAabb = Model_GetAABB(model, item.position);
		if (Collision_IsOverlapAABB(playerAabb, itemAabb)) {
			item.active = false;
			++g_hitCount;
		}
	}
}

void Item_Draw()
{
	for (const auto& item : g_items) {
		if (!item.active) {
			continue;
		}

		if (item.modelIndex < 0 || item.modelIndex >= static_cast<int>(g_itemModels.size())) {
			continue;
		}

		MODEL* model = g_itemModels[item.modelIndex];
		if (!model) {
			continue;
		}

		const XMMATRIX rotation =
			XMMatrixRotationRollPitchYaw(item.rotation.x, item.rotation.y, item.rotation.z);
		const XMMATRIX translation = XMMatrixTranslation(item.position.x, item.position.y, item.position.z);
		ModelDraw(model, rotation * translation);
	}
}

int Item_GetHitCount()
{
	return g_hitCount;
}

void Item_ResetHitCount()
{
	g_hitCount = 0;
}