#include "stdafx.h"
# include "game_player.h"

GamePlayer::GamePlayer(GameWorld* world, const Float2& position)
	: GameCharacter(world, position, 100, "player")
{
}

void GamePlayer::Update(float delta_time)
{
	move(delta_time);
	// 今回は追従確認のため攻撃はまだ呼ばない
}

void GamePlayer::Draw() const
{
	Circle{ GetPosition(), 24.0 }.draw(Palette::Dodgerblue);
}

void GamePlayer::Damage(const GameDamage&)
{
	// 今回は未実装
}

void GamePlayer::move(float delta_time)
{
	Float2 dir{};

	if (KeyW.pressed())
	{
		dir.y -= 1.0f;
	}
	if (KeyS.pressed())
	{
		dir.y += 1.0f;
	}
	if (KeyA.pressed())
	{
		dir.x -= 1.0f;
	}
	if (KeyD.pressed())
	{
		dir.x += 1.0f;
	}

	if (!dir.isZero())
	{
		dir = dir.normalized();
	}

	constexpr float PLAYER_MOVE_SPEED = 220.0f;
	auto next = (GetPosition() + dir * PLAYER_MOVE_SPEED * delta_time);
	next.x = Clamp(next.x, 0.0f, static_cast<float>(Scene::Width()));
	next.y = Clamp(next.y, 0.0f, static_cast<float>(Scene::Height()));
	SetPosition(next);
}

void GamePlayer::attack()
{
	// スラッシュ攻撃は次の段階で追加
}
