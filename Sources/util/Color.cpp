#include "Color.h"

#include <cmath>

namespace
{
	const float kByteToFloat = 1.0f / 255.0f;
	const float kHueEpsilon = 1.0e-6f;

	float Clamp01(float v)
	{
		if (v < 0.0f) return 0.0f;
		if (v > 1.0f) return 1.0f;
		return v;
	}

	int ClampByte(int v)
	{
		if (v < 0) return 0;
		if (v > 255) return 255;
		return v;
	}

	int ToByte(float v)
	{
		return (int)(Clamp01(v) * 255.0f + 0.5f);
	}

	float FromByte(int v)
	{
		return ClampByte(v) * kByteToFloat;
	}

	// Maps any finite hue onto [0, 1), including negatives.
	float Wrap01(float h)
	{
		h = h - std::floor(h);
		if (h < 0.0f) h += 1.0f;
		if (h >= 1.0f) h = 0.0f;
		return h;
	}

	int PackARGB(float r, float g, float b, float a)
	{
		const unsigned int aa = (unsigned int)ToByte(a);
		const unsigned int rr = (unsigned int)ToByte(r);
		const unsigned int gg = (unsigned int)ToByte(g);
		const unsigned int bb = (unsigned int)ToByte(b);
		return (int)((aa << 24) | (rr << 16) | (gg << 8) | bb);
	}

	float SrgbToLinearChannel(float c)
	{
		c = Clamp01(c);
		if (c <= 0.04045f)
			return c / 12.92f;
		return std::pow((c + 0.055f) / 1.055f, 2.4f);
	}

	float LinearToSrgbChannel(float c)
	{
		c = Clamp01(c);
		if (c <= 0.0031308f)
			return c * 12.92f;
		return 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
	}

	float HueFromRgb(float r, float g, float b, float maxc, float delta)
	{
		if (delta <= kHueEpsilon)
			return 0.0f;

		float h;
		if (maxc == r)
			h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
		else if (maxc == g)
			h = (b - r) / delta + 2.0f;
		else
			h = (r - g) / delta + 4.0f;

		h *= (1.0f / 6.0f);
		if (h < 0.0f) h += 1.0f;
		if (h >= 1.0f) h -= 1.0f;
		return h;
	}

	void ChannelMinMax(float r, float g, float b, float& minc, float& maxc)
	{
		maxc = r;
		if (g > maxc) maxc = g;
		if (b > maxc) maxc = b;

		minc = r;
		if (g < minc) minc = g;
		if (b < minc) minc = b;
	}

	float HueToRgb(float p, float q, float t)
	{
		if (t < 0.0f) t += 1.0f;
		if (t > 1.0f) t -= 1.0f;
		if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
		if (t < 0.5f) return q;
		if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
		return p;
	}
}

Color::Color()
:	r(0.0f),
	g(0.0f),
	b(0.0f),
	a(1.0f)
{}

Color::Color(float r_, float g_, float b_, float a_)
:	r(r_),
	g(g_),
	b(b_),
	a(a_)
{}

Color::Color(int r_, int g_, int b_, int a_)
:	r(FromByte(r_)),
	g(FromByte(g_)),
	b(FromByte(b_)),
	a(FromByte(a_))
{}

Color Color::fromRGB(int rgb)
{
	return Color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 255);
}

