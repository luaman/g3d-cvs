/**
  @file gfxmeter/main.cpp

  @author Morgan McGuire, matrix@graphics3d.com
 */

#include "G3D/G3DAll.h"
#include "GLG3D/GLG3D.h"

#include "App.h"
#ifdef G3D_WIN32
#   include <direct.h>
#endif

//#define FAST
 
static const float gfxMeterVersion = 1.0f;

int CPU_speed_in_MHz();

/**
 FPS   = frames per second
 VPS   = vertices per second

 RAM   = vertices stored in main memory
 VBO   = vertices stored in vertex buffer object
 VBO16 = using smaller indices
 VBOI  = vertices stored in VBO, interleaved

 First array element is low coherence, second is high coherence
 */
void measureVertexPerformance(
    OSWindow* w,     
    int&   numTris,
    float  beginEndFPS[2],
    float  drawElementsRAMFPS[2], 
    float  drawElementsVBOFPS[2], 
    float  drawElementsVBO16FPS[2], 
    float  drawElementsVBOIFPS[2],
    float  drawElementsVBOPeakFPS[2],
    float& drawArraysVBOPeakFPS);

void shaderVersions(
    std::string& regStr,
    std::string& asmStr,
    std::string& glslStr);


void App::showSplashScreen() {
    Texture::Settings textureSettings;
    textureSettings.wrapMode = WrapMode::CLAMP;
    Texture::Ref gfxMeterTexture = 
        Texture::fromFile("gears.jpg", ImageFormat::AUTO(), Texture::DIM_2D, textureSettings);

    // Load the font
    if (reportFont.isNull()) {
        reportFont = GFont::fromFile(dataDir + "arial.fnt");
    }

    renderDevice->push2D();
    
        renderDevice->setColorClearValue(Color3::white());
        renderDevice->clear();

        int s = gfxMeterTexture->texelWidth();
        int w = 800, h = 600;
        renderDevice->setTexture(0, gfxMeterTexture);
        Draw::rect2D(Rect2D::xywh(w/2-s/2,h/2-s/2,s,s), renderDevice);

        // Second time through, render some text
        if (reportFont.notNull()) {
            reportFont->draw2D(renderDevice, "Profiling your system...", Vector2(w/2, h/2+s/2 + 10), 
                19, Color3::black(), Color4::clear(), GFont::XALIGN_CENTER);
        }

    renderDevice->pop2D();
    window()->swapGLBuffers();
}


