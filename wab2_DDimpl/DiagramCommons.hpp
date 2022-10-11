#pragma once
#include <stdexcept>

/*
* \brief a simple 2d point
*/
struct Point
{
	double x = 0, y = 0;
};

struct Dimension
{
	double width, height; //TODO: add constraints: min: 0
};

/*
* View
*/
struct Bounds
{
	Point pos;
	Dimension dim; //TODO: we don't need 2 dimensions, right?
};

/*
* \brief RGB-Color
*/
struct Color
{
	Color(int r, int g, int b)
	{
		if (r < 0 || g < 0 || b < 0) throw std::invalid_argument("A passed argument was < 0.\nConstraints r, g, b >= 0 && r, g, b <=255");
		if (r > 255 || g > 255 || b > 255) throw std::invalid_argument("A passed argument was > 255.\nConstraints r, g, b >= 0 && r, g, b <=255");

		red = r;
		green = g;
		blue = b;
	}

	int red, green, blue; //TODO: add constraints: min:0, max:255

	bool operator==(const Color& other)
	{
		return red == other.red && green == other.green && blue == other.blue;
	}
};

/*
* Enums
*/

enum class AlignmentKind
{
	start,
	end,
	center
};

namespace KnownColor
{
	const Color maroon = Color(0x80, 0x00, 0x00);
	const Color red = Color(0xff, 0x00, 0x00);
	const Color orange = Color(0xff, 0xa5, 0x00);
	const Color yellow = Color(0xff, 0xff, 0x00);
	const Color	olive = Color(0x80, 0x80, 0x00);
	const Color purple = Color(0x80, 0x00, 0x80);
	const Color	fuchsia = Color(0xff, 0x00, 0xff);
	const Color white = Color(0xff, 0xff, 0xff);
	const Color lime = Color(0x00, 0xff, 0x00);
	const Color green = Color(0x00, 0x80, 0x00);
	const Color navy = Color(0x00, 0x00, 0x80);
	const Color blue = Color(0x00, 0x00, 0xff);
	const Color aqua = Color(0x00, 0xff, 0xff);
	const Color teal = Color(0x00, 0x80, 0x80);
	const Color black = Color(0x00, 0x00, 0x00);
	const Color silver = Color(0xc0, 0xc0, 0xc0);
	const Color gray = Color(0x80, 0x80, 0x80);
};