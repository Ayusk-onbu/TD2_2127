#include "Bullet.h"
#include "EnemyManager.h"
#include "MapChipField.h"
#include "CameraSystem.h"
#include "Player.h"

void Bullet::Initialize(ModelObject* model, const Vector3& position, const Vector3& velocity, EnemyManager* enemyManager, MapChipField* mapChipField, Player* player) {
	assert(model);
	assert(enemyManager);
	assert(player);

	model_ = model;
	velocity_ = velocity;
	enemyManager_ = enemyManager;
	mapChipField_ = mapChipField;
	player_ = player;

	worldTransform_.Initialize();
	model_->worldTransform_.set_.Translation(position);
	model_->worldTransform_.set_.Scale(kBulletSize);
}

void Bullet::Update() {
	lifeTimer_--;
	if (lifeTimer_ <= 0) {
		isDead_ = true;
	}

	model_->worldTransform_.get_.Translation() += velocity_;

	AABB myAABB = GetAABB();
	const std::list<Enemy*>& enemies = enemyManager_->GetEnemies();
#pragma region 敵との当たり判定
	for (Enemy* enemy : enemies) {
		if (enemy->IsDead()) {
			continue;
		}

		AABB enemyAABB = enemy->GetAABB();

		if ((myAABB.min.x <= enemyAABB.max.x && myAABB.max.x >= enemyAABB.min.x) && // X軸
		    (myAABB.min.y <= enemyAABB.max.y && myAABB.max.y >= enemyAABB.min.y) && // Y軸
		    (myAABB.min.z <= enemyAABB.max.z && myAABB.max.z >= enemyAABB.min.z)) { // Z軸

			enemy->OnCollision();
			isDead_ = true;
			if (player_->IsInAir()) {
				player_->ResetAirShot();
			}
			break;
		}
	}
#pragma endregion

#pragma region 地形との当たり判定/反射

	Vector3 nextPosition = model_->worldTransform_.get_.Translation() + velocity_;

	MapChipField::IndexSet currentIndexSet = mapChipField_->GetMapChipIndexSetByPosition(model_->worldTransform_.get_.Translation());

	MapChipField::IndexSet xNextIndexSet = mapChipField_->GetMapChipIndexSetByPosition({nextPosition.x, model_->worldTransform_.get_.Translation().y, 0.0f});
	MapChipField::MapChipType xChipType = mapChipField_->GetMapChipTypeByIndex(xNextIndexSet.xIndex, xNextIndexSet.yIndex);
	if (xChipType == MapChipField::MapChipType::kBlock && currentIndexSet.xIndex != xNextIndexSet.xIndex) {
		if (currentBound_ < LimitBound) {
			velocity_.x *= -1.0f;
			currentBound_++;
		} else {
			isDead_ = true;
		}
	}

	MapChipField::IndexSet yNextIndexSet = mapChipField_->GetMapChipIndexSetByPosition({model_->worldTransform_.get_.Translation().x, nextPosition.y, 0.0f});
	MapChipField::MapChipType yChipType = mapChipField_->GetMapChipTypeByIndex(yNextIndexSet.xIndex, yNextIndexSet.yIndex);
	if (yChipType == MapChipField::MapChipType::kBlock && currentIndexSet.yIndex != yNextIndexSet.yIndex) {
		if (currentBound_ < LimitBound) {
			velocity_.y *= -1.0f;
			currentBound_++;
		} else {
			isDead_ = true;
		}
	}
#pragma endregion
	model_->LocalToWorld();
}

void Bullet::Draw() { 
	model_->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(model_->worldTransform_.mat_));
 	model_->Draw(); 
}

AABB Bullet::GetAABB() {
	AABB aabb;
	// 弾の中心座標
	Vector3 center = model_->worldTransform_.get_.Translation();
	// 弾のサイズ
	Vector3 halfSize = {kBulletSize.x * 0.5f, kBulletSize.y * 0.5f, kBulletSize.z * 0.5f};
	aabb.min = {center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z};
	aabb.max = {center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z};
	return aabb;
}
