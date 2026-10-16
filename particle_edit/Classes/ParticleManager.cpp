#include "ParticleManager.h"
#include "ParticleExamples.h"
#include "ParticleSystem.h"
#include "Texture2D.h"
#include "ParticleConfig.h"   // 若还没有这个文件，见文末说明
#include "SDL.h"
#include <cstdio>
#include <vector>
#include "ImGuiFileDialog.h"

// ============================================================
// 单例
// ============================================================
static ParticleManager* g_instance = nullptr;

ParticleManager* ParticleManager::create()
{
    if (!g_instance) g_instance = new ParticleManager();
    return g_instance;
}

void ParticleManager::destroy()
{
    delete g_instance;
    g_instance = nullptr;
}

ParticleManager::ParticleManager()
{ 
}

ParticleManager::~ParticleManager()
{
    if (_particles) { delete _particles;      _particles = nullptr; }
    if (_previewTexture) { delete _previewTexture; _previewTexture = nullptr; }
 
}

// ============================================================
// 初始化
// ============================================================
void ParticleManager::init(SDL_Renderer* ren,int w,int h)
{
   
    _renderer = ren;

    _particles = ParticleExample::create();
    if (!_particles) return;

    _particles->setRenderer(_renderer);

    // 默认 FIRE
    applyPreset(ParticleExample::FIRE);
    ensurePreviewTexture(w, h);
}

// ============================================================
// 三个接口
// ============================================================

// ---------- 应用预设 ----------
void ParticleManager::applyPreset(ParticleExample::PatticleStyle style)
{
    if (!_particles) return;

    _particles->setStyle(style);
    snprintf(_statusMsg, sizeof(_statusMsg), "Preset: style=%d", (int)style);
}

// ---------- 图片路径加载 ----------
bool ParticleManager::loadDragTexture(const char* path)
{
	snprintf(_texturePath, sizeof(_texturePath), "%s", path);
	return loadTexture(path);
}
bool ParticleManager::loadTexture(const char* path)
{
    if (!_particles) {
        snprintf(_statusMsg, sizeof(_statusMsg), "No particle");
        return false;
    }
    if (!path || !*path) {
        snprintf(_statusMsg, sizeof(_statusMsg), "Texture path empty");
        return false;
    }

    // ---------- 1. 读文件到内存 ----------
    SDL_RWops* rw = SDL_RWFromFile(path, "rb");
    if (!rw) {
        snprintf(_statusMsg, sizeof(_statusMsg), "Open failed: %s (%s)", path, SDL_GetError());
        return false;
    }

    Sint64 fileSize = SDL_RWsize(rw);
    if (fileSize <= 0) {
        SDL_RWclose(rw);
        snprintf(_statusMsg, sizeof(_statusMsg), "Empty file: %s", path);
        return false;
    }

    std::vector<unsigned char> buf((size_t)fileSize);
    size_t readBytes = SDL_RWread(rw, buf.data(), 1, buf.size());
    SDL_RWclose(rw);

    if (readBytes != buf.size()) {
        snprintf(_statusMsg, sizeof(_statusMsg), "Read failed: %s", path);
        return false;
    }

    // ---------- 2. 解码成 Texture2D ----------
    Texture2D* tex = new Texture2D();
	if (!tex) {
		snprintf(_statusMsg, sizeof(_statusMsg), "Out of memory");
		return false;
	}
    if (!tex->loadMemData(_renderer, buf.data(), (int)buf.size())) {
        delete tex;
        snprintf(_statusMsg, sizeof(_statusMsg), "Decode failed: %s", path);
        return false;
    }

    // ---------- 3. 替换到粒子（setTexture 会 delete 旧的） ----------
    _particles->setTexture(tex);

    // ---------- 4. 更新路径显示 ----------
    snprintf(_texturePath, sizeof(_texturePath), "%s", path);
    snprintf(_statusMsg, sizeof(_statusMsg), "Texture loaded: %s", path);
    return true;
}

