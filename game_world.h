#pragma once

class GameObject;
class GamePlayer;
# include "game_object_container.h"
# include <Siv3D.hpp>

class GameWorld
{
private:
	static constexpr int CONTAINER_CAPACITY{ 400 };
	GameObjectContainer m_container{ CONTAINER_CAPACITY };
	GamePlayer* m_player{ nullptr };

public:
	~GameWorld();

	void Register(GameObject* game_object);
	void Update(float delta_time);
	void Draw() const;
	GameObjectContainer GetGameObjects(Circle area) const;

	void SetPlayer(GamePlayer* player) { m_player = player; }
	GamePlayer* GetPlayer() const { return m_player; }
};

