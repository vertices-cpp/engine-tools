#include "ParticleManager.h"
#include "ParticleExamples.h"
#include "ParticleSystem.h"
#include "Texture2D.h"
#include "ParticleConfig.h"
#include "SimpleFileDialog.h"

#if defined(_WIN32)
#include "SDL.h"
#else
#include "SDL2/SDL.h"
#endif

#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

// ═══════════════════════════════════════════════════════════════
// 文件对话框（保留原有互斥 + GetDefaultDir 逻辑）
// ═══════════════════════════════════════════════════════════════
void ParticleManager::drawFileDialogs()
{
	static SimpleFileDialog s_texDialog;
	static SimpleFileDialog s_plistDialog;

	if (_openTextureDialog && !s_plistDialog.IsOpen()) {
		s_texDialog.Open("Choose Texture",
			".png,.jpg,.jpeg,.bmp,.tga",
			"");
		_openTextureDialog = false;
	}
	if (_openPlistDialog && !s_texDialog.IsOpen()) {
		s_plistDialog.Open("Choose Particle Plist", ".plist", "");
		_openPlistDialog = false;
	}

	if (s_texDialog.Display()) {
		std::string path = s_texDialog.GetFilePath();
		s_texDialog.Close();
		loadTexture(path.c_str());
	}
	if (s_plistDialog.Display()) {
		std::string path = s_plistDialog.GetFilePath();
		s_plistDialog.Close();
		snprintf(_plistPath, sizeof(_plistPath), "%s", path.c_str());
		loadFromPlist(_plistPath);
	}
}

// ═══════════════════════════════════════════════════════════════
// 单例 / 生命周期
// ═══════════════════════════════════════════════════════════════
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

ParticleManager::ParticleManager() {}

ParticleManager::~ParticleManager()
{
	if (_particles) { delete _particles;      _particles = nullptr; }
	if (_previewTexture) { delete _previewTexture; _previewTexture = nullptr; }
}

void ParticleManager::init(SDL_Renderer* ren, int w, int h)
{
	_renderer = ren;
	_particles = ParticleExample::create();
	if (!_particles) return;

	_particles->setRenderer(_renderer);
	applyPreset(ParticleExample::FIRE);
	ensurePreviewTexture(w, h);
}

// ═══════════════════════════════════════════════════════════════
// 预设 / 纹理 / plist
// ═══════════════════════════════════════════════════════════════
void ParticleManager::applyPreset(ParticleExample::PatticleStyle style)
{
	if (!_particles) return;
	_particles->setStyle(style);
	snprintf(_statusMsg, sizeof(_statusMsg), "Preset: style=%d", (int)style);
}

bool ParticleManager::loadDragTexture(const char* path)
{
	snprintf(_texturePath, sizeof(_texturePath), "%s", path);
	return loadTexture(path);
}

bool ParticleManager::loadTexture(const char* path)
{
	if (!_particles) { snprintf(_statusMsg, sizeof(_statusMsg), "No particle"); return false; }
	if (!path || !*path) { snprintf(_statusMsg, sizeof(_statusMsg), "Texture path empty"); return false; }

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

	Texture2D* tex = new Texture2D();
	if (!tex) { snprintf(_statusMsg, sizeof(_statusMsg), "Out of memory"); return false; }
	if (!tex->loadMemData(_renderer, buf.data(), (int)buf.size())) {
		delete tex;
		snprintf(_statusMsg, sizeof(_statusMsg), "Decode failed: %s", path);
		return false;
	}

	_particles->setTexture(tex);
	snprintf(_texturePath, sizeof(_texturePath), "%s", path);
	snprintf(_statusMsg, sizeof(_statusMsg), "Texture loaded: %s", path);
	return true;
}

