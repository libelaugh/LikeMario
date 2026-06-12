#include "stdafx.h"
# include "game_world.h"
# include "game_object.h"
# include <algorithm>

GameWorld::~GameWorld()
{
	for (auto* object : m_container.GetRaw())
	{
		delete object;
	}
}

void GameWorld::Register(GameObject* game_object)
{
	if (!game_object)
	{
		return;
	}

	m_container.Add(game_object);
}

void GameWorld::Update(float delta_time)
{
	auto currentObjects = m_container.GetRaw();
	for (auto* object : currentObjects)
	{
		if (object && !object->IsDead())
		{
			object->Update(delta_time);
		}
	}

	auto& objects = m_container.GetRaw();
	objects.erase(std::remove_if(objects.begin(), objects.end(),
		GameObject * object
		{
			if (!object)
			{
				return true;
			}
			if (object->IsDead())
			{
				delete object;
				return true;
			}
			return false;
		}),
		objects.end());
}

void GameWorld::Draw() const
{
	for (auto* object : m_container.GetRaw())
	{
		if (object && !object->IsDead())
		{
			object->Draw();
		}
	}
}

GameObjectContainer GameWorld::GetGameObjects(Circle area) const
{
	GameObjectContainer result;
	for (auto* object : m_container.GetRaw())
	{
		if (!object || object->IsDead())
		{
			continue;
		}

		if (area.intersects(object->GetCollision()))
		{
			result.Add(object);
		}
	}
	return result;
}
``
