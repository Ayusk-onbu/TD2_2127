#include "StageEditor.h"
#include "TextureManager.h"
#include "Fngine.h"
#include "CameraSystem.h"
#include "Matrix4x4.h"
#include "Engine/Input/InputManager.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <imguiManager.h>

// 画面サイズ（Mouse::GetPosition の正規化をピクセルへ戻すために使用）
// Mouse::GetPosition() が 1280x720 基準で正規化している前提で合わせる
static constexpr int kScreenW = 1280;
static constexpr int kScreenH = 720;

//======================
// コンストラクタ等
//======================
StageEditor::StageEditor() {}
StageEditor::~StageEditor() {}

void StageEditor::Initialize() {

    OutputDebugStringA("[StageEditor] Init called\n");

    if (!eng_) {
        // エンジン未Bindなら何もしない（戻ってもOK）
        return;
    }

    // タイル配列を初期化（全部0 = 空）
    tiles_.assign(gridCols_ * gridRows_, 0);

    // 白テクスチャ（線用）
    whiteTexId_ = TextureManager::GetInstance()->LoadTexture("resources/GridLine.png");
    if (whiteTexId_ < 0) {
        whiteTexId_ = TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
    }

    // 1×1スプライト初期化（線に伸ばして使う）
    auto& d3d = eng_->GetD3D12System();
    gridLine_.Initialize(d3d, 1.0f, 1.0f);

    // ======== モデルテンプレートの読み込み ========
    modelTemplates_.clear();

    {
        auto m = std::make_unique<ModelObject>();
        m->Initialize(d3d, "Plane.obj");  // 例
		m->SetFngine(eng_);            // ★ 追加：Fngineセット必須
        m->textureHandle_ = whiteTexId_;
        modelTemplates_.push_back(std::move(m));
    }
    {
        auto m = std::make_unique<ModelObject>();
        m->Initialize(d3d, "Plane.obj");
        m->SetFngine(eng_);
        m->textureHandle_ = whiteTexId_;
        modelTemplates_.push_back(std::move(m));
    }

    OutputDebugStringA("[StageEditor] Model templates loaded.\n");
}

void StageEditor::Finalize() {
    // TextureManager が参照カウントなら特に不要
}

//======================
// 画面→セル座標変換（ピクセル）
//======================
bool StageEditor::ScreenToCell(int sx, int sy, int& outCx, int& outCy) const {
    if (sx < 0 || sy < 0) return false;
    outCx = static_cast<int>(sx / cellSize_);
    outCy = static_cast<int>(sy / cellSize_);
    if (outCx < 0 || outCx >= gridCols_) return false;
    if (outCy < 0 || outCy >= gridRows_) return false;
    return true;
}

//======================
// モデル配置ヘルパ
//======================
Vector3 StageEditor::CellCenterToWorld(int cx, int cy) const {
    const float wx = (cx + 0.5f) * worldCellSize_;
    const float wz = (cy + 0.5f) * worldCellSize_;
    return { wx, baseY_, wz };
}

void StageEditor::PlaceModelAtCell(int cx, int cy, int tileId) {
    for (auto& m : placedModels_) {
        if (m.cx == cx && m.cy == cy) { m.tileId = tileId; return; }
    }
    placedModels_.push_back({ tileId, cx, cy, 0.0f });

    if (tileId >= 1 && tileId <= (int)modelTemplates_.size()) {
        auto inst = std::make_unique<ModelObject>();
        auto& d3d = eng_->GetD3D12System();

        // テンプレのModelDataでGPUリソース初期化
        inst->Initialize(d3d, modelTemplates_[tileId - 1]->GetModelData());

        // ★ 追加：ここで fngine_ をセット（Draw() が fngine_ を使うため必須）
        inst->SetFngine(eng_);
        inst->textureHandle_ = whiteTexId_;

        // 位置セット
        Vector3 pos = CellCenterToWorld(cx, cy);
        inst->worldTransform_.set_.Translation(pos);

        modelInstances_.push_back(std::move(inst));
    }
}




void StageEditor::EraseModelAtCell(int cx, int cy) {
    for (size_t i = 0; i < placedModels_.size(); ++i) {
        if (placedModels_[i].cx == cx && placedModels_[i].cy == cy) {
            placedModels_.erase(placedModels_.begin() + i);
            if (i < modelInstances_.size()) {
                modelInstances_.erase(modelInstances_.begin() + i);
            }
            return;
        }
    }
}