bool ParticleManager::resetTexture()
{
	if (!_particles) return false;

	Texture2D* tex = new Texture2D();
	if (!tex->loadMemData(_renderer,
		(unsigned char*)__firePngData,
		sizeof(__firePngData)))
	{
		delete tex;
		snprintf(_statusMsg, sizeof(_statusMsg), "Reset texture failed");
		return false;
	}
	_particles->setTexture(tex);
	snprintf(_texturePath, sizeof(_texturePath), "fire.png");
	snprintf(_statusMsg, sizeof(_statusMsg), "Texture reset to default");
	return true;
}

bool ParticleManager::saveToPlist(const char* path)
{
    if (!_particles || !path || !*path) {
        snprintf(_statusMsg, sizeof(_statusMsg), "Save: bad path");
        return false;
    }

    ParticleConfig cfg;
    cfg.collectFrom(_particles);
    
    if (_texturePath[0]) {
        // ★ 核心修改：只截取纯文件名（把前面的目录砍掉），防止无限套娃
        std::string texPath(_texturePath);
        size_t pos = texPath.find_last_of("/\\");
        if (pos != std::string::npos) {
            cfg.textureFileName = texPath.substr(pos + 1);
        } else {
            cfg.textureFileName = texPath;
        }
    }

    // 处理保存路径（如果没斜杠自动补全到默认目录）
    std::string realPath = path;
    if (realPath.find('/') == std::string::npos && realPath.find('\\') == std::string::npos) {
        std::string defDir = SimpleFileDialog::GetDefaultDir();
		if (!defDir.empty() && defDir.back() != '/' && defDir.back() != '\\') {
			defDir += '/';
		}
        realPath = defDir + realPath;
    }

    if (cfg.saveToPlist(realPath.c_str())) {
        snprintf(_plistPath, sizeof(_plistPath), "%s", realPath.c_str());
        snprintf(_statusMsg, sizeof(_statusMsg), "Saved: %s", realPath.c_str());
        return true;
    }
    snprintf(_statusMsg, sizeof(_statusMsg), "Save failed: %s", realPath.c_str());
    return false;
}

bool ParticleManager::loadDragPlist(const char* path)
{
	snprintf(_plistPath, sizeof(_plistPath), "%s", path);
	return loadFromPlist(path);
}


bool ParticleManager::loadFromPlist(const char* path)
{
    if (!_particles || !path || !*path) {
        snprintf(_statusMsg, sizeof(_statusMsg), "Load: bad path");
        return false;
    }

    // 处理加载路径
    std::string realPath = path;
    if (realPath.find('/') == std::string::npos && realPath.find('\\') == std::string::npos) {
        std::string defDir = SimpleFileDialog::GetDefaultDir();
		if (!defDir.empty() && defDir.back() != '/' && defDir.back() != '\\') {
			defDir += '/';
		}
        realPath = defDir + realPath;
    }

    ParticleConfig cfg;
    if (!cfg.loadFromPlist(realPath.c_str())) {
        snprintf(_statusMsg, sizeof(_statusMsg), "Load failed: %s", realPath.c_str());
        return false;
    }

    cfg.applyTo(_particles);
	_particles->setStyle(ParticleExample::NONE);
    // ★ 核心修改：智能解析纹理路径
    std::string plistPath = convertPathFormatToUnixStyle(realPath);
    std::string plistDir = "";
    size_t pos = plistPath.find_last_of('/');
    if (pos != std::string::npos) {
        plistDir = plistPath.substr(0, pos + 1);
    }

    std::string finalTexPath = "";

    // 如果存的路径里包含 '/'，说明是个绝对路径，直接用
    if (cfg.textureFileName.find('/') != std::string::npos || cfg.textureFileName.find('\\') != std::string::npos) {
        finalTexPath = cfg.textureFileName;
    } else {
        // 如果是相对路径，拼在 plist 同级目录下
        finalTexPath = plistDir + cfg.textureFileName;
    }

    // 如果文件不存在，尝试在 GetDefaultDir() 里找（防呆）
    FILE* fp = fopen(finalTexPath.c_str(), "rb");
    if (!fp) {
        std::string defDir = SimpleFileDialog::GetDefaultDir();
        if (defDir.back() != '/' && defDir.back() != '\\') defDir += '/';
        std::string tryPath = defDir + cfg.textureFileName;
        fp = fopen(tryPath.c_str(), "rb");
        if (fp) {
            fclose(fp);
            finalTexPath = tryPath;
        }
    } else {
        fclose(fp);
    }

    snprintf(_texturePath, sizeof(_texturePath), "%s", finalTexPath.c_str());
    loadDragTexture(_texturePath);

    snprintf(_plistPath, sizeof(_plistPath), "%s", realPath.c_str());
    snprintf(_statusMsg, sizeof(_statusMsg), "Loaded: %s", realPath.c_str());
    return true;
}