void App::onInit() {
    defaultController->setActive(false);
	showDebugText = false;
	showRenderingStats = false;

	popup = NONE;

    defaultCamera.setPosition(Vector3(0, 2, 10));
    defaultCamera.lookAt(Vector3(0, 2, 0));

    // Init the fun stuff (MD2s)
    knight.load("pknight");
    knight.cframe.translation = Vector3(-5, 0, 0);

    ogre.load("ogro");
    ogre.cframe.translation = Vector3(-1.5, 0, 0);

    knight.cframe.lookAt(ogre.cframe.translation + Vector3(0,0,1));
    ogre.cframe.lookAt(knight.cframe.translation + Vector3(0,0,1));

    //window()->swapGLBuffers();while(true);

    // Choose a card logo
    {
        std::string filename = "";
        if (beginsWith(GLCaps::vendor(), "NVIDIA")) {
            filename = "nvidia.jpg";
        } else if (beginsWith(GLCaps::vendor(), "ATI")) {
            filename = "ati.jpg";
        }

        if (filename != "") {
            Texture::Settings textureSettings;
            textureSettings.wrapMode = WrapMode::CLAMP;
            cardLogo = Texture::fromFile(filename, ImageFormat::AUTO(), Texture::DIM_2D, textureSettings);
        }
    }

    // Choose chip logo
    {
        std::string filename = "";
        if (endsWith(toLower(System::cpuVendor()), "intel")) {
            filename = "intel.jpg";
        } else if (endsWith(toLower(System::cpuVendor()), "amd")) {
            filename = "amd.jpg";
        }

        if (filename != "") {
            Texture::Settings textureSettings;
            textureSettings.wrapMode = WrapMode::CLAMP;
            chipLogo = Texture::fromFile(filename, ImageFormat::AUTO(), Texture::DIM_2D, textureSettings);
        }

#       ifdef G3D_WIN32
            double speed = CPU_speed_in_MHz() * 1e6;
            if (speed > 1e9) {
                chipSpeed = format("%.1f GHz", speed / 1e9);
            } else if (speed > 10e6) {
                chipSpeed = format("%.1f MHz", speed / 1e6);
            }
            // Probably a bad result if speed is less than 1 MHz
#       endif
    }

    // Choose os logo
    {
        std::string filename = "";
        std::string os = System::operatingSystem ();

        if (beginsWith(os, "Windows 5.0")) {
            filename = "win2k.jpg";
        } else if (beginsWith(os, "Windows 5.1")) {
            filename = "winxp.jpg";
        }

        if (filename != "") {
            Texture::Settings textureSettings;
            textureSettings.wrapMode = WrapMode::CLAMP;
            osLogo = Texture::fromFile(filename, ImageFormat::AUTO(), Texture::DIM_2D, textureSettings);
        }
    }

    titleFont = GFont::fromFile(dataDir + "carbon.fnt");

    shaderVersions(combineShader, asmShader, glslShader);
    computeFeatureRating();

    logPrintf("Shaders:\n");
    logPrintf("   Combiners: %s\n", combineShader.c_str());
    logPrintf("   Assembly:  %s\n", asmShader.c_str());
    logPrintf("   GLSL:      %s\n", glslShader.c_str());
    logPrintf("\n\n");

#   ifndef FAST
    {
        measureVertexPerformance(
            window(),
            vertexPerformance.numTris, 
            vertexPerformance.beginEndFPS, 
            vertexPerformance.drawElementsRAMFPS,
            vertexPerformance.drawElementsVBOFPS,
            vertexPerformance.drawElementsVBO16FPS,
            vertexPerformance.drawElementsVBOIFPS,
            vertexPerformance.drawElementsVBOPeakFPS,
            vertexPerformance.drawArraysVBOPeakFPS);

        logPrintf("\nDetailed Performance Tests\n\n");
        logPrintf("   * Vertex Rate\n");
        logPrintf("    %d tris, 2 lights, 1 texture, and 4 attributes\n\n", vertexPerformance.numTris); 
        logPrintf("                             Low Coherence [ High Coherence ]\n");
        logPrintf("    Method                           FPS   [  FPS  |Mverts/sec]\n");
        logPrintf("   ------------------------------------------------+---------\n");
        logPrintf("    glBegin/glEndFPS:                %5.1f [ %5.1f | %5.1f ]\n", vertexPerformance.beginEndFPS[0], vertexPerformance.beginEndFPS[1], vertexPerformance.beginEndFPS[1] * 3 * vertexPerformance.numTris / 1e6);
        logPrintf("    glDrawElements:                  %5.1f [ %5.1f | %5.1f ]\n", vertexPerformance.drawElementsRAMFPS[0], vertexPerformance.drawElementsRAMFPS[1], vertexPerformance.drawElementsRAMFPS[1] * 3 * vertexPerformance.numTris / 1e6);
        logPrintf("        + VBO                        %5.1f [ %5.1f | %5.1f ]\n", vertexPerformance.drawElementsVBOFPS[0], vertexPerformance.drawElementsVBOFPS[1], vertexPerformance.drawElementsVBOFPS[1] * 3 * vertexPerformance.numTris / 1e6);
        logPrintf("        + uint16 index               %5.1f [ %5.1f | %5.1f ]\n", vertexPerformance.drawElementsVBO16FPS[0], vertexPerformance.drawElementsVBO16FPS[1], vertexPerformance.drawElementsVBO16FPS[1] * 3 * vertexPerformance.numTris / 1e6);
        logPrintf("        + interleaved                %5.1f [ %5.1f | %5.1f ]\n", vertexPerformance.drawElementsVBOIFPS[0], vertexPerformance.drawElementsVBOIFPS[1], vertexPerformance.drawElementsVBOIFPS[1] * 3 * vertexPerformance.numTris / 1e6);
        logPrintf("        without shading              %5.1f [ %5.1f | %5.1f ]\n", vertexPerformance.drawElementsVBOPeakFPS[0], vertexPerformance.drawElementsVBOPeakFPS[1], vertexPerformance.drawElementsVBOPeakFPS[1] * 3 * vertexPerformance.numTris / 1e6);
        logPrintf("    glDrawArrays Peak:                     [ %5.1f | %5.1f ]\n", vertexPerformance.drawArraysVBOPeakFPS, vertexPerformance.drawArraysVBOPeakFPS * 3 * vertexPerformance.numTris / 1e6);
        logPrintf("\n\n");
    }
#   endif

    countBugs();
    
    sky = NULL;//Sky::create(NULL, dataDir + "sky/");
}

