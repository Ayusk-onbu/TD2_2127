#include "CameraController.h"
#include "ImGuiManager.h"
#include "Camera.h"
#include <algorithm>

void GameCameraController::Update(Camera& camera) {

	/*今回のカメラ
	**ターゲットを追跡する
	**画面外には出ないようにする
	**少し遅れて追従する
	*/
	



	float theta = 0;camera.GetTheta() = std::clamp(camera.GetTheta(), -89.0f, 89.0f); // X軸回転は-90度から90度まで
	theta = Deg2Rad(camera.GetTheta());
	float phi = 0;phi = Deg2Rad(camera.GetPhi());
	// ここから変換
	
	camera.targetPos_.y += cameraPlusPosY_;
	// カメラの移動制限
	camera.targetPos_.x = std::clamp(camera.targetPos_.x, 20.0f, 20.0f); // 左右の制限
	//　下の処理だと酔いやすそうなのでLerpなどで追うような形にする
	camera.targetPos_.y = std::clamp(camera.targetPos_.y, 10.0f, 100.0f); // 地面より下に行かないようにする

	camera.GetTranslation().x = camera.targetPos_.x + camera.GetRadius() * std::cos(theta) * std::cos(phi);
	camera.GetTranslation().y = (camera.targetPos_.y) + camera.GetRadius() * std::sin(theta);
	camera.GetTranslation().z = camera.targetPos_.z + camera.GetRadius() * std::cos(theta) * std::sin(phi);

	

	camera.GetViewProjectionMatrix() = (Matrix4x4::Make::LookAt(camera.GetTranslation(), camera.targetPos_, camera.up_, camera.xAxis_, camera.yAxis_, camera.zAxis_));

	camera.GetViewProjectionMatrix() = Matrix4x4::Multiply(camera.GetViewProjectionMatrix(), Matrix4x4::Make::PerspectiveFov(
		camera.GetProjection().fovY, camera.GetProjection().aspectRatio,
		camera.GetProjection().nearClip, camera.GetProjection().farClip));

	ImGuiManager::GetInstance()->Text("GameCamera");
	ImGuiManager::GetInstance()->DrawDrag("cameraPlusPosY", cameraPlusPosY_);
}