// ═══════════════════════════════════════════════════════════════
// 更新 / 预览
// ═══════════════════════════════════════════════════════════════
void ParticleManager::update(float dt)
{
	if (!_playing || !_particles) return;
	_particles->update(dt * _timeScale);
}

void ParticleManager::ensurePreviewTexture(int w, int h)
{
	// ★ 防抖：1~4px 抖动不重建 RT
	if (_previewTexture
		&& std::abs(w - (int)_contentRect.w) <= 4
		&& std::abs(h - (int)_contentRect.h) <= 4)
	{
		return;
	}

	if (_previewTexture) { delete _previewTexture; _previewTexture = nullptr; }
	_previewTexture = new Texture2D();

	_contentRect.w = (float)w;
	_contentRect.h = (float)h;
	_particles->setPosition(Vec2(w / 2.0f, h / 2.0f));
	_previewTexture->createTarget(_renderer, (float)w, (float)h);
}

void ParticleManager::renderToPreview()
{
	if (!_previewTexture || !_particles) return;
	_particles->setScale(_zoom);
	_particles->draw(_previewTexture);
}

// ═══════════════════════════════════════════════════════════════
//══════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════
// 主 UI：自适应布局（宽屏三栏，窄屏上下Tab）
// ═══════════════════════════════════════════════════════════════
void ParticleManager::drawUI()
{
	const ImGuiViewport* vp = ImGui::GetMainViewport();
	const ImVec2 pos = vp->WorkPos;
	const ImVec2 sz = vp->WorkSize;

	const float toolbarH = ImGui::GetFrameHeight() + 16.0f;
	
	// 判断是否窄屏（手机竖屏通常 < 900）
	bool isNarrow = (sz.x < 900.0f);

	if (isNarrow)
	{
		// ── 窄屏（手机）布局 ──
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(ImVec2(sz.x, toolbarH));
		ImGui::Begin("##toolbar", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
		drawToolbar();
		ImGui::End();

		float bodyH = sz.y - toolbarH;
		float previewH = bodyH * 0.40f;

		ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + toolbarH));
		ImGui::SetNextWindowSize(ImVec2(sz.x, previewH));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("##preview", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoBringToFrontOnFocus);
		drawPreview();
		ImGui::End();
		ImGui::PopStyleVar();

		float bottomH = bodyH - previewH;
		ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + toolbarH + previewH));
		ImGui::SetNextWindowSize(ImVec2(sz.x, bottomH));
		ImGui::Begin("##bottom_tabs", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBringToFrontOnFocus);
		
		if (ImGui::BeginTabBar("Tabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Properties"))
			{
				drawPropertyEditor();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Files / Presets"))
			{
				drawPresetPanel();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
		
		drawFileDialogs();
		return;
	}

	// ── 宽屏（PC / 平板横屏）走原有三栏布局 ──
	
	// ★ 核心修改：分隔条宽度动态计算，在手机上（大字）能宽达 36 像素，极容易点中
	float kSplitW = ImGui::GetFontSize(); 
	if (kSplitW < 12.0f) kSplitW = 12.0f;

	static float s_leftW = 0.0f;
	static float s_rightW = 0.0f;
	static float s_prevWinW = 0.0f;

	if (s_prevWinW <= 0.0f) {
		s_leftW = sz.x * 0.20f;
		s_rightW = sz.x * 0.30f;
		s_prevWinW = sz.x;
	}
	else if (std::abs(sz.x - s_prevWinW) > 1.0f) {
		float k = sz.x / s_prevWinW;
		s_leftW *= k;
		s_rightW *= k;
		s_prevWinW = sz.x;
	}

	float sideMax = sz.x - 2.0f * kSplitW - 240.0f;
	if (s_leftW + s_rightW > sideMax && s_leftW + s_rightW > 0.0f) {
		float k = sideMax / (s_leftW + s_rightW);
		s_leftW *= k;
		s_rightW *= k;
	}
	if (s_leftW < 160.0f) s_leftW = 160.0f;
	if (s_rightW < 160.0f) s_rightW = 160.0f;

	const float bodyY = pos.y + toolbarH;
	const float bodyH = sz.y - toolbarH;

	// 顶部工具栏
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(ImVec2(sz.x, toolbarH));
	ImGui::Begin("##toolbar", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
	drawToolbar();
	ImGui::End();

	// 左栏
	ImGui::SetNextWindowPos(ImVec2(pos.x, bodyY));
	ImGui::SetNextWindowSize(ImVec2(s_leftW, bodyH));
	ImGui::Begin("##preset_panel", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus);
	drawPresetPanel();
	ImGui::End();

	// 中栏
	const float midX = pos.x + s_leftW + kSplitW;
	const float midW = sz.x - s_leftW - s_rightW - 2.0f * kSplitW;
	ImGui::SetNextWindowPos(ImVec2(midX, bodyY));
	ImGui::SetNextWindowSize(ImVec2(midW, bodyH));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("##preview", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBringToFrontOnFocus);
	drawPreview();
	ImGui::End();
	ImGui::PopStyleVar();

	// 右栏
	const float rightX = pos.x + sz.x - s_rightW;
	ImGui::SetNextWindowPos(ImVec2(rightX, bodyY));
	ImGui::SetNextWindowSize(ImVec2(s_rightW, bodyH));
	ImGui::Begin("##properties", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus);
	drawPropertyEditor();
	ImGui::End();

	// ── 左侧分隔条 ──
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::SetNextWindowPos(ImVec2(pos.x + s_leftW, bodyY));
	ImGui::SetNextWindowSize(ImVec2(kSplitW, bodyH));
	ImGui::Begin("##splitter_l", nullptr, 
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
	
	ImGui::InvisibleButton("##lbtn", ImVec2(kSplitW, bodyH));
	// ★ 画一条虚线或者实线，视觉上指示可以拖拽
	{
		ImVec2 p1 = ImGui::GetWindowPos();
		ImVec2 p2 = ImVec2(p1.x + kSplitW * 0.5f, p1.y + bodyH);
		p1.x += kSplitW * 0.5f;
		ImU32 col = ImGui::GetColorU32((ImGui::IsItemHovered() || ImGui::IsItemActive()) ? ImGuiCol_SeparatorActive : ImGuiCol_Separator);
		ImGui::GetWindowDrawList()->AddLine(p1, p2, col, ImGui::IsItemActive() ? 3.0f : 1.0f);
	}

	if (ImGui::IsItemActive()) {
		s_leftW += ImGui::GetIO().MouseDelta.x;
		if (s_leftW < 160.0f) s_leftW = 160.0f;
		float maxL = sz.x - s_rightW - 2.0f * kSplitW - 240.0f;
		if (s_leftW > maxL) s_leftW = maxL;
	}
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	ImGui::End();
	ImGui::PopStyleVar();

	// ── 右侧分隔条 ──
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::SetNextWindowPos(ImVec2(rightX - kSplitW, bodyY));
	ImGui::SetNextWindowSize(ImVec2(kSplitW, bodyH));
	ImGui::Begin("##splitter_r", nullptr, 
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
	
	ImGui::InvisibleButton("##rbtn", ImVec2(kSplitW, bodyH));
	// ★ 画线指示
	{
		ImVec2 p1 = ImGui::GetWindowPos();
		ImVec2 p2 = ImVec2(p1.x + kSplitW * 0.5f, p1.y + bodyH);
		p1.x += kSplitW * 0.5f;
		ImU32 col = ImGui::GetColorU32((ImGui::IsItemHovered() || ImGui::IsItemActive()) ? ImGuiCol_SeparatorActive : ImGuiCol_Separator);
		ImGui::GetWindowDrawList()->AddLine(p1, p2, col, ImGui::IsItemActive() ? 3.0f : 1.0f);
	}

	if (ImGui::IsItemActive()) {
		s_rightW -= ImGui::GetIO().MouseDelta.x;
		if (s_rightW < 160.0f) s_rightW = 160.0f;
		float maxR = sz.x - s_leftW - 2.0f * kSplitW - 240.0f;
		if (s_rightW > maxR) s_rightW = maxR;
	}
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	ImGui::End();
	ImGui::PopStyleVar();

	drawFileDialogs();
}

// ═══════════════════════════════════════════════════════════════
// 工具栏：完全根据字体动态计算尺寸，适应大字体
// ═══════════════════════════════════════════════════════════════
void ParticleManager::drawToolbar()
{
	ImGui::SetCursorPosY(6.0f);
	ImGui::SetCursorPosX(8.0f);

	float btnW = ImGui::GetFontSize() * 4.0f; // 按钮宽度随字号变宽

	if (ImGui::Button(_playing ? "Pause" : "Play", ImVec2(btnW, 0)))
		_playing = !_playing;

	ImGui::SameLine();
	if (ImGui::Button("Restart", ImVec2(btnW, 0))) {
		if (_particles) _particles->resetSystem();
	}

	ImGui::SameLine();
	ImGui::Text("Time");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0f);
	ImGui::SliderFloat("##timescale", &_timeScale, 0.01f, 10.0f, "%.2fx");

	ImGui::SameLine();
	ImGui::Text("Zoom");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0f);
	ImGui::SliderFloat("##zoom", &_zoom, 0.1f, 10.0f, "%.2fx");

	ImGui::SameLine();
	ImGui::Checkbox("Mouse Emitter", &_mouseAsEmitter);

	// FPS 靠右，防止重叠
	float fpsW = ImGui::GetFontSize() * 7.0f;
	if (ImGui::GetContentRegionAvail().x > fpsW) {
		ImGui::SameLine(ImGui::GetWindowWidth() - fpsW);
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	}
}

// ═══════════════════════════════════════════════════════════════
// 右栏：属性编辑器（解决重叠问题）
// ═══════════════════════════════════════════════════════════════
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

	// ★ 核心修改：输入框占 45% 宽度，右侧留 55% 给文字，绝不重叠
	float availW = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(availW * 0.45f);

	// ─────────────────────────────────────────────
	// Blend
	// ─────────────────────────────────────────────
	bool additive = (p->getBlendFunc() == BlendFunc::ADDITIVE);
	bool alpha_premultiplied = (p->getBlendFunc() == BlendFunc::ALPHA_PREMULTIPLIED);

	if (ImGui::Checkbox("Additive", &additive)) {
		if (additive)
			p->setBlendFunc(BlendFunc::ADDITIVE);
	}
	if (ImGui::Checkbox("Premultiplied Alpha", &alpha_premultiplied)) {
		if (alpha_premultiplied)
			p->setBlendFunc(BlendFunc::ALPHA_PREMULTIPLIED);
	}

	// ─────────────────────────────────────────────
	// General
	// ─────────────────────────────────────────────
	if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
	{
		int maxP = p->getTotalParticles();
		if (ImGui::DragInt("Max Particles", &maxP, 1, 1, 50000))
			p->setTotalParticles(maxP);

		float dur = p->getDuration();
		if (ImGui::DragFloat("Duration", &dur, 0.05f, -1.0f, 10000.0f, "%.2f"))
			p->setDuration(dur);
		ImGui::SameLine();
		ImGui::TextDisabled("(-1=inf)");

		float er = p->getEmissionRate();
		if (ImGui::DragFloat("Emission Rate", &er, 1.0f, 0.0f, 50000.0f, "%.1f"))
			p->setEmissionRate(er);
	}

	// ─────────────────────────────────────────────
	// Emitter
	// ─────────────────────────────────────────────
	if (ImGui::CollapsingHeader("Emitter", ImGuiTreeNodeFlags_DefaultOpen))
	{
		int mode = (int)p->getEmitterMode();
		const char* modes[] = { "Gravity", "Radius" };
		if (ImGui::Combo("Emitter Mode", &mode, modes, 2))
			p->setEmitterMode((ParticleSystem::Mode)mode);

		float angle = p->getAngle();
		if (ImGui::DragFloat("Angle", &angle, 1.0f, -360.0f, 360.0f))
			p->setAngle(angle);

		float angleVar = p->getAngleVar();
		if (ImGui::DragFloat("Angle Var", &angleVar, 1.0f, 0.0f, 360.0f))
			p->setAngleVar(angleVar);

		Vec2 pos = p->getPosition();
		float pv[2] = { pos.x, pos.y };
		if (ImGui::DragFloat2("Position", pv, 1.0f, -50000.0f, 50000.0f))
			p->setPosition(Vec2(pv[0], pv[1]));

		Vec2 posVar = p->getPosVar();
		float pvv[2] = { posVar.x, posVar.y };
		if (ImGui::DragFloat2("Position Var", pvv, 1.0f, 0.0f, 20000.0f))
			p->setPosVar(Vec2(pvv[0], pvv[1]));
	}

	// ─────────────────────────────────────────────
	// Gravity / Radius 模式
	// ─────────────────────────────────────────────
	if (p->getEmitterMode() == ParticleSystem::Mode::GRAVITY)
	{
		if (ImGui::CollapsingHeader("Gravity Mode", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Vec2 g = p->getGravity();
			float gv[2] = { g.x, g.y };
			if (ImGui::DragFloat2("Gravity", gv, 1.0f, -50000.0f, 50000.0f))
				p->setGravity(Vec2(gv[0], gv[1]));

			float speed = p->getSpeed();
			if (ImGui::DragFloat("Speed", &speed, 1.0f, -50000.0f, 50000.0f))
				p->setSpeed(speed);

			float speedVar = p->getSpeedVar();
			if (ImGui::DragFloat("Speed Var", &speedVar, 1.0f, 0.0f, 50000.0f))
				p->setSpeedVar(speedVar);

			float ra = p->getRadialAccel();
			if (ImGui::DragFloat("Radial Accel", &ra, 1.0f, -50000.0f, 50000.0f))
				p->setRadialAccel(ra);

			float raVar = p->getRadialAccelVar();
			if (ImGui::DragFloat("Radial Accel Var", &raVar, 1.0f, 0.0f, 50000.0f))
				p->setRadialAccelVar(raVar);

			float ta = p->getTangentialAccel();
			if (ImGui::DragFloat("Tangential Accel", &ta, 1.0f, -50000.0f, 50000.0f))
				p->setTangentialAccel(ta);

			float taVar = p->getTangentialAccelVar();
			if (ImGui::DragFloat("Tangential Accel Var", &taVar, 1.0f, 0.0f, 50000.0f))
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
			if (ImGui::DragFloat("Start Radius", &sr, 1.0f, 0.0f, 50000.0f))
				p->setStartRadius(sr);

			float srVar = p->getStartRadiusVar();
			if (ImGui::DragFloat("Start Radius Var", &srVar, 1.0f, 0.0f, 50000.0f))
				p->setStartRadiusVar(srVar);

			float er2 = p->getEndRadius();
			if (ImGui::DragFloat("End Radius", &er2, 1.0f, -1.0f, 50000.0f))
				p->setEndRadius(er2);

			float erVar = p->getEndRadiusVar();
			if (ImGui::DragFloat("End Radius Var", &erVar, 1.0f, 0.0f, 50000.0f))
				p->setEndRadiusVar(erVar);

			float rps = p->getRotatePerSecond();
			if (ImGui::DragFloat("Rotate/Sec", &rps, 1.0f, -36000.0f, 36000.0f))
				p->setRotatePerSecond(rps);

			float rpsVar = p->getRotatePerSecondVar();
			if (ImGui::DragFloat("Rotate/Sec Var", &rpsVar, 1.0f, 0.0f, 36000.0f))
				p->setRotatePerSecondVar(rpsVar);
		}
	}

	// ─────────────────────────────────────────────
	// Particle
	// ─────────────────────────────────────────────
	if (ImGui::CollapsingHeader("Particle", ImGuiTreeNodeFlags_DefaultOpen))
	{
		float ss = p->getStartSize();
		if (ImGui::DragFloat("Start Size", &ss, 1.0f, 0.0f, 20000.0f))
			p->setStartSize(ss);

		float ssVar = p->getStartSizeVar();
		if (ImGui::DragFloat("Start Size Var", &ssVar, 1.0f, 0.0f, 20000.0f))
			p->setStartSizeVar(ssVar);

		float es = p->getEndSize();
		if (ImGui::DragFloat("End Size", &es, 1.0f, -1.0f, 20000.0f))
			p->setEndSize(es);
		ImGui::SameLine();
		ImGui::TextDisabled("(-1=same)");

		float esVar = p->getEndSizeVar();
		if (ImGui::DragFloat("End Size Var", &esVar, 1.0f, 0.0f, 20000.0f))
			p->setEndSizeVar(esVar);

		float life = p->getLife();
		if (ImGui::DragFloat("Life", &life, 0.01f, 0.001f, 10000.0f, "%.3f"))
			p->setLife(life);

		float lifeVar = p->getLifeVar();
		if (ImGui::DragFloat("Life Var", &lifeVar, 0.01f, 0.0f, 10000.0f, "%.3f"))
			p->setLifeVar(lifeVar);

		float sspin = p->getStartSpin();
		if (ImGui::DragFloat("Start Spin", &sspin, 1.0f, -36000.0f, 36000.0f))
			p->setStartSpin(sspin);

		float sspinVar = p->getStartSpinVar();
		if (ImGui::DragFloat("Start Spin Var", &sspinVar, 1.0f, 0.0f, 36000.0f))
			p->setStartSpinVar(sspinVar);

		float espin = p->getEndSpin();
		if (ImGui::DragFloat("End Spin", &espin, 1.0f, -36000.0f, 36000.0f))
			p->setEndSpin(espin);

		float espinVar = p->getEndSpinVar();
		if (ImGui::DragFloat("End Spin Var", &espinVar, 1.0f, 0.0f, 36000.0f))
			p->setEndSpinVar(espinVar);
	}

	// ─────────────────────────────────────────────
	// Color
	// ─────────────────────────────────────────────
	if (ImGui::CollapsingHeader("Color", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto EditColor = [&](const char* label, Color4F cur,
			void (ParticleSystem::*setter)(const Color4F&))
		{
			float v[4] = { cur.r, cur.g, cur.b, cur.a };
			if (ImGui::ColorEdit4(label, v,
				ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf))
			{
				(p->*setter)(Color4F{ v[0], v[1], v[2], v[3] });
			}
		};
		EditColor("Start Color", p->getStartColor(), &ParticleSystem::setStartColor);
		EditColor("Start Color Var", p->getStartColorVar(), &ParticleSystem::setStartColorVar);
		EditColor("End Color", p->getEndColor(), &ParticleSystem::setEndColor);
		EditColor("End Color Var", p->getEndColorVar(), &ParticleSystem::setEndColorVar);
	}

	// ─────────────────────────────────────────────
	// Behavior
	// ─────────────────────────────────────────────
	if (ImGui::CollapsingHeader("Behavior"))
	{
		int pt = (int)p->getPositionType();
		const char* pts[] = { "FREE", "RELATIVE", "GROUPED" };
		if (ImGui::Combo("Position Type", &pt, pts, 3))
			p->setPositionType((ParticleSystem::PositionType)pt);

		bool paused = p->isPaused();
		if (ImGui::Checkbox("Pause Emission", &paused)) {
			if (paused) p->pauseEmissions();
			else        p->resumeEmissions();
		}

		bool active = p->isActive();
		if (ImGui::Checkbox("Active", &active)) {
			if (active) p->start();
			else        p->stop();
		}
	}

	ImGui::PopItemWidth();
	ImGui::EndChild();
}
// ═══════════════════════════════════════════════════════════════
// 左栏：类型 + 纹理 + plist
// ═══════════════════════════════════════════════════════════════
void ParticleManager::drawPresetPanel()
{
	ParticleExample* p = _particles;
	if (!p) return;

	ImGui::PushItemWidth(-1);

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

	// ── Texture ──
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Texture");
	ImGui::InputText("##texPath", _texturePath, sizeof(_texturePath));

	float avail = ImGui::GetContentRegionAvail().x;
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	float halfW = (avail - spacing) * 0.5f;

	if (ImGui::Button("Browse...", ImVec2(halfW, 0)))
		_openTextureDialog = true;
	ImGui::SameLine();
	if (ImGui::Button("Load##tex", ImVec2(halfW, 0)))
		loadTexture(_texturePath);
	if (ImGui::Button("Clear##tex", ImVec2(-1, 0)))
		resetTexture();

	// ── Plist ──
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Plist");
	ImGui::InputText("##plistPath", _plistPath, sizeof(_plistPath));

	if (ImGui::Button("Browse...##plist", ImVec2(halfW, 0)))
		_openPlistDialog = true;
	ImGui::SameLine();
	if (ImGui::Button("Load##plist", ImVec2(halfW, 0)))
		loadFromPlist(_plistPath);
	if (ImGui::Button("Save##plist", ImVec2(-1, 0)))
		saveToPlist(_plistPath);

	// ── Status ──
	if (_statusMsg[0]) {
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextWrapped("%s", _statusMsg);
	}

	ImGui::PopItemWidth();
}

// ═══════════════════════════════════════════════════════════════
// 中栏：预览
// ═══════════════════════════════════════════════════════════════
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
		ImGui::Image((ImTextureID)(intptr_t)tex, avail, ImVec2(0, 0), ImVec2(1, 1));
	else
		ImGui::Dummy(avail);

	ImVec2 mouse = ImGui::GetMousePos();
	ImVec2 imgMin = ImGui::GetItemRectMin();
	ImVec2 imgMax = ImGui::GetItemRectMax();

	float relX = mouse.x - imgMin.x;
	float relY = mouse.y - imgMin.y;
	bool inside = (relX >= 0 && relY >= 0 && relX < avail.x && relY < avail.y);

	float scaleX = (float)w / _contentRect.w;
	float scaleY = (float)h / _contentRect.h;
	if (scaleX <= 0) scaleX = 1;
	if (scaleY <= 0) scaleY = 1;

	float texX = relX / scaleX;
	float texY = relY / scaleY;

	// 鼠标当发射器
	if (_mouseAsEmitter && inside &&
		(ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
			ImGui::IsMouseDown(ImGuiMouseButton_Right)))
	{
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && _particles) {
			_particles->setPosition(Vec2(texX, texY));
		}
		if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && _particles) {
			_particles->setPosition(Vec2(texX, texY));
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
		texX, texY);

	dl->AddText(ImVec2(imgMin.x + 8, imgMin.y + 8),
		IM_COL32(255, 220, 100, 255), buf);

	if (inside) {
		dl->AddLine(ImVec2(imgMin.x + relX - 10, imgMin.y + relY),
			ImVec2(imgMin.x + relX + 10, imgMin.y + relY),
			IM_COL32(255, 100, 100, 180));
		dl->AddLine(ImVec2(imgMin.x + relX, imgMin.y + relY - 10),
			ImVec2(imgMin.x + relX, imgMin.y + relY + 10),
			IM_COL32(255, 100, 100, 180));
	}
}