void ParticleManager::drawFileDialogs()
{
    if (_openTextureDialog)
    {
        IGFD::FileDialogConfig config;
        config.path = ".";   // 初始目录
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseTextureDlg", "Choose Texture",
            ".png,.jpg,.jpeg,.bmp,.tga", config);
        _openTextureDialog = false;
    }
    if (_openPlistDialog)
    {
        IGFD::FileDialogConfig config;
        config.path = ".";   // 初始目录
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChoosePlistDlg", "Choose Particle Plist",
            ".plist", config);
        _openPlistDialog = false;
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseTextureDlg"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
            loadTexture(path.c_str());
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("ChoosePlistDlg"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
            snprintf(_plistPath, sizeof(_plistPath), "%s", path.c_str());
            loadFromPlist(_plistPath);
        }
        ImGuiFileDialog::Instance()->Close();
    }
}
bool ParticleManager::resetTexture()
{
    if (!_particles) return false;

    // 创建默认 fire.png 纹理
    Texture2D* tex = new Texture2D();
    if (!tex->loadMemData(_renderer,
        (unsigned char*)__firePngData,
        sizeof(__firePngData)))
    {
        delete tex;
        snprintf(_statusMsg, sizeof(_statusMsg), "Reset texture failed");
        return false;
    }

    _particles->setTexture(tex);   // setTexture 会 delete 旧纹理

    // 路径显示恢复默认
    snprintf(_texturePath, sizeof(_texturePath), "fire.png");
    snprintf(_statusMsg, sizeof(_statusMsg), "Texture reset to default");
    return true;
}

// ---------- plist 保存 ----------
bool ParticleManager::saveToPlist(const char* path)
{
    if (!_particles || !path || !*path) {
        snprintf(_statusMsg, sizeof(_statusMsg), "Save: bad path");
        return false;
    }

    ParticleConfig cfg;
    cfg.collectFrom(_particles);
	// ★ 补上纹理文件名
	if (_texturePath[0]) {
		cfg.textureFileName =  _texturePath ;
	}
    if (cfg.saveToPlist(path)) {
        snprintf(_statusMsg, sizeof(_statusMsg), "Saved: %s", path);
        return true;
    }
    snprintf(_statusMsg, sizeof(_statusMsg), "Save failed: %s", path);
    return false;
}

// ---------- plist 加载 ----------
bool ParticleManager::loadDragPlist(const char* path)
{
	snprintf(_plistPath, sizeof(_plistPath), "%s",  path);
	return loadFromPlist(path);
}
bool ParticleManager::loadFromPlist(const char* path)
{
 
    if (!_particles || !path || !*path) {
        snprintf(_statusMsg, sizeof(_statusMsg), "Load: bad path");
        return false;
    }

    ParticleConfig cfg;
    if (!cfg.loadFromPlist(path)) {
        snprintf(_statusMsg, sizeof(_statusMsg), "Load failed: %s", path);
        return false;
    }

    cfg.applyTo(_particles);   // 里面会调 setTotalParticles（前提：会 resize）

	std::string texturePath = convertPathFormatToUnixStyle(path);
	texturePath = texturePath.substr(0,texturePath.rfind("/")+1) + cfg.textureFileName;
	strcpy(_texturePath ,texturePath.c_str());
	loadDragTexture(_texturePath);

    snprintf(_statusMsg, sizeof(_statusMsg), "Loaded: %s", path);
    return true;
}

// ============================================================
// 更新
// ============================================================
void ParticleManager::update(float dt)
{
    if (!_playing || !_particles) return;
    _particles->update(dt * _timeScale);
}

// ============================================================
// 预览
// ============================================================
void ParticleManager::ensurePreviewTexture(int w, int h)
{
    if (_previewTexture && w == _contentRect.w && h == _contentRect.h) return;

    if (_previewTexture) { delete _previewTexture; _previewTexture = nullptr; }

    _previewTexture = new Texture2D();     // 你已有的空壳
    _contentRect.w = w;
    _contentRect.h = h;
    _particles->setPosition(Vec2(w / 2, h / 2));
    // 如果你给 Texture2D 加了"创建空目标纹理"接口，在这里调用
      _previewTexture->createTarget(_renderer, w,h);
}

void ParticleManager::renderToPreview()
{
    // 用户后续接：把粒子画到 _previewTexture
    // 这里的实现依赖你 ParticleSystem::draw 的签名
    if (!_previewTexture || !_particles) return;

    _particles->setScale(_zoom);
    _particles->draw(_previewTexture);

}

