#include "GameScene.h"
#include "ImGuiManager.h"
#include "CameraSystem.h"
#include "Transition/TransitionHub.h"
#include "stageeditor/Stageeditor.h"
#include "InputManager.h"
#include "Easing.h"

GameScene::~GameScene() {
	for (auto& row : blocks_) {
		for (auto* worldTransformBlock : row) {
			delete worldTransformBlock; // 各ワールド変換を解放
		}
		row.clear(); // 行をクリア
	}
	delete mapChipField_;
	delete bulletManager_;
	delete enemyManager_;
	Log::ViewFile("Path GameScene ~");
}

void GameScene::Initialize() {
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

	playerModel_ = std::make_unique<ModelObject>();
	playerModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	blockModel_ = std::make_unique<ModelObject>();
	blockModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	blockModel_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/cube/cube.jpg");
	bulletModel_ = std::make_unique<ModelObject>();
	bulletModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	bulletModel_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/cube/cube.jpg");
	enemyModel_ = std::make_unique<ModelObject>();
	enemyModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	enemyModel_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/GridLine.png");
	arrowModel_ = std::make_unique<ModelObject>();
	arrowModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	mapChipField_ = new MapChipField;
	// ステージ番号に応じたCSVファイルを読み込む
	int stageIndex = 0; // 今回はステージ1固定
	std::string filename = "Resources/stage/stage" + std::to_string(stageIndex + 1) + ".csv";
	mapChipField_->LoadMapChipCsv(filename.c_str());
	GenerateBlocks();

	enemyManager_ = new EnemyManager();
	enemyManager_->Initialize(enemyModel_.get());
	enemyManager_->SetFngine(p_fngine_);
	enemyManager_->SpawnEnemy({ 30.0f, 10.0f, 0.0f });

	bulletManager_ = new BulletManager();
	bulletManager_->Initialize(bulletModel_.get(), enemyManager_, mapChipField_);
	bulletManager_->SetFngine(p_fngine_);

	// 自キャラ生成
	player_ = std::make_unique<Player>();
	// 座標をマップチップ番号で指定
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(4, 126);
	// 自キャラの初期化
	player_->Initialize(playerModel_.get(), arrowModel_.get(), playerPosition, bulletManager_, p_fngine_);

	player_->SetMapChipField(mapChipField_);

	// どこかの初期化処理


	// Particle　テンプレート
	modelEmitter.SetTexture(TextureManager::GetInstance()->LoadTexture("resources/cube/cube.jpg"));
	modelEmitter.SetModelData(blockModel_->GetModelData());
	modelEmitter.SetEmitter(playerPosition);
	modelEmitter.SetDirection({ 0.0f, 1.0f, 0.0f }); // 真上
	modelEmitter.SetSpeed(0.4f);
	modelEmitter.SetParticleLife(140);
	modelEmitter.SetSpawnCount(1);
	modelEmitter.SetSpawnInterval(10);
	modelEmitter.SetStartAlpha(1.0f);
	modelEmitter.SetEndAlpha(0.0f);
	modelEmitter.SetStartScale({ 1.0f,1.0f,1.0f });
	modelEmitter.SetEndScale({ 0.1f,0.1f,0.1f });
	modelEmitter.SetSpawnArea({ {-10.5f,0.0f,-10.5f}, {10.5f,0.0f,10.5f} });
	modelEmitter.SetStartRotation({ 0.0f,0.0f,0.0f });
	modelEmitter.SetEndRotation({ Deg2Rad(360.0f),Deg2Rad(360.0f),Deg2Rad(360.0f) });
	modelEmitter.SetFngine(p_fngine_);

	p_fngine_->GetMusic().GetBGM().SoundPlayWave(MediaAudioDecoder::DecodeAudioFile(L"resources/maou_bgm_fantasy02.mp3"));
	p_fngine_->GetMusic().GetBGM().SetPlayAudioBuf();

	// カメラコントローラ(なんか追加しないとか)
	CameraSystem::GetInstance()->MakeCamera("DebugCamera", CameraType::Debug);
	CameraSystem::GetInstance()->MakeCamera("GameCamera", CameraType::Game);
	CameraSystem::GetInstance()->SetActiveCamera("GameCamera");

	CameraSystem::GetInstance()->GetActiveCamera()->GetRadius() = titleCameraRadius_;
	GameUpdate();
}

void GameScene::Update() {
	CameraSystem::GetInstance()->Update();

	TitleUpdate();
	if (isGameStart_ == true) {
		GameUpdate();
	}

	modelEmitter.Update();
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
	else if (ImGui::Button("DebugmodeAdd")) {
		CameraSystem::GetInstance()->GetActiveCamera()->AddControllers(CameraType::Debug);
	}
	ImGui::End();
#endif // _DEBUG
}

void GameScene::GameUpdate() {
	// ゲームの更新処理をここに移動
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
					//enemy->OnCollision();    // 敵を倒す 死ぬと色々めんどい
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

}

void GameScene::TitleUpdate() {
	CameraSystem::GetInstance()->GetActiveCamera()->targetPos_ = playerModel_->worldTransform_.get_.Translation();
	// タイトル画面の更新処理
	if (isGameStart_ == false) {
		ImGuiManager::GetInstance()->Text("Title Scene Update");
		// ゲームを開始していない状態
		if (isTitleFirst_ == false) {
			titleTimer_ += 1.0f / 60.0f; // 仮に60FPSとして時間を進める	
			if (titleTimer_ >= titleLoopTime_) {
				//　時間がタイトルのループタイムを超えたら初期値に戻す
				titleTimer_ = 0.0f;
			}
			if (InputManager::GetJump()) {
				// 最初にタイトルのフラグを立てる(遷移が始まる)
				isTitleFirst_ = true;
				titleToGameFadeTimer_ = 0.0f;
			}
			ImGuiManager::GetInstance()->DrawDrag("titleTimer", titleTimer_);
			ImGuiManager::GetInstance()->DrawDrag("titleLoopTimer", titleLoopTime_);
		}
		else if (isTitleFirst_) {
			// 最初のフラグが立っていれば
			titleToGameFadeTimer_ += 1.0f / 60.0f; // 仮に60FPSとして時間を進める

			// ここに時間によるFadeやイージング処理を書く
			// カメラの半径をイージングで変化
			titleCameraRadius_ = Easing_Float(30.0f, 50.0f, titleToGameFadeTimer_, titleToGameFadeDuration_, EASINGTYPE::InSine);

			if (titleToGameFadeTimer_ >= titleToGameFadeDuration_) {
				isGameStart_ = true;
			}
			ImGuiManager::GetInstance()->DrawDrag("titleToFadeTimer", titleToGameFadeTimer_);
			ImGuiManager::GetInstance()->DrawDrag("titleToGameFadeDuration", titleToGameFadeDuration_);
		}
		// 半径を代入
		CameraSystem::GetInstance()->GetActiveCamera()->GetRadius() = titleCameraRadius_;
		ImGuiManager::GetInstance()->DrawDrag("titleCameraRadius", titleCameraRadius_);
	}
}

void GameScene::Draw() {
	// 自キャラの描画
	player_->Draw();
	
	modelEmitter.Draw();

	// ブロックの描画
	for (uint32_t i = 0; i < blocks_.size(); ++i) {
		for (uint32_t j = 0; j < blocks_[i].size(); ++j) {
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
				blocks_[i][j]->Initialize(p_fngine_->GetD3D12System(), blockModel_->GetModelData());
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