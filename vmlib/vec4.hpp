#ifndef VEC4_HPP_7524F057_7AA7_4C99_AA52_DB0B5A3F8CAA
#define VEC4_HPP_7524F057_7AA7_4C99_AA52_DB0B5A3F8CAA
// SOLUTION_TAGS: gl-(ex-[^12]|cw-2)

#include <cmath>
#include <cassert>
#include <cstdlib>

struct Vec4f
{
	float x, y, z, w;

	constexpr 
	float& operator[] (std::size_t aI) noexcept
	{
		assert( aI < 4 );
		return aI[&x]; // This is a bit sketchy.
	}
	constexpr 
	float operator[] (std::size_t aI) const noexcept
	{
		assert( aI < 4 );
		return aI[&x]; // This is a bit sketchy.
	}
};


constexpr
Vec4f operator+( Vec4f aVec ) noexcept
{
	return aVec;
}
constexpr
Vec4f operator-( Vec4f aVec ) noexcept
{
	return { -aVec.x, -aVec.y, -aVec.z, -aVec.w };
}

constexpr
Vec4f operator+( Vec4f aLeft, Vec4f aRight ) noexcept
{
	return Vec4f{
		aLeft.x + aRight.x,
		aLeft.y + aRight.y,
		aLeft.z + aRight.z,
		aLeft.w + aRight.w
	};
}
constexpr
Vec4f operator-( Vec4f aLeft, Vec4f aRight ) noexcept
{
	return Vec4f{
		aLeft.x - aRight.x,
		aLeft.y - aRight.y,
		aLeft.z - aRight.z,
		aLeft.w - aRight.w
	};
}


constexpr
Vec4f operator*( float aScalar, Vec4f aVec ) noexcept
{
	return Vec4f{ 
		aScalar * aVec.x, 
		aScalar * aVec.y, 
		aScalar * aVec.z, 
		aScalar * aVec.w 
	};
}
constexpr
Vec4f operator*( Vec4f aVec, float aScalar ) noexcept
{
	return aScalar * aVec;
}

constexpr
Vec4f operator/( Vec4f aVec, float aScalar ) noexcept
{
	return Vec4f{ 
		aVec.x / aScalar,
		aVec.y / aScalar,
		aVec.z / aScalar,
		aVec.w / aScalar
	};
}


constexpr
Vec4f& operator+=( Vec4f& aLeft, Vec4f aRight ) noexcept
{
	aLeft.x += aRight.x;
	aLeft.y += aRight.y;
	aLeft.z += aRight.z;
	aLeft.w += aRight.w;
	return aLeft;
}
constexpr
Vec4f& operator-=( Vec4f& aLeft, Vec4f aRight ) noexcept
{
	aLeft.x -= aRight.x;
	aLeft.y -= aRight.y;
	aLeft.z -= aRight.z;
	aLeft.w -= aRight.w;
	return aLeft;
}

constexpr
Vec4f& operator*=( Vec4f& aLeft, float aRight ) noexcept
{
	aLeft.x *= aRight;
	aLeft.y *= aRight;
	aLeft.z *= aRight;
	aLeft.w *= aRight;
	return aLeft;
}
constexpr
Vec4f& operator/=( Vec4f& aLeft, float aRight ) noexcept
{
	aLeft.x /= aRight;
	aLeft.y /= aRight;
	aLeft.z /= aRight;
	aLeft.w /= aRight;
	return aLeft;
}


// A few common functions:

constexpr
float dot( Vec4f aLeft, Vec4f aRight ) noexcept
{
	return aLeft.x * aRight.x 
		+ aLeft.y * aRight.y
		+ aLeft.z * aRight.z
		+ aLeft.w * aRight.w
	;
}

inline Vec4f cross(Vec4f const& a, Vec4f const& b)
{
	// Cross product formula:
	// x = a.y * b.z - a.z * b.y
	// y = a.z * b.x - a.x * b.z
	// z = a.x * b.y - a.y * b.x
	return Vec4f{
		a.y * b.z - a.z * b.y,  // x component
		a.z * b.x - a.x * b.z,  // y component
		a.x * b.y - a.y * b.x,  // z component
		0.0f                     // w component is 0 for vectors
	};
}

inline
float length(Vec4f aVec) noexcept
{
	// The standard function std::sqrt() is not marked as constexpr. length()
	// calls std::sqrt() unconditionally, so length() cannot be marked
	// constexpr itself.
	return std::sqrt(dot(aVec, aVec));
}

inline Vec4f normalize(Vec4f v)
{
	// Calculate the length (magnitude) of the vector using Pythagorean theorem
	float vec_length = length(v);

	// Avoid division by zero
	if (vec_length < 1e-6f) {  // Small epsilon value for floating point comparison
		return v;          // Return original vector if it's too close to zero
	}

	// Scale each component by 1/length to normalize
	return Vec4f{
		v.x / vec_length,
		v.y / vec_length,
		v.z / vec_length,
		v.w            // w component typically stays the same (0 for vectors, 1 for points)
	};
}


#endif // VEC4_HPP_7524F057_7AA7_4C99_AA52_DB0B5A3F8CAA
