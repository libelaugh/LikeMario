#include "stdafx.h"
# include "game_enemy.h"
# include "game_player.h"
# include "game_world.h"

GameEnemy::GameEnemy(GameWorld* world, const Float2& position)
	: GameCharacter(world, position, 30, "enemy")
{
}

void GameEnemy::Update(float delta_time)
{
	auto* player = GetWorld()->GetPlayer();
	if (!player)
	{
		return;
	}

	Float2 dir = player->GetPosition() - GetPosition();
	if (!dir.isZero())
	{
		dir = dir.normalized();
	}

	constexpr float ENEMY_MOVE_SPEED = 100.0f;
	SetPosition(GetPosition() + dir * ENEMY_MOVE_SPEED * delta_time);
}

void GameEnemy::Draw() const
{
	Circle{ GetPosition(), 20.0 }.draw(Palette::Orange);
}

void GameEnemy::Damage(const GameDamage&)
{
	// まだ未使用
}
