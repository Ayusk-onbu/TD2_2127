#include "TransitionHub.h"
#include "SceneDirector.h"
#include "Scene.h"
#include "SpriteObject.h"
#include "Matrix4x4.h"
#include "Vector4.h"
#include "TextureManager.h"
#include "Fngine.h"
#include "CameraSystem.h"

namespace Transition {

	struct Hub::Impl {
		Fngine* eng = nullptr;
		SceneDirector* dir = nullptr;
		int sw = 1280, sh = 720;
		bool initialized = false;

		SpriteObject sprite;
		int texId = -1;

		enum class State { Idle, FadeOut, FadeIn } state = State::Idle;
		float t = 0.f, dt = 1.f / 60.f, alpha = 0.f, outSec = 0.5f, inSec = 0.5f;

		std::function<Scene* ()> pendingFactory = nullptr;

		TransitionType type = TransitionType::Fade;

		static float Clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
		static float Max(float a, float b) { return a > b ? a : b; }

		// --- 置き換え: X/Y両対応のオフセット計算 ---
		void ComputeSlideOffset(float progress01, bool entering, float& outX, float& outY) const {
			outX = 0.0f; outY = 0.0f;
			if (type == TransitionType::Fade) return;

			const float W = float(sw);
			const float H = float(sh);

			switch (type) {
			case TransitionType::FadeSlideLeft:
				// +W → 0 → -W （左へ流れる）
				if (entering) outX = +W * (1.0f - progress01);
				else          outX = -W * (progress01);
				break;

			case TransitionType::FadeSlideRight:
				// -W → 0 → +W （右へ流れる）
				if (entering) outX = -W * (1.0f - progress01);
				else          outX = +W * (progress01);
				break;

			case TransitionType::FadeSlideUp:
				// +Yが下の前提：+H（下）→ 0 → -H（上）= 上へ流れる
				if (entering) outY = +H * (1.0f - progress01); // 下から入る
				else          outY = -H * (progress01);        // 上へ抜ける
				break;

			case TransitionType::FadeSlideDown:
				// -H（上）→ 0 → +H（下）= 下へ流れる
				if (entering) outY = -H * (1.0f - progress01); // 上から入る
				else          outY = +H * (progress01);        // 下へ抜ける
				break;

			default:
				break;
			}
		}


		static Matrix4x4 MakeTranslate(float tx, float ty) {
			Matrix4x4 M{};
			M.m[0][0] = M.m[1][1] = M.m[2][2] = M.m[3][3] = 1.0f;
			M.m[3][0] = tx; M.m[3][1] = ty;
			return M;
		}
	};

	Hub& Hub::I() { static Hub s; return s; }
	Hub::Hub() :impl_(new Impl) {}
	Hub::~Hub() {}