Color Color::fromARGB(int argb)
{
	return Color((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF, (argb >> 24) & 0xFF);
}

Color Color::fromHSV(float h, float s, float v, float alpha)
{
	s = Clamp01(s);
	v = Clamp01(v);
	alpha = Clamp01(alpha);
	h = Wrap01(h);

	if (s <= 0.0f)
		return Color(v, v, v, alpha);

	float scaled = h * 6.0f;
	if (scaled >= 6.0f) scaled = 0.0f;

	const int sector = (int)scaled;
	const float f = scaled - (float)sector;

	const float p = v * (1.0f - s);
	const float q = v * (1.0f - s * f);
	const float t = v * (1.0f - s * (1.0f - f));

	switch (sector)
	{
		case 0:  return Color(v, t, p, alpha);
		case 1:  return Color(q, v, p, alpha);
		case 2:  return Color(p, v, t, alpha);
		case 3:  return Color(p, q, v, alpha);
		case 4:  return Color(t, p, v, alpha);
		default: return Color(v, p, q, alpha);
	}
}

Color Color::fromHSB(float h, float s, float brightness, float alpha)
{
	return fromHSV(h, s, brightness, alpha);
}

Color Color::fromHSL(float h, float s, float l, float alpha)
{
	s = Clamp01(s);
	l = Clamp01(l);
	alpha = Clamp01(alpha);
	h = Wrap01(h);

	if (s <= 0.0f)
		return Color(l, l, l, alpha);

	const float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
	const float p = 2.0f * l - q;

	return Color(
		HueToRgb(p, q, h + 1.0f / 3.0f),
		HueToRgb(p, q, h),
		HueToRgb(p, q, h - 1.0f / 3.0f),
		alpha
	);
}

Color Color::getHSBColor(float h, float s, float b)
{
	return fromHSB(h, s, b, 1.0f);
}

int Color::HSBtoRGB(float h, float s, float b)
{
	return fromHSB(h, s, b, 1.0f).getRGB();
}

void Color::RGBtoHSB(int r, int g, int b, float outHSB[3])
{
	if (!outHSB)
		return;

	Color(r, g, b).toHSV(outHSB[0], outHSB[1], outHSB[2]);
}

Color Color::lerp(const Color& from, const Color& to, float t)
{
	t = Clamp01(t);
	return Color(
		from.r + (to.r - from.r) * t,
		from.g + (to.g - from.g) * t,
		from.b + (to.b - from.b) * t,
		from.a + (to.a - from.a) * t
	);
}

Color Color::lerpLinear(const Color& from, const Color& to, float t)
{
	t = Clamp01(t);
	const Color a = from.srgbToLinear();
	const Color b = to.srgbToLinear();
	return lerp(a, b, t).linearToSrgb();
}

int Color::getRGB() const
{
	return PackARGB(r, g, b, a);
}

int Color::getARGB() const
{
	return PackARGB(r, g, b, a);
}

int Color::getABGR() const
{
	const unsigned int aa = (unsigned int)ToByte(a);
	const unsigned int rr = (unsigned int)ToByte(r);
	const unsigned int gg = (unsigned int)ToByte(g);
	const unsigned int bb = (unsigned int)ToByte(b);
	return (int)((aa << 24) | (bb << 16) | (gg << 8) | rr);
}

int Color::getRed() const
{
	return ToByte(r);
}

int Color::getGreen() const
{
	return ToByte(g);
}

int Color::getBlue() const
{
	return ToByte(b);
}

int Color::getAlpha() const
{
	return ToByte(a);
}

void Color::toHSV(float& h, float& s, float& v) const
{
	const float rr = Clamp01(r);
	const float gg = Clamp01(g);
	const float bb = Clamp01(b);

	float minc, maxc;
	ChannelMinMax(rr, gg, bb, minc, maxc);

	v = maxc;
	if (maxc <= kHueEpsilon)
	{
		h = 0.0f;
		s = 0.0f;
		return;
	}

	const float delta = maxc - minc;
	s = delta / maxc;
	h = HueFromRgb(rr, gg, bb, maxc, delta);
}

void Color::toHSB(float& h, float& s, float& brightness) const
{
	toHSV(h, s, brightness);
}

void Color::toHSL(float& h, float& s, float& l) const
{
	const float rr = Clamp01(r);
	const float gg = Clamp01(g);
	const float bb = Clamp01(b);

	float minc, maxc;
	ChannelMinMax(rr, gg, bb, minc, maxc);

	l = (maxc + minc) * 0.5f;
	const float delta = maxc - minc;
	if (delta <= kHueEpsilon)
	{
		h = 0.0f;
		s = 0.0f;
		return;
	}

	const float denom = 1.0f - std::fabs(2.0f * l - 1.0f);
	s = (denom > kHueEpsilon) ? (delta / denom) : 0.0f;
	h = HueFromRgb(rr, gg, bb, maxc, delta);
}

Color Color::srgbToLinear() const
{
	return Color(
		SrgbToLinearChannel(r),
		SrgbToLinearChannel(g),
		SrgbToLinearChannel(b),
		a
	);
}

Color Color::linearToSrgb() const
{
	return Color(
		LinearToSrgbChannel(r),
		LinearToSrgbChannel(g),
		LinearToSrgbChannel(b),
		a
	);
}

Color Color::multiply(const Color& other) const
{
	return Color(r * other.r, g * other.g, b * other.b, a * other.a);
}

Color Color::scale(float s, bool scaleAlpha) const
{
	return Color(r * s, g * s, b * s, scaleAlpha ? a * s : a);
}

Color Color::withAlpha(float alpha) const
{
	return Color(r, g, b, alpha);
}

Color Color::premultiplied() const
{
	return Color(r * a, g * a, b * a, a);
}

Color Color::brighter(float amount) const
{
	amount = Clamp01(amount);
	return Color(
		r + (1.0f - r) * amount,
		g + (1.0f - g) * amount,
		b + (1.0f - b) * amount,
		a
	);
}

Color Color::darker(float amount) const
{
	amount = Clamp01(amount);
	const float s = 1.0f - amount;
	return Color(r * s, g * s, b * s, a);
}

Color Color::clamped() const
{
	return Color(Clamp01(r), Clamp01(g), Clamp01(b), Clamp01(a));
}

float Color::luminance() const
{
	const Color lin = srgbToLinear();
	return 0.2126f * lin.r + 0.7152f * lin.g + 0.0722f * lin.b;
}

bool Color::operator==(const Color& o) const
{
	return getARGB() == o.getARGB();
}

bool Color::operator!=(const Color& o) const
{
	return !(*this == o);
}

Color Color::operator*(const Color& o) const
{
	return multiply(o);
}

Color Color::operator*(float s) const
{
	return scale(s, false);
}

Color Color::operator+(const Color& o) const
{
	return Color(r + o.r, g + o.g, b + o.b, a + o.a);
}

Color operator*(float s, const Color& c)
{
	return c * s;
}

const Color Color::WHITE(1.0f, 1.0f, 1.0f);
const Color Color::BLACK(0.0f, 0.0f, 0.0f);
const Color Color::RED(1.0f, 0.0f, 0.0f);
const Color Color::GREEN(0.0f, 1.0f, 0.0f);
const Color Color::BLUE(0.0f, 0.0f, 1.0f);
const Color Color::YELLOW(1.0f, 1.0f, 0.0f);
const Color Color::CYAN(0.0f, 1.0f, 1.0f);
const Color Color::MAGENTA(1.0f, 0.0f, 1.0f);
const Color Color::TRANSPARENT(0.0f, 0.0f, 0.0f, 0.0f);
