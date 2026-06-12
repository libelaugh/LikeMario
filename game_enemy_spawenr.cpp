#include "stdafx.h"
# include "game_enemy_spawenr.h"
# include "game_enemy.h"
# include "game_world.h"

void GameEnemySpawner::Update(float delta_time)
{
	m_elapsed += delta_time;
	if (m_elapsed < m_spawnInterval)
	{
		return;
	}

	m_elapsed = 0.0;

	const Float2 jitter = RandomVec2(40.0);
	auto* enemy = new GameEnemy(GetWorld(), GetPosition() + jitter);
	GetWorld()->Register(enemy);
}

void GameEnemySpawner::Draw() const
{
	Circle{ GetPosition(), 12.0 }.drawFrame(3.0, Palette::Red);
}
