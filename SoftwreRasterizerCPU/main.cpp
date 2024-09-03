#include <vector>
#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define __STDC_LIB_EXT1__ // sprintfのエラー対応
#include <stb_image_write.h>

using u8 = uint8_t;
using s32 = int32_t;
using u32 = uint32_t;
using f32 = float;
struct Color32;
struct Coolor8;

u8 color32to8(f32 v);
f32 color8to32(u8 v);

struct Color32
{
	f32 r, g, b;
};

struct Color8
{
	u8 r, g, b;

	Color8() = default;

	Color8(u8 r, u8 g, u8 b) : r(r), g(g), b(b) {}

	Color8(const Color32& color)
	{
		r = color32to8(color.r);
		g = color32to8(color.g);
		b= color32to8(color.b);
	}
};

u8 color32to8(f32 v)
{
	return static_cast<u8>(std::clamp(v * 255.f, 0.f, 255.f));
}

f32 color8to32(u8 v)
{
	return v * 255.0f;
}

// Reference
// https://fgiesen.wordpress.com/2013/02/06/the-barycentric-conspirac/
// https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
// https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-rasterizer-stage-rules

struct Vector2
{
	s32 x, y;
};

struct Vertex
{
	Vector2 pos;
	Color32 color;
};

s32 orient2d(const Vector2& a, const Vector2& b, const Vector2& c)
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// Rasterization Rules(The top-left rule)
bool isTopLeft(const Vector2& a, const Vector2& b)
{
	return (a.y == b.y && a.x < b.x) || (a.y > b.y);
}

void rasterize(Color8* colors, u32 width, u32 height, const Vertex* vertices, const u32 (&indices)[3])
{
	const Vertex& v0 = vertices[indices[0]];
	const Vertex& v1 = vertices[indices[1]];
	const Vertex& v2 = vertices[indices[2]];

	s32 minx = std::min(std::min(v0.pos.x, v1.pos.x), v2.pos.x);
	s32 miny = std::min(std::min(v0.pos.y, v1.pos.y), v2.pos.y);
	s32 maxx = std::max(std::max(v0.pos.x, v1.pos.x), v2.pos.x);
	s32 maxy = std::max(std::max(v0.pos.y, v1.pos.y), v2.pos.y);

	// TODO: clipping

	minx = std::max(minx, 0);
	miny = std::max(miny, 0);
	maxx = std::min(maxx, static_cast<s32>(width) - 1);
	maxy = std::min(maxy, static_cast<s32>(height) - 1);

	s32 b0 = isTopLeft(v2.pos, v1.pos) ? 0 : -1;
	s32 b1 = isTopLeft(v0.pos, v2.pos) ? 0 : -1;
	s32 b2 = isTopLeft(v1.pos, v0.pos) ? 0 : -1;

	s32 a21 = v2.pos.y - v1.pos.y, b21 = v1.pos.x - v2.pos.x;
	s32 a02 = v0.pos.y - v2.pos.y, b02 = v2.pos.x - v0.pos.x;
	s32 a10 = v1.pos.y - v0.pos.y, b10 = v0.pos.x - v1.pos.x;

	Vector2 p = { minx, miny };
	s32 det0y = orient2d(v2.pos, v1.pos, p);
	s32 det1y = orient2d(v0.pos, v2.pos, p);
	s32 det2y = orient2d(v1.pos, v0.pos, p);

	for (; p.y <= maxy; p.y++)
	{
		s32 det0 = det0y;
		s32 det1 = det1y;
		s32 det2 = det2y;

		for (p.x=minx; p.x <= maxx; p.x++)
		{
			if ((det0 + b0) >= 0 && (det1 + b1) >= 0 && (det2 + b2) >= 0)
			{
				// 重心座標
				s32 totalWeight = det0 + det1 + det2;
				f32 w0 = det0 / static_cast<f32>(totalWeight);
				f32 w1 = det1 / static_cast<f32>(totalWeight);
				f32 w2 = 1.0f - w0 - w1;

				Color32 c0(v0.color);
				Color32 c1(v1.color);
				Color32 c2(v2.color);

				u8 r = color32to8(c0.r * w0 + c1.r * w1 + c2.r * w2);
				u8 g = color32to8(c0.g * w0 + c1.g * w1 + c2.g * w2);
				u8 b = color32to8(c0.b * w0 + c1.b * w1 + c2.b * w2);

				colors[p.x + width * p.y] = { r, g, b };
			}
			else
			{
				// ピクセルを書き込まなかった捜査範囲
				//colors[p.x + width * p.y] = { 128, 128, 128 };
			}

			det0 += a21;
			det1 += a02;
			det2 += a10;
		}

		det0y += b21;
		det1y += b02;
		det2y += b10;
	}
}

int main()
{
	constexpr u32 width = 128, height = 128;

	std::vector<Color8> colors(width * height);
	std::fill(colors.begin(), colors.end(), Color8(0, 0, 0));

	Vertex vertices0[] = {
		{{ width / 2, 0 }, { 1.0f, 0.0f, 0.0f }},
		{{ width / 4, height / 2 }, { 0.0f, 1.0f, 0.0f }},
		{{ width - width / 4, height / 2 }, { 0.0f, 0.0f, 1.0f }}
	};
	Vertex vertices1[] = {
		{{ width / 4, height / 2 }, { 1.0f, 0.0f, 0.0f }},
		{{ 0, height - 1 }, { 0.0f, 1.0f, 0.0f }},
		{{ width / 2, height - 1 }, { 0.0f, 0.0f, 1.0f }}
	};
	Vertex vertices2[] = {
		{{ width - width / 4, height / 2 }, { 1.0f, 0.0f, 0.0f }},
		{{ width / 2, height - 1 }, { 0.0f, 1.0f, 0.0f }},
		{{ width - 1, height - 1 }, { 0.0f, 0.0f, 1.0f }}
	};
	u32 indices[] = { 0, 1, 2 };
	rasterize(colors.data(), width, height, vertices0, indices);
	rasterize(colors.data(), width, height, vertices1, indices);
	rasterize(colors.data(), width, height, vertices2, indices);

	if (stbi_write_bmp("image.bmp", width, height, 3, colors.data()))
	{
		return 0;
	}
	return 1;
}