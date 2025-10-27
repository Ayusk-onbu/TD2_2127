#include "GameScene.h"
#include "ImGuiManager.h"
#include "CameraSystem.h"
#include "Transition/TransitionHub.h"
#include "stageeditor/Stageeditor.h"
#include "InputManager.h"
GameScene::~GameScene() {
	
	Log::ViewFile("Path GameScene ~");
}

void GameScene::Initialize() {
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

	playerModel_ = new ModelObject;
	playerModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	blockModel_ = new ModelObject;
	blockModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	blockModel_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/cube/cube.jpg");
	bulletModel_ = new ModelObject;
	bulletModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	bulletModel_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/cube/cube.jpg");
	enemyModel_ = new ModelObject;
	enemyModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	enemyModel_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/cube/cube.jpg");
	arrowModel_ = new ModelObject;
	arrowModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	mapChipField_ = new MapChipField;
	// ステージ番号に応じたCSVファイルを読み込む
	int stageIndex = 0; // 今回はステージ1固定
	std::string filename = "Resources/stage/stage" + std::to_string(stageIndex + 1) + ".csv";
	mapChipField_->LoadMapChipCsv(filename.c_str());
	GenerateBlocks();

	enemyManager_ = new EnemyManager();
	enemyManager_->Initialize(enemyModel_);
	enemyManager_->SetFngine(p_fngine_);
	enemyManager_->SpawnEnemy({ 30.0f, 10.0f, 0.0f });

	bulletManager_ = new BulletManager();
	bulletManager_->Initialize(bulletModel_, enemyManager_, mapChipField_);
	bulletManager_->SetFngine(p_fngine_);

	// 自キャラ生成
	player_ = std::make_unique<Player>();
	// 座標をマップチップ番号で指定
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(4, 18);
	// 自キャラの初期化
	player_->Initialize(playerModel_, arrowModel_, playerPosition, bulletManager_,p_fngine_);

	player_->SetMapChipField(mapChipField_);

	// カメラコントローラ(なんか追加しないとか)
	CameraSystem::GetInstance()->MakeCamera("DebugCamera", CameraType::Debug);
	CameraSystem::GetInstance()->MakeCamera("GameCamera", CameraType::Game);
	CameraSystem::GetInstance()->SetActiveCamera("GameCamera");
}

void GameScene::Update(){
	CameraSystem::GetInstance()->Update();

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
				}
				else {

					player_->OnCollision();
				}

				break; // 一体処理したらループを抜ける
			}
		}
	}
	else {
		// リザルト画面に行こう

	}

	// ブロックの更新
	for (std::vector<ModelObject*>& worldTransformBlockLine : blocks_) {
		for (ModelObject* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}

			// アフィン変換の作成
			worldTransformBlock->LocalToWorld();
		}
	}


#ifdef _DEBUG
	auto& key = InputManager::GetKey();
	if (key.PressedKey(DIK_F2)) {
		Fngine* eng = /* ここはあなたの環境で取得する */ p_fngine_; // 例
		Transition::FadeToSlideRight([eng] {
			auto* ed = new StageEditor();
			ed->BindEngine(eng);       // ★ 必須：StageEditorにエンジンを渡す
			return ed;
			}, 0.8f, 0.8f);
	}

	ImGui::Begin("Camera");
	if (ImGui::Button("DebugMode")) {
		CameraSystem::GetInstance()->SetActiveCamera("DebugCamera");
	}
	else if (ImGui::Button("GameMode")) {
		CameraSystem::GetInstance()->SetActiveCamera("GameCamera");
	}
	ImGui::End();
#endif // _DEBUG
}

void GameScene::Draw() {
	// 自キャラの描画
	player_->Draw();

	// ブロックの描画
	for (uint32_t i = 0; i < blocks_.size(); ++i) {
		for (uint32_t j = 0; j <blocks_[i].size(); ++j) {
			ModelObject* worldTransformBlock = blocks_[i][j];
			if (!worldTransformBlock) {
				continue;
			}

			worldTransformBlock->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(worldTransformBlock->worldTransform_.mat_));
			worldTransformBlock->Draw();
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

	blocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; i++) {
		blocks_[i].resize(numBlockHorizontal);
	}

	// ブロックの生成
	for (uint32_t i = 0; i < numBlockVirtical; i++) {
		for (uint32_t j = 0; j < numBlockHorizontal; j++) {
			// MapChipField からマップチップのタイプを取得
			MapChipField::MapChipType mapChipType = mapChipField_->GetMapChipTypeByIndex(j, i);

			if (mapChipType != MapChipField::MapChipType::kBlank) {
				blocks_[i][j] = new ModelObject();
				blocks_[i][j]->Initialize(p_fngine_->GetD3D12System(),blockModel_->GetModelData());
				blocks_[i][j]->SetFngine(p_fngine_);
				blocks_[i][j]->textureHandle_ = blockModel_->textureHandle_;
				blocks_[i][j]->worldTransform_.get_.Translation().x = kBlockWidth * j;
				blocks_[i][j]->worldTransform_.get_.Translation().y = kBlockHeight * (numBlockVirtical - 1 - i);
			}
			else {
				// ブロックがない場合は nullptr を設定
				blocks_[i][j] = nullptr;
			}
		}
	}
}