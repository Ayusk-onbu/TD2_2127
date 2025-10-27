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
#include "Transition/TransitionHub.h"
#include "Game/GameScene.h"
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
    gridCols_ = kInitCols;
    gridRows_ = kInitRows;
    // タイル配列を初期化（全部0 = 空）
    tiles_.assign(gridCols_ * gridRows_, 0);
    ResizeGrid(kInitCols, kInitRows, GridAnchor::TopLeft);
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
        m->Initialize(d3d, "cube/cube.obj");  // 例
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
//bool StageEditor::ScreenToCell(int sx, int sy, int& outCx, int& outCy) const {
//    if (sx < 0 || sy < 0) return false;
//    outCx = static_cast<int>(sx / cellSize_);
//    outCy = static_cast<int>(sy / cellSize_);
//    if (outCx < 0 || outCx >= gridCols_) return false;
//    if (outCy < 0 || outCy >= gridRows_) return false;
//    return true;
//}

bool StageEditor::ScreenToCell(int sx, int sy, int& outCx, int& outCy) const {
    // gridOffset_ を考慮してセル計算（左上原点、+Y下）
    const float gx = (sx - gridOffset_.x) / cellSize_;
    const float gy = (sy - gridOffset_.y) / cellSize_;
    const int cx = static_cast<int>(std::floor(gx));
    const int cy = static_cast<int>(std::floor(gy));
    if (cx < 0 || cy < 0 || cx >= gridCols_ || cy >= gridRows_) return false;
    outCx = cx; outCy = cy;
    return true;
}


// 置き換え：元の ScreenToCell は使わず、この関数を使う
bool StageEditor::ScreenToCellUnbounded(int sx, int sy, int& outCx, int& outCy) const {
    // 画面左上原点のピクセル座標(sx,sy)から、グリッド座標へ
    // gridOffset_ は Draw 側と共有（スクロール）
    const float gx = (sx - gridOffset_.x) / cellSize_;
    const float gy = (sy - gridOffset_.y) / cellSize_;
    outCx = static_cast<int>(std::floor(gx));
    outCy = static_cast<int>(std::floor(gy));
    return true; // 範囲外でも true
}

void StageEditor::ExpandToIncludeCell(int cx, int cy) {
    // 必要量を計算
    int addLeft = (cx < 0) ? (-cx) : 0;
    int addTop = (cy < 0) ? (-cy) : 0;
    int addRight = (cx >= gridCols_) ? (cx - gridCols_ + 1) : 0;
    int addBottom = (cy >= gridRows_) ? (cy - gridRows_ + 1) : 0;

    if (addLeft == 0 && addTop == 0 && addRight == 0 && addBottom == 0) return; // 追加不要

    const int newCols = gridCols_ + addLeft + addRight;
    const int newRows = gridRows_ + addTop + addBottom;

    // 1) 新しい配列を用意
    std::vector<int> newTiles(newCols * newRows, 0);

    // 2) 旧タイルを新配列へコピー（左/上に追加した分だけオフセットして配置）
    for (int y = 0; y < gridRows_; ++y) {
        for (int x = 0; x < gridCols_; ++x) {
            const int nx = x + addLeft;
            const int ny = y + addTop;
            newTiles[ny * newCols + nx] = tiles_[y * gridCols_ + x];
        }
    }

    // 3) モデル座標をシフト
    for (auto& pm : placedModels_) {
        pm.cx += addLeft;
        pm.cy += addTop;
    }
    // 4) インスタンスも座標再反映（位置を更新）
    for (size_t i = 0; i < placedModels_.size() && i < modelInstances_.size(); ++i) {
        const auto& pm = placedModels_[i];
        Vector3 pos = CellCenterToWorld(pm.cx, pm.cy);
        modelInstances_[i]->worldTransform_.set_.Translation(pos);
    }

    // 5) 新サイズを反映
    tiles_.swap(newTiles);
    gridCols_ = newCols;
    gridRows_ = newRows;
}

// ======== 手動拡張（四辺に任意セル数） ========
void StageEditor::ExpandBy(int addLeft, int addTop, int addRight, int addBottom)
{
    // 既存の ExpandToIncludeCell を使って簡易に拡張します。
    // 左/上を増やすと既存(0,0)が右下にずれるので、順番は左→上→右→下の順でOK。
    if (addLeft > 0) { ExpandToIncludeCell(-addLeft, 0); }
    if (addTop > 0) { ExpandToIncludeCell(0, -addTop); }
    if (addRight > 0) { ExpandToIncludeCell(gridCols_ - 1 + addRight, 0); }
    if (addBottom > 0) { ExpandToIncludeCell(0, gridRows_ - 1 + addBottom); }
}

