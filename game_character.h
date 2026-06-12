#pragma once
# include <algorithm>
# include "game_object.h"
# include "game_damage.h"

class GameCharacter : public GameObject
{
private:
	int m_hp_capacity{};
	int m_hp{};

public:
	GameCharacter(GameWorld* world, const Float2& position, int hp, const std::string& tag)
		: GameObject(world, position, tag)
		, m_hp_capacity(hp)
		, m_hp(hp)
	{
	}

protected:
	int GetHp() const { return m_hp; }
	int GetHpCapacity() const { return m_hp_capacity; }
	int DecreaseHP(int amount)
	{
		m_hp = std::max(m_hp - amount, 0);
		return m_hp;
	}
};