// ============================================================
// 主 UI：三栏
// ============================================================
void ParticleManager::drawUI()
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 pos = vp->WorkPos;
    const ImVec2 sz = vp->WorkSize;

    const float toolbarH = 40.0f;
    const float leftW = 280.0f;
    const float rightW = 480.0f;
    const float bodyY = pos.y + toolbarH;
    const float bodyH = sz.y - toolbarH;

    // ---------- 顶部工具栏 ----------
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(ImVec2(sz.x, toolbarH));
    ImGui::Begin("##toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    drawToolbar();
    ImGui::End();

    // ---------- 左：类型 + 图片 + plist ----------
    ImGui::SetNextWindowPos(ImVec2(pos.x, bodyY));
    ImGui::SetNextWindowSize(ImVec2(leftW, bodyH));
    ImGui::Begin("##preset_panel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    drawPresetPanel();
    ImGui::End();

    // ---------- 中：预览 ----------
    ImGui::SetNextWindowPos(ImVec2(pos.x + leftW, bodyY));
    ImGui::SetNextWindowSize(ImVec2(sz.x - leftW - rightW, bodyH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##preview", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    drawPreview();
    ImGui::End();
    ImGui::PopStyleVar();

    // ---------- 右：属性 ----------
    ImGui::SetNextWindowPos(ImVec2(pos.x + sz.x - rightW, bodyY));
    ImGui::SetNextWindowSize(ImVec2(rightW, bodyH));
    ImGui::Begin("##properties", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
     
    drawPropertyEditor();
    ImGui::End();

    drawFileDialogs();
}

// ============================================================
// 工具栏
// ============================================================
void ParticleManager::drawToolbar()
{
    ImGui::SetCursorPosY(6.0f);
    ImGui::SetCursorPosX(8.0f);

    if (ImGui::Button(_playing ? "Pause" : "Play", ImVec2(70, 0)))
        _playing = !_playing;

    ImGui::SameLine();
    if (ImGui::Button("Restart", ImVec2(70, 0))) {
        if (_particles) _particles->resetSystem();
    }

    ImGui::SameLine(180);
    ImGui::Text("Time");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##timescale", &_timeScale, 0.1f, 3.0f, "%.2fx");

    ImGui::SameLine(340);
    ImGui::Text("Zoom");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##zoom", &_zoom, 0.25f, 3.0f, "%.2fx");

//     ImGui::SameLine(500);
//     ImGui::Checkbox("Grid", &_showGrid);

    ImGui::SameLine(620);
    ImGui::Checkbox("Mouse as Emitter", &_mouseAsEmitter);

    ImGui::SameLine(ImGui::GetWindowWidth() - 150);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);


}

// ============================================================
// 左栏：类型 + 图片 + plist
// ============================================================
void ParticleManager::drawPresetPanel()
{
    ParticleExample* p = _particles;
    if (!p) return;

    ImGui::PushItemWidth(-1);

    // ============================================================
    // 1. Type
    // ============================================================
    ImGui::SetCursorPosY(10.0f);
    ImGui::Text("Type");
    ImGui::Separator();

    static const char* kStyleNames[] = {
        "NONE", "FIRE", "FIRE_WORK", "SUN", "GALAXY", "FLOWER",
        "METEOR", "SPIRAL", "EXPLOSION", "SMOKE", "SNOW", "RAIN"
    };
    const int kStyleCount = IM_ARRAYSIZE(kStyleNames);

    int curStyle = (int)p->getStyle();
    if (ImGui::Combo("##style", &curStyle, kStyleNames, kStyleCount))
        applyPreset((ParticleExample::PatticleStyle)curStyle);

    if (ImGui::Button("Reset to preset", ImVec2(-1, 0)))
        applyPreset(p->getStyle());

    // ============================================================
    // 2. Texture
    // ============================================================
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Texture");

    ImGui::InputText("##texPath", _texturePath, sizeof(_texturePath));

    float avail = ImGui::GetContentRegionAvail().x;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float halfW = (avail - spacing) * 0.5f;

    if (ImGui::Button("Browse...", ImVec2(halfW, 0)))
        _openTextureDialog = true;                       // ★ 只设标志
     

    ImGui::SameLine();
    if (ImGui::Button("Load##tex", ImVec2(halfW, 0)))
        loadTexture(_texturePath);

    if (ImGui::Button("Clear##tex", ImVec2(-1, 0)))
        resetTexture();
    // ============================================================
    // 3. Plist
    // ============================================================
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Plist");

    ImGui::InputText("##plistPath", _plistPath, sizeof(_plistPath));

    if (ImGui::Button("Browse...##plist", ImVec2(halfW, 0)))
        _openPlistDialog = true;                         // ★ 只设标志

    ImGui::SameLine();
    if (ImGui::Button("Load##plist", ImVec2(halfW, 0)))
        loadFromPlist(_plistPath);

    if (ImGui::Button("Save##plist", ImVec2(-1, 0)))
        saveToPlist(_plistPath);

    // ============================================================
    // 4. Status
    // ============================================================
    if (_statusMsg[0])
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextWrapped("%s", _statusMsg);
    }

    ImGui::PopItemWidth();
}

// ============================================================
// 中栏：预览
// ============================================================
void ParticleManager::drawPreview()
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    int w = (int)avail.x;
    int h = (int)avail.y;
    if (w <= 0 || h <= 0) return;

    ensurePreviewTexture(w, h);
    renderToPreview();

    SDL_Texture* tex = _previewTexture ? _previewTexture->getTexture() : nullptr;

    if (tex)
    {
        ImGui::Image((ImTextureID)(intptr_t)tex, avail, ImVec2(0, 0), ImVec2(1, 1));
    }
    else
    {
        ImGui::Dummy(avail);
    }

    // ★ 显示信息
    ImVec2 mouse = ImGui::GetMousePos();
    ImVec2 imgMin = ImGui::GetItemRectMin();       // Image 左上角在屏幕的位置
    ImVec2 imgMax = ImGui::GetItemRectMax();

    float relX = mouse.x - imgMin.x;   // 鼠标相对 Image 左上角
    float relY = mouse.y - imgMin.y;

    bool inside = (relX >= 0 && relY >= 0 && relX < avail.x && relY < avail.y);

    // 缩放系数：显示尺寸 vs 纹理尺寸
    float scaleX = (float)w / (float)_contentRect.w;
    float scaleY = (float)h / (float)_contentRect.h;

    float texX = relX / scaleX;
    float texY = relY / scaleY;
    if (_mouseAsEmitter && inside && (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right)))
    {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			Vec2 mousePos(texX, texY);
			_particles->setPosition(mousePos);
		}
		if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			
			Vec2 mousePos(texX, texY);
			_particles->setPosition(mousePos);
			_particles->resetSystem();
		}
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();

    char buf[1024];
    snprintf(buf, sizeof(buf),
        "Preview RT: %.0f x %.0f\n"
        "Display:    %.0f x %.0f\n"
        "Mouse img:  %.0f, %.0f %s\n"
        "Mouse tex:  %.0f, %.0f",
        _contentRect.w, _contentRect.h,
        avail.x, avail.y,
        relX, relY, inside ? "[in]" : "[out]",
        relX / scaleX, relY / scaleY);

    dl->AddText(ImVec2(imgMin.x + 8, imgMin.y + 8),
        IM_COL32(255, 220, 100, 255), buf);

    // 鼠标十字准星
    if (inside)
    {
        dl->AddLine(ImVec2(imgMin.x + relX - 10, imgMin.y + relY),
            ImVec2(imgMin.x + relX + 10, imgMin.y + relY),
            IM_COL32(255, 100, 100, 180));
        dl->AddLine(ImVec2(imgMin.x + relX, imgMin.y + relY - 10),
            ImVec2(imgMin.x + relX, imgMin.y + relY + 10),
            IM_COL32(255, 100, 100, 180));
    }
}

