#version 460 core

// * Inputs / Outputs
in vec2 TexCoords;
out vec4 FragColour;

// * Macrodefinitions
#define FLOAT_MAX 3.402823466e+38
#define FLOAT_MIN 1.175494351e-38
#define PI 3.14159265358979323846
#define LN_2 0.693147180559945309

// * Uniforms
uniform bool test;
uniform float u_time;

uniform bool doGammaCorrection;
uniform ivec2 resolution;
uniform sampler2D prevFrameTexture;

uniform bool doTemporalAntiAliasing;
uniform int renderedFrameCount;

uniform bool doPixelSampling;
uniform int samplingMethod;
uniform int samplesPerPixel;

uniform dvec2 zoomOn_w;

uniform dvec2 centerCoords;
uniform dvec2 dimensions;
uniform dvec2 scale;
uniform dvec2 testDvec2;

uniform int fractalType;
uniform int maxFractalIterations;

#define MAX_GRADIENT_SIZE 10
uniform bool smoothColouring;
uniform int gradientSize;
uniform vec3 gradient[MAX_GRADIENT_SIZE];
uniform float gradientDegree;

// * Colour calculation
vec3 RGBToHSL(vec3 rgb)
{
    // Make r, g, and b fractions of 1
    float r = rgb.r, g = rgb.g, b = rgb.b;

    // Find greatest and smallest channel values
    float cmin = min(r, min(g, b));
    float cmax = max(r, max(g, b));
    float delta = cmax - cmin;
    float h = 0.0;
    float s = 0.0;
    float l = 0.0;
    
    // Calculate hue
    if (delta == 0.0)
        h = 0.0;
    // Red is max
    else if (cmax == r)
        h = mod(((g - b) / delta), 6.0);
    // Green is max
    else if (cmax == g)
        h = (b - r) / delta + 2.0;
    // Blue is max
    else
        h = (r - g) / delta + 4.0;

    h *= 60.0;
        
    // Make negative hues positive behind 360°
    if (h < 0)
        h += 360.0;

    // Calculate lightness
    l = (cmax + cmin) / 2.0;

    // Calculate saturation
    s = delta == 0.0
        ? 0.0
        : delta / (1.0 - abs(2.0 * l - 1.0));
        
    // Multiply l and s by 100
    s *= 100.0;
    l *= 100.0;

    return vec3(h, s, l);
}

vec3 HSLToRGB(vec3 hsl)
{
    float h = hsl.x, s = hsl.y, l = hsl.z;

    // Must be fractions of 1
    s /= 100.0;
    l /= 100.0;

    float c = (1.0 - abs(2.0 * l - 1.0)) * s;
    float x = c * (1.0 - abs(mod(h / 60.0, 2.0) - 1.0));
    float m = l - c / 2.0;
    float r = 0.0, g = 0.0, b = 0.0;

    if (0.0 <= h && h < 60.0) {
        r = c; g = x; b = 0.0;
    } else if (60.0 <= h && h < 120.0) {
        r = x; g = c; b = 0.0;
    } else if (120.0 <= h && h < 180.0) {
        r = 0.0; g = c; b = x;
    } else if (180.0 <= h && h < 240.0) {
        r = 0.0; g = x; b = c;
    } else if (240.0 <= h && h < 300.0) {
        r = x; g = 0.0; b = c;
    } else if (300.0 <= h && h < 360.0) {
        r = c; g = 0.0; b = x;
    }

    r += m;
    g += m;
    b += m;

    return vec3(r, g, b);
}

vec3 gradientValue(float a)
{
    // `a` in [0.0, 1.0]

    a = pow(a, gradientDegree);

    // Calculation breaks down on a == 1.0, so here's a base case
    if (a == 1.0) return gradient[gradientSize - 1];

    // Calculate index of both values and alpha between both values
    float offset = 1.0 / float(gradientSize - 1.0);
    int i = int(a / offset);
    float a1 = (a - i*offset) / offset;

    if (test)
    {
        // No HSL conversion
        return mix(gradient[i], gradient[i+1], a1);
    }

    // Linear interpolate both colours based on the alpha value
    // Use HSL for better Hue mixing
    vec3 hsl1 = RGBToHSL(gradient[i]);
    vec3 hsl2 = RGBToHSL(gradient[i+1]);

    // Convert to vec3, then linear interpolate, then back to hsl, then to rgb
    vec3 hsl = mix(hsl1, hsl2, a1);

    return HSLToRGB(hsl);
}

vec3 colMap(dvec2 z, int iteration)
{
    if (iteration < maxFractalIterations)
    {
        float alpha;
        
        if (smoothColouring)
        {
            float log_zn = log(float(dot(z, z))) / 2.0;
            float nu = log(log_zn / LN_2) / LN_2;
            float colIndex = float(iteration) + 1.0 - nu;
            alpha = sqrt(maxFractalIterations*colIndex) / float(maxFractalIterations);
        }
        else
        {
            alpha = iteration / float(maxFractalIterations);
        }

        return gradientValue(alpha);

    }

    return vec3(0.0);
}