void App::onCleanup() {}


void App::onSimulation(RealTime sdt, SimTime dt, SimTime idt) {
	// Add physical simulation here

    GameTime deltaTime = 0.02;

    knight.doSimulation(deltaTime);
    ogre.doSimulation(deltaTime);
}


void App::onUserInput(UserInput* ui) {    
    Vector2 mouse = ui->mouseXY();

    if (ui->keyPressed(GKey::LEFT_MOUSE)) {
        if (popup == NONE) {
            if (performanceButton.contains(mouse)) {
                popup = PERFORMANCE;
            }
        } else {
            popup = NONE;
        }
    }

    if (ui->keyPressed(GKey::ESCAPE)) {
        if (popup == NONE) {
            // Even when we aren't in debug mode, quit on escape.
            m_endProgram = true;
        } else {
            popup = NONE;
        }
    }

}


/** Converts a bug count into a quality rating*/
const char* quality(int bugCount) {
    static const char* q[] = {"A+", "A-", "B+", "B-", "C+", "C-", "D+", "D-", "F"};
    return q[iClamp(bugCount, 0, 8)];
}


void App::doFunStuff() {
    renderDevice->pushState();
        GCamera camera;
        camera.setCoordinateFrame(Vector3(0,1,10));
        camera.lookAt(Vector3(0,2.8f,0));

        renderDevice->setProjectionAndCameraMatrix(camera);

        knight.renderShadow(renderDevice);
        ogre.renderShadow(renderDevice);

        renderDevice->enableLighting();
        renderDevice->setLight(0, GLight::directional(Vector3(-1,1,2).direction(), Color3(.8f,.8f,.7f)));
        renderDevice->setLight(1, GLight::directional(Vector3(.5f,-1,1).direction(), Color3::red() * 0.2f));
        renderDevice->setAmbientLightColor(Color3(.5f,.5f,.6f));

        knight.render(renderDevice);
        ogre.render(renderDevice);

    renderDevice->popState();
}


static void drawBar(RenderDevice* rd, int value, const Vector2& p) {
    float s = rd->width() * 0.35f / 100.0f;
    Draw::rect2D(Rect2D::xywh(p.x, p.y, 100 * s, 20), rd, Color3::white() * 0.9f);
    Draw::rect2D(Rect2D::xywh(p.x, p.y, value * s, 20), rd, Color3::yellow());
    Draw::rect2DBorder(Rect2D::xywh(p.x, p.y, 100 * s, 20), rd, Color3::black());
}


