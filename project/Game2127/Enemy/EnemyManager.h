#pragma once
#include "Enemy.h"
#include "Fngine.h"
#include "ModelObject.h"
#include <list>

class EnemyManager {
public:
	~EnemyManager();

	// 初期化
	void Initialize(ModelObject* model);
	// 更新
	void Update();
	// 描画
	void Draw();

	// 敵の生成
	void SpawnEnemy(const Vector3& position);

	const std::list<Enemy*>& GetEnemies() const { return enemies_; }

	void SetFngine(Fngine* fngine) { fngine_ = fngine; }

private:
	ModelObject* model_ = nullptr;
	std::list<Enemy*> enemies_;
	Fngine* fngine_ = nullptr;
};