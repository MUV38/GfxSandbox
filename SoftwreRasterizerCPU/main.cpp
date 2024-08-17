#include <vector>
#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define __STDC_LIB_EXT1__ // sprintfÇÃÉGÉâÅ[ëŒâû
#include <stb_image_write.h>

using u8 = uint8_t;
using s32 = int32_t;
using u32 = uint32_t;
struct Color32;
struct Coolor8;

u8 color32to8(float v);

struct Color32
{
	float r, g, b;
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

u8 color32to8(float v)
{
	return static_cast<u8>(std::clamp(v * 255.f, 0.f, 255.f));
}

int main()
{
	constexpr u32 width = 128, height = 128;

	std::vector<Color8> colors(width * height);
	std::fill(colors.begin(), colors.end(), Color8(0, 0, 0));

	if (stbi_write_bmp("image.bmp", width, height, 3, colors.data()))
	{
		return 0;
	}
	return 1;
}