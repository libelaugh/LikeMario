#pragma once
# include <vector>

class GameObject;
class Circle;

class GameObjectContainer
{
private:
	std::vector<GameObject*> m_objects;

public:
	explicit GameObjectContainer(int capacity = 0)
	{
		if (capacity > 0)
		{
			m_objects.reserve(static_cast<size_t>(capacity));
		}
	}

	void Add(GameObject* game_object)
	{
		m_objects.push_back(game_object);
	}

	const std::vector<GameObject*>& GetRaw() const
	{
		return m_objects;
	}

	std::vector<GameObject*>& GetRaw()
	{
		return m
