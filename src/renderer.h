#ifndef RENDERER_H
#define RENDERER_H

#include <stdlib.h>
#include <vector>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_double.h>
#include "utils.h"
#include "shader.h"
#include "fullQuad.h"

#define SHOW_VEC2I(NAME, V) ImGui::Text(NAME ": %d, %d", V.x, V.y);
#define SHOW_VEC2D(NAME, V) ImGui::Text(NAME ": %Lf, %Lf", V.x, V.y);

class Renderer
{
public:
    
    Renderer () {}

    Renderer(const Window *window)
    {
        // Compile and link shader programs
        shader = Shader("./src/shaders/quad.vert", "./src/shaders/main.frag");

        setResolution(window->resolution());
        resetDefaultFractalValues();

        initGradient();
    }

    void onUpdate()
    {
        renderedFrameCount = 0;
        skipAA = 2;  // Skip anti aliasing for the next 2 frames
    }
    
    void renderScene(int prevTextureUnit, FullQuad *quad)
    {
        // Set uniforms
        setSettingsUniforms(prevTextureUnit);
        setGradientUniforms();

        // Render scene
        shader.use();
        quad->render();

        renderedFrameCount++;
    }

    void resetDefaultFractalValues()
    {
        switch (fractalType)
        {
        case MANDELBROT:

            defaultDimensions = glm::dvec2(2.47, 2.24);
            defaultCenter = glm::dvec2(-0.765, 0.0);
            
        break;
        case JULIA:

            defaultDimensions = glm::dvec2(3.0, 2.0);
            defaultCenter = glm::dvec2(0.0, 0.0);

        break;
        case LERP:

            defaultDimensions = glm::dvec2(3.0, 2.0);
            defaultCenter = glm::dvec2(0.0, 0.0);
        
        break;
        }
        
        dimensions = defaultDimensions;
        centerCoords = defaultCenter;
        zoomFactor = 1.0;

        onUpdate();
    }

    void setResolution(glm::ivec2 newResolution)
    {
        resolution = newResolution;
        onUpdate();
    }

    void setZoomOn(bool mouseInsideWindow, ImVec2 mousePos)
    {
        zoomOn_w = mouseInsideWindow
        ? glm::dvec2((double)mousePos.x, resolution.y - (double)mousePos.y)
        : centerCoords;
    }

    void setSettingsUniforms(GLint prevTextureUnit)
    {
        // Recalculate some things first
        doTemporalAntiAliasing = skipAA ? --skipAA > 1 : doTAA;
        dimensions = defaultDimensions / (double)zoomFactor;
        scale = glm::dvec2((double)resolution.x, (double)resolution.y) / dimensions;

        shader.setBool("test", test);
        shader.setBool("doPixelSampling", doPixelSampling);
        shader.setBool("doGammaCorrection", doGammaCorrection);
        shader.setBool("doTemporalAntiAliasing", doTemporalAntiAliasing);

        shader.setFloat("u_time", (float)glfwGetTime() / 1000.0f);

        shader.setInt("renderedFrameCount", renderedFrameCount);
        shader.setInt("samplingMethod", samplingMethod);
        shader.setInt("samplesPerPixel", samplesPerPixel);
        shader.setInt("prevFrameTexture", prevTextureUnit);
        shader.setInt("fractalType", fractalType);
        shader.setInt("maxFractalIterations", maxFractalIterations);

        shader.setVec2i("resolution", resolution);

        shader.setVec2d("dimensions", dimensions);
        shader.setVec2d("centerCoords", centerCoords);
        shader.setVec2d("testDvec2", testDvec2);
        shader.setVec2d("scale", scale);

        shader.setFloat("gradientDegree", gradientDegree);
    }


    // * ImGui Menus

    void dataGui()
    {
        ImGui::Text("%.4f FPS", ImGui::GetIO().Framerate);
        ImGui::Text("%d Frames sampled", renderedFrameCount);
        SHOW_VEC2I("Resolution", resolution);
        SHOW_VEC2D("Scale", scale);
        SHOW_VEC2D("Dimensions", dimensions);
        SHOW_VEC2D("Zoom on", zoomOn_w);
    }

    void renderingMenu()
    {
        bool updated = false;

        updated |= ImGui::Checkbox("Test", &test);
        
        updated |= ImGui::Checkbox("Gamma Correction", &doGammaCorrection);
        updated |= ImGui::Checkbox("Temporal Anti-Aliasing", &doTAA);
        
        if (doTAA)
        {
            // We want this on when doing temporal anti aliasing
            doPixelSampling = true;
        }
        else
        {
            updated |= ImGui::Checkbox("Pixel Sampling", &doPixelSampling);
        }
        
        // Pixel sampling
        if (doPixelSampling)
        {
            // Pick pixel sampling method
            ImGui::SeparatorText("Pixel Sampling Method");
            updated |= ImGui::RadioButton("Random point", &samplingMethod, 0); ImGui::SameLine();
            updated |= ImGui::RadioButton("Jittered Grid", &samplingMethod, 1);

            if (!doTAA)
            {
                // Don't give grid sampling as an option when temporal anti aliasing is on since it isn't not random
                ImGui::SameLine();
                updated |= ImGui::RadioButton("Grid", &samplingMethod, 2);
            }
            else if (samplingMethod == 2)
            {
                // If grid sampling, set to default random sampling method
                samplingMethod = 0;
            }

            // Set number of pixel samples
            updated |= ImGui::SliderInt("Samples per pixel", &(samplesPerPixel), 1, 20, samplingMethod == 1 ? "%d^2" : "%d");
        }

        if (updated) onUpdate();
    }

