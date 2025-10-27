#include "BulletManager.h"

BulletManager::~BulletManager() {
	for (Bullet* bullet : bullets_) {
		delete bullet;
	}
	bullets_.clear();
}

void BulletManager::Initialize(ModelObject* model, EnemyManager* enemyManager, MapChipField* mapChipField) {
	model_ = model;
	enemyManager_ = enemyManager;
	mapChipField_ = mapChipField;
}
void BulletManager::Update() {

	for (Bullet* bullet : bullets_) {
		bullet->Update();
	}

	bullets_.remove_if([](Bullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
	});
}

void BulletManager::Draw() {
	for (Bullet* bullet : bullets_) {
 		bullet->Draw();
	}
}

void BulletManager::SpawnBullet(const Vector3& position, const Vector3& velocity, Player* player) {
	Bullet* newBullet = new Bullet();
	ModelObject* bulletModel = new ModelObject();
	bulletModel->Initialize(fngine_->GetD3D12System(), model_->GetModelData());
	bulletModel->SetFngine(fngine_);
	bulletModel->textureHandle_ = model_->textureHandle_;
	newBullet->Initialize(bulletModel,position, velocity, enemyManager_, mapChipField_, player);

 	bullets_.push_back(newBullet);
}