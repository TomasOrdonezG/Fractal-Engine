#ifndef COLOUR_H
#define COLOUR_H

#include <glm/glm.hpp>
#include <vector>

namespace Colour
{
    struct RGB
    {
        // { [0.0, 1.0], [0.0, 1.0], [0.0, 1.0] }
        float r, g, b;
        
        RGB(float r, float g, float b)
            : r(r), g(g), b(b)
        {}
    };

    struct HSL
    {
        // { [0.0, 100.0], [0.0, 100.0], [0.0, 360.0] }
        float h, s, l;
        
        HSL(float h, float s, float l)
            : h(h), s(s), l(l)
        {}
    };

    HSL RGBToHSL(RGB& rgb)
    {
        // Make r, g, and b fractions of 1
        float r = rgb.r, g = rgb.g, b = rgb.b;

        // Find greatest and smallest channel values
        float cmin = glm::min(r, glm::min(g, b));
        float cmax = glm::max(r, glm::max(g, b));
        float delta = cmax - cmin;
        float h = 0.0f;
        float s = 0.0f;
        float l = 0.0f;
        
        // Calculate hue
        if (delta == 0.0f)
            h = 0.0f;
        // Red is max
        else if (cmax == r)
            h = glm::mod(((g - b) / delta), 6.0f);
        // Green is max
        else if (cmax == g)
            h = (b - r) / delta + 2.0f;
        // Blue is max
        else
            h = (r - g) / delta + 4.0f;

        h *= 60.0f;
            
        // Make negative hues positive behind 360°
        if (h < 0)
            h += 360.0f;

        // Calculate lightness
        l = (cmax + cmin) / 2.0f;

        // Calculate saturation
        s = delta == 0.0f
            ? 0.0f
            : delta / (1.0f - glm::abs(2.0f * l - 1.0f));
            
        // Multiply l and s by 100
        s *= 100.0f;
        l *= 100.0f;

        return HSL(h, s, l);
    }

    RGB HSLToRGB(HSL& hsl)
    {
        float h = hsl.h, s = hsl.s, l = hsl.l;

        // Must be fractions of 1
        s /= 100.0f;
        l /= 100.0f;

        float c = (1.0f - glm::abs(2.0f * l - 1.0f)) * s;
        float x = c * (1.0f - glm::abs(glm::mod(h / 60.0f, 2.0f) - 1.0f));
        float m = l - c / 2.0f;
        float r = 0.0f, g = 0.0f, b = 0.0f;

        if (0.0f <= h && h < 60.0f) {
            r = c; g = x; b = 0.0f;
        } else if (60.0f <= h && h < 120.0f) {
            r = x; g = c; b = 0.0f;
        } else if (120.0f <= h && h < 180.0f) {
            r = 0.0f; g = c; b = x;
        } else if (180.0f <= h && h < 240.0f) {
            r = 0.0f; g = x; b = c;
        } else if (240.0f <= h && h < 300.0f) {
            r = x; g = 0.0f; b = c;
        } else if (300.0f <= h && h < 360.0f) {
            r = c; g = 0.0f; b = x;
        }

        r += m;
        g += m;
        b += m;

        return RGB(r, g, b);
    }

    glm::vec3 toGlmVec3(RGB rgb)
    {
        return glm::vec3(rgb.r, rgb.g, rgb.b);
    }
   
    glm::vec3 toGlmVec3(HSL hsl)
    {
        return glm::vec3(hsl.h, hsl.s, hsl.l);
    }

    class Gradient
    {
    public:

        std::vector<RGB> colours;
        int size = 0;
    
        Gradient(RGB c1, RGB c2)
        {
            colours.push_back(c1);
            colours.push_back(c2);
            size = 2;
        }

        void insert(RGB c)
        {
            colours.push_back(c);
            size++;
        }

        void remove(int i)
        {
            colours.erase(colours.begin() + i);
            size--;
        }

        void swapForwards(int i)
        {
            if (i < 0 || i + 1 >= size) return;
            RGB temp = colours[i];
            colours[i] = colours[i + 1];
            colours[i + 1] = temp;
        }

        void swapBackwards(int i)
        {
            swapForwards(i - 1);
        }

        RGB value(float a)
        {
            // `a` in [0.0, 1.0]

            // Calculation breaks down on a == 1.0, so here's a base case
            if (a == 1.0) return colours[size - 1];

            // Calculate index of both values and alpha between both values
            float offset = 1.0f / (float)(size - 1.0f);
            int i = (int)(a / offset);
            a = (a - i*offset) / offset;

            // Linear interpolate both colours based on the alpha value
            // Use HSL for better Hue mixing
            HSL hsl1 = RGBToHSL(colours[i + 1]);
            HSL hsl2 = RGBToHSL(colours[i]);

            // Convert to vec3, then linear interpolate, then back to hsl, then to rgb
            glm::vec3 hslOut = glm::mix(toGlmVec3(hsl1), toGlmVec3(hsl2), a);
            HSL hsl(hslOut.x, hslOut.y, hslOut.z);

            return HSLToRGB(hsl);
        }

    };

}

#endif