Rect2D App::drawPopup(const char* title) {
    int w = renderDevice->width();
    int h = renderDevice->height();

    // Drop shadow
    renderDevice->pushState();
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        Rect2D rect = Rect2D::xywh(w/2 - 20, h/2 - 20, w/2, h/2);
        Draw::rect2D(rect + Vector2(5, 5), renderDevice, Color4(0, 0, 0, 0.15f));
    renderDevice->popState();

    // White box
    Draw::rect2D(rect, renderDevice, Color3::white());
    Draw::rect2DBorder(rect, renderDevice, Color3::black());

    // The close box
    Draw::rect2DBorder(Rect2D::xywh(rect.x1() - 16, rect.y0(), 16, 16), renderDevice, Color3::black());
    renderDevice->setColor(Color3::black());
    renderDevice->beginPrimitive(RenderDevice::LINES);
        renderDevice->sendVertex(Vector2(rect.x1() - 14, rect.y0() + 2));
        renderDevice->sendVertex(Vector2(rect.x1() - 2, rect.y0() + 14));   
        renderDevice->sendVertex(Vector2(rect.x1() - 2, rect.y0() + 2));
        renderDevice->sendVertex(Vector2(rect.x1() - 14, rect.y0() + 14));   
    renderDevice->endPrimitive();

    float s = w * 0.013;
    titleFont->draw2D(renderDevice, title, Vector2(rect.x0() + 4, rect.y0()), s * 1.5, Color3::black(), Color4::clear(), GFont::XALIGN_LEFT, GFont::YALIGN_TOP);

    return rect;
}


