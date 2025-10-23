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

		impl_->sprite.SetColor(Vector4(0, 0, 0, impl_->alpha));
		auto& cmd = impl_->eng->GetCommand();
		auto& pso = impl_->eng->GetPSO();
		auto& light = impl_->eng->GetLight();
		auto& tex = TextureManager::GetInstance()->GetTexture(impl_->texId);
		pso.SetBlendState(BLENDMODE::AlphaBlend);
		impl_->sprite.Draw(cmd, pso, light, tex);
	}

	void Hub::SetDelta(float dt) { impl_->dt = dt; }

	void Hub::FadeTo(std::function<Scene* ()> factory, float outSec, float inSec) {
		if (impl_->state != Impl::State::Idle) return;
		impl_->pendingFactory = std::move(factory);
		impl_->outSec = Impl::Max(0.0001f, outSec);
		impl_->inSec = Impl::Max(0.0001f, inSec);
		impl_->t = 0.f; impl_->alpha = 0.f;
		impl_->state = Impl::State::FadeOut;
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
