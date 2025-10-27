#include "EnemyManager.h"

EnemyManager::~EnemyManager() {
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
}

void EnemyManager::Initialize(ModelObject* model) {
	model_ = model;
}

void EnemyManager::Update() {
	// 敵をすべて更新
	for (Enemy* enemy : enemies_) {
		enemy->Update();
	}

	enemies_.remove_if([](Enemy* enemy) {
		if (enemy->IsDead()) {
			delete enemy;
			return true;
		}
		return false;
	});
}

void EnemyManager::Draw() {
	// 敵をすべて描画
	for (Enemy* enemy : enemies_) {
		enemy->Draw();
	}
}

void EnemyManager::SpawnEnemy(const Vector3& position) {
	// 敵を生成し、初期化してリストに追加
	Enemy* newEnemy = new Enemy();
	ModelObject* enemyModel = new ModelObject();
	enemyModel->Initialize(fngine_->GetD3D12System(), model_->GetModelData());
	enemyModel->SetFngine(fngine_);
	enemyModel->textureHandle_ = model_->textureHandle_;
	newEnemy->Initialize(enemyModel, position);
	enemies_.push_back(newEnemy);
}