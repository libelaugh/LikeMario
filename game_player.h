#pragma once
# include "game_world.h"
# include "game_character.h"

class GamePlayer : public GameCharacter
{
public:
	GamePlayer(GameWorld* world, const Float2& position);

	void Update(float delta_time) override;
	void Draw() const override;
	void Damage(const GameDamage&) override;

	Circle GetCollision() const override
	{
		return { GetPosition(), 24.0 };
	}

private:
	void move(float delta_time);
	void attack();
};
