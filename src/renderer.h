#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>
#include <stdlib.h>
#include <imgui/imgui.h>
#include "utils.h"
#include "shader.h"
#include "fullQuad.h"

class Renderer
{
public:

    // Renderer settings
    float u_time;
    int renderedFrameCount = 0;
    int samplingMethod = 0;
    int samplesPerPixel = 1;
    bool test = false;
    bool doPixelSampling = true;
    bool doGammaCorrection = true;

    // States
    bool doTAA = true;

    Renderer () {}

    Renderer(float windowAspectRatio)
    {
        // Compile and link shader programs
        shader = Shader("./src/shaders/quad.vert", "./src/shaders/main.frag");
    }

    void onUpdate()
    {
        renderedFrameCount = 0;
        skipAA = 2;  // Skip anti aliasing for the next 2 frames
    }
    
    void renderScene(const Window *window, int prevTextureUnit, FullQuad *quad)
    {
        debugMenu();
        
        // Set uniforms
        setSettingsUniforms(window, prevTextureUnit);

        // Render scene
        shader.use();
        quad->render();

        renderedFrameCount++;
    }

    void debugMenu()
    {
        static bool debug = false;
        if (ImGui::IsKeyPressed(ImGuiKey_H)) debug = !debug;
        
        #define DEBUG_VEC3(V) ImGui::Text(#V ": (%.4f, %.4f, %.4f)", V.x, V.y, V.z);

        if (debug)
        {
            // DEBUG HERE
        }
    }

    void setSettingsUniforms(const Window *window, GLint prevTextureUnit)
    {
        doTemporalAntiAliasing = skipAA ? --skipAA > 1 : doTAA;
        shader.setBool("test", test);
        shader.setBool("doPixelSampling", doPixelSampling);
        shader.setBool("doGammaCorrection", doGammaCorrection);
        shader.setBool("doTemporalAntiAliasing", doTemporalAntiAliasing);

        shader.setFloat("u_time", (float)glfwGetTime() / 1000.0f);

        shader.setInt("renderedFrameCount", renderedFrameCount);
        shader.setInt("samplingMethod", samplingMethod);
        shader.setInt("samplesPerPixel", samplesPerPixel);
        shader.setInt("prevFrameTexture", prevTextureUnit);

        shader.setVec2i("resolution", glm::vec2((float)window->width, (float)window->height));
    }

private:

    // Shader programs
    Shader shader;
    
    // States
    int skipAA = 0;
    bool doTemporalAntiAliasing = true;

};

#endif