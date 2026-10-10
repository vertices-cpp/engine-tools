
// 只在一个 cpp 里定义 STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#include "Texture2D.h" 
#include <cassert>

#define OG_ENABLE_PREMULTIPLIED_ALPHA 1

const BlendFunc BlendFunc::DISABLE = { BlendFactor::ONE, BlendFactor::ZERO };
const BlendFunc BlendFunc::ALPHA_PREMULTIPLIED = { BlendFactor::ONE, BlendFactor::ONE_MINUS_SRC_ALPHA };
const BlendFunc BlendFunc::ALPHA_NON_PREMULTIPLIED = { BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA };
const BlendFunc BlendFunc::ADDITIVE = { BlendFactor::SRC_ALPHA, BlendFactor::ONE };

Texture2D::Texture2D() :_texture(nullptr), _imageSize(Size::ZERO), mPitch(0), mPixels(nullptr)
{
}


Texture2D::~Texture2D()
{
	free();
}

void Texture2D::free()
{
	if (_texture != nullptr)
	{
		SDL_DestroyTexture(_texture);
		_texture = nullptr;
	}
}
void Texture2D::setRGBA(Color4B color)
{
	//调制纹理rgb
	SDL_SetTextureColorMod(_texture, color.r, color.g, color.b);
	SDL_SetTextureAlphaMod(_texture, color.a);
}

void Texture2D::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
	//调制纹理rgb
	SDL_SetTextureColorMod(_texture, red, green, blue);
}
void Texture2D::SetTextureAlphaMod(Uint8 a) {
	SDL_SetTextureAlphaMod(_texture, a);
}

void Texture2D::setBlendMode(SDL_BlendMode blending)
{
	SDL_SetTextureBlendMode(_texture, blending);
}


bool Texture2D::loadMemData(SDL_Renderer* renderer, unsigned char* data, int len)
{
	free();

	int w = 0, h = 0, comp = 0;
	// 强制 4 通道 = RGBA
	unsigned char* pixels = stbi_load_from_memory(data, len, &w, &h, &comp, 4);
	if (!pixels) {
		printf("stbi_load_from_memory failed: %s\n", stbi_failure_reason());
		return false;
	}

	// 预乘 alpha（直接操作 RGBA 字节）
#if OG_ENABLE_PREMULTIPLIED_ALPHA != 0
	const int total = w * h;
	for (int i = 0; i < total; ++i) {
		unsigned char* p = pixels + i * 4;   // R,G,B,A
		unsigned char a = p[3];
		p[0] = (unsigned char)((p[0] * a) / 255);
		p[1] = (unsigned char)((p[1] * a) / 255);
		p[2] = (unsigned char)((p[2] * a) / 255);

	}
	_hasPremultipliedAlpha = true;
#else
	_hasPremultipliedAlpha = false;   // ★ 显式标记
#endif


	// stb 给的内存顺序是 R,G,B,A —— 对应 SDL 的 ABGR8888
	_texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_STATIC,
		w, h);
	if (!_texture) {
		stbi_image_free(pixels);
		return false;
	}

	SDL_SetTextureBlendMode(_texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(_texture, nullptr, pixels, w * 4);

	_imageSize = Size((float)w, (float)h);

	stbi_image_free(pixels);   // 必须用 stbi_image_free，不是 free
	return true;
}
// Texture2D.cpp
bool Texture2D::createTarget(SDL_Renderer* renderer, float w, float h)
{
	free();   // 释放旧的

	_texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,   // 关键
		w, h);

	if (!_texture) return false;

	SDL_SetTextureBlendMode(_texture, SDL_BLENDMODE_BLEND);
	_imageSize.w = (float)w;
	_imageSize.h = (float)h;
	return true;
}
void Texture2D::createTexture(SDL_Renderer* renderer, Texture2D* texture, const Rect & rect)
{

	auto getTexture = texture;

	float w = rect.w, h = rect.h;

	SDL_Texture *tex = SDL_GetRenderTarget(renderer);

	_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, (int)w, (int)h);
	SDL_SetTextureBlendMode(_texture, SDL_BLENDMODE_BLEND);

	// 1. 保存原渲染目标
	SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);

	SDL_SetRenderTarget(renderer, _texture);
	//创建目标纹理
	Rect dest = { 0,0,w,h };
	
	Vec2 center = { rect.w / 2,rect.h / 2 };
 
	getTexture->render(renderer,rect, dest, Vec2(0,0),
		center, Color4B(255, 255, 255, 255), false, false);
	// 	SDL_Rect rrr = { 0,0,w,h };
	// 	SDL_Rect ddd = { 0,0,w,h };
	//	SDL_RenderCopyEx(SDLView::getInstance()->getRender(), getTexture.get()->getTexture(),  &rrr, &ddd, 0, 0,SDL_FLIP_NONE);
	_imageSize.w = w;
	_imageSize.h = h;

	// 4. 切到新目标，把源纹理渲染上来
	SDL_SetRenderTarget(renderer, oldTarget);
}

void Texture2D::render(SDL_Renderer* renderer, const Rect& clip, const Rect &dest, Vec2 rotate, const Vec2 &pointer, const Color4B &color, bool flipX, bool flipY)
{
	quad.TextureCvRenderer(renderer, _texture, _imageSize, clip, dest, rotate, pointer, color, flipX, flipY);
}
