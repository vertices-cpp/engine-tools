 
#include "Type.h"
  inline Vec2 Vec2::operator*(const Size &rhs)
    {
        return Vec2(x * rhs.w, y * rhs.h);
    }

const Vec2 Vec2::ZERO(0.0f, 0.0f);
const Size Size::ZERO(0.0f, 0.0f); 
const Rect Rect::ZERO(0.0f, 0.0f,0.0f, 0.0f);
