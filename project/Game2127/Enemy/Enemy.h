#pragma once
#include <cassert>
#include "Fngine.h"
#include "ModelObject.h"
#include "AABB.h"

class Enemy {
public:



	// 初期化
	void Initialize(ModelObject* model, const Vector3& position);
	// 更新
	void Update();
	// 描画
	void Draw();

	bool IsDead() const { return isDead_; }
	void OnCollision() { isDead_ = true; }
	AABB GetAABB();

private:
	ModelObject* model_ = nullptr;
	bool isDead_ = false;

	// 敵のサイズ
	static inline const Vector3 kEnemySize = {1.8f, 1.8f, 1.8f};
};