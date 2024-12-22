#ifndef MAT44_HPP_E7187A26_469E_48AD_A3D2_63150F05A4CA
#define MAT44_HPP_E7187A26_469E_48AD_A3D2_63150F05A4CA
// SOLUTION_TAGS: gl-(ex-[^12]|cw-2)

#include <cmath>
#include <cassert>
#include <cstdlib>

#include "vec3.hpp"
#include "vec4.hpp"

/** Mat44f: 4x4 matrix with floats
 *
 * See vec2f.hpp for discussion. Similar to the implementation, the Mat44f is
 * intentionally kept simple and somewhat bare bones.
 *
 * The matrix is stored in row-major order (careful when passing it to OpenGL).
 *
 * The overloaded operator () allows access to individual elements. Example:
 *    Mat44f m = ...;
 *    float m12 = m(1,2);
 *    m(0,3) = 3.f;
 *
 * The matrix is arranged as:
 *
 *   ⎛ 0,0  0,1  0,2  0,3 ⎞
 *   ⎜ 1,0  1,1  1,2  1,3 ⎟
 *   ⎜ 2,0  2,1  2,2  2,3 ⎟
 *   ⎝ 3,0  3,1  3,2  3,3 ⎠
 */
struct Mat44f
{
	float v[16];

	constexpr
	float& operator() (std::size_t aI, std::size_t aJ) noexcept
	{
		assert( aI < 4 && aJ < 4 );
		return v[aI*4 + aJ];
	}
	constexpr
	float const& operator() (std::size_t aI, std::size_t aJ) const noexcept
	{
		assert( aI < 4 && aJ < 4 );
		return v[aI*4 + aJ];
	}
};

// Identity matrix
constexpr Mat44f kIdentity44f = { {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
} };

// Common operators for Mat44f.
// Note that you will need to implement these yourself.

constexpr
Mat44f operator*(Mat44f const& aLeft, Mat44f const& aRight) noexcept
{
	Mat44f mat{};
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				mat(i, j) += aLeft(i, k) * aRight(k, j);
			}
		}
	}
	return mat;
}

constexpr
Vec4f operator*(Mat44f const& aLeft, Vec4f const& aRight) noexcept
{
	return {
		aLeft(0, 0) * aRight.x + aLeft(0, 1) * aRight.y + aLeft(0, 2) * aRight.z + aLeft(0, 3) * aRight.w,
		aLeft(1, 0) * aRight.x + aLeft(1, 1) * aRight.y + aLeft(1, 2) * aRight.z + aLeft(1, 3) * aRight.w,
		aLeft(2, 0) * aRight.x + aLeft(2, 1) * aRight.y + aLeft(2, 2) * aRight.z + aLeft(2, 3) * aRight.w,
		aLeft(3, 0) * aRight.x + aLeft(3, 1) * aRight.y + aLeft(3, 2) * aRight.z + aLeft(3, 3) * aRight.w,
	};
}

// Functions:

Mat44f invert( Mat44f const& aM ) noexcept;

inline
Mat44f transpose( Mat44f const& aM ) noexcept
{
	Mat44f ret;
	for( std::size_t i = 0; i < 4; ++i )
	{
		for( std::size_t j = 0; j < 4; ++j )
			ret(j,i) = aM(i,j);
	}
	return ret;
}

inline
Mat44f make_rotation_x(float aAngle) noexcept
{
	Mat44f mat{};
	mat(0, 0) = 1;
	mat(1, 1) = cos(aAngle);
	mat(1, 2) = -sin(aAngle);
	mat(2, 1) = sin(aAngle);
	mat(2, 2) = cos(aAngle);
	mat(3, 3) = 1;

	return mat;
}


inline
Mat44f make_rotation_y(float aAngle) noexcept
{
	Mat44f mat{};
	mat(1, 1) = 1;
	mat(0, 0) = cos(aAngle);
	mat(2, 0) = -sin(aAngle);
	mat(0, 2) = sin(aAngle);
	mat(2, 2) = cos(aAngle);
	mat(3, 3) = 1;

	return mat;
}

inline
Mat44f make_rotation_z(float aAngle) noexcept
{
	Mat44f mat{};
	mat(2, 2) = 1;
	mat(0, 0) = cos(aAngle);
	mat(0, 1) = -sin(aAngle);
	mat(1, 0) = sin(aAngle);
	mat(1, 1) = cos(aAngle);
	mat(3, 3) = 1;

	return mat;
}

inline
Mat44f make_translation(Vec3f aTranslation) noexcept
{
	Mat44f translated_mat = kIdentity44f;
	translated_mat(0, 3) = aTranslation.x;
	translated_mat(1, 3) = aTranslation.y;
	translated_mat(2, 3) = aTranslation.z;
	return translated_mat;
}


inline
Mat44f make_scaling(float aSX, float aSY, float aSZ) noexcept
{
	Mat44f mat{};
	mat(0, 0) = aSX;
	mat(1, 1) = aSY;
	mat(2, 2) = aSZ;
	mat(3, 3) = 1.f;

	return mat;
}

inline
Mat44f make_perspective_projection(float aFovInRadians, float aAspect, float aNear, float aFar) noexcept
{
	Mat44f mat{};
	float s = 1 / tan(aFovInRadians / 2);

	mat(0, 0) = s / aAspect;
	mat(1, 1) = s;
	mat(2, 2) = -(aFar + aNear) / (aFar - aNear);
	mat(2, 3) = -2 * ((aFar * aNear) / (aFar - aNear));
	mat(3, 2) = -1;

	return mat;
}

// Specialised Mat44 func to generate co-ordinate system for camera
inline
Mat44f make_look_at(Vec4f const& eye, Vec4f const& target, Vec4f const& up)
{
	// Calculate camera coordinate system vectors
	Vec4f forward = normalize(target - eye);  // Z axis
	Vec4f right = normalize(cross(forward, up));  // X axis
	Vec4f true_up = cross(right, forward);  // Y axis

	// Create rotation matrix (world space to camera space)
	Mat44f rotation = {
		right.x,   right.y,   right.z,   0.0f,
		true_up.x, true_up.y, true_up.z, 0.0f,
		-forward.x,-forward.y,-forward.z, 0.0f,
		0.0f,      0.0f,      0.0f,      1.0f
	};

	// Create translation matrix (move scene so camera is at origin)
	Mat44f translation = make_translation({ -eye.x, -eye.y, -eye.z });

	// Final view matrix combines both transforms
	return rotation * translation;
}

#endif // MAT44_HPP_E7187A26_469E_48AD_A3D2_63150F05A4CA
