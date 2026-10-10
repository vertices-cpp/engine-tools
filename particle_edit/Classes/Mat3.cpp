#include "Mat3.h"


//Mat3 Mat3::operator*(const Mat3 & r)const
//{
//	Mat3 R;
//	R.a = a * r.a + b * r.c;
//	R.b = a * r.b + b * r.d;
//	R.c = c * r.a + d * r.c;
//	R.d = c * r.b + d * r.d;
//	R.tx = a * r.tx + b * r.ty + tx;
//	R.ty = c * r.tx + d * r.ty + ty; 
//
//	return R;
//}
 
Mat3  Mat3::operator*(const Mat3 & r)const
{
	Mat3 R;
	R.a = a * r.a + c * r.b;
	R.b = b * r.a + d * r.b;
	R.c = a * r.c + c * r.d;
	R.d = b * r.c + d * r.d;
	R.tx = a * r.tx + c * r.ty + tx;
	//cout << "开始\nx=" << a << " * " << r.tx << "+" << c << " * " << r.ty << " +" << tx << endl;
	R.ty = b * r.tx + d * r.ty + ty;
// 	cout << "y=" << b << " * " << r.tx << "+" << d << " * " << r.ty << " +" << ty << endl;
// 	cout << "(" << *this << " )(" << r << ")" << endl;
	return R;
}


Mat3 Mat3::createRotationSDL(Vec2 center, Vec2 rotate)
{
	float radX = -(rotate.x * 0.01745329252f);
	float radY = -(rotate.y * 0.01745329252f);

	Mat3 R;

	R.a = cosf(radY); R.c = -sinf(radY);
	R.b =  sinf(radX); R.d = cosf(radX);
	 

	Mat3 T_center = createTranslation(center.x, center.y);
	Mat3 T_neg = createTranslation(-center.x, -center.y);

//	return   R  ;
 	return T_center * R * T_neg;
}


Mat3 Mat3::createRotation(Vec2 center, float angle) {
	float rad = angle * 0.01745329252f;
	float c = cosf(rad), s = sinf(rad);
	Mat3 R;
	R.a = c; R.c = -s;
	R.b = s; R.d = c;
	R.tx = R.ty = 0.0f;


	Mat3  T_top = createTranslation(center.x, center.y);
	Mat3 T_neg = createTranslation(-center.x, -center.y);

	return T_top * R * T_neg;

}

// bool Mat3::inverse()
// {
// 	float det = a * d - b * c;
// 	if (std::fabs(det) < 1e-6f)	// 阈值可根据需求调整
// 		return false;
// 
// 	float invDet = 1.0f / det;
// 	float new_a = d * invDet;
// 	float new_b = -b * invDet;
// 	float new_c = -c * invDet;
// 	float new_d = a * invDet;
// 	float new_tx = (b * ty - d * tx) * invDet;
// 	float new_ty = (c * tx - a * ty) * invDet;
// 
// 	a = new_a;
// 	b = new_b;
// 	c = new_c;
// 	d = new_d;
// 	tx = new_tx;
// 	ty = new_ty;
// 	return true;
// }


bool Mat3::inverse()
{
	float det = a * d - b * c; 
		if (std::fabs(det) < 1e-6f)
			return false; 

			float invDet = 1.0f / det; 
			float new_a = d * invDet; 
			float new_b = -b * invDet; 
			float new_c = -c * invDet; 
			float new_d = a * invDet; 

			// 列向量右乘体系下的正确平移逆变换[cite: 4]
			float new_tx = (c * ty - d * tx) * invDet;
	float new_ty = (b * tx - a * ty) * invDet;

	a = new_a; 
		b = new_b; 
		c = new_c; 
		d = new_d; 
		tx = new_tx; 
		ty = new_ty; 
		return true; 
}


Mat3 Mat3::getInversed() const
{
	Mat3 mat(*this);
	mat.inverse();
	return mat;
}

const Mat3  Mat3::IDENTITY = Mat3(
	1.0f, 0.0f,
	0.0f, 1.0f,
	0.0f, 0.0f);