void  App::onGraphics (RenderDevice *rd, Array< PosedModelRef > &posed3D, Array< PosedModel2DRef > &posed2D) {
    rd->setColorClearValue(Color3::white());
    rd->clear();

    doFunStuff();

    rd->push2D();

        int w = rd->width();
        int h = rd->height();

        ///////////////////////////////////////
        // Left panel
#       define LABEL(str) p.y += titleFont->draw2D(rd, str, p - Vector2((float)w * 0.0075f, 0), s * 2, Color3::white() * 0.4f).y
#       define PRINT(str) p.y += reportFont->draw2D(rd, str, p, s, Color3::black()).y

        int x0 = int(w * 0.015f);
        // Cursor position
        Vector2 p(x0, h * 0.02f);

        // Font size
        float s = w * 0.013;

        LABEL("Shaders");
        PRINT(std::string("Combiners: ") + combineShader);
        PRINT(std::string("Assembly:   ") + asmShader);
        PRINT(std::string("GLSL:         ") + glslShader);

        p.y += s * 2;
        LABEL("Extensions");
        PRINT(std::string("FSAA:                           ") + ((GLCaps::supports("WGL_ARB_multisample") || GLCaps::supports("GL_ARB_multisample")) ? "Yes" : "No"));
        PRINT(std::string("Two-sided Stencil:        ") + ((GLCaps::supports_two_sided_stencil() ? "Yes" : "No")));
        PRINT(std::string("Stencil Wrap:               ") + (GLCaps::supports("GL_EXT_stencil_wrap") ? "Yes" : "No"));
        PRINT(std::string("Texture Compression: ") + (GLCaps::supports("GL_EXT_texture_compression_s3tc") ? "Yes" : "No"));
        PRINT(std::string("Shadow Maps:             ") + (GLCaps::supports("GL_ARB_shadow") ? "Yes" : "No"));
        PRINT(std::string("Frame Buffer Object:   ") + (GLCaps::supports("GL_EXT_framebuffer_object") ? "Yes" : "No"));
        PRINT(std::string("Vertex Arrays:              ") + (GLCaps::supports_GL_ARB_vertex_buffer_object() ? "Yes" : "No"));
        
            
        ///////////////////////////////////////
        // Right Panel
        x0 = int(w * 0.6f);
        // Cursor position
        p = Vector2(x0, h * 0.02f);

        // Graphics Card
        LABEL("Graphics Card");
        rd->setTexture(0, cardLogo);
        Draw::rect2D(Rect2D::xywh(p.x - s * 6, p.y, s * 5, s * 5), rd);
        rd->setTexture(0, NULL);

        PRINT(GLCaps::vendor().c_str());
        PRINT(GLCaps::renderer().c_str());
        PRINT(format("Driver Version %s", GLCaps::driverVersion().c_str()));

#       ifdef G3D_WIN32        
            PRINT(format("%d MB Video RAM", DXCaps::videoMemorySize() / (1024 * 1024)));
            {
                uint32 ver = DXCaps::version();
                PRINT(format("DirectX %d.%d", ver/100, ver%100));
            }
#       endif

        p.y += s * 2;

        // Processor
        LABEL("Processor");
        rd->setTexture(0, chipLogo);
        Draw::rect2D(Rect2D::xywh(p.x - s * 6, p.y, s * 5, s * 5), rd);
        rd->setTexture(0, NULL);

        PRINT(System::cpuVendor().c_str());
        PRINT(System::cpuArchitecture().c_str());

        Array<std::string> features;
        if (System::has3DNow()) {
            features.append("3DNow");
        }
        if (System::hasMMX()) {
            features.append("MMX");
        }
        if (System::hasSSE()) {
            features.append("SSE");
        }
        if (System::hasSSE2()) {
            features.append("SSE2");
        }
        if (chipSpeed != "") {
            PRINT(chipSpeed + " " + stringJoin(features, '/'));
        } else {
            PRINT(stringJoin(features, '/'));
        }

        p.y += s * 2;

        // Operating System
        LABEL("OS");
        rd->setTexture(0, osLogo);
        Draw::rect2D(Rect2D::xywh(p.x - s * 6, p.y - s * 2, s * 5, s * 5), rd);
        rd->setTexture(0, NULL);


        if (beginsWith(System::operatingSystem(), "Windows 5.0")) {
            PRINT("Windows 2000");
        } else if (beginsWith(System::operatingSystem(), "Windows 5.1")) {
            PRINT("Windows XP");
        }
        PRINT(System::operatingSystem().c_str());

        p.y += s * 3;

        x0 = int(w - s * 10);
        titleFont->draw2D(rd, "Features", p - Vector2(w * 0.0075f, 0), s * 2, Color3::white() * 0.4f);
        p.y += reportFont->draw2D(rd, format("f%d", featureRating), Vector2(x0, p.y), s*2, Color3::red() * 0.5).y;
        drawBar(rd, featureRating, p);

        // Designed to put NV40 at 50
        performanceRating = log(rd->stats().frameRate) * 15.0f;

        p.y += s * 4;
        performanceButton = Rect2D::xywh(p,
            titleFont->draw2D(rd, "Speed", p - Vector2(w * 0.0075f, 0), s * 2, Color3::white() * 0.4f));

		{
			float spd = iRound(performanceRating * 10) / 10.0f;
	        p.y += reportFont->draw2D(rd, format("%5.1f", spd), Vector2(x0 - s*2, p.y), s*2, Color3::red() * 0.5).y;
		}
        drawBar(rd, (int)min(performanceRating, 100.0f), p);

        p.y += s * 4;
        titleFont->draw2D(rd, "Quality", p - Vector2(w * 0.0075f, 0), s * 2, Color3::white() * 0.4f);
        p.y += reportFont->draw2D(rd, quality(bugCount), Vector2(x0, p.y), s*2, Color3::red() * 0.5f).y;
        drawBar(rd, iClamp(100 - bugCount * 10, 0, 100), p);

#       undef PRINT


        p.y = h - 50;
#       define PRINT(str) p.y += reportFont->draw2D(rd, str, p, 8, Color3::black()).y;

        PRINT("These ratings are based on the performance of G3D apps.");
        PRINT("They may not be representative of overall 3D performance.");
        PRINT("Speed is based on both processor and graphics card. Upgrading");
        PRINT("your graphics driver may improve Quality and Features.");
#       undef  PRINT
#       undef LABEL

        switch (popup) {
        case NONE:
            break;

        case PERFORMANCE:
            {
                //  Draw the popup box
                Rect2D box = drawPopup("Performance Details");
                p.x = box.x0() + 10;
                p.y = box.y0() + 30;

                Vector2 spacing(box.width() / 6.5, 0);

                std::string str;

                float factor = 3 * vertexPerformance.numTris / 1e6;

#               define PRINT(cap, val)   \
                    reportFont->draw2D(rd, cap, p, s, Color3::black());\
                    reportFont->draw2D(rd, (vertexPerformance.val[0] > 0) ? \
                        format("%5.1f", vertexPerformance.val[0]) : \
                        std::string("X"), p + spacing * 3, s, Color3::red() * 0.5, Color4::clear(), GFont::XALIGN_RIGHT);\
                    reportFont->draw2D(rd, (vertexPerformance.val[0] > 0) ? \
                        format("%5.1f", factor * vertexPerformance.val[0]) : \
                        std::string("X"), p + spacing * 4, s, Color3::red() * 0.5, Color4::clear(), GFont::XALIGN_RIGHT);\
                    reportFont->draw2D(rd, (vertexPerformance.val[1] > 0) ? \
                        format("%5.1f", vertexPerformance.val[1]) : \
                        std::string("X"), p + spacing * 5, s, Color3::red() * 0.5, Color4::clear(), GFont::XALIGN_RIGHT);\
                    p.y += reportFont->draw2D(rd, (vertexPerformance.val[1] > 0) ? \
                        format("%5.1f", factor * vertexPerformance.val[1]) : \
                        std::string("X"), p + spacing * 6, s, Color3::red() * 0.5, Color4::clear(), GFont::XALIGN_RIGHT).y;

                reportFont->draw2D(rd, "Incoherent", p + spacing * 3.5, s, Color3::black(), Color4::clear(), GFont::XALIGN_RIGHT);
                p.y += reportFont->draw2D(rd, "Coherent", p + spacing * 5.5, s, Color3::black(), Color4::clear(), GFont::XALIGN_RIGHT).y;

                reportFont->draw2D(rd, "FPS*", p + spacing * 3, s, Color3::black(), Color4::clear(), GFont::XALIGN_RIGHT);
                reportFont->draw2D(rd, "MVerts/s", p + spacing * 4, s, Color3::black(), Color4::clear(), GFont::XALIGN_RIGHT);
                reportFont->draw2D(rd, "FPS*", p + spacing * 5, s, Color3::black(), Color4::clear(), GFont::XALIGN_RIGHT).y;
                p.y += reportFont->draw2D(rd, "MVerts/s", p + spacing * 6, s, Color3::black(), Color4::clear(), GFont::XALIGN_RIGHT).y;
                
                PRINT("glBegin/glEnd", beginEndFPS);
                PRINT("glDrawElements", drawElementsRAMFPS); 
                PRINT("  + VBO", drawElementsVBOFPS);
                PRINT("  + uint16", drawElementsVBO16FPS);
                PRINT("  + interleaving", drawElementsVBOIFPS);
                PRINT("  (without shading)", drawElementsVBOPeakFPS);

                reportFont->draw2D(rd, "glDrawArrays", p, s, Color3::black());
                reportFont->draw2D(rd, (vertexPerformance.drawArraysVBOPeakFPS > 0) ? \
                    format("%5.1f", vertexPerformance.drawArraysVBOPeakFPS) : \
                    std::string("X"), p + spacing * 5, s, Color3::red() * 0.5, Color4::clear(), GFont::XALIGN_RIGHT);\
                p.y += reportFont->draw2D(rd, (vertexPerformance.drawArraysVBOPeakFPS > 0) ? \
                    format("%5.1f", factor * vertexPerformance.drawArraysVBOPeakFPS) : \
                    std::string("X"), p + spacing * 6, s, Color3::red() * 0.5, Color4::clear(), GFont::XALIGN_RIGHT).y;

#               undef PRINT

                p.y += s;
                p.y += reportFont->draw2D(rd, format("* FPS at %d k polys/frame.", 
                    iRound(vertexPerformance.numTris / 1000.0)), p + Vector2(20, 0), s, Color3::black()).y;
            }
        }

    rd->pop2D();
}


