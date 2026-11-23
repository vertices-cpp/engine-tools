#ifndef _MAT3_H_
#define _MAT3_H_

#include <cmath>
#include <iostream>
#include "Type.h"

#define MATH_DEG_TO_RAD(x)          ((x) * 0.0174532925f)
#define MATH_RAD_TO_DEG(x)          ((x)* 57.29577951f)
 
struct Mat3
{
	static const Mat3 IDENTITY;
	float a, b,					// 0 1 2
		c, d,					// 3 4 5
		tx, ty;					// 6 7 8
	Mat3() :a(1.0f), b(0.0f), c(0.0f), d(1.0f), tx(0.0f), ty(0.0f) {}
	Mat3(float a, float b, float c, float d, float tx, float ty)
		: a(a), b(b), c(c), d(d), tx(tx), ty(ty) {}

	Mat3(const Mat3& r) :a(r.a), b(r.b), c(r.c), d(r.d), tx(r.tx), ty(r.ty) {}

	void reset() {
		*this = Mat3::IDENTITY;
	}
	 
	Mat3 operator*(const Mat3 & r)const;
	static Mat3 createScale(float sx, float sy) {
		Mat3 S;
		S.a = sx; S.b = 0.0f;
		S.c = 0.0f; S.d = sy;
		S.tx = S.ty = 0.0f;
		return S;
	}
 
	static Mat3 createTranslation(float x, float y)
	{
		Mat3 R;
		R.tx = x, R.ty = y;
		return R;
	}
	// SDL2 (Y 轴向下) 的标准旋转矩阵构造
	static  Mat3 createRotationSDL(Vec2 center, Vec2 rotate);
	static Mat3 createRotation(Vec2 center, float angle);
	 

	inline void  transformPoint(const Vec2& point, Vec2* dst) const
	{
		Mat3::transformVec3((float*)this, point.x, point.y, (float*)dst);
	}
	static void  transformVec3(const float* m, float x, float y, float* dst)
	{
		dst[0] = x * m[0] + y * m[2] + m[4];
		dst[1] = x * m[1] + y * m[3] + m[5];
	}

// 	static Mat3 createSkew(float skewX, float skewY) // 单位：度
// 	{
// 		float radX = skewX * 0.01745329252f;
// 		float radY = skewY * 0.01745329252f;
// 
// 		Mat3 mat;
// 		// 1 和 4 位置保持 1，表示不缩放
// 		mat.a = 1.0f;
// 		mat.d = 1.0f;
// 		// 2 位置和 3 位置填入正切值（注意符号，通常 SDL 的 Y 轴向下，但倾斜公式不变）
// 		mat.b = tanf(radY); // SkewY（影响 X 方向的切变）
// 		mat.c = tanf(radX); // SkewX（影响 Y 方向的切变）
// 		return mat;
// 	}
	static Mat3 createSkew(float skewX, float skewY) // 单位：度 
	{
		float radX = skewX * 0.01745329252f;  
			float radY = skewY * 0.01745329252f;  

			Mat3 mat; 
			mat.a = 1.0f;  
			mat.d = 1.0f;  

			// skewX 影响 X 轴向 Y 轴的倾斜程度 (Y 分量加上 X * tan(radX))
			// skewY 影响 Y 轴向 X 轴的倾斜程度 (X 分量加上 Y * tan(radY))
			mat.c = tanf(radY); // SkewY 
		mat.b = tanf(radX); // SkewX 
		return mat; 
	}
 
	bool inverse();
	Mat3 getInversed() const;
	void out(Vec2* ret)const
	{
		out(ret->x, ret->y);
	}
	void out(float &x, float &y)const
	{
		float oldX = x, oldY = y;
		x = a * oldX + c * oldY + tx;	// 必须乘 oldY
		y = b * oldX + d * oldY + ty;	// 必须乘 oldX
	}

	friend std::ostream & operator<<(std::ostream & os, const Mat3 & r)
	{
		os <<"a:"<< r.a << " b:" << r.b << " c:" << r.c << " d:" << r.d << " tx=" << r.tx << " ty=" << r.
			ty << " \n";
		return os;
	}

};
 

#endif