	void Hub::Setup(Fngine* eng, SceneDirector* dir, int screenW, int screenH) {
		impl_->eng = eng; impl_->dir = dir; impl_->sw = screenW; impl_->sh = screenH;
		if (!impl_->initialized) {
			impl_->sprite.Initialize(impl_->eng->GetD3D12System(), float(impl_->sw), float(impl_->sh));

			// 単位行列の取得はあなたの環境のAPI名に合わせて
			Matrix4x4 I{};
			I.m[0][0] = I.m[1][1] = I.m[2][2] = I.m[3][3] = 1.0f;
			impl_->sprite.SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawUI(I), I, I);

			impl_->sprite.SetColor(Vector4(0, 0, 0, 0));
			if (impl_->texId < 0) {
				impl_->texId = TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
			}
			impl_->initialized = true;
		}
	}

	void Hub::TickAndDraw() {
		if (impl_->state == Impl::State::Idle || !impl_->eng) return;

		if (impl_->state == Impl::State::FadeOut) {
			impl_->t += impl_->dt; impl_->alpha = Impl::Impl::Clamp01(impl_->t / impl_->outSec);
			if (impl_->alpha >= 1.f) {
				if (impl_->pendingFactory) {
					impl_->dir->EnqueueImmediateSwitch(impl_->pendingFactory());
					impl_->pendingFactory = nullptr;
				}
				impl_->t = 0.f; impl_->state = Impl::State::FadeIn;
			}
		}
		else if (impl_->state == Impl::State::FadeIn) {
			impl_->t += impl_->dt; impl_->alpha = 1.f - Impl::Clamp01(impl_->t / impl_->inSec);
			if (impl_->alpha <= 0.f) { impl_->alpha = 0.f; impl_->state = Impl::State::Idle; }
		}

		// スライド進捗 0→1（既存ロジックのまま）
		const bool entering = (impl_->state == Impl::State::FadeOut);
		float p = entering ? impl_->alpha : (1.0f - impl_->alpha);

		// ←変更: XとYを両方計算
		float offsetX = 0.0f, offsetY = 0.0f;
		impl_->ComputeSlideOffset(p, entering, offsetX, offsetY);

		// ←変更: Yも平行移動に反映
		Matrix4x4 T = Hub::Impl::MakeTranslate(offsetX, offsetY);
		Matrix4x4 WVP = CameraSystem::GetInstance()->GetActiveCamera()->DrawUI(T);
		impl_->sprite.SetWVPData(WVP, T, T);

		// フェード量（黒カーテンの不透明度）
		impl_->sprite.SetColor(Vector4(0, 0, 0, impl_->alpha));

		auto& cmd = impl_->eng->GetCommand();
		auto& pso = impl_->eng->GetPSO();
		auto& light = impl_->eng->GetLight();
		auto& tex = TextureManager::GetInstance()->GetTexture(impl_->texId);
		pso.SetBlendState(BLENDMODE::AlphaBlend);
		impl_->sprite.Draw(cmd, pso, light, tex);
	}

	void Hub::SetDelta(float dt) { impl_->dt = dt; }

	//==================================
	// fade
	//==================================
	void Hub::FadeTo(std::function<Scene* ()> factory, float outSec, float inSec) {
		if (impl_->state != Impl::State::Idle) return;
		impl_->pendingFactory = std::move(factory);
		impl_->outSec = Impl::Max(0.0001f, outSec);
		impl_->inSec = Impl::Max(0.0001f, inSec);
		impl_->t = 0.f; impl_->alpha = 0.f;
		impl_->state = Impl::State::FadeOut;
	}

	void Hub::SetTransitionType(TransitionType type) { impl_->type = type; }

	void FadeToWith(TransitionType type,
		std::function<Scene* ()> factory,
		float outSec, float inSec)
	{
		Hub::I().SetTransitionType(type);
		Hub::I().FadeTo(std::move(factory), outSec, inSec);
	}

	void FadeToFade(std::function<Scene* ()> f, float o, float i) {
		FadeToWith(TransitionType::Fade, std::move(f), o, i);
	}
	void FadeToSlideLeft(std::function<Scene* ()> f, float o, float i) {
		FadeToWith(TransitionType::FadeSlideLeft, std::move(f), o, i);
	}
	void FadeToSlideRight(std::function<Scene* ()> f, float o, float i) {
		FadeToWith(TransitionType::FadeSlideRight, std::move(f), o, i);
	}
	void FadeToSlideUp(std::function<Scene* ()> f, float o, float i) {
		FadeToWith(TransitionType::FadeSlideUp, std::move(f), o, i);
	}
	void FadeToSlideDown(std::function<Scene* ()> f, float o, float i) {
		FadeToWith(TransitionType::FadeSlideDown, std::move(f), o, i);
	}


	void Hub::Shutdown() {
		if (!impl_) return;
		// TextureManager を参照カウント型にしているならここでデクリメント/アンロード
		// 例: if (impl_->texId >= 0) TextureManager::GetInstance()->Unload(impl_->texId);

		// SpriteObject 側に Finalize/Release があるなら呼ぶ
		// 例: impl_->sprite.Finalize();

		//impl_->initialized = false;
		delete impl_;

	}


	void Hub::FadeOverlay(float, float, float) { /* 省略：必要なら後で */ }


	// ---- 薄いラッパ ----
	void Setup(Fngine& e, SceneDirector& d, int w, int h) { Hub::I().Setup(&e, &d, w, h); }
	void TickAndDraw() { Hub::I().TickAndDraw(); }
	void SetDelta(float dt) { Hub::I().SetDelta(dt); }
	void FadeTo(std::function<Scene* ()> f, float o, float i) { Hub::I().FadeTo(std::move(f), o, i); }
	void FadeOverlay(float a, float b, float dur) { Hub::I().FadeOverlay(a, b, dur); }
	// 薄いラッパ
	void Shutdown() { Hub::I().Shutdown(); }
} // namespace Transition
