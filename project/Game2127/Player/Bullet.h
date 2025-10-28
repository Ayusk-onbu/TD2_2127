#pragma once
#include "EnemyManager.h"
#include "MapChipField.h"
#include <cassert>
#include "AABB.h"

class MapChipField;
class EnemyManager;
class Player;

class Bullet {
public:
	// 初期化
	void Initialize(ModelObject* model, const Vector3& position, const Vector3& velocity, EnemyManager* enemyManager, MapChipField* mapChipField, Player* player);
	// 更新
	void Update();
	// 描画
	void Draw();

	// 生存フラグ
	bool IsDead() const { return isDead_; }

	AABB GetAABB();

private:
	WorldTransform worldTransform_;
	ModelObject* model_ = nullptr;
	Vector3 velocity_ = {};
	MapChipField* mapChipField_;
	Player* player_ = nullptr;
	
	static inline const int32_t kLifeTime = 120;
	int32_t lifeTimer_ = kLifeTime;
	bool isDead_ = false;
	const int LimitBound = 1;
	int currentBound_ = 0;
	static inline const Vector3 kBulletSize = {0.4f, 0.4f, 0.2f};

	EnemyManager* enemyManager_ = nullptr;
};