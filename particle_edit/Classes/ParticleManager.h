#pragma once
#include "imgui.h"
#include "ParticleExamples.h"

struct SDL_Renderer;
struct SDL_Texture;
class Texture2D;

class ParticleManager {
public:
    static ParticleManager* create();
    static void destroy();
    Rect _contentRect;

    void init(SDL_Renderer* ren, int w, int h);
    ~ParticleManager();

    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

    void update(float dt);
    void drawUI();
  
	ParticleExample *getPexample() { return _particles; }
private:
    ParticleManager();

    SDL_Renderer*    _renderer = nullptr;
    ParticleExample* _particles = nullptr;

    bool  _playing = true;
    float _timeScale = 1.0f;
    float _zoom = 1.0f;
    bool  _showGrid = false;

    Texture2D* _previewTexture = nullptr;

    void ensurePreviewTexture(int w, int h);
    void renderToPreview();

    // ---------- 路径 + 状态 ----------
public:
    char _texturePath[512] = "fire.png";
    char _plistPath[512] = "particle.plist";
    char _statusMsg[256] = "";
private:

    // ---------- 三个接口 ----------
    void applyPreset(ParticleExample::PatticleStyle style);
public:
	bool loadDragTexture(const char * path);
	bool loadTexture(const char* path);      // ★ 图片路径
private:
    void drawFileDialogs();
    bool resetTexture();
 //   void drawGrid();
public:
 
	bool loadDragPlist(const char * path);
	bool loadFromPlist(const char* path);    // ★ plist 加载
private:
    bool saveToPlist(const char* path);      // ★ plist 保存

    // ---------- UI ----------
    bool _openTextureDialog = false;   // ★ drawPresetPanel 里置位
    bool _openPlistDialog = false;   // ★
    bool _mouseAsEmitter = false;   // ★ 鼠标位置当发射点

    void drawToolbar();
    void drawPresetPanel();     // 左栏：类型 + 图片 + plist
    void drawPreview();         // 中栏
    void drawPropertyEditor();  // 右栏
};