// * Fractal generation

int fractalRecurrence(dvec2 z, dvec2 c)
{
    int iteration = 0;
    while (dot(z, z) <= 4.0 && iteration < maxFractalIterations)
    {
        // z_n+1 = z_n*z_n + c
        z = dvec2(z.x*z.x - z.y*z.y, 2*z.x*z.y) + c;
        iteration++;
    }
    return iteration;
}

vec3 mandelbrotSet(dvec2 uv)
{
    dvec2 c = dvec2(uv);
    dvec2 z = dvec2(0.0);

    int iteration = fractalRecurrence(z, c);

    return colMap(z, iteration);
}

vec3 juliaSet(dvec2 uv)
{
    dvec2 z = dvec2(uv);
    dvec2 c = dvec2(-0.5251993);

    int iteration = fractalRecurrence(z, c);

    return colMap(z, iteration);
}

vec3 mandelbrotJuliaLerp(dvec2 uv)
{
    dvec2 c = mix(uv, dvec2(-0.5251993), testDvec2.x);
    dvec2 z = mix(dvec2(0.0), uv, testDvec2.y);

    int iteration = fractalRecurrence(z, c);

    return colMap(z, iteration);
}

vec3 calculateColour(vec2 coord)
{
    // Normalize coords and translate to the desired x, y ranges
    dvec2 uv = dvec2(coord / scale);
    uv += centerCoords - dimensions/2.0;

    // Scale to fit aspect ratio
    uv.y *= resolution.y / double(resolution.x);
    
    // Render fractal
    switch (fractalType)
    {
        case 0:  // * MANDELBROT SET
        
            return mandelbrotSet(uv);
            
        break;
        case 1:  // * JULIA SET
        
            return juliaSet(uv);
            
        break;
        case 2:  // * MANDELBROT-JULIA LERP

            return mandelbrotJuliaLerp(uv);
            
        break;
    }
}

// * Utility functions
float rand()
{
    return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233)) * u_time) * 43758.5453);
}

vec3 gammaCorrect(vec3 linear)
{
    return vec3(sqrt(linear.x), sqrt(linear.y), sqrt(linear.z));
}

vec3 postProcess(vec3 colour)
{

    if (doGammaCorrection)
    {
        colour = gammaCorrect(colour);
    }
    
    if (doTemporalAntiAliasing)
    {
        // Average colour with previous frame
        vec3 prevColour = texture(prevFrameTexture, TexCoords).xyz;
        colour = mix(prevColour, colour, 1.0 / (renderedFrameCount + 1));
    }

    return colour;

}

// * Pixel sampling methods
vec3 randomPointSample()
{
    vec3 colour = vec3(0.0);
    vec2 pixelCenter = gl_FragCoord.xy + 0.5;

    for (int i = 0; i < samplesPerPixel; i++)
    {
        // Sample random window coords
        vec2 offset = vec2(rand() - 0.5, rand() - 0.5);
        vec2 sampledCoord = pixelCenter + offset;

        // Calculate colour
        colour += calculateColour(sampledCoord);
    }

    return colour / float(samplesPerPixel);
}

vec3 gridSample()
{
    vec3 colour = vec3(0.0);

    for (int i = 0; i < samplesPerPixel; i++)
    {
        for (int j = 0; j < samplesPerPixel; j++)
        {
            // Sample random window coords
            vec2 offset = (vec2(i, j) + 0.5) / float(samplesPerPixel);
            vec2 sampledCoord = gl_FragCoord.xy + offset;

            // Calculate colour
            colour += calculateColour(sampledCoord);
        }
    }

    return colour / float(samplesPerPixel*samplesPerPixel);
}

vec3 jitteredGridSample()
{
    vec3 colour = vec3(0.0);

    for (int i = 0; i < samplesPerPixel; i++)
    {
        for (int j = 0; j < samplesPerPixel; j++)
        {
            // Sample random window coords with jitter
            vec2 offset = (vec2(i, j) + vec2(rand(), rand())) / float(samplesPerPixel);
            vec2 sampledCoord = gl_FragCoord.xy + offset;

            // Calculate colour
            colour += calculateColour(sampledCoord);
        }
    }

    return colour / float(samplesPerPixel*samplesPerPixel);
}

void main()
{

    vec3 currentColour;

    if (doTemporalAntiAliasing || doPixelSampling)
    {
        // Sample pixel based on some sampling method
        if (samplingMethod == 0)
            currentColour = randomPointSample();
        else if (samplingMethod == 1)
            currentColour = jitteredGridSample();
        else if (samplingMethod == 2)
            currentColour = gridSample();
    }
    else
    {
        // No sampling, calculate colour at the pixel's center
        currentColour = calculateColour(gl_FragCoord.xy + 0.5);
    }

    currentColour = postProcess(currentColour);
    FragColour = vec4(currentColour, 1.0);
}