void App::countBugs() {
    bugCount = 0;

    logPrintf("\n\nBugs:\n");
    if (GLCaps::hasBug_glMultiTexCoord3fvARB()) {
        ++bugCount;
        logPrintf("   Detected glMultiTexCoord3fvARB bug\n\n");
    } 
    

    if (GLCaps::hasBug_normalMapTexGen()) {
        ++bugCount;
        logPrintf("   Detected normalMapTexGen bug\n\n");
    } 

    if (GLCaps::hasBug_slowVBO()) {
        ++bugCount;
        logPrintf("   Detected slowVBO bug\n\n");
    }

    if (GLCaps::hasBug_redBlueMipmapSwap()) {
        ++bugCount;
        logPrintf("   Detected redBlueMipmapSwap bug\n\n");
    }

    if (beginsWith(GLCaps::renderer(), "RADEON") &&
        GLCaps::supports_GL_ARB_shadow() &&
        GLCaps::supports_GL_ARB_shading_language_100()) {
        // Slow shadow map and GLSL texture binding bugs.
        // TODO: add actual tests
        logPrintf("   Detected slowShadowMap bug\n\n");
        logPrintf("   Detected GLSL Texture Binding bug\n\n");
        bugCount += 2;
    }

    if (GLCaps::hasBug_mipmapGeneration()) {
        ++bugCount;
        logPrintf("   Detected mipmapGeneration bug\n\n");
    } 
    
}


