#include <vdb.h>
#undef min
#undef max

#include <cmath>
#include <vector>
#include <limits>
#include <numbers>

class Vec3
{
public:
	float x, y, z;

	Vec3 operator-(const Vec3& rhs) const
	{
		return { x - rhs.x, y - rhs.y, z - rhs.z };
	}

	Vec3 operator*(const float s) const
	{
		return { x * s, y * s, z * s };
	}

	Vec3& operator+=(const Vec3& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}
};

Vec3 operator*(float s, const Vec3& v)
{
	return { s * v.x, s * v.y, s * v.z };
}

Vec3 operator+(const Vec3& v0, const Vec3& v1)
{
	return { v0.x + v1.x, v0.y + v1.y, v0.z + v1.z };
}

Vec3 operator/(const Vec3& v, float s)
{
	return { v.x / s, v.y / s, v.z / s };
}

struct Vertex
{
	Vec3 pos;
};

float lerp(float v0, float v1, float t)
{
	return v0 + (v1 - v0) * t;
}

Vec3 lerp(const Vec3& v0, const Vec3& v1, float t)
{
	return Vec3{
		lerp(v0.x, v1.x, t),
		lerp(v0.y, v1.y, t),
		lerp(v0.z, v1.z, t)
	};
}

Vec3 cubicBezier(const Vec3& p0, const Vec3& p1, const Vec3& p2, float t)
{
	Vec3 v0 = lerp(p0, p1, t);
	Vec3 v1 = lerp(p1, p2, t);
	return lerp(v0, v1, t);
}

Vec3 cubicBezierDerivative(const Vec3& p0, const Vec3& p1, const Vec3& p2, float t)
{
	float omt = 1 - t;
	return 2 * omt * (p1 - p0) + 2 * t * (p2 - p1);
}

float dot(const Vec3& v0, const Vec3& v1)
{
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

Vec3 cross(const Vec3& v0, const Vec3& v1)
{
	return {
		v0.y * v1.z - v0.z * v1.y,
		v0.z * v1.x - v0.x * v1.z,
		v0.x * v1.y - v0.y * v1.x
	};
}

float length(const Vec3& v)
{
	return std::sqrtf(dot(v, v));
}

Vec3 normalize(const Vec3& v)
{
	return v / length(v);
}

class Xorshift32
{
public:
	Xorshift32(uint32_t seed)
		: mState(seed)
	{}

	uint32_t next()
	{
		mState ^= mState << 13;
		mState ^= mState >> 17;
		mState ^= mState << 5;
		return mState;
	}

	float nextf()
	{
		return next() / static_cast<float>(std::numeric_limits<uint32_t>::max());
	}

private:
	uint32_t mState;
};

int main()
{
	constexpr float GrassHeight = 10.0f;
	constexpr float GrassWidth = 1.0f;

	constexpr int VertexCount = 15;
	Vertex vertices[VertexCount];

	Vec3 cp[3] = {
		{ 0.0f, 0.0, 0.0f },
		{ 0.0f, GrassHeight * 0.5f, 0.0f },
		{ 0.0f, GrassHeight, 0.0f }
	};
	for (int i = 0; i < VertexCount; ++i)
	{
		float sign = i & 1 ? -1 : 1;
		float halfWidth = GrassWidth * 0.5f;

		float t = (i / 2) / static_cast<float>(VertexCount / 2);
		Vec3 binormal = normalize(cubicBezierDerivative(cp[0], cp[1], cp[2], t));
		Vec3 normal = cross(Vec3{ 1, 0, 0 }, binormal);
		Vec3 tangent = cross(binormal, normal);

		vertices[i].pos = cubicBezier(cp[0], cp[1], cp[2], t);
		vertices[i].pos += (i < (VertexCount - 1) ? sign * halfWidth : 0.0f) * tangent;
	}

	//    14
	// 13 /_\ 12
	// 11 |_| 10
	//  9 |_|  8
	//  7 |_|  6
	//  5 |_|  4
	//  3 |_|  2
	//  1 |_|  0
	int indices[] = {
		0, 1, 2,
		2, 3, 1,
		2, 3, 4,
		4, 5, 3,
		4, 5, 6,
		6, 7, 5,
		6, 7, 8,
		8, 9, 7,
		8, 9, 10,
		10, 11, 9,
		10, 11, 12,
		12, 13, 11,
		12, 13, 14
	};

	constexpr int PointCount = 1000;
	constexpr float Radius = 50.0f;

	Xorshift32 rng(123456789);
	std::vector<Vec3> points(PointCount);
	for (int i = 0; i < PointCount; ++i)
	{
		float r = rng.nextf() * Radius;
		float theta = rng.nextf() * 2.0f * std::numbers::pi_v<float>;
		points[i] = { r * std::cosf(theta), 0, r * std::sinf(theta) };
	}

	vdb_frame();

	vdb_color(0, 1, 1);

#if 0
	for (int i = 0; i < VertexCount; ++i)
	{
		vdb_point(vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z);
	}
#endif
	for (const auto& p : points)
	{
		vdb_point(p.x, p.y, p.z);
	}

	vdb_color(0, 1, 0);

	for (const auto& p : points)
	{
		for (int i = 0; i < (std::size(indices) / 3); ++i)
		{
			const Vertex& v0 = vertices[indices[i * 3 + 0]];
			const Vertex& v1 = vertices[indices[i * 3 + 1]];
			const Vertex& v2 = vertices[indices[i * 3 + 2]];
			
			const Vec3 p0 = v0.pos + p;
			const Vec3 p1 = v1.pos + p;
			const Vec3 p2 = v2.pos + p;

			vdb_triangle(
				p0.x, p0.y, p0.z,
				p1.x, p1.y, p1.z,
				p2.x, p2.y, p2.z
			);
		}
	}

	return 0;
}