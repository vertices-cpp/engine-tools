#ifndef _TEXTURE_2D_H_
#define _TEXTURE_2D_H_

//#include "PlatformMacros.h"
#include "Type.h"
#if defined(_WIN32)
#include <SDL.h>
 
#else
#include <SDL2/SDL.h>
 
#endif
#include <algorithm>
//SDL 2.06版本之前没有只能用这个近似代替
enum class BlendFunc
{
	DISABLE = SDL_BLENDMODE_NONE,
	ALPHA_PREMULTIPLIED = SDL_BLENDMODE_BLEND,
	ALPHA_NON_PREMULTIPLIED = SDL_BLENDMODE_BLEND,
	ADDITIVE = SDL_BLENDMODE_ADD

};

//typedef char Uint8;

template<class _Ty = float>
void og_swap(_Ty& _Left, _Ty& _Right)
{
	_Ty _Tmp = _Left;
	_Left = _Right;
	_Right = _Tmp;
}

struct SDL_Texture;

class Texture2D
{ 
	SDL_Texture * _texture;
	Size _imageSize;

	int mPitch;
	void* mPixels;
 	bool _hasPremultipliedAlpha;
public:
	Texture2D();
	~Texture2D();
	//void setRenderer(SDL_Renderer *ren) { renderer = ren; }
	void free();
	void setRGBA(Color4B color);
	void setColor(Uint8 r, Uint8 g, Uint8 b);
 	void SetTextureAlphaMod(Uint8 a);
	void setMultipliedAlpha(bool malpha) {
		_hasPremultipliedAlpha = malpha;
	}
	bool hasPremultipliedAlpha() {
		return _hasPremultipliedAlpha;
	}
	void setBlendMode(BlendFunc blendFunc);

	SDL_Texture* getTexture() { return _texture; }
	Size getImageSize() { return _imageSize; }

	bool loadMemData(SDL_Renderer* renderer, unsigned char * data, int len);
// 支持直接加载原始像素数据 (不用经过 stb_image 解码)
bool loadRawPixels(SDL_Renderer* renderer, const unsigned char* pixels, int w, int h);
	bool createTarget(SDL_Renderer* renderer, float w, float h);

	void createTexture(SDL_Renderer* renderer, Texture2D* texture, const Rect &r);

	void render(SDL_Renderer* renderer, const Rect& clip, const Rect &dest, Vec2 rotate, const Vec2 &pointer, const Color4B &color, bool flipX, bool flipY);
	void render(SDL_Renderer *renderer,SDL_Rect * dest, SDL_Rect * clip = nullptr, double degress = 0.0, SDL_Point * pointer = nullptr, SDL_RendererFlip flip = SDL_FLIP_NONE);

private:
	Texture2D(const Texture2D &) = delete;
	Texture2D &operator =(const Texture2D &) = delete;
};



#endif
