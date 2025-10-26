#pragma once
#include "Bullet.h"
#include <list>
#include"EnemyManager.h"
#include "MapChipField.h"
#include "Fngine.h"
#include "ModelObject.h"
class EnemyManager;
class Player;

class BulletManager {
public:
	~BulletManager();

	// 初期化
	void Initialize(ModelObject* model, EnemyManager* enemyManager, MapChipField* mapChipField);
	// 更新
	void Update();
	// 描画
	void Draw();

	// 弾の生成
	void SpawnBullet(const Vector3& position, const Vector3& velocity, Player* player);

	void SetFngine(Fngine* fngine) { fngine_ = fngine; }	

private:
	
	ModelObject* model_ = nullptr;
	// 弾のリスト
	std::list<Bullet*> bullets_;
	MapChipField* mapChipField_ = nullptr;

	EnemyManager* enemyManager_ = nullptr;

	Fngine* fngine_ = nullptr;
};