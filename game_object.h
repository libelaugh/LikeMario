#pragma once
# include <Siv3D.hpp>
# include <string>

class GameWorld;
class GameDamage;

class GameObject
{
private:
	GameWorld* m_world{};
	Float2 m_position{};
	std::string m_tag;
	bool m_isDead{ false };

public:
	GameObject(GameWorld* world, const Float2& position, const std::string& tag)
		: m_world(world)
		, m_position(position)
		, m_tag(tag)
	{
	}

	virtual ~GameObject() = default;

	virtual void Update(float delta_time) = 0;
	virtual void Draw() const = 0;
	virtual void Damage(const GameDamage&) {}
	virtual Circle GetCollision() const
	{
		return { m_position, 0.0 };
	}

	GameWorld* GetWorld() const { return m_world; }
	const Float2& GetPosition() const { return m_position; }
	void SetPosition(const Float2& position) { m_position = position; }
	const std::string& GetTag() const { return m_tag; }

	bool IsDead() const { return m_isDead; }
	void Destroy() {
		m_isDead = true;
	}
};
