#include "GameScene.h"
#include "ImGuiManager.h"
#include "CameraSystem.h"
#include "Transition/TransitionHub.h"
#include "stageeditor/Stageeditor.h"
#include "InputManager.h"
#include "Easing.h"
#include"Clear/ClearScene.h"
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

//矩形の当たり判定
static inline bool Intersects(const AABB& a, const RectF& r) {
	return (a.min.x <= r.x1 && a.max.x >= r.x0) &&
		(a.min.y <= r.y1 && a.max.y >= r.y0);
}

void GameScene::Initialize() {
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

	playerModel_ = std::make_unique<ModelObject>();
	playerModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	blockModel_ = std::make_unique<ModelObject>();
	blockModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	blockModel_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/stageBlock.jpg");
	bulletModel_ = std::make_unique<ModelObject>();
	bulletModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	bulletModel_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/GridLine.png");
	enemyModel_ = std::make_unique<ModelObject>();
	enemyModel_->Initialize(p_fngine_->GetD3D12System(), "enemy.obj", "resources/enemy");
	enemyModel_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/enemy/enemy.png");
	arrowModel_ = std::make_unique<ModelObject>();
	arrowModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	goalModel_ = std::make_unique<ModelObject>();
	goalModel_->Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	goalModel_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/GridLine.png");
	mapChipField_ = new MapChipField;
	// ステージ番号に応じたCSVファイルを読み込む
	int stageIndex = 0; // 今回はステージ1固定
	std::string filename = "Resources/stage/stage" + std::to_string(stageIndex + 1) + ".csv";
	mapChipField_->LoadMapChipCsv(filename.c_str());
	

	enemyManager_ = new EnemyManager();
	enemyManager_->Initialize(enemyModel_.get());
	enemyManager_->SetFngine(p_fngine_);
	

	GenerateBlocks();

	bulletManager_ = new BulletManager();
	bulletManager_->Initialize(bulletModel_.get(), enemyManager_, mapChipField_);
	bulletManager_->SetFngine(p_fngine_);

	// 自キャラ生成
	player_ = std::make_unique<Player>();
	// 座標をマップチップ番号で指定
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(2, 126);
	// 自キャラの初期化
	player_->Initialize(playerModel_.get(), arrowModel_.get(), playerPosition, bulletManager_, p_fngine_);

	player_->SetMapChipField(mapChipField_);

	// どこかの初期化処理
	titleSprite_.Initialize(p_fngine_->GetD3D12System(), 800.0f, 400.0f);
	titleWorld_.Initialize();
	titleWorld_.set_.Scale({ 0.85f,0.85f,0.85f });
	titleWorld_.set_.Translation({ 640.0f - 400.0f * titleWorld_.get_.Scale().x,100.0f,0.0f});
	titleTextureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/Title/title.png");

	pressSpaceSprite_.Initialize(p_fngine_->GetD3D12System(), 800.0f, 400.0f);
	pressSpaceWorld_.Initialize();
	pressSpaceWorld_.set_.Scale({ 0.575f,0.575f,0.575f });
	pressSpaceWorld_.set_.Translation({ 640.0f - 400.0f * pressSpaceWorld_.get_.Scale().x,150.0f + 300.0f,0.0f });
	pressSpaceTextureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/Title/press_space.png");

	// Particle　テンプレート
	modelEmitter.SetTexture(TextureManager::GetInstance()->LoadTexture("resources/GridLine.png"));
	modelEmitter.SetModelData(blockModel_->GetModelData());
	modelEmitter.SetEmitter(playerPosition);
	modelEmitter.SetDirection({ 0.0f, 1.0f, 0.0f }); // 真上
	modelEmitter.SetSpeed(0.28f);
	modelEmitter.SetParticleLife(140);
	modelEmitter.SetSpawnCount(1);
	modelEmitter.SetSpawnInterval(30);
	modelEmitter.SetColor({ 0.5f,0.5f,0.5f });
	modelEmitter.SetStartAlpha(1.0f);
	modelEmitter.SetEndAlpha(0.0f);
	modelEmitter.SetStartScale({ 1.0f,1.0f,1.0f });
	modelEmitter.SetEndScale({ -0.1f,-0.1f,-0.1f });
	modelEmitter.SetSpawnArea({ {-15.5f,0.0f,-10.5f}, {15.5f,0.0f,10.5f} });
	modelEmitter.SetStartRotation({ 0.0f,0.0f,0.0f });
	modelEmitter.SetEndRotation({ Deg2Rad(360.0f),Deg2Rad(360.0f),Deg2Rad(360.0f) });
	modelEmitter.SetFngine(p_fngine_);

	TRSprite_.Initialize(p_fngine_->GetD3D12System(), 4.0f, 8.0f);
	TRWorld_.Initialize();
	TRWorld_.set_.Rotation({ Deg2Rad(0),Deg2Rad(180) ,Deg2Rad(180) });
	TRWorld_.set_.Scale({ 1.5f,1.5f,1.5f });
	uvTR_.Initialize();
	uvTR_.set_.Scale({ 0.2f, 1.0f, 1.0f });
	TRWorld_.set_.Translation(mapChipField_->GetMapChipPositionByIndex(15, 120));
	TRTextureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/TutorialSprite.png");

	backgroundModel_.Initialize(p_fngine_->GetD3D12System(), "cube.obj", "resources/cube");
	backgroundModel_.SetFngine(p_fngine_);
	backgroundModel_.worldTransform_.set_.Scale({ 250.0f,250.0f,0.0f });
	backgroundModel_.worldTransform_.set_.Translation({ -60.0f,50.0f,100.0f });
	backgroundModel_.SetColor({ 0.529f,0.808f,0.902f,1.0f });
	backgroundModel_.SetLightEnable(false);
	backgroundModel_.textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/ulthimaSky.png");

	

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

		TRTimer_ += 1.0f / 7.5f;
		if (TRTimer_ <= 3.0f) {
			// 最初がOnGround
			if (player_->IsOnG()) {
				uvTR_.set_.Translation({ 0.0f,0.0f,0.0f });
			}
			// 次がJump
			else if (player_->IsCanShot()) {
				uvTR_.set_.Translation({ 0.4f,0.0f,0.0f });
			}
		}
		else if (TRTimer_ <= 6.0f) {
			// 最初がOnGround
			if (player_->IsOnG()) {
				uvTR_.set_.Translation({ 0.2f,0.0f,0.0f });
			}
			// 次がJump
			else if (player_->IsCanShot()) {
				uvTR_.set_.Translation({ 0.6f,0.0f,0.0f });
			}
		}
		else if (TRTimer_ <= 9.0f) {
			// 最初がOnGround
			if (player_->IsOnG()) {
				uvTR_.set_.Translation({ 0.4f,0.0f,0.0f });
			}
			// 次がJump
			else if (player_->IsCanShot()) {
				uvTR_.set_.Translation({ 0.8f,0.0f,0.0f });
			}
		}
		else {
			TRTimer_ = 0.0f;
		}
	}

	Vector3 pos = CameraSystem::GetInstance()->GetActiveCamera()->GetTranslation();
	pos.y -= 12.0f;
	pos.z = playerModel_->worldTransform_.get_.Translation().z + 10.0f;
	modelEmitter.SetEmitter(pos);
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
					GameSceneSe_.SoundPlayWave(MediaAudioDecoder::DecodeAudioFile(L"resources/dead.mp3"), false);
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

	// ① y座標でゴール判定を無効化（ここだけで制御）
	if (playerModel_->worldTransform_.get_.Translation().y <= 248.0f) {
		return; // まだ判定しない
	}

	// ② ゴール3つとの交差だけを見る（最大3回）
	AABB pa = player_->GetAABB();
	for (int i = 0; i < goalCount_; ++i) {
		if (Intersects(pa, goalWorldRects_[i])) {
			Transition::FadeToWith(
				Transition::TransitionType::FadeSlideUp,
				[]() { return new ClearScene(); },
				0.6f, 1.0f
			);
			return;
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

			// ここからタイトルのボタンを推す前の処理
			if (titleTimer_ <= titleLoopTime_ / 2.0f) {
				// タイトルのイージング処理
				Easing(titleWorld_.get_.Translation(), { 640.0f - 400.0f * titleWorld_.get_.Scale().x,100.0f,0.0f },
					{ 640.0f - 400.0f * titleWorld_.get_.Scale().x,100.0f + 15.0f,0.0f }, titleTimer_, titleLoopTime_ / 2.0f, EASINGTYPE::OutSine);
				// PressSpaceのイージング処理
				Easing(pressSpaceWorld_.get_.Translation(), { 640.0f - 400.0f * pressSpaceWorld_.get_.Scale().x,150.0f + 300.0f,0.0f },
					{ 640.0f - 400.0f * pressSpaceWorld_.get_.Scale().x,150.0f + 300.0f + 15.0f,0.0f }, titleTimer_, titleLoopTime_ / 2.0f, EASINGTYPE::OutSine);
			}
			else {
				// タイトルのイージング処理
				Easing(titleWorld_.get_.Translation(), { 640.0f - 400.0f * titleWorld_.get_.Scale().x,100.0f + 15.0f,0.0f },
					{ 640.0f - 400.0f * titleWorld_.get_.Scale().x,100.0f,0.0f }, titleTimer_, titleLoopTime_, EASINGTYPE::InSine);
				// PressSpaceのイージング処理
				Easing(pressSpaceWorld_.get_.Translation(), { 640.0f - 400.0f * pressSpaceWorld_.get_.Scale().x,150.0f + 300.0f + 15.0f,0.0f },
					{ 640.0f - 400.0f * pressSpaceWorld_.get_.Scale().x,150.0f + 300.0f,0.0f }, titleTimer_, titleLoopTime_, EASINGTYPE::InSine);
			}

			// ここまでタイトルの処理

			if (titleTimer_ >= titleLoopTime_) {
				//　時間がタイトルのループタイムを超えたら初期値に戻す
				titleTimer_ = 0.0f;
			}
			if (InputManager::GetJump()) {
				// 最初にタイトルのフラグを立てる(遷移が始まる)
				isTitleFirst_ = true;
				titleToGameFadeTimer_ = 0.0f;
				GameSceneSe_.SoundPlayWave(MediaAudioDecoder::DecodeAudioFile(L"resources/start.mp3"), false);
			}
			ImGuiManager::GetInstance()->DrawDrag("titleTimer", titleTimer_);
			ImGuiManager::GetInstance()->DrawDrag("titleLoopTimer", titleLoopTime_);
		}
		else if (isTitleFirst_) {
			// 最初のフラグが立っていれば
			titleToGameFadeTimer_ += 1.0f / 60.0f; // 仮に60FPSとして時間を進める

			// ここから時間によるFadeやイージング処理を書く
			// カメラの半径をイージングで変化
			titleCameraRadius_ = Easing_Float(30.0f, 50.0f, titleToGameFadeTimer_, titleToGameFadeDuration_, EASINGTYPE::InSine);

			// タイトルたちのαを薄くする
			float titleAlpha = Easing_Float(1.0f, 0.0f, titleToGameFadeTimer_, titleToGameFadeDuration_, EASINGTYPE::OutSine);
			titleSprite_.SetColor({ 1.0f,1.0f,1.0f,titleAlpha });
			pressSpaceSprite_.SetColor({ 1.0f,1.0f,1.0f,titleAlpha });

			// ここまで時間によるFadeやイージング処理を書く

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

	backgroundModel_.LocalToWorld();
	backgroundModel_.SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(backgroundModel_.worldTransform_.mat_));
	backgroundModel_.Draw();

	// 自キャラの描画
	player_->Draw();

	bulletManager_->Draw();
	enemyManager_->Draw();

	if (!isGameStart_) {
		titleWorld_.LocalToWorld();
		titleSprite_.SetWVPData(
			CameraSystem::GetInstance()->GetActiveCamera()->DrawUI(titleWorld_.mat_),
			titleWorld_.mat_,
			Matrix4x4::Make::Identity()
		);
		titleSprite_.Draw(p_fngine_->GetCommand(),p_fngine_->GetPSO(),p_fngine_->GetLight(),TextureManager::GetInstance()->GetTexture(titleTextureHandle_));

		pressSpaceWorld_.LocalToWorld();
		pressSpaceSprite_.SetWVPData(
			CameraSystem::GetInstance()->GetActiveCamera()->DrawUI(pressSpaceWorld_.mat_),
			pressSpaceWorld_.mat_,
			Matrix4x4::Make::Identity()
		);
		pressSpaceSprite_.Draw(p_fngine_->GetCommand(), p_fngine_->GetPSO(), p_fngine_->GetLight(), TextureManager::GetInstance()->GetTexture(pressSpaceTextureHandle_));
	}

	TRWorld_.LocalToWorld();
	uvTR_.LocalToWorld();
	TRSprite_.SetWVPData(
		CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(TRWorld_.mat_),
		TRWorld_.mat_,
		uvTR_.mat_
	);
	TRSprite_.Draw(p_fngine_->GetCommand(), p_fngine_->GetPSO(), p_fngine_->GetLight(), TextureManager::GetInstance()->GetTexture(TRTextureHandle_));


	// 自キャラの描画
	player_->Draw();
	p_fngine_->GetPSO().SetBlendState(BLENDMODE::Additive);
	modelEmitter.Draw();
	p_fngine_->GetPSO().SetBlendState(BLENDMODE::AlphaBlend);
	
}

