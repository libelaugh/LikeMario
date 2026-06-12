# include <Siv3D.hpp> // Siv3D v0.6.16
# include "game_world.h"
# include "game_player.h"
# include "game_enemy_spawenr.h"

void Main()
{
	Scene::SetBackground(ColorF(0.15, 0.18, 0.22));

	GameWorld world;

	auto* player = new GamePlayer(&world, Float2{ 400, 300 });
	world.SetPlayer(player);
	world.Register(player);

	auto* spawner = new GameEnemySpawner(&world, Float2{ 100, 100 });
	world.Register(spawner);

	while (System::Update())
	{
		const float delta_time = static_cast<float>(Scene::DeltaTime());

		world.Update(delta_time);
		world.Draw();

		SimpleGUI::Headline(U"WASD で移動");
		SimpleGUI::Text(U"赤い円 = スポナー / オレンジ = 敵 / 青 = プレイヤー", Vec2{ 20, 40 });
	}
}
``