//======================
// Update: 入力処理（配置/消去/保存/読込）
//======================
void StageEditor::Update() {
    auto& key = InputManager::GetKey();
    auto& mouse = InputManager::GetMouse();

    // ImGuiのUI上にマウスがある間はクリック配置を抑止
    if (ImGui::GetIO().WantCaptureMouse) {
        paintHold_ = eraseHold_ = false;
        // ただしショートカット（Ctrl+S / Ctrl+L 等）は下で拾えるよう return しない
    }

    // ---------------------
    // 1) タイルIDの選択（簡易）
    //    数字キー 1..9 で currentTileId_ を切替
    // ---------------------
    for (int k = 1; k <= 9; ++k) {
        const int dik = (k == 0) ? DIK_0 : (DIK_1 + (k - 1));
        if (key.PressedKey(dik)) {
            currentTileId_ = k;
        }
    }

    // ---------------------
    // 2) クリックで配置/消去（ドラッグ対応）
    //    左: 配置 currentTileId_
    //    右: 消去 eraseTileId_ (0)
    // ---------------------
    Vector2 posN = { 0, 0 };
    mouse.GetPosition(posN); // 正規化座標：x in [-0.5,+0.5], y in [-0.5,+0.5]（上が+）

    // 正規化 → ピクセルに戻す（左上原点、+Y下に変換）
    const int sx = static_cast<int>((posN.x + 0.5f) * kScreenW);
    const int sy = static_cast<int>((-posN.y + 0.5f) * kScreenH);

    const bool lDown = mouse.IsButtonPress(0); // 左ボタン押下中
    const bool rDown = mouse.IsButtonPress(1); // 右ボタン押下中

    int cx, cy;
    if (ScreenToCell(sx, sy, cx, cy)) {
        const int idx = IndexOf(cx, cy);

        // 左ボタン：配置（押しっぱなしでドラッグ塗り）
        if (lDown) {
            tiles_[idx] = currentTileId_;
            PlaceModelAtCell(cx, cy, currentTileId_);
            paintHold_ = true;
        }
        else if (!lDown && paintHold_) {
            paintHold_ = false;
        }

        // 右ボタン：消去
        if (rDown) {
            tiles_[idx] = eraseTileId_;
            EraseModelAtCell(cx, cy);
            eraseHold_ = true;
        }
        else if (!rDown && eraseHold_) {
            eraseHold_ = false;
        }

        // 任意: 回転ショートカット（左クリック中に Q/E）
        if (lDown && key.PressKey(DIK_Q)) {
            for (auto& m : placedModels_) if (m.cx == cx && m.cy == cy) { m.rotY -= 90.f; break; }
        }
        if (lDown && key.PressKey(DIK_E)) {
            for (auto& m : placedModels_) if (m.cx == cx && m.cy == cy) { m.rotY += 90.f; break; }
        }
    }
    else {
        // 画面外に出たらドラッグフラグだけ解除
        if (!lDown) paintHold_ = false;
        if (!rDown) eraseHold_ = false;
    }

    // ---------------------
    // 3) クイック保存/読込
    //    Ctrl+S で保存 / Ctrl+L で読込
    // ---------------------
    const bool ctrl = (key.PressKey(DIK_LCONTROL) || key.PressKey(DIK_RCONTROL));
    if (ctrl && key.PressedKey(DIK_S)) {
        SaveCSV("stage.csv");
    }
    if (ctrl && key.PressedKey(DIK_L)) {
        LoadCSV("stage.csv");
    }
}

//======================
// Draw: グリッド＋ホバー強調＋配置マーカー
//======================
void StageEditor::Draw() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList(); // 画面全体に描く
    const ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    // 設定値（必要ならImGui化）
    static bool  showGrid = true;
    static float thickness = lineThickness_;
    static ImVec2 offset(0.0f, 0.0f); // スクロール用

    // ======== グリッド線 ========
    if (showGrid) {
        const float half = 0.5f; // ピクセルセンタ補正
        const ImU32 gridColorU32 = IM_COL32(
            (int)(gridColor_.x * 255.0f),
            (int)(gridColor_.y * 255.0f),
            (int)(gridColor_.z * 255.0f),
            (int)(gridColor_.w * 255.0f)
        );

        // 垂直線
        for (float x = std::fmod(offset.x, cellSize_); x < screenSize.x; x += cellSize_) {
            dl->AddLine(ImVec2(x + half, 0), ImVec2(x + half, screenSize.y), gridColorU32, thickness);
        }
        // 水平線
        for (float y = std::fmod(offset.y, cellSize_); y < screenSize.y; y += cellSize_) {
            dl->AddLine(ImVec2(0, y + half), ImVec2(screenSize.x, y + half), gridColorU32, thickness);
        }
    }

    // ======== ホバー中セルのハイライト ========
    {
        auto& mouse = InputManager::GetMouse();
        Vector2 posN{ 0,0 };
        mouse.GetPosition(posN);
        const int sx = static_cast<int>((posN.x + 0.5f) * kScreenW);
        const int sy = static_cast<int>((-posN.y + 0.5f) * kScreenH);
        int cx, cy;
        if (ScreenToCell(sx, sy, cx, cy)) {
            const float x0 = cx * cellSize_ + offset.x;
            const float y0 = cy * cellSize_ + offset.y;
            const float x1 = x0 + cellSize_;
            const float y1 = y0 + cellSize_;
            dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1),
                IM_COL32(80, 160, 255, 40));
            dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1),
                IM_COL32(80, 160, 255, 140), 0.0f, 0, 2.0f);
        }
    }

    // ======== 配置済みモデルのマーキング（デバッグ可視化） ========
    for (const auto& m : placedModels_) {
        const float x0 = m.cx * cellSize_ + offset.x;
        const float y0 = m.cy * cellSize_ + offset.y;
        const float cxp = x0 + cellSize_ * 0.5f;
        const float cyp = y0 + cellSize_ * 0.5f;
        dl->AddCircleFilled(ImVec2(cxp, cyp), 4.0f, IM_COL32(255, 200, 0, 220));
        char buf[16]; snprintf(buf, sizeof(buf), "%d", m.tileId);
        dl->AddText(ImVec2(x0 + 2, y0 + 2), IM_COL32(255, 255, 255, 220), buf);
    }

    // ======== グリッド設定UI ========
    if (ImGui::Begin("Grid Settings")) {
        ImGui::Checkbox("Show Grid", &showGrid);
        ImGui::SliderFloat("Cell Size (px)", &cellSize_, 8.0f, 128.0f, "%.1f");
        ImGui::SliderFloat("Line Thickness", &thickness, 1.0f, 4.0f, "%.1f");
        ImGui::DragFloat2("Offset", &offset.x, 1.0f);

        ImGui::SeparatorText("Placement");
        ImGui::SliderInt("Current Tile ID", &currentTileId_, 1, 9);
        ImGui::SliderFloat("World Cell Size", &worldCellSize_, 0.25f, 5.0f, "%.2f");
        ImGui::SliderFloat("Base Y", &baseY_, -5.0f, 5.0f, "%.2f");
        ImGui::Text("LeftClick=Place, RightClick=Erase, Q/E=Rotate (holding)");
        ImGui::End();
    }

    // ※ 本物の3Dモデル描画はあなたの通常レンダーパスで
    // placedModels_ を走査し CellCenterToWorld() を使って WorldTransform へ反映してください
    auto* cam = CameraSystem::GetInstance()->GetActiveCamera();
    for (auto& inst : modelInstances_) {
        inst->LocalToWorld();
        inst->SetWVPData(cam->DrawCamera(inst->worldTransform_.mat_));
        inst->Draw();
    }
}

