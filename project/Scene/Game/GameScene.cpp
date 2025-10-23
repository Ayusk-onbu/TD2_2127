#include "GameScene.h"
#include "BulletManager.h"


#include"Matrix4x4.h"

int GameScene::selectedStageIndex_ = 0;

// デストラクタ
GameScene::~GameScene() {

	delete player_;
	delete blockModel_;
	for (auto& row : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : row) {
			delete worldTransformBlock; // 各ワールド変換を解放
		}
		row.clear(); // 行をクリア
	}
	worldTransformBlocks_.clear();
	//delete debugCamera_;
	delete mapChipField_;
	delete cameraController_;
	delete bulletManager_;
	delete bulletModel_;
	delete enemyManager_;
	delete enemyModel_;
	delete arrowModel_;
}

void GameScene::Initialize(int stageIndex) {

	playerModel_ = Model::CreateFromOBJ("player", true);
	blockModel_ = Model::Create();
	bulletModel_ = Model::CreateFromOBJ("cube", true);
	enemyModel_ = Model::CreateFromOBJ("enemy", true);
	arrowModel_ = Model::CreateFromOBJ("player", true);
	camera_.Initialize();
	mapChipField_ = new MapChipField;
	// ステージ番号に応じたCSVファイルを読み込む
	std::string filename = "Resources/stage/stage" + std::to_string(stageIndex + 1) + ".csv";
	mapChipField_->LoadMapChipCsv(filename.c_str());
	GenerateBlocks();

	enemyManager_ = new EnemyManager();
	enemyManager_->Initialize(enemyModel_, &camera_);

	enemyManager_->SpawnEnemy({ 30.0f, 10.0f, 0.0f });

	bulletManager_ = new BulletManager();
	bulletManager_->Initialize(bulletModel_, &camera_, enemyManager_, mapChipField_);

	// 自キャラ生成
	player_ = new Player();
	// 座標をマップチップ番号で指定
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(4, 18);
	// 自キャラの初期化
	player_->Initialize(playerModel_, arrowModel_, &camera_, playerPosition, bulletManager_);

	player_->SetMapChipField(mapChipField_);

	// カメラコントローラ
	// 生成
	//cameraController_ = new CameraController();
	//// カメラをセット（初期化前にセット）
	//cameraController_->SetCamera(&camera_);
	//// 初期化
	//cameraController_->Initialize();
	//// 対象をセット (player_が生成された後なので安全)
	//cameraController_->SetTarget(player_);
	//// リセット(瞬間合わせ)
	//cameraController_->Reset();

	//// デバッグカメラの生成
	//debugCamera_ = new DebugCamera(1280, 720);
}

Scene GameScene::Update() {
	// 自キャラの更新
	player_->Update();
	bulletManager_->Update();
	enemyManager_->Update();

	if (!player_->IsDead()) {
		AABB playerAABB = player_->GetAABB();
		const std::list<Enemy*>& enemies = enemyManager_->GetEnemies();

		for (Enemy* enemy : enemies) {
			if (enemy->IsDead()) {
				continue;
			}

			AABB enemyAABB = enemy->GetAABB();


			if ((playerAABB.min.x <= enemyAABB.max.x && playerAABB.max.x >= enemyAABB.min.x) && (playerAABB.min.y <= enemyAABB.max.y && playerAABB.max.y >= enemyAABB.min.y)) {


				if (player_->GetVelocity().y < 0 && playerAABB.min.y > enemyAABB.min.y) {
					// 踏みつけ成功
					enemy->OnCollision();    // 敵を倒す
					player_->OnEnemyStomp(); // プレイヤーに踏んだことを通知
				} else {

					player_->OnCollision();
				}

				break; // 一体処理したらループを抜ける
			}
		}
	}

	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}

			worldTransformBlock->LocalToWorld();
		}
	}

	//// カメラコントローラーの更新
	//cameraController_->Update();
	//camera_.UpdateMatrix();
	//camera_.TransferMatrix();

	return Scene::kGame;
}

void GameScene::Draw() {

	// 自キャラの描画
	player_->Draw();

	// ブロックの描画
	for (uint32_t i = 0; i < worldTransformBlocks_.size(); ++i) {
		for (uint32_t j = 0; j < worldTransformBlocks_[i].size(); ++j) {
			WorldTransform* worldTransformBlock = worldTransformBlocks_[i][j];
			if (!worldTransformBlock) {
				continue;
			}

			blockModel_->Draw(*worldTransformBlock, camera_);
		}
	}
	bulletManager_->Draw();
	enemyManager_->Draw();
}

void GameScene::GenerateBlocks() {
	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVertical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; i++) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	// ブロックの生成
	for (uint32_t i = 0; i < numBlockVirtical; i++) {
		for (uint32_t j = 0; j < numBlockHorizontal; j++) {
			// MapChipField からマップチップのタイプを取得
			MapChipField::MapChipType mapChipType = mapChipField_->GetMapChipTypeByIndex(j, i);

			if (mapChipType != MapChipField::MapChipType::kBlank) {
				worldTransformBlocks_[i][j] = new WorldTransform();
				worldTransformBlocks_[i][j]->Initialize();

				Vector3 tempTranslation = worldTransformBlocks_[i][j]->get_.Translation();
				tempTranslation.x = kBlockWidth * j;
				tempTranslation.y = kBlockHeight * (numBlockVirtical - 1 - i);
				worldTransformBlocks_[i][j]->set_.Translation(tempTranslation);
			} else {
				// ブロックがない場合は nullptr を設定
				worldTransformBlocks_[i][j] = nullptr;
			}
		}
	}
}