void App::computeFeatureRating() {

    if ((glslShader != "None") &&
        GLCaps::supports("GL_ARB_shadow") &&
        GLCaps::supports("GL_EXT_framebuffer_object") &&
        GLCaps::supports("GL_EXT_stencil_wrap")) {

        // NV47 w/ good drivers
        featureRating = 60;

    } else if ((glslShader != "None") &&
        GLCaps::supports("GL_ARB_shadow") &&
        GLCaps::supports("GL_EXT_stencil_wrap")) {

        // R800 w/ good drivers
        featureRating = 50;

    } else if ((asmShader != "None") &&
        GLCaps::supports("GL_ARB_shadow") &&
        GLCaps::supports("GL_EXT_stencil_wrap")) {

        // NV30
        featureRating = 40;

    } else if ((asmShader != "None") &&
        GLCaps::supports("GL_EXT_stencil_wrap")) {

        featureRating = 35;

    } else if (asmShader != "None") {

        // PS 2.0 card
        featureRating = 30;

    } else if ((combineShader != "None") &&
        GLCaps::supports_GL_ARB_multitexture() &&
        GLCaps::supports_GL_ARB_texture_cube_map()) {

        // PS 1.4 card
        featureRating = 20;

    } else if (
        GLCaps::supports_GL_ARB_multitexture() &&
        GLCaps::supports_GL_ARB_texture_cube_map()) {

        featureRating = 15;

    } else if (GLCaps::supports_GL_ARB_texture_cube_map()) {

        featureRating = 14;

    } else {

        featureRating = 10;
    }
}

App::App(const GApp::Settings& settings) : GApp(settings) {

    window()->setCaption(format("gfx-meter %03.1f", gfxMeterVersion));

#   ifndef FAST
        showSplashScreen();
#   endif

    if (reportFont.isNull()) {
        reportFont = GFont::fromFile(dataDir + "arial.fnt");
    }

}


App::~App() {}

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;
    
	settings.useDeveloperTools = false;

    settings.window.fsaaSamples = 1;
    settings.window.fullScreen = true;
	settings.window.framed = false;
    settings.dataDir = "./";
    settings.window.defaultIconFilename = "g3d.ico";

    if (!fileExists(settings.window.defaultIconFilename)) {
        // We are probably running in the debugger and launched from the wrong directory
		chdir("../build/win-i386-vc8.0/bin/gfxmeter");
    }

    App(settings).run();
    return 0;
}
