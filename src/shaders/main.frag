#version 330 core

// * Inputs / Outputs
in vec2 TexCoords;
out vec4 FragColour;

// * Macrodefinitions
#define FLOAT_MAX 3.402823466e+38
#define FLOAT_MIN 1.175494351e-38
#define PI 3.14159265358979323846

// * Uniforms
uniform bool test;
uniform bool doPixelSampling;
uniform int samplingMethod;
uniform bool doGammaCorrection;
uniform bool doTemporalAntiAliasing;
uniform float u_time;
uniform int renderedFrameCount;
uniform int samplesPerPixel;
uniform ivec2 resolution;
uniform sampler2D prevFrameTexture;

// * Colour calculation
vec3 colMap(float value)
{
    return vec3(
        sin(sin(value - 0.6)),
        sin(sin(value)),
        sin(sin(value + 0.8))
    );
}

vec3 calculateColour(vec2 coord)
{
    vec2 uv = vec2(coord.x / float(resolution.x), coord.y / float(resolution.y)) - 0.5;
    uv.y *= resolution.y / float(resolution.x);
    uv *= (-cos(u_time * 0.1) + 1.3) * 0.4;
    uv = uv.yx;
    uv += vec2(0.1, 0.65);

    vec2 c = uv;
    vec2 z = c;
    float l = 0.0;
    float sum = length(z);
    vec2 newZ;
    vec2 gyroscope = vec2(
        sin(u_time*0.59),
        cos(u_time*0.33)
    );

    int maxIterations = 40;
    for (int i = 0; i < maxIterations; i++)
    {
        c += (gyroscope.yx - 1.0)*0.01*float(i);
        newZ = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
        sum += length(newZ - z);
        z = newZ;
        l = length(z);
        if (l > 2.0) break;
    }

    vec2 dir = z - c;
    vec3 col = vec3(dir, 0.0);
    uv.x = mod((atan(dir.y, dir.x) / PI * 0.5 + 0.5) * 6.0 + (u_time + 0.9*sin(u_time)) * 4.0, 1.0);
    uv.y = mod(length(0.5*dir), 1.0);
    col = colMap(0.2*sum - 0.1*l + 0.1*u_time).yzx;
    return col;
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