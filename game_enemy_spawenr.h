#pragma once
# include "game_object.h"

class GameEnemySpawner : public GameObject
{
private:
	double m_spawnInterval{ 2.0 };
	double m_elapsed{ 0.0 };

public:
	GameEnemySpawner(GameWorld* world, const Float2& position)
		: GameObject(world, position, "spawner")
	{
	}

	void Update(float delta_time) override;
	void Draw() const override;
	void Damage(const GameDamage&) override {}

	Circle GetCollision() const override
	{
		return { GetPosition(), 12.0 };
	}
};