// ============================================================
// 右栏：属性编辑器
// ============================================================
void ParticleManager::drawPropertyEditor()
{
    ParticleExample* p = _particles;
    if (!p) {
        ImGui::SetCursorPosY(20);
        ImGui::SetCursorPosX(20);
        ImGui::TextDisabled("No particle");
        return;
    }

    ImGui::SetCursorPosY(8.0f);
    ImGui::Text("Properties");
    ImGui::Separator();

    ImGui::BeginChild("##prop_scroll", ImVec2(0, 0), false,
        ImGuiWindowFlags_AlwaysVerticalScrollbar);

    ImGui::PushItemWidth(220.0f);

    // ---------- General ----------
    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int maxP = p->getTotalParticles();
        if (ImGui::DragInt("Max Particles", &maxP, 1, 1, 10000))
            p->setTotalParticles(maxP);

        float dur = p->getDuration();
        if (ImGui::DragFloat("Duration", &dur, 0.05f, -1.0f, 1000.0f, "%.2f"))
            p->setDuration(dur);
        ImGui::SameLine();
        ImGui::TextDisabled("(-1=inf)");

        float er = p->getEmissionRate();
        if (ImGui::DragFloat("Emission Rate", &er, 1.0f, 0.0f, 5000.0f, "%.1f"))
            p->setEmissionRate(er);
    }

    // ---------- Emitter ----------
    if (ImGui::CollapsingHeader("Emitter", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int mode = (int)p->getEmitterMode();
        const char* modes[] = { "Gravity", "Radius" };
        if (ImGui::Combo("Mode", &mode, modes, 2))
            p->setEmitterMode((ParticleSystem::Mode)mode);

        float angle = p->getAngle();
        if (ImGui::DragFloat("Angle", &angle, 1.0f, -360.0f, 360.0f))
            p->setAngle(angle);

        float angleVar = p->getAngleVar();
        if (ImGui::DragFloat("Angle Var", &angleVar, 1.0f, 0.0f, 360.0f))
            p->setAngleVar(angleVar);

        Vec2 pos = p->getPosition();
        float pv[2] = { pos.x, pos.y };
        if (ImGui::DragFloat2("Position", pv, 1.0f, -5000.0f, 5000.0f)) {
            Vec2 np(pv[0], pv[1]);
            p->setPosition(np);
        }

        Vec2 posVar = p->getPosVar();
        float pvv[2] = { posVar.x, posVar.y };
        if (ImGui::DragFloat2("Position Var", pvv, 1.0f, 0.0f, 2000.0f))
            p->setPosVar(Vec2(pvv[0], pvv[1]));
    }

    // ---------- Gravity / Radius ----------
    if (p->getEmitterMode() == ParticleSystem::Mode::GRAVITY)
    {
        if (ImGui::CollapsingHeader("Gravity Mode", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Vec2 g = p->getGravity();
            float gv[2] = { g.x, g.y };
            if (ImGui::DragFloat2("Gravity", gv, 1.0f, -5000.0f, 5000.0f))
                p->setGravity(Vec2(gv[0], gv[1]));

            float speed = p->getSpeed();
            if (ImGui::DragFloat("Speed", &speed, 1.0f, -5000.0f, 5000.0f))
                p->setSpeed(speed);

            float speedVar = p->getSpeedVar();
            if (ImGui::DragFloat("Speed Var", &speedVar, 1.0f, 0.0f, 5000.0f))
                p->setSpeedVar(speedVar);

            float ra = p->getRadialAccel();
            if (ImGui::DragFloat("Radial Accel", &ra, 1.0f, -5000.0f, 5000.0f))
                p->setRadialAccel(ra);

            float raVar = p->getRadialAccelVar();
            if (ImGui::DragFloat("Radial Accel Var", &raVar, 1.0f, 0.0f, 5000.0f))
                p->setRadialAccelVar(raVar);

            float ta = p->getTangentialAccel();
            if (ImGui::DragFloat("Tangential Accel", &ta, 1.0f, -5000.0f, 5000.0f))
                p->setTangentialAccel(ta);

            float taVar = p->getTangentialAccelVar();
            if (ImGui::DragFloat("Tangential Accel Var", &taVar, 1.0f, 0.0f, 5000.0f))
                p->setTangentialAccelVar(taVar);

            bool rotDir = p->getRotationIsDir();
            if (ImGui::Checkbox("Rotation Is Dir", &rotDir))
                p->setRotationIsDir(rotDir);
        }
    }
    else
    {
        if (ImGui::CollapsingHeader("Radius Mode", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float sr = p->getStartRadius();
            if (ImGui::DragFloat("Start Radius", &sr, 1.0f, 0.0f, 5000.0f))
                p->setStartRadius(sr);

            float srVar = p->getStartRadiusVar();
            if (ImGui::DragFloat("Start Radius Var", &srVar, 1.0f, 0.0f, 5000.0f))
                p->setStartRadiusVar(srVar);

            float er2 = p->getEndRadius();
            if (ImGui::DragFloat("End Radius", &er2, 1.0f, -1.0f, 5000.0f))
                p->setEndRadius(er2);

            float erVar = p->getEndRadiusVar();
            if (ImGui::DragFloat("End Radius Var", &erVar, 1.0f, 0.0f, 5000.0f))
                p->setEndRadiusVar(erVar);

            float rps = p->getRotatePerSecond();
            if (ImGui::DragFloat("Rotate/Sec", &rps, 1.0f, -3600.0f, 3600.0f))
                p->setRotatePerSecond(rps);

            float rpsVar = p->getRotatePerSecondVar();
            if (ImGui::DragFloat("Rotate/Sec Var", &rpsVar, 1.0f, 0.0f, 3600.0f))
                p->setRotatePerSecondVar(rpsVar);
        }
    }

    // ---------- Particle ----------
    if (ImGui::CollapsingHeader("Particle", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float ss = p->getStartSize();
        if (ImGui::DragFloat("Start Size", &ss, 1.0f, 0.0f, 2000.0f))
            p->setStartSize(ss);

        float ssVar = p->getStartSizeVar();
        if (ImGui::DragFloat("Start Size Var", &ssVar, 1.0f, 0.0f, 2000.0f))
            p->setStartSizeVar(ssVar);

        float es = p->getEndSize();
        if (ImGui::DragFloat("End Size", &es, 1.0f, -1.0f, 2000.0f))
            p->setEndSize(es);
        ImGui::SameLine();
        ImGui::TextDisabled("(-1=same)");

        float esVar = p->getEndSizeVar();
        if (ImGui::DragFloat("End Size Var", &esVar, 1.0f, 0.0f, 2000.0f))
            p->setEndSizeVar(esVar);

        float life = p->getLife();
        if (ImGui::DragFloat("Life", &life, 0.05f, 0.01f, 200.0f, "%.2f"))
            p->setLife(life);

        float lifeVar = p->getLifeVar();
        if (ImGui::DragFloat("Life Var", &lifeVar, 0.05f, 0.0f, 200.0f, "%.2f"))
            p->setLifeVar(lifeVar);

        float sspin = p->getStartSpin();
        if (ImGui::DragFloat("Start Spin", &sspin, 1.0f, -3600.0f, 3600.0f))
            p->setStartSpin(sspin);

        float sspinVar = p->getStartSpinVar();
        if (ImGui::DragFloat("Start Spin Var", &sspinVar, 1.0f, 0.0f, 3600.0f))
            p->setStartSpinVar(sspinVar);

        float espin = p->getEndSpin();
        if (ImGui::DragFloat("End Spin", &espin, 1.0f, -3600.0f, 3600.0f))
            p->setEndSpin(espin);

        float espinVar = p->getEndSpinVar();
        if (ImGui::DragFloat("End Spin Var", &espinVar, 1.0f, 0.0f, 3600.0f))
            p->setEndSpinVar(espinVar);
    }

    // ---------- Color ----------
    if (ImGui::CollapsingHeader("Color", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto EditColor = [&](const char* label, Color4F cur, void(*setter)(const Color4F&))
        {
            float v[4] = { cur.r, cur.g, cur.b, cur.a };
            if (ImGui::ColorEdit4(label, v,
                ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf))
            {
                Color4F nc; nc.r = v[0]; nc.g = v[1]; nc.b = v[2]; nc.a = v[3];
                setter(nc);
            }
        };

        // 直接用成员方法包装（避开 std::function）
        {
            Color4F c = p->getStartColor();
            float v[4] = { c.r, c.g, c.b, c.a };
            if (ImGui::ColorEdit4("Start Color", v,
                ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf))
            {
                Color4F nc; nc.r = v[0]; nc.g = v[1]; nc.b = v[2]; nc.a = v[3];
                p->setStartColor(nc);
            }
        }
        {
            Color4F c = p->getStartColorVar();
            float v[4] = { c.r, c.g, c.b, c.a };
            if (ImGui::ColorEdit4("Start Color Var", v,
                ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf))
            {
                Color4F nc; nc.r = v[0]; nc.g = v[1]; nc.b = v[2]; nc.a = v[3];
                p->setStartColorVar(nc);
            }
        }
        {
            Color4F c = p->getEndColor();
            float v[4] = { c.r, c.g, c.b, c.a };
            if (ImGui::ColorEdit4("End Color", v,
                ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf))
            {
                Color4F nc; nc.r = v[0]; nc.g = v[1]; nc.b = v[2]; nc.a = v[3];
                p->setEndColor(nc);
            }
        }
        {
            Color4F c = p->getEndColorVar();
            float v[4] = { c.r, c.g, c.b, c.a };
            if (ImGui::ColorEdit4("End Color Var", v,
                ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf))
            {
                Color4F nc; nc.r = v[0]; nc.g = v[1]; nc.b = v[2]; nc.a = v[3];
                p->setEndColorVar(nc);
            }
        }
    }

    // ---------- Blend ----------
    if (ImGui::CollapsingHeader("Blend"))
    {
        bool additive = p->isBlendAdditive();
        if (ImGui::Checkbox("Additive", &additive))
            p->setBlendAdditive(additive);
    }

    ImGui::PopItemWidth();
    ImGui::EndChild();
}
