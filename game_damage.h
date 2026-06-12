#pragma once

class GameDamage
{
private:
	float m_lightning{};
	float m_fire{};
	float m_ice{};
	float m_slash{};
	float m_strike{};
	float m_thrust{};

public:
	void SetLightning(float damage) { m_lightning = damage; }
	void SetFire(float damage) { m_fire = damage; }
	void SetIce(float damage) { m_ice = damage; }
	void SetSlash(float damage) { m_slash = damage; }
	void SetStrike(float damage) { m_strike = damage; }
	void SetThrust(float damage) { m_thrust = damage; }

	float GetLightning() const { return m_lightning; }
	float GetFire() const { return m_fire; }
	float GetIce() const { return m_ice; }
	float GetSlash() const { return m_slash; }
	float GetStrike() const { return m_strike; }
	float GetThrust() const { return m_thrust; }
};