void GameScene::GenerateBlocks() {
	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVertical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	//ゴール収集の初期化
	goalCount_ = 0;


	blocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; i++) {
		blocks_[i].resize(numBlockHorizontal);
	}

	// ブロックの生成
	for (uint32_t i = 0; i < numBlockVirtical; i++) {
		for (uint32_t j = 0; j < numBlockHorizontal; j++) {
			// MapChipField からマップチップのタイプを取得
			MapChipField::MapChipType mapChipType = mapChipField_->GetMapChipTypeByIndex(j, i);

			// ★ゴールのワールド矩形を記録（最大3）
			if (mapChipType == MapChipField::MapChipType::kGoal && goalCount_ < 3) {
				float gx0 = j * kBlockWidth;
				float gy0 = kBlockHeight * (numBlockVirtical - 1 - i);
				goalWorldRects_[goalCount_++] = RectF{ gx0, gy0, gx0 + kBlockWidth, gy0 + kBlockHeight };
				// 見た目で置きたいなら下の“ブロック生成”もそのまま通す/通さないはお好みで
				// 見た目も出す場合（専用モデル）
				blocks_[i][j] = new ModelObject();
				blocks_[i][j]->Initialize(p_fngine_->GetD3D12System(), goalModel_->GetModelData());
				blocks_[i][j]->SetFngine(p_fngine_);
				blocks_[i][j]->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f }); // 半透明にしてみる
				blocks_[i][j]->textureHandle_ = goalModel_->textureHandle_;
				blocks_[i][j]->worldTransform_.get_.Translation().x = gx0;
				blocks_[i][j]->worldTransform_.get_.Translation().y = gy0;

				continue;
			}


			if (mapChipType == MapChipField::MapChipType::kTrap) {
				// === 敵スポーン ===
				Vector3 pos;
				pos.x = kBlockWidth * j;
				pos.y = kBlockHeight * (numBlockVirtical - 1 - i);
				pos.z = 0.0f;
				
				enemyManager_->SpawnEnemy(pos);

				// ブロックは置かない（当たり判定衝突を避ける）
				blocks_[i][j] = nullptr;

				//// 以降の地形系は空白として扱えるよう 0 に正規化（任意）
				//mapChipField_->SetTileValueByIndex(j, i, 0); // ←※あれば
				continue;
			}

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