//======================
// CSV: 保存
//======================
bool StageEditor::SaveCSV(const std::string& path) const {
    std::ofstream ofs(path);
    if (!ofs) return false;

    // 1) サイズ
    ofs << gridCols_ << "," << gridRows_ << "\n";

    // 2) タイル
    for (int y = 0; y < gridRows_; ++y) {
        for (int x = 0; x < gridCols_; ++x) {
            ofs << tiles_[IndexOf(x, y)];
            if (x + 1 < gridCols_) ofs << ",";
        }
        ofs << "\n";
    }

    // 3) モデル
    ofs << "#MODELS " << placedModels_.size() << "\n";
    for (auto& m : placedModels_) {
        ofs << m.tileId << "," << m.cx << "," << m.cy << "," << m.rotY << "\n";
    }
    return true;
}

//======================
// CSV: 読込
//======================
bool StageEditor::LoadCSV(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) return false;

    std::string line;

    // 1) サイズ
    if (!std::getline(ifs, line)) return false;
    {
        std::istringstream ss(line);
        std::string a, b;
        if (!std::getline(ss, a, ',')) return false;
        if (!std::getline(ss, b, ',')) return false;
        int cols = std::stoi(a);
        int rows = std::stoi(b);
        if (cols != gridCols_ || rows != gridRows_) {
            gridCols_ = cols; gridRows_ = rows;
            tiles_.assign(gridCols_ * gridRows_, 0);
        }
    }

    // 2) タイル本体
    int y = 0;
    placedModels_.clear();
    while (y < gridRows_ && std::getline(ifs, line)) {
        if (!line.empty() && line.rfind("#MODELS", 0) == 0) break; // モデルセクションへ
        std::istringstream ss(line);
        std::string tok; int x = 0;
        while (x < gridCols_ && std::getline(ss, tok, ',')) {
            tiles_[IndexOf(x, y)] = tok.empty() ? 0 : std::stoi(tok);
            ++x;
        }
        while (x < gridCols_) { tiles_[IndexOf(x, y)] = 0; ++x; }
        ++y;
    }
    while (y < gridRows_) { for (int x = 0; x < gridCols_; ++x) tiles_[IndexOf(x, y)] = 0; ++y; }

    // 3) モデル
    if (!line.empty() && line.rfind("#MODELS", 0) == 0) {
        // "#MODELS n"
        size_t count = 0;
        {
            std::istringstream head(line.substr(7));
            head >> count;
        }
        for (size_t i = 0; i < count && std::getline(ifs, line); ++i) {
            std::istringstream ms(line);
            std::string a, b, c, d;
            if (std::getline(ms, a, ',') && std::getline(ms, b, ',')
                && std::getline(ms, c, ',') && std::getline(ms, d, ',')) {
                PlacedModel m;
                m.tileId = std::stoi(a);
                m.cx = std::stoi(b);
                m.cy = std::stoi(c);
                m.rotY = std::stof(d);
                placedModels_.push_back(m);
            }
        }
    }
    return true;
}
