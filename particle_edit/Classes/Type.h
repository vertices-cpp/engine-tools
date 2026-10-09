
#ifndef _TYPE_H_
#define _TYPE_H_

#include <algorithm>
#include <cassert>

struct  Vec2 {
    float x, y;
    Vec2():x(0.f),y(0.0f){}
    Vec2(float x_,float y_):x(x_),y(y_){}
    inline float getAngle() const {
        return atan2f(y, x);
    }
    inline Vec2& Vec2::operator+=(const Vec2& v)
    {
        add(v);
        return *this;
    }
    inline void  add(const Vec2& v)
    {
        x += v.x;
        y += v.y;
    }
    inline Vec2  operator-(const Vec2& v) const
    {
        Vec2 result(*this);
        result.subtract(v);
        return result;
    }
    inline void  subtract(const Vec2& v)
    {
        x -= v.x;
        y -= v.y;
    }
    inline void  set(float xx, float yy)
    {
        this->x = xx;
        this->y = yy;
    }
    Vec2 operator*(const Vec2 &rhs)
    {
        return Vec2(x * rhs.x, y * rhs.y);
    }
    inline Vec2 Vec2::operator*(float s) const
    {
        Vec2 result(*this);
        result.scale(s);
        return result;
    }
    inline void  scale(float scalar)
    {
        x *= scalar;
        y *= scalar;
    }
    static const Vec2 ZERO;
};
inline float clampf(float value, float min_inclusive, float max_inclusive)
{
    if (min_inclusive > max_inclusive) {
        std::swap(min_inclusive, max_inclusive);
    }
    return value < min_inclusive ? min_inclusive : value < max_inclusive ? value : max_inclusive;
}
 

struct  Size {
    float w, h;
    Size():w(0.0f), h(0.0f){}
    Size(float w_, float h_) :w(w_), h(h_) {}
    Size Size::operator-(const Size& right) const
    {
        return Size(this->w - right.w, this->h - right.h);
    }
    Size Size::operator/(float a) const
    {
        //	CCASSERT(a!=0, "CCSize division by 0.");
        assert(a != 0);
        return Size(this->w / a, this->h / a);
    }
     operator Vec2()const
    {
        return Vec2(w, h);
    }
    static const Size ZERO;
}; 
struct Rect {
    float x, y, w, h;
    Rect() { setRect(0.0f, 0.0f, 0.0f, 0.0f); }
    Rect(float x, float y, float width, float height)
    {
        setRect(x, y, width, height);
    }
    Rect::Rect(const Rect& other)
    {
        setRect(other. x, other. y, other. w, other. h);
    }
    void setRect(float x_, float y_, float width, float height)
    {
        // CGRect can support width<0 or height<0
        ////CCASSERT(width >= 0.0f && height >= 0.0f, "width and height of Rect must not less than 0.");

         x = x_;
         y = y_;

        w = width;
         h = height;
    }
    Rect& operator=(const Size &rhs)
    {
        w = rhs.w;
        h = rhs.h;
        return *this;
    }
    static const Rect ZERO;
}; 

struct  Color4F {
    float r, g, b, a;
};


struct  Color4B
{
    inline void set(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    Color4B::Color4B()
    {}

    Color4B::Color4B(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
        : r(_r)
        , g(_g)
        , b(_b)
        , a(_a)
    {}
    
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0; 
};
struct  Tex2F {
    Tex2F(float _u, float _v) : u(_u), v(_v) {}

    Tex2F() {}

    float u = 0.f;
    float v = 0.f;
};
struct V2F_C4B_T2F
{
    /// vertices (2F)
    Vec2       vertices;
    /// colors (4B)
    Color4B        colors;
    /// tex coords (2F)
    Tex2F          texCoords;
};
struct  V2F_C4B_T2F_Quad
{
    /// top left
    V2F_C4B_T2F    tl;
    /// top right
    V2F_C4B_T2F    tr;
    /// bottom left
    V2F_C4B_T2F    bl;
    /// bottom right
    V2F_C4B_T2F    br;

};

#endif
