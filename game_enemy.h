#pragma once
# include "game_character.h"

class GameEnemy : public GameCharacter
{
public:
	GameEnemy(GameWorld* world, const Float2& position);

	void Update(float delta_time) override;
	void Draw() const override;
	void Damage(const GameDamage&) override;

	Circle GetCollision() const override
	{
		return { GetPosition(), 20.0 };
	}
};