// ===== 追加実装: グリッドリサイズ（内容は可能な限り保持） =====
bool StageEditor::ResizeGrid(int newCols, int newRows, GridAnchor anchor)
{
    if (newCols <= 0 || newRows <= 0) return false;

    const int oldCols = gridCols_;
    const int oldRows = gridRows_;

    // 変化なしなら何もしない
    if (newCols == oldCols && newRows == oldRows) return true;

    // 新しい配列を消しIDで初期化
    std::vector<int> newTiles;
    newTiles.assign(newCols * newRows, eraseTileId_);

    // 既存→新配列へのオフセット（アンカーで決定）
    int offX = 0;
    int offY = 0;
    if (anchor == GridAnchor::TopLeft) {
        offX = 0;
        offY = 0;
    }
    else { // Center
        offX = (newCols - oldCols) / 2;
        offY = (newRows - oldRows) / 2;
    }

    // タイルのコピー（はみ出す分は捨て）
    for (int y = 0; y < oldRows; ++y) {
        for (int x = 0; x < oldCols; ++x) {
            const int nx = x + offX;
            const int ny = y + offY;
            if (0 <= nx && nx < newCols && 0 <= ny && ny < newRows) {
                newTiles[ny * newCols + nx] = tiles_[y * oldCols + x];
            }
        }
    }

    // モデルの座標（セル座標）も同様にシフト＆範囲外は削除
    if (offX != 0 || offY != 0) {
        std::vector<PlacedModel> kept;
        kept.reserve(placedModels_.size());
        for (auto& m : placedModels_) {
            const int nx = m.cx + offX;
            const int ny = m.cy + offY;
            if (0 <= nx && nx < newCols && 0 <= ny && ny < newRows) {
                auto nm = m;
                nm.cx = nx;
                nm.cy = ny;
                kept.push_back(nm);
            }
        }
        placedModels_.swap(kept);
    }

    // 反映
    tiles_.swap(newTiles);
    gridCols_ = newCols;
    gridRows_ = newRows;

    return true;
}



