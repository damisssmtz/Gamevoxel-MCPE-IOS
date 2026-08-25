#pragma once

// Packed ints follow Java / Minecraft ARGB (0xAARRGGBB).
// Channels are stored as floats so blending and colour-space
// conversion keep precision; packing happens on demand.
class Color
{
public:
	float r;
	float g;
	float b;
	float a;

	Color();
	Color(float r, float g, float b, float a = 1.0f);
	Color(int r, int g, int b, int a = 255);

	static Color fromRGB(int rgb);
	static Color fromARGB(int argb);
	static Color fromHSV(float h, float s, float v, float a = 1.0f);
	static Color fromHSB(float h, float s, float brightness, float a = 1.0f);
	static Color fromHSL(float h, float s, float l, float a = 1.0f);

	// Java AWT / Minecraft compatibility. Hue wraps to [0, 1).
	static Color getHSBColor(float h, float s, float b);
	static int HSBtoRGB(float h, float s, float b);
	static void RGBtoHSB(int r, int g, int b, float outHSB[3]);

	static Color lerp(const Color& from, const Color& to, float t);
	static Color lerpLinear(const Color& from, const Color& to, float t);

	int getRGB() const;
	int getARGB() const;
	int getABGR() const;

	int getRed() const;
	int getGreen() const;
	int getBlue() const;
	int getAlpha() const;

	void toHSV(float& h, float& s, float& v) const;
	void toHSB(float& h, float& s, float& brightness) const;
	void toHSL(float& h, float& s, float& l) const;

	Color srgbToLinear() const;
	Color linearToSrgb() const;

	Color multiply(const Color& other) const;
	Color scale(float s, bool scaleAlpha = false) const;
	Color withAlpha(float alpha) const;
	Color premultiplied() const;
	Color brighter(float amount = 0.3f) const;
	Color darker(float amount = 0.3f) const;
	Color clamped() const;

	// Rec. 709 relative luminance, computed in linear space.
	float luminance() const;

	bool operator==(const Color& o) const;
	bool operator!=(const Color& o) const;
	Color operator*(const Color& o) const;
	Color operator*(float s) const;
	Color operator+(const Color& o) const;
	friend Color operator*(float s, const Color& c);

	static const Color WHITE;
	static const Color BLACK;
	static const Color RED;
	static const Color GREEN;
	static const Color BLUE;
	static const Color YELLOW;
	static const Color CYAN;
	static const Color MAGENTA;
	static const Color TRANSPARENT;
};
