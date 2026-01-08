/*==============================================================================

　　　衝突の制御[collision.h]
														 Author : Tanaka Kouki
														 Date   : 2025/11/05
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef COLLISION_H
#define COLLISION_H

#include<d3d11.h>
#include<DirectXMath.h>

struct Sphere {
	DirectX::XMFLOAT3 center;
	float radius;
};

struct Circle {
	DirectX::XMFLOAT2 center;
	float radius;
};

struct Box {
	DirectX::XMFLOAT2 center;
	float half_width;
	float half_height;
};

struct AABB {
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;

	DirectX::XMFLOAT3 GetCenter() const {
		DirectX::XMFLOAT3 center{};
		center.x = min.x + (max.x - min.x) * 0.5f;
		center.y = min.y + (max.y - min.y) * 0.5f;
		center.z = min.z + (max.z - min.z) * 0.5f;
		return center;
	}

	DirectX::XMFLOAT3 GetHalf() const {
		DirectX::XMFLOAT3 half{};
		half.x = max.x - min.x;
		half.y = max.y - min.y;
		half.z = max.z - min.z;
		return half;
	}
};

struct Hit {
	bool isHit;
	DirectX::XMFLOAT3 normal;
};

bool Collision_IsOverlapSphere(const Sphere& a, const Sphere& b);
bool Collision_IsOverlapSphere(const Sphere& a, const DirectX::XMFLOAT3& point);
bool Collision_IsOverlapCircle(const Circle& a, const Circle& b);
bool Collision_IsOverlapBox(const Box& a, const Box& b);
bool Collision_IsOverlapAABB(const AABB& a, const AABB& b);

//aのどの面にbが衝突したか？
Hit Collision_IsHitAABB(const AABB& a, const AABB& b);

//本来はdebugcollision.cpp作る
void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Collision_DebugFinalize();
void Collision_DebugDraw(const Circle& circle, const DirectX::XMFLOAT4& color={1.0f,0.0f,0.0f,1.0f});
void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4& color = { 1.0f,0.0f,0.0f,1.0f });



#endif//COLLISION_H