//======================
// モデル配置ヘルパ
//======================
Vector3 StageEditor::CellCenterToWorld(int cx, int cy) const {
    const float wx = (cx + 0.5f) * worldCellSize_;
    const float wy = (cy + 0.5f) * worldCellSize_;
    return { wx, wy, baseY_ };
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
        // StageEditor::PlaceModelAtCell(...) で Translation の直後に追記
// 位置
        inst->worldTransform_.set_.Translation(CellCenterToWorld(cx, cy));

        // スケール（どちらか片方が通るはず）
        inst->worldTransform_.set_.Scale({ defaultScale_, defaultScale_, defaultScale_ });
        // inst->worldTransform_.set_.Scaling({ defaultScale_, defaultScale_, defaultScale_ }); // ←こっちの命名の可能性も

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
	CameraSystem::GetInstance()->Update();
    auto& key = InputManager::GetKey();
    auto& mouse = InputManager::GetMouse();

    // ImGuiのUI上にマウスがある間はクリック配置を抑止
    if (ImGui::GetIO().WantCaptureMouse) {
        paintHold_ = eraseHold_ = false;
        // ただしショートカット（Ctrl+S / Ctrl+L 等）は下で拾えるよう return しない
    }

   // ---------------------
   // 0)GameScene へ遷移
   // ---------------------
    if (key.PressedKey(DIK_F2)) {
        // ① エディタ内容をゲーム用CSVに吐く（ここは StageEditor のメンバーなので this でOK）
        this->ExportForGameScene("Resources/stage/stage1.csv", 100, 20);

        // ② そのまま GameScene へ遷移（エディタを new し直さない！）
        Fngine* eng = p_fngine_; // あなたの環境で取得
        Transition::FadeToSlideRight([eng] {
            auto* gs = new GameScene();
            //gs->BindEngine(eng);              // 必要なら
            return gs;
            }, 0.8f, 0.8f);
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

    //const bool lDown = mouse.IsButtonPress(0); // 左ボタン押下中
    //const bool rDown = mouse.IsButtonPress(1); // 右ボタン押下中

    // 範囲外でもセル座標を得る
    int cx, cy;
    if (ScreenToCellUnbounded(sx, sy, cx, cy)) {

        const bool lDown = mouse.IsButtonPress(0);
        const bool rDown = mouse.IsButtonPress(1);

        // 範囲外は描画/配置ロジックに入らず終了（＝拡張もしない）
        if (!InBounds(cx, cy)) {
            if (!lDown) paintHold_ = false;
            if (!rDown) eraseHold_ = false;
            return;
        }

        // ここから下は「範囲内」確定
        const int idx = IndexOf(cx, cy);

        // 左ボタン：ペイント
        if (lDown) {
            tiles_[idx] = currentTileId_;
            PlaceModelAtCell(cx, cy, currentTileId_);
            paintHold_ = true;
        }
        else if (paintHold_) {
            paintHold_ = false;
        }

        // 右ボタン：消し
        if (rDown) {
            tiles_[idx] = eraseTileId_;
            EraseModelAtCell(cx, cy);
            eraseHold_ = true;
        }
        else if (eraseHold_) {
            eraseHold_ = false;
        }

        // 追加：回転ショートカット（必要なければ削除OK）
        if (lDown && key.PressKey(DIK_Q)) {
            for (auto& m : placedModels_) if (m.cx == cx && m.cy == cy) { m.rotY -= 90.f; break; }
        }
        if (lDown && key.PressKey(DIK_E)) {
            for (auto& m : placedModels_) if (m.cx == cx && m.cy == cy) { m.rotY += 90.f; break; }
        }
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

    // ===== 手動スクロール（中ボタン or Space+左ドラッグ） =====
    {
        Vector2 posN{ 0,0 };
        mouse.GetPosition(posN);
        ImVec2 mousePx{
            (posN.x + 0.5f) * kScreenW,
            (-posN.y + 0.5f) * kScreenH
        };

        const bool mid = mouse.IsButtonPress(2);
        const bool space = key.PressKey(DIK_SPACE);
        const bool lbtn = mouse.IsButtonPress(0);

        const bool beginPan = (!panning_) && (mid || (space && lbtn)) && !ImGui::GetIO().WantCaptureMouse;
        const bool endPan = panning_ && !(mid || (space && lbtn));

        if (beginPan) {
            panning_ = true;
            panStartMouse_ = mousePx;
            panStartOffset_ = gridOffset_;
        }
        else if (endPan) {
            panning_ = false;
        }

        if (panning_) {
            ImVec2 delta{ mousePx.x - panStartMouse_.x, mousePx.y - panStartMouse_.y };
            gridOffset_.x = panStartOffset_.x + delta.x;
            gridOffset_.y = panStartOffset_.y + delta.y;
        }
    }

}

//======================
// Draw: グリッド＋ホバー強調＋配置マーカー
//======================
void StageEditor::Draw() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList(); // 画面全体に描く
    const ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    // 設定値
    static bool  showGrid = true;
    static float thickness = lineThickness_;

    // ======== グリッド線 ========
    if (showGrid) {
        const float half = 0.5f;
        const ImU32 gridColorU32 = IM_COL32(
            (int)(gridColor_.x * 255.0f),
            (int)(gridColor_.y * 255.0f),
            (int)(gridColor_.z * 255.0f),
            (int)(gridColor_.w * 255.0f)
        );

        // 垂直線
        for (float x = std::fmod(gridOffset_.x, cellSize_); x < screenSize.x; x += cellSize_) {
            dl->AddLine(ImVec2(x + half, 0), ImVec2(x + half, screenSize.y), gridColorU32, thickness);
        }
        // 水平線
        for (float y = std::fmod(gridOffset_.y, cellSize_); y < screenSize.y; y += cellSize_) {
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
            const float x0 = cx * cellSize_ + gridOffset_.x;
            const float y0 = cy * cellSize_ + gridOffset_.y;
            const float x1 = x0 + cellSize_;
            const float y1 = y0 + cellSize_;
            dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1),
                IM_COL32(80, 160, 255, 40));
            dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1),
                IM_COL32(80, 160, 255, 140), 0.0f, 0, 2.0f);
        }
    }

    // ======== 配置済みモデルのマーカー ========
    for (const auto& m : placedModels_) {
        const float x0 = m.cx * cellSize_ + gridOffset_.x;
        const float y0 = m.cy * cellSize_ + gridOffset_.y;
        const float cxp = x0 + cellSize_ * 0.5f;
        const float cyp = y0 + cellSize_ * 0.5f;
        dl->AddCircleFilled(ImVec2(cxp, cyp), 4.0f, IM_COL32(255, 200, 0, 220));
        char buf[16]; snprintf(buf, sizeof(buf), "%d", m.tileId);
        dl->AddText(ImVec2(x0 + 2, y0 + 2), IM_COL32(255, 255, 255, 220), buf);
    }

    // ======== UI ========
    if (ImGui::Begin("Grid Settings")) {
        ImGui::Checkbox("Show Grid", &showGrid);
        ImGui::SliderFloat("Cell Size (px)", &cellSize_, 8.0f, 128.0f, "%.1f");
        ImGui::SliderFloat("Line Thickness", &thickness, 1.0f, 4.0f, "%.1f");
        ImGui::DragFloat2("Offset", &gridOffset_.x, 1.0f); // ← gridOffset_ を直接操作

        ImGui::SeparatorText("Placement");
        ImGui::SliderInt("Current Tile ID", &currentTileId_, 1, 9);
        ImGui::SliderFloat("World Cell Size", &worldCellSize_, 0.25f, 5.0f, "%.2f");
        ImGui::SliderFloat("Base Y", &baseY_, -5.0f, 5.0f, "%.2f");
        ImGui::Text("LeftClick=Place, RightClick=Erase, Q/E=Rotate (holding)");

        // ===== ImGui: 手動拡張UI（任意） =====
        static int grow = 1;
        ImGui::SeparatorText("Manual Expand");
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Cells", &grow);
        grow = (grow < 1) ? 1 : grow;

        if (ImGui::Button("+Left")) { ExpandBy(grow, 0, 0, 0); }
        ImGui::SameLine();
        if (ImGui::Button("+Right")) { ExpandBy(0, 0, grow, 0); }
        ImGui::SameLine();
        if (ImGui::Button("+Top")) { ExpandBy(0, grow, 0, 0); }
        ImGui::SameLine();
        if (ImGui::Button("+Bottom")) { ExpandBy(0, 0, 0, grow); }

        // ★ 既存の "Grid Settings" パネル内のどこかに追記
        ImGui::SeparatorText("Grid Size");

        // 現在値のローカル編集用バッファ
        static int uiCols = 0, uiRows = 0;
        static int anchorIdx = 0; // 0:TopLeft, 1:Center

        if (uiCols == 0) uiCols = GridCols();
        if (uiRows == 0) uiRows = GridRows();

        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Cols", &uiCols);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Rows", &uiRows);

        const char* anchorItems[] = { "Top-Left", "Center" };
        ImGui::SetNextItemWidth(120);
        ImGui::Combo("Anchor", &anchorIdx, anchorItems, IM_ARRAYSIZE(anchorItems));

        ImGui::SameLine();
        if (ImGui::Button("Apply Size")) {
            uiCols = (uiCols < 1) ? 1 : uiCols;
            uiRows = (uiRows < 1) ? 1 : uiRows;
            ResizeGrid(uiCols, uiRows, anchorIdx == 0 ? GridAnchor::TopLeft : GridAnchor::Center);
            // 適用後の値で同期
            uiCols = GridCols();
            uiRows = GridRows();
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset to Init")) {
            ResizeGrid(kInitCols, kInitRows, GridAnchor::TopLeft);
            uiCols = GridCols();
            uiRows = GridRows();
        }

        ImGui::Text("Current: %d x %d", GridCols(), GridRows());


        ImGui::End();
    }

    // ======== モデル描画 ========
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

bool StageEditor::ExportForGameScene(const char* outPath, int outCols, int outRows) const {
    std::ofstream ofs(outPath);
    if (!ofs) { OutputDebugStringA("[StageEditor] Export failed (open).\n"); return false; }

    // MapChipField::LoadMapChipCsv が期待する形式：
    // ・サイズ行なし
    // ・行ごとに outCols 個の 0/1 をカンマ区切り
    // ・行は "上から下へ" outRows 行
    // ここでは editor の tiles_ (gridCols_ x gridRows_) を参照しつつ
    // はみ出しは 0 でパディング、超過はクリップ。

    for (int y = 0; y < outRows; ++y) {
        for (int x = 0; x < outCols; ++x) {
            int v = 0;
            if (0 <= x && x < gridCols_ && 0 <= y && y < gridRows_) {
                v = tiles_[IndexOf(x, y)];
            }
            ofs << v;
            if (x + 1 < outCols) ofs << ",";
        }
        ofs << "\n";
    }

    OutputDebugStringA("[StageEditor] Exported for GameScene.\n");
    return true;
}