    void fractalMenu()
    {
        bool updated = false;
        bool reset = false;

        reset |= ImGui::SliderInt("Fractal", (int*)&fractalType, 0, 2, fractalTitles[fractalType]);

        if (fractalType == LERP)
        {
            updated |= ImGui::DragDouble2("Lerp Alpha", &testDvec2.x, 0.1 / (zoomFactor*100.0), 0.0, 0.0, "%.6f");
        }

        reset |= ImGui::Button("Reset");

        updated |= ImGui::DragDouble2("Center coordinates", &centerCoords.x, 0.1 / (zoomFactor*100.0), 0.0, 0.0, "%.6f");
        updated |= ImGui::DragDouble("Zoom Factor", &zoomFactor, 1.0 + (zoomFactor/1000.0), 0.5, 1000000.0);
        updated |= ImGui::DragInt("Max iterations", &maxFractalIterations, 1, 1, 10000);

        updated |= reset;
        if (reset) resetDefaultFractalValues();
        if (updated) onUpdate();
    }

    void colourMenu()
    {
        bool updated = false;

        updated |= ImGui::Checkbox("Smooth colouring", &smoothColouring);
        updated |= ImGui::DragFloat("Gradient Degree", &gradientDegree, 0.1, 0.0, 10.0, "%.6f");
        char title[16];

        // Iterate through every gradient colour
        for (int i = 0; i < gradient.size(); i++)
        {
            sprintf(title, "##%d", i);
            updated |= ImGui::ColorEdit3(title, &(gradient[i].r));
            ImGui::SameLine();

            // Delete colour button
            sprintf(title, "X##%d", i);
            if (gradient.size() > 2 && ImGui::Button(title))
            {
                gradient.erase(gradient.begin() + i);
                updated = true;
            }
            ImGui::SameLine();

            // Move up and down buttons
            sprintf(title, "##%dup", i);
            if (ImGui::ArrowButton(title, ImGuiDir_Up) && i > 0)
            {
                glm::vec3 temp = gradient[i];
                gradient[i] = gradient [i - 1];
                gradient[i - 1] = temp;
                updated = true;
            }
            ImGui::SameLine();
            sprintf(title, "##%ddown", i);
            if (ImGui::ArrowButton(title, ImGuiDir_Down) && i < gradient.size() - 1)
            {
                glm::vec3 temp = gradient[i];
                gradient[i] = gradient [i + 1];
                gradient[i + 1] = temp;
                updated = true;
            }
        }
        
        static glm::vec3 newCol(1.0, 1.0, 1.0);
        ImGui::Separator();
        ImGui::ColorEdit3("New", &(newCol.r));
        if (ImGui::Button("+"))
        {
            gradient.push_back(newCol);
            newCol = glm::vec3(1.0, 1.0, 1.0);
            updated = true;
        }
        
        if (updated) onUpdate();
    }


    // * Event callback functions
    void mouseClickCallback(ImVec2 mousePos)
    {

    }
    
    void mouseDragCallback(ImVec2 dpos)
    {
        // Update center coordinates based on the scale of the image, and the mouse drag distance
        centerCoords -= glm::dvec2((double)dpos.x, -(double)dpos.y) / scale;
        onUpdate();
    }

    void mouseScrollCallback(float yOffset)
    {
        zoomFactor *= 1.0 + yOffset*0.3;
        onUpdate();
    }

private:

    Shader shader;
    
    // States
    int skipAA = 0;
    bool doTemporalAntiAliasing = true;
    bool doTAA = true;

    // Renderer settings
    float u_time;
    double zoomFactor = 1.0;
    int renderedFrameCount = 0;
    int samplingMethod = 0;
    int samplesPerPixel = 1;
    bool test = false;
    bool doGammaCorrection = false;
    bool doPixelSampling = true;

    // Fractal settings
    enum FractalType { MANDELBROT, JULIA, LERP } fractalType = MANDELBROT;
    const char* fractalTitles[3] = { "Mandelbrot", "Julia", "Lerp" };
    int maxFractalIterations = 50;

    glm::ivec2 resolution;
    glm::dvec2 zoomOn_w;
    glm::dvec2 scale;

    glm::dvec2 defaultCenter;
    glm::dvec2 centerCoords;
    
    glm::dvec2 defaultDimensions;
    glm::dvec2 dimensions;

    glm::dvec2 testDvec2 = glm::dvec2(0.0);

    // Gradient implementation
    std::vector<glm::vec3> gradient;
    int maxGradientSize = 10;
    bool smoothColouring = false;
    float gradientDegree = 1.0;

    void initGradient()
    {
        // Initial gradient
        gradient.push_back(glm::vec3(0.0, 0.0, 0.0));
        gradient.push_back(glm::vec3(1.0, 0.0, 0.0));
        gradient.push_back(glm::vec3(1.0, 1.0, 0.0));
        gradient.push_back(glm::vec3(1.0, 1.0, 1.0));
    }

    void setGradientUniforms()
    {
        GLint location = glGetUniformLocation(shader.ID, "gradient");
        glUniform3fv(location, gradient.size(), &gradient[0][0]);

        shader.setInt("gradientSize", gradient.size());
        shader.setBool("smoothColouring", smoothColouring);
    }

};

#endif