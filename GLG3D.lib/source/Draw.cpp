/**
 @file Draw.cpp
  
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2003-10-29
 @edited  2006-02-18
 */
#include "G3D/platform.h"
#include "G3D/Rect2D.h"
#include "G3D/AABox.h"
#include "G3D/Box.h"
#include "G3D/Ray.h"
#include "G3D/Sphere.h"
#include "G3D/Line.h"
#include "G3D/LineSegment.h"
#include "G3D/Capsule.h"
#include "G3D/Cylinder.h"
#include "GLG3D/Draw.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GLCaps.h"
#include "G3D/MeshAlg.h"
#include "G3D/PhysicsFrameSpline.h"

namespace G3D {

const int Draw::WIRE_SPHERE_SECTIONS = 26;
const int Draw::SPHERE_SECTIONS = 40;

const int Draw::SPHERE_PITCH_SECTIONS = 20;
const int Draw::SPHERE_YAW_SECTIONS = 40;

void Draw::physicsFrameSpline(const PhysicsFrameSpline& spline, RenderDevice* rd) {
    rd->pushState();
    for (int i = 0; i < spline.control.size(); ++i) {
        const CFrame& c = spline.control[i];

        Draw::axes(c, rd, Color3::red(), Color3::green(), Color3::blue(), 0.5f);
        Draw::sphere(Sphere(c.translation, 0.1f), rd, Color3::white(), Color4::clear());
    }

    const int N = spline.control.size() * 30;
    CFrame last = spline.evaluate(0);
    const float a = 0.5f;
    rd->setLineWidth(1);
    rd->beginPrimitive(PrimitiveType::LINES);
    for (int i = 1; i < N; ++i) {
        float t = (spline.control.size() - 1) * i / (N - 1.0f);
        const CFrame& cur = spline.evaluate(t);
        rd->setColor(Color4(1,1,1,a));
        rd->sendVertex(last.translation);
        rd->sendVertex(cur.translation);

        rd->setColor(Color4(1,0,0,a));
        rd->sendVertex(last.rightVector() + last.translation);
        rd->sendVertex(cur.rightVector() + cur.translation);

        rd->setColor(Color4(0,1,0,a));
        rd->sendVertex(last.upVector() + last.translation);
        rd->sendVertex(cur.upVector() + cur.translation);

        rd->setColor(Color4(0,0,1,a));
        rd->sendVertex(-last.lookVector() + last.translation);
        rd->sendVertex(-cur.lookVector() + cur.translation);

        last = cur;
    }
    rd->endPrimitive();
    rd->popState();
}

/** Draws a single sky-box vertex.  Called from Draw::skyBox. (s,t) are
    texture coordinates for the case where the cube map is not used.*/
static void skyVertex(RenderDevice* renderDevice, 
                 bool cube,
                 const Texture::Ref* texture,
                 float x, float y, float z, float s, float t) {
    const float w = 0;
    static bool explicitTexCoord = GLCaps::hasBug_normalMapTexGen();

    if (cube) {
        if (explicitTexCoord) {
            glTexCoord3f(x, y, z);
        } else {
            // Texcoord generation will move this normal to tex coord 0
            renderDevice->setNormal(Vector3(x,y,z));
        }
    } else {
        if (!GLCaps::supports_GL_EXT_texture_edge_clamp()) {
            // Move the edge coordinates towards the center just
            // enough that the black clamped border isn't sampled.
            if (s == 0) {
                s += (0.6f / (float)texture[0]->width());
            } else if (s == 1) {
                s -= (0.6f / (float)texture[0]->width());
            }
            if (t == 0) {
                t += (0.6f / (float)texture[0]->height());
            } else if (t == 1) {
                t -= (0.6f / (float)texture[0]->height());
            }
        }
        renderDevice->setTexCoord(0, Vector2(s, t));
    }
    
    renderDevice->sendVertex(Vector4(x,y,z,w));
}


void Draw::skyBox(RenderDevice* renderDevice, const Texture::Ref& cubeMap, const Texture::Ref* texture) {
    enum Direction {UP = 0, LT = 1, RT = 2, BK = 3, FT = 4, DN = 5};
    renderDevice->pushState();

    // Make a camera with an infinite view frustum
    GCamera camera = renderDevice->projectionAndCameraMatrix();
    camera.setFarPlaneZ(-finf());
    renderDevice->setProjectionAndCameraMatrix(camera);

    bool cube = (cubeMap.notNull());

    if (cube) {
        renderDevice->setTexture(0, cubeMap);

        if (! GLCaps::hasBug_normalMapTexGen()) {
            // On old Radeon Mobility, explicit cube map coordinates don't work right.
            // We instead put cube map coords in the normals and use texgen to copy
            // them over
            glActiveTextureARB(GL_TEXTURE0_ARB + 0);
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
            glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
            glEnable(GL_TEXTURE_GEN_R);
        } else {
            // Hope that we're on a card where explicit texcoords work
        }

        CoordinateFrame cframe = renderDevice->cameraToWorldMatrix();
        cframe.translation = Vector3::zero();
        renderDevice->setTextureMatrix(0, cframe);

    } else {
        // In the 6-texture case, the sky box is rotated 90 degrees
        // (this is because the textures are loaded incorrectly)
 
        renderDevice->setObjectToWorldMatrix(Matrix3::fromAxisAngle(Vector3::unitY(), toRadians(-90)));
        renderDevice->setTexture(0, texture[BK]);
    }

    float s = 1;
    renderDevice->beginPrimitive(PrimitiveType::QUADS);
        skyVertex(renderDevice, cube, texture, -s, +s, -s, 0, 0);
        skyVertex(renderDevice, cube, texture, -s, -s, -s, 0, 1);
        skyVertex(renderDevice, cube, texture, +s, -s, -s, 1, 1);
        skyVertex(renderDevice, cube, texture, +s, +s, -s, 1, 0);
	renderDevice->endPrimitive();
        
    if (! cube) {
        renderDevice->setTexture(0, texture[LT]);
    }

    renderDevice->beginPrimitive(PrimitiveType::QUADS);
        skyVertex(renderDevice, cube, texture, -s, +s, +s, 0, 0);
        skyVertex(renderDevice, cube, texture, -s, -s, +s, 0, 1);
        skyVertex(renderDevice, cube, texture, -s, -s, -s, 1, 1);
        skyVertex(renderDevice, cube, texture, -s, +s, -s, 1, 0);
	renderDevice->endPrimitive();

    
    if (! cube) {
        renderDevice->setTexture(0, texture[FT]);
    }

    renderDevice->beginPrimitive(PrimitiveType::QUADS);
        skyVertex(renderDevice, cube, texture, +s, +s, +s, 0, 0);
        skyVertex(renderDevice, cube, texture, +s, -s, +s, 0, 1);
        skyVertex(renderDevice, cube, texture, -s, -s, +s, 1, 1);
        skyVertex(renderDevice, cube, texture, -s, +s, +s, 1, 0);
	renderDevice->endPrimitive();

    if (! cube) {
        renderDevice->setTexture(0, texture[RT]);
    }

    renderDevice->beginPrimitive(PrimitiveType::QUADS);
        skyVertex(renderDevice, cube, texture, +s, +s, +s, 1, 0);
        skyVertex(renderDevice, cube, texture, +s, +s, -s, 0, 0);
        skyVertex(renderDevice, cube, texture, +s, -s, -s, 0, 1);
        skyVertex(renderDevice, cube, texture, +s, -s, +s, 1, 1);
	renderDevice->endPrimitive();

    if (! cube) {
        renderDevice->setTexture(0, texture[UP]);
    }

    renderDevice->beginPrimitive(PrimitiveType::QUADS);
        skyVertex(renderDevice, cube, texture, +s, +s, +s, 1, 1);
        skyVertex(renderDevice, cube, texture, -s, +s, +s, 1, 0);
        skyVertex(renderDevice, cube, texture, -s, +s, -s, 0, 0);
        skyVertex(renderDevice, cube, texture, +s, +s, -s, 0, 1);
	renderDevice->endPrimitive();

    if (! cube) {
        renderDevice->setTexture(0, texture[DN]);
    }

    renderDevice->beginPrimitive(PrimitiveType::QUADS);
        skyVertex(renderDevice, cube, texture, +s, -s, -s, 0, 0);
        skyVertex(renderDevice, cube, texture, -s, -s, -s, 0, 1);
        skyVertex(renderDevice, cube, texture, -s, -s, +s, 1, 1);
        skyVertex(renderDevice, cube, texture, +s, -s, +s, 1, 0);
	renderDevice->endPrimitive();

    if (! GLCaps::hasBug_normalMapTexGen() && cube) {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_GEN_R);
    }
    renderDevice->popState();

}



void Draw::poly2DOutline(const Array<Vector2>& polygon, RenderDevice* renderDevice, const Color4& color) {
    if (polygon.length() == 0) {
        return;
    }
    
    renderDevice->beginPrimitive(PrimitiveType::LINE_STRIP);
        renderDevice->setColor(color);
        for (int i = 0; i < polygon.length(); ++i) {
            renderDevice->sendVertex(polygon[i]);
        }
        renderDevice->sendVertex(polygon[0]);
    renderDevice->endPrimitive();
}


void Draw::poly2D(const Array<Vector2>& polygon, RenderDevice* renderDevice, const Color4& color) {
    if (polygon.length() == 0) {
        return;
    }
    
    renderDevice->beginPrimitive(PrimitiveType::TRIANGLE_FAN);
        renderDevice->setColor(color);
        for (int i = 0; i < polygon.length(); ++i) {
            renderDevice->sendVertex(polygon[i]);
        }
    renderDevice->endPrimitive();
}

/** \param dirDist Distance at which to render directional lights */
static void drawLight(const GLight& light, RenderDevice* rd, bool showEffectSpheres, float dirDist) {
    if (light.position.w != 0) {
        // Point light
        Draw::sphere(Sphere(light.position.xyz(), 0.1f), rd, light.color, Color4::clear());
        if (showEffectSpheres) {
            Sphere s = light.effectSphere();
            if (s.radius < finf()) {
                Draw::sphere(s, rd, Color4::clear(), Color4(light.color / max(0.01f, light.color.max()), 0.5f));
            }
        }
    } else {
        // Directional light
        Draw::sphere(Sphere(light.position.xyz() * dirDist, 0.1f * dirDist), rd, light.color / max(0.01f, light.color.max()), Color4::clear());
    }
}

void Draw::lighting(Lighting::Ref lighting, RenderDevice* rd, bool showEffectSpheres) {
    rd->pushState();
        rd->setObjectToWorldMatrix(CFrame());
        rd->setShader(NULL);
        rd->disableLighting();
        const GCamera& camera = rd->projectionAndCameraMatrix();
        float dirDist = min(200.0f, (float)fabs((float)camera.farPlaneZ()) * 0.9f);
        for (int L = 0; L < lighting->lightArray.size(); ++L) {
            drawLight(lighting->lightArray[L], rd, showEffectSpheres, dirDist);
        }
        for (int L = 0; L < lighting->shadowedLightArray.size(); ++L) {
            drawLight(lighting->shadowedLightArray[L], rd, showEffectSpheres, dirDist);
        }
    rd->popState();
}

void Draw::axes(
    RenderDevice*       renderDevice,
    const Color4&       xColor,
    const Color4&       yColor,
    const Color4&       zColor,
    float               scale) {

    axes(CFrame(), renderDevice, xColor, yColor, zColor, scale);
}


void Draw::arrow(
    const Vector3&      start,
    const Vector3&      direction,
    RenderDevice*       renderDevice,
    const Color4&       color,
    float               scale) {

    Vector3 tip = start + direction;
    // Create a coordinate frame at the tip
    Vector3 u = direction;
    Vector3 v;
    if (abs(u.x) < abs(u.y)) {
        v = Vector3::unitX();
    } else {
        v = Vector3::unitY();
    }
    Vector3 w = u.cross(v).direction();
    v = w.cross(u).direction();
    Vector3 back = tip - u * 0.3f * scale;

    RenderDevice::ShadeMode oldShadeMode = renderDevice->shadeMode();
    Color4 oldColor = renderDevice->color();

    renderDevice->setShadeMode(RenderDevice::SHADE_SMOOTH);
    renderDevice->setColor(color);

    float r = scale * 0.1f;
    // Arrow head.  Need this beginprimitive call to sync up G3D and OpenGL
    renderDevice->beginPrimitive(PrimitiveType::TRIANGLES);
        for (int a = 0; a < SPHERE_SECTIONS; ++a) {
            float angle0 = a * (float)twoPi() / SPHERE_SECTIONS;
            float angle1 = (a + 1) * (float)twoPi() / SPHERE_SECTIONS;
            Vector3 dir0(cos(angle0) * v + sin(angle0) * w);
            Vector3 dir1(cos(angle1) * v + sin(angle1) * w);
            glNormal(dir0);
            glVertex(tip);

            glVertex(back + dir0 * r);

            glNormal(dir1);
            glVertex(back + dir1 * r);                
        }
    renderDevice->endPrimitive();
    renderDevice->minGLStateChange(SPHERE_SECTIONS * 5);

    // Back of arrow head
    renderDevice->beforePrimitive();
    glBegin(GL_TRIANGLE_FAN);
        glNormal(-u);
        for (int a = 0; a < SPHERE_SECTIONS; ++a) {
            float angle = a * (float)twoPi() / SPHERE_SECTIONS;
            Vector3 dir = sin(angle) * v + cos(angle) * w;
            glVertex(back + dir * r);
        }
    glEnd();
    renderDevice->afterPrimitive();
    renderDevice->minGLStateChange(SPHERE_SECTIONS);

    renderDevice->setColor(oldColor);
    renderDevice->setShadeMode(oldShadeMode);
    lineSegment(LineSegment::fromTwoPoints(start, back), renderDevice, color, scale);
}


void Draw::axes(
    const CoordinateFrame& cframe,
    RenderDevice*       renderDevice,
    const Color4&       xColor,
    const Color4&       yColor,
    const Color4&       zColor,
    float               scale) {

    Vector3 c = cframe.translation;
    Vector3 x = cframe.rotation.column(0).direction() * 2 * scale;
    Vector3 y = cframe.rotation.column(1).direction() * 2 * scale;
    Vector3 z = cframe.rotation.column(2).direction() * 2 * scale;

    Draw::arrow(c, x, renderDevice, xColor, scale);
    Draw::arrow(c, y, renderDevice, yColor, scale);
    Draw::arrow(c, z, renderDevice, zColor, scale);
  
    // Text label scale
    const float xx = -3;
    const float yy = xx * 1.4f;

    // Project the 3D locations of the labels
    Vector4 xc2D = renderDevice->project(c + x * 1.1f);
    Vector4 yc2D = renderDevice->project(c + y * 1.1f);
    Vector4 zc2D = renderDevice->project(c + z * 1.1f);

    // If coordinates are behind the viewer, transform off screen
    Vector2 x2D = (xc2D.w > 0) ? xc2D.xy() : Vector2(-2000, -2000);
    Vector2 y2D = (yc2D.w > 0) ? yc2D.xy() : Vector2(-2000, -2000);
    Vector2 z2D = (zc2D.w > 0) ? zc2D.xy() : Vector2(-2000, -2000);

    // Compute the size of the labels
    float xS = (xc2D.w > 0) ? clamp(10 * xc2D.w * scale, .1f, 5) : 0;
    float yS = (yc2D.w > 0) ? clamp(10 * yc2D.w * scale, .1f, 5) : 0;
    float zS = (zc2D.w > 0) ? clamp(10 * zc2D.w * scale, .1f, 5) : 0;

    renderDevice->push2D();
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        renderDevice->setLineWidth(2);

        renderDevice->beginPrimitive(PrimitiveType::LINES);
            // X
            renderDevice->setColor(xColor * 0.8f);
            renderDevice->sendVertex(Vector2(-xx,  yy) * xS + x2D);
            renderDevice->sendVertex(Vector2( xx, -yy) * xS + x2D);
            renderDevice->sendVertex(Vector2( xx,  yy) * xS + x2D);
            renderDevice->sendVertex(Vector2(-xx, -yy) * xS + x2D);

            // Y
            renderDevice->setColor(yColor * 0.8f);
            renderDevice->sendVertex(Vector2(-xx,  yy) * yS + y2D);
            renderDevice->sendVertex(Vector2(  0,  0)  * yS + y2D);
            renderDevice->sendVertex(Vector2(  0,  0)  * yS + y2D);
            renderDevice->sendVertex(Vector2(  0, -yy) * yS + y2D);
            renderDevice->sendVertex(Vector2( xx,  yy) * yS + y2D);
            renderDevice->sendVertex(Vector2(  0,  0)  * yS + y2D);
        renderDevice->endPrimitive();

        renderDevice->beginPrimitive(PrimitiveType::LINE_STRIP);
            // Z
            renderDevice->setColor(zColor * 0.8f);    
            renderDevice->sendVertex(Vector2( xx,  yy) * zS + z2D);
            renderDevice->sendVertex(Vector2(-xx,  yy) * zS + z2D);
            renderDevice->sendVertex(Vector2( xx, -yy) * zS + z2D);
            renderDevice->sendVertex(Vector2(-xx, -yy) * zS + z2D);
        renderDevice->endPrimitive();
    renderDevice->pop2D();
}


void Draw::ray(
    const Ray&          ray,
    RenderDevice*       renderDevice,
    const Color4&       color,
    float               scale) {
    Draw::arrow(ray.origin(), ray.direction(), renderDevice, color, scale);
}


void Draw::plane(
    const Plane&         plane, 
    RenderDevice*        renderDevice,
    const Color4&        solidColor,
    const Color4&        wireColor) {

    renderDevice->pushState();
    CoordinateFrame cframe0 = renderDevice->objectToWorldMatrix();

    Vector3 N, P;
    
    {
        float d;
        plane.getEquation(N, d);
        float distance = -d;
        P = N * distance;
    }

    CoordinateFrame cframe1(P);
    cframe1.lookAt(P + N);

    renderDevice->setObjectToWorldMatrix(cframe0 * cframe1);

    renderDevice->setShadeMode(RenderDevice::SHADE_SMOOTH);
    renderDevice->enableTwoSidedLighting();

    if (solidColor.a > 0.0f) {
        // Draw concentric circles around the origin.  We break them up to get good depth
        // interpolation and reasonable shading.

        renderDevice->setPolygonOffset(0.7f);

        if (solidColor.a < 1.0f) {
            renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        }

        renderDevice->setNormal(Vector3::unitZ());
        renderDevice->setColor(solidColor);

        renderDevice->setCullFace(RenderDevice::CULL_NONE);
        // Infinite strip
        const int numStrips = 12;
        float r1 = 100;
        renderDevice->beginPrimitive(PrimitiveType::QUAD_STRIP);
            for (int i = 0; i <= numStrips; ++i) {
                float a = i * (float)twoPi() / numStrips;
                float c = cosf(a);
                float s = sinf(a);

                renderDevice->sendVertex(Vector3(c * r1, s * r1, 0));
                renderDevice->sendVertex(Vector4(c, s, 0, 0));
            }
        renderDevice->endPrimitive();

        // Finite strips.  
        float r2 = 0;
        const int M = 4;
        for (int j = 0; j < M; ++j) {
            r2 = r1;
            r1 = r1 / 3;
            if (j == M - 1) {
                // last pass
                r1 = 0;
            }

            renderDevice->beginPrimitive(PrimitiveType::QUAD_STRIP);
                for (int i = 0; i <= numStrips; ++i) {
                    float a = i * (float)twoPi() / numStrips;
                    float c = cosf(a);
                    float s = sinf(a);

                    renderDevice->sendVertex(Vector3(c * r1, s * r1, 0));
                    renderDevice->sendVertex(Vector3(c * r2, s * r2, 0));
                }
            renderDevice->endPrimitive();
        }


    }

    if (wireColor.a > 0) {
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        renderDevice->setLineWidth(1.5);

        renderDevice->beginPrimitive(PrimitiveType::LINES);
        {
            renderDevice->setColor(wireColor);
            renderDevice->setNormal(Vector3::unitZ());

            // Lines to infinity
            renderDevice->sendVertex(Vector4( 1, 0, 0, 0));
            renderDevice->sendVertex(Vector4( 0, 0, 0, 1));

            renderDevice->sendVertex(Vector4(-1, 0, 0, 0));
            renderDevice->sendVertex(Vector4( 0, 0, 0, 1));

            renderDevice->sendVertex(Vector4(0, -1, 0, 0));
            renderDevice->sendVertex(Vector4(0,  0, 0, 1));

            renderDevice->sendVertex(Vector4(0,  1, 0, 0));
            renderDevice->sendVertex(Vector4(0,  0, 0, 1));
        }
        renderDevice->endPrimitive();

        renderDevice->setLineWidth(0.5);

        renderDevice->beginPrimitive(PrimitiveType::LINES);

            // Horizontal and vertical lines
            const int numStrips = 10;
            const float space = 1;
            const float Ns = numStrips * space;
            for (int x = -numStrips; x <= numStrips; ++x) {
                float sx = x * space;
                renderDevice->sendVertex(Vector3( Ns, sx, 0));
                renderDevice->sendVertex(Vector3(-Ns, sx, 0));

                renderDevice->sendVertex(Vector3(sx,  Ns, 0));
                renderDevice->sendVertex(Vector3(sx, -Ns, 0));
            }

        renderDevice->endPrimitive();
    }

    renderDevice->popState();
}
 


void Draw::capsule(
    const Capsule&       capsule, 
    RenderDevice*        renderDevice,
    const Color4&        solidColor,
    const Color4&        wireColor) {

    CoordinateFrame cframe(capsule.point(0));

    Vector3 Y = (capsule.point(1) - capsule.point(0)).direction();
    Vector3 X = (abs(Y.dot(Vector3::unitX())) > 0.9) ? Vector3::unitY() : Vector3::unitX();
    Vector3 Z = X.cross(Y).direction();
    X = Y.cross(Z);        
    cframe.rotation.setColumn(0, X);
    cframe.rotation.setColumn(1, Y);
    cframe.rotation.setColumn(2, Z);

    float radius = capsule.radius();
    float height = (capsule.point(1) - capsule.point(0)).magnitude();

    // Always render upright in object space
    Sphere sphere1(Vector3::zero(), radius);
    Sphere sphere2(Vector3(0, height, 0), radius);

    Vector3 top(0, height, 0);

    renderDevice->pushState();

        renderDevice->setObjectToWorldMatrix(renderDevice->objectToWorldMatrix() * cframe);
        renderDevice->setShadeMode(RenderDevice::SHADE_SMOOTH);

        if (solidColor.a > 0) {
            int numPasses = 1;

            if (solidColor.a < 1) {
                // Multiple rendering passes to get front/back blending correct
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                numPasses = 2;
                renderDevice->setCullFace(RenderDevice::CULL_FRONT);
                renderDevice->setDepthWrite(false);
            }

            renderDevice->setColor(solidColor);
            for (int k = 0; k < numPasses; ++k) {
                sphereSection(sphere1, renderDevice, solidColor, false, true);
                sphereSection(sphere2, renderDevice, solidColor, true, false);

                // Cylinder faces
                renderDevice->beginPrimitive(PrimitiveType::QUAD_STRIP);
                    for (int y = 0; y <= SPHERE_SECTIONS; ++y) {
                        const float yaw0 = y * (float)twoPi() / SPHERE_SECTIONS;
                        Vector3 v0 = Vector3(cosf(yaw0), 0, sinf(yaw0));

                        renderDevice->setNormal(v0);
                        renderDevice->sendVertex(v0 * radius);
                        renderDevice->sendVertex(v0 * radius + top);
                    }
                renderDevice->endPrimitive();

                renderDevice->setCullFace(RenderDevice::CULL_BACK);
            }

        }

        if (wireColor.a > 0) {
            renderDevice->setDepthWrite(true);
            renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

            wireSphereSection(sphere1, renderDevice, wireColor, false, true);
            wireSphereSection(sphere2, renderDevice, wireColor, true, false);

            // Line around center
            renderDevice->setColor(wireColor);
            Vector3 center(0, height / 2, 0);
            renderDevice->setLineWidth(2);
            renderDevice->beginPrimitive(PrimitiveType::LINES);
                for (int y = 0; y < WIRE_SPHERE_SECTIONS; ++y) {
                    const float yaw0 = y * (float)twoPi() / WIRE_SPHERE_SECTIONS;
                    const float yaw1 = (y + 1) * (float)twoPi() / WIRE_SPHERE_SECTIONS;

                    Vector3 v0(cosf(yaw0), 0, sinf(yaw0));
                    Vector3 v1(cosf(yaw1), 0, sinf(yaw1));

                    renderDevice->setNormal(v0);
                    renderDevice->sendVertex(v0 * radius + center);
                    renderDevice->setNormal(v1);
                    renderDevice->sendVertex(v1 * radius + center);
                }

                // Edge lines
                for (int y = 0; y < 8; ++y) {
                    const float yaw = y * (float)pi() / 4;
                    const Vector3 x(cosf(yaw), 0, sinf(yaw));
        
                    renderDevice->setNormal(x);
                    renderDevice->sendVertex(x * radius);
                    renderDevice->sendVertex(x * radius + top);
                }
            renderDevice->endPrimitive();
        }

    renderDevice->popState();
}


void Draw::cylinder(
    const Cylinder&      cylinder, 
    RenderDevice*        renderDevice,
    const Color4&        solidColor,
    const Color4&        wireColor) {

    CoordinateFrame cframe;
    
    cylinder.getReferenceFrame(cframe);

    float radius = cylinder.radius();
    float height = cylinder.height();

    // Always render upright in object space
    Vector3 bot(0, -height / 2, 0);
    Vector3 top(0, height / 2, 0);

    renderDevice->pushState();

        renderDevice->setObjectToWorldMatrix(renderDevice->objectToWorldMatrix() * cframe);
        renderDevice->setShadeMode(RenderDevice::SHADE_SMOOTH);

        if (solidColor.a > 0) {
            int numPasses = 1;

            if (solidColor.a < 1) {
                // Multiple rendering passes to get front/back blending correct
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                numPasses = 2;
                renderDevice->setCullFace(RenderDevice::CULL_FRONT);
                renderDevice->setDepthWrite(false);
            }

            renderDevice->setColor(solidColor);
            for (int k = 0; k < numPasses; ++k) {

                // Top
                renderDevice->beginPrimitive(PrimitiveType::TRIANGLE_FAN);
                    renderDevice->setNormal(Vector3::unitY());
                    renderDevice->sendVertex(top);
                    for (int y = 0; y <= SPHERE_SECTIONS; ++y) {
                        const float yaw0 = -y * (float)twoPi() / SPHERE_SECTIONS;
                        Vector3 v0 = Vector3(cosf(yaw0), 0, sinf(yaw0));

                        renderDevice->sendVertex(v0 * radius + top);
                    }
                renderDevice->endPrimitive();

                // Bottom
                renderDevice->beginPrimitive(PrimitiveType::TRIANGLE_FAN);
                    renderDevice->setNormal(-Vector3::unitY());
                    renderDevice->sendVertex(bot);
                    for (int y = 0; y <= SPHERE_SECTIONS; ++y) {
                        const float yaw0 = y * (float)twoPi() / SPHERE_SECTIONS;
                        Vector3 v0 = Vector3(cosf(yaw0), 0, sinf(yaw0));

                        renderDevice->sendVertex(v0 * radius + bot);
                    }
                renderDevice->endPrimitive();

                // Cylinder faces
                renderDevice->beginPrimitive(PrimitiveType::QUAD_STRIP);
                    for (int y = 0; y <= SPHERE_SECTIONS; ++y) {
                        const float yaw0 = y * (float)twoPi() / SPHERE_SECTIONS;
                        Vector3 v0 = Vector3(cosf(yaw0), 0, sinf(yaw0));

                        renderDevice->setNormal(v0);
                        renderDevice->sendVertex(v0 * radius + bot);
                        renderDevice->sendVertex(v0 * radius + top);
                    }
                renderDevice->endPrimitive();

                renderDevice->setCullFace(RenderDevice::CULL_BACK);
            }

        }

        if (wireColor.a > 0) {
            renderDevice->setDepthWrite(true);
            renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

            // Line around center
            renderDevice->setColor(wireColor);
            renderDevice->setLineWidth(2);
            renderDevice->beginPrimitive(PrimitiveType::LINES);
                for (int z = 0; z < 3; ++z) {
                    Vector3 center(0, 0.0, 0);
                    for (int y = 0; y < WIRE_SPHERE_SECTIONS; ++y) {
                        const float yaw0 = y * (float)twoPi() / WIRE_SPHERE_SECTIONS;
                        const float yaw1 = (y + 1) * (float)twoPi() / WIRE_SPHERE_SECTIONS;

                        Vector3 v0(cos(yaw0), 0, sin(yaw0));
                        Vector3 v1(cos(yaw1), 0, sin(yaw1));

                        renderDevice->setNormal(v0);
                        renderDevice->sendVertex(v0 * radius + center);
                        renderDevice->setNormal(v1);
                        renderDevice->sendVertex(v1 * radius + center);
                    }
                }

                // Edge lines
                for (int y = 0; y < 8; ++y) {
                    const float yaw = y * (float)pi() / 4;
                    const Vector3 x(cos(yaw), 0, sin(yaw));
                    const Vector3 xr = x * radius;
    
                    // Side
                    renderDevice->setNormal(x);
                    renderDevice->sendVertex(xr + bot);
                    renderDevice->sendVertex(xr + top);

                    // Top
                    renderDevice->setNormal(Vector3::unitY());
                    renderDevice->sendVertex(top);
                    renderDevice->sendVertex(xr + top);

                    // Bottom
                    renderDevice->setNormal(Vector3::unitY());
                    renderDevice->sendVertex(bot);
                    renderDevice->sendVertex(xr + bot);
                }
            renderDevice->endPrimitive();
        }

    renderDevice->popState();
}

    
void Draw::vertexNormals(
    const MeshAlg::Geometry&    geometry,
    RenderDevice*               renderDevice,
    const Color4&               color,
    float                       scale) {

    renderDevice->pushState();
        renderDevice->setColor(color);
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        const Array<Vector3>& vertexArray = geometry.vertexArray;
        const Array<Vector3>& normalArray = geometry.normalArray;

        const float D = clamp(5.0f / ::powf((float)vertexArray.size(), 0.25f), 0.1f, .8f) * scale;
        
        renderDevice->setLineWidth(1);
        renderDevice->beginPrimitive(PrimitiveType::LINES);
            for (int v = 0; v < vertexArray.size(); ++v) {
                renderDevice->sendVertex(vertexArray[v] + normalArray[v] * D);
                renderDevice->sendVertex(vertexArray[v]);
            }
        renderDevice->endPrimitive();
        
        renderDevice->setLineWidth(2);
        renderDevice->beginPrimitive(PrimitiveType::LINES);
            for (int v = 0; v < vertexArray.size(); ++v) {
                renderDevice->sendVertex(vertexArray[v] + normalArray[v] * D * .96f);
                renderDevice->sendVertex(vertexArray[v] + normalArray[v] * D * .84f);
            }
        renderDevice->endPrimitive();

        renderDevice->setLineWidth(3);
        renderDevice->beginPrimitive(PrimitiveType::LINES);
            for (int v = 0; v < vertexArray.size(); ++v) {
                renderDevice->sendVertex(vertexArray[v] + normalArray[v] * D * .92f);
                renderDevice->sendVertex(vertexArray[v] + normalArray[v] * D * .84f);
            }
        renderDevice->endPrimitive();
    renderDevice->popState();
}


void Draw::vertexVectors(
    const Array<Vector3>&       vertexArray,
    const Array<Vector3>&       directionArray,
    RenderDevice*               renderDevice,
    const Color4&               color,
    float                       scale) {

    renderDevice->pushState();
        renderDevice->setColor(color);
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        const float D = clamp(5.0f / ::powf((float)vertexArray.size(), 0.25f), 0.1f, 0.8f) * scale;
        
        renderDevice->setLineWidth(1);
        renderDevice->beginPrimitive(PrimitiveType::LINES);
            for (int v = 0; v < vertexArray.size(); ++v) {
                renderDevice->sendVertex(vertexArray[v] + directionArray[v] * D);
                renderDevice->sendVertex(vertexArray[v]);
            }
        renderDevice->endPrimitive();
        
        renderDevice->setLineWidth(2);
        renderDevice->beginPrimitive(PrimitiveType::LINES);
            for (int v = 0; v < vertexArray.size(); ++v) {
                renderDevice->sendVertex(vertexArray[v] + directionArray[v] * D * .96f);
                renderDevice->sendVertex(vertexArray[v] + directionArray[v] * D * .84f);
            }
        renderDevice->endPrimitive();

        renderDevice->setLineWidth(3);
        renderDevice->beginPrimitive(PrimitiveType::LINES);
            for (int v = 0; v < vertexArray.size(); ++v) {
                renderDevice->sendVertex(vertexArray[v] + directionArray[v] * D * .92f);
                renderDevice->sendVertex(vertexArray[v] + directionArray[v] * D * .84f);
            }
        renderDevice->endPrimitive();
    renderDevice->popState();
}


void Draw::line(
    const Line&         line,
    RenderDevice*       renderDevice,
    const Color4&       color) {

    renderDevice->pushState();
        renderDevice->setShadeMode(RenderDevice::SHADE_SMOOTH);
        renderDevice->setColor(color);
        renderDevice->setLineWidth(2);
        renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        Vector3 v0 = line.point();
        Vector3 d  = line.direction();
        renderDevice->beginPrimitive(PrimitiveType::LINE_STRIP);
            // Off to infinity
            renderDevice->sendVertex(Vector4(-d, 0));

            for (int i = -10; i <= 10; i += 2) {
                renderDevice->sendVertex(v0 + d * (float)i * 100.0f);
            }

            // Off to infinity
            renderDevice->sendVertex(Vector4(d, 0));
        renderDevice->endPrimitive();
    renderDevice->popState();
}

void Draw::lineSegment(
    const LineSegment&  lineSegment,
    RenderDevice*       renderDevice,
    const Color4&       color,
    float               scale) {

    renderDevice->pushState();

        renderDevice->setShadeMode(RenderDevice::SHADE_SMOOTH);
        renderDevice->setColor(color);
        
        // Compute perspective line width
        Vector3 v0 = lineSegment.point(0);
        Vector3 v1 = lineSegment.point(1);

        Vector4 s0 = renderDevice->project(v0);
        Vector4 s1 = renderDevice->project(v1);

        float L = 2 * scale;
        if ((s0.w > 0) && (s1.w > 0)) {
            L = 15.0f * (s0.w + s1.w) / 2.0f;
        } else if (s0.w > 0) {
            L = max(15 * s0.w, 10.0f);
        } else if (s1.w > 0) {
            L = max(15.0f * s1.w, 10.0f);
        }

        renderDevice->setLineWidth(L);

        // Find the object space vector perpendicular to the line
        // that points closest to the eye.
        Vector3 eye = renderDevice->objectToWorldMatrix().pointToObjectSpace(renderDevice->cameraToWorldMatrix().translation);
        Vector3 E = eye - v0;
        Vector3 V = v1 - v0;
        Vector3 U = E.cross(V);
        Vector3 N = V.cross(U).direction();

        //renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        renderDevice->beginPrimitive(PrimitiveType::LINES);        
            renderDevice->setNormal(N);
            renderDevice->sendVertex(v0);
            renderDevice->sendVertex(v1);
        renderDevice->endPrimitive();
    renderDevice->popState();
}


void Draw::box(
    const AABox&        _box,
    RenderDevice*       renderDevice,
    const Color4&       solidColor,
    const Color4&       wireColor) {

    box(Box(_box), renderDevice, solidColor, wireColor);
}


void Draw::box(
    const Box&          box,
    RenderDevice*       renderDevice,
    const Color4&       solidColor,
    const Color4&       wireColor) {

    renderDevice->pushState();
        renderDevice->setShadeMode(RenderDevice::SHADE_SMOOTH);

        if (solidColor.a > 0) {
            int numPasses = 1;

            if (solidColor.a < 1) {
                // Multiple rendering passes to get front/back blending correct
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                numPasses = 2;
                renderDevice->setCullFace(RenderDevice::CULL_FRONT);
                renderDevice->setDepthWrite(false);
            } else {
                renderDevice->setCullFace(RenderDevice::CULL_BACK);
            }

            renderDevice->setColor(solidColor);
            for (int k = 0; k < numPasses; ++k) {
                renderDevice->beginPrimitive(PrimitiveType::QUADS);
                    for (int i = 0; i < 6; ++i) {
                        Vector3 v0, v1, v2, v3;
                        box.getFaceCorners(i, v0, v1, v2, v3);

                        Vector3 n = (v1 - v0).cross(v3 - v0);
                        renderDevice->setNormal(n.direction());
                        renderDevice->sendVertex(v0);
                        renderDevice->sendVertex(v1);
                        renderDevice->sendVertex(v2);
                        renderDevice->sendVertex(v3);
                    }
                renderDevice->endPrimitive();
                renderDevice->setCullFace(RenderDevice::CULL_BACK);
            }
        }

        if (wireColor.a > 0) {
            renderDevice->setDepthWrite(true);
            renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
            renderDevice->setColor(wireColor);
            renderDevice->setLineWidth(2);

            Vector3 c = box.center();
            Vector3 v;

            // Wire frame
            renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
            renderDevice->beginPrimitive(PrimitiveType::LINES);

                // Front and back
                for (int i = 0; i < 8; i += 4) {
                    for (int j = 0; j < 4; ++j) {
                        v = box.corner(i + j);
                        renderDevice->setNormal((v - c).direction());
                        renderDevice->sendVertex(v);

                        v = box.corner(i + ((j + 1) % 4));
                        renderDevice->setNormal((v - c).direction());
                        renderDevice->sendVertex(v);
                    }
                }

                // Sides
                for (int i = 0; i < 4; ++i) {
                    v = box.corner(i);
                    renderDevice->setNormal((v - c).direction());
                    renderDevice->sendVertex(v);

                    v = box.corner(i + 4);
                    renderDevice->setNormal((v - c).direction());
                    renderDevice->sendVertex(v);
                }


            renderDevice->endPrimitive();
        }
    renderDevice->popState();
}


void Draw::wireSphereSection(
    const Sphere&       sphere,
    RenderDevice*       renderDevice,
    const Color4&       color,
    bool                top,
    bool                bottom) {
    
    int sections = WIRE_SPHERE_SECTIONS;
    int start = top ? 0 : (sections / 2);
    int stop = bottom ? sections : (sections / 2);

    renderDevice->pushState();
        renderDevice->setShadeMode(RenderDevice::SHADE_SMOOTH);
        renderDevice->setColor(color);
        renderDevice->setLineWidth(2);
        renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        renderDevice->setCullFace(RenderDevice::CULL_BACK);
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        float radius = sphere.radius;
        const Vector3& center = sphere.center;

        // Wire frame
        for (int y = 0; y < 8; ++y) {
            const float yaw = y * (float)pi() / 4;
            const Vector3 x(cos(yaw) * radius, 0, sin(yaw) * radius);
            //const Vector3 z(-sin(yaw) * radius, 0, cos(yaw) * radius);

            renderDevice->beginPrimitive(PrimitiveType::LINE_STRIP);
                for (int p = start; p <= stop; ++p) {
                    const float pitch0 = p * (float)pi() / (sections * 0.5f);

                    Vector3 v0 = cosf(pitch0) * x + Vector3::unitY() * radius * sinf(pitch0);
                    renderDevice->setNormal(v0.direction());
                    renderDevice->sendVertex(v0 + center);
                }
            renderDevice->endPrimitive();
        }


        int a = bottom ? -1 : 0;
        int b = top ? 1 : 0; 
        for (int p = a; p <= b; ++p) {
            const float pitch = p * (float)pi() / 6;

            renderDevice->beginPrimitive(PrimitiveType::LINE_STRIP);
                for (int y = 0; y <= sections; ++y) {
                    const float yaw0 = y * (float)pi() / 13;
                    Vector3 v0 = Vector3(cos(yaw0) * cos(pitch), sin(pitch), sin(yaw0) * cos(pitch)) * radius;
                    renderDevice->setNormal(v0.direction());
                    renderDevice->sendVertex(v0 + center);
                }
            renderDevice->endPrimitive();
        }

    renderDevice->popState();
}


void Draw::sphereSection(
    const Sphere&       sphere,
    RenderDevice*       renderDevice,
    const Color4&       color,
    bool                top,
    bool                bottom) {

    // The first time this is invoked we create a series of quad strips in a vertex array.
    // Future calls then render from the array. 

    CoordinateFrame cframe = renderDevice->objectToWorldMatrix();

    // Auto-normalization will take care of normal length
    cframe.translation += cframe.rotation * sphere.center;
    cframe.rotation = cframe.rotation * sphere.radius;


    // Track enable bits individually instead of using slow glPush
    bool resetNormalize = false;
    bool usedRescaleNormal = false;
    if (GLCaps::supports("GL_EXT_rescale_normal")) {
        // Switch to using GL_RESCALE_NORMAL, which should be slightly faster.
        resetNormalize = (glIsEnabled(GL_NORMALIZE) == GL_TRUE);
        usedRescaleNormal = true;
        glDisable(GL_NORMALIZE);
        glEnable(GL_RESCALE_NORMAL);
    }

    renderDevice->pushState();
        renderDevice->setObjectToWorldMatrix(cframe);

        renderDevice->setColor(color);
        renderDevice->setShadeMode(RenderDevice::SHADE_SMOOTH);

        static bool initialized = false;
        static VertexRange vbuffer;
        static Array< uint16 > stripIndexArray;

        if (! initialized) {
            // The normals are the same as the vertices for a sphere
            Array<Vector3> vertex;

            stripIndexArray.resize(iFloor((pi() / (float)SPHERE_PITCH_SECTIONS) + 2 * 
                                   (twoPi() / SPHERE_YAW_SECTIONS)));

            int i = 0;

            for (int p = 0; p < SPHERE_PITCH_SECTIONS; ++p) {
                const float pitch0 = p * (float)pi() / SPHERE_PITCH_SECTIONS;
                const float pitch1 = (p + 1) * (float)pi() / SPHERE_PITCH_SECTIONS;

                const float sp0 = sin(pitch0);
                const float sp1 = sin(pitch1);
                const float cp0 = cos(pitch0);
                const float cp1 = cos(pitch1);

                for (int y = 0; y <= SPHERE_YAW_SECTIONS; ++y) {
                    const float yaw = -y * (float)twoPi() / SPHERE_YAW_SECTIONS;

                    const float cy = cos(yaw);
                    const float sy = sin(yaw);

                    const Vector3 v0(cy * sp0, cp0, sy * sp0);
                    const Vector3 v1(cy * sp1, cp1, sy * sp1);

                    vertex.append(v0, v1);
                    stripIndexArray.append(i, i + 1);
                    i += 2;
                }

                const Vector3& degen = Vector3(1.0f * sp1, cp1, 0.0f * sp1);

                vertex.append(degen, degen);
                stripIndexArray.append(i, i + 1);
                i += 2;    
            }

            VertexBufferRef area = VertexBuffer::create(vertex.size() * sizeof(Vector3), VertexBuffer::WRITE_ONCE);
            vbuffer = VertexRange(vertex, area);
            initialized = true;
        }

        renderDevice->beginIndexedPrimitives();
            renderDevice->setNormalArray(vbuffer);
            renderDevice->setVertexArray(vbuffer);
            if (top) {
                renderDevice->sendIndices(PrimitiveType::QUAD_STRIP, (stripIndexArray.length() / 2),
                    stripIndexArray.getCArray());
            }
            if (bottom) {
                renderDevice->sendIndices(PrimitiveType::QUAD_STRIP, (stripIndexArray.length() / 2), 
                    stripIndexArray.getCArray() + (stripIndexArray.length() / 2));
            }
        renderDevice->endIndexedPrimitives();

    renderDevice->popState();

    if (usedRescaleNormal) {
        glDisable(GL_RESCALE_NORMAL);
        if (resetNormalize) {
            glEnable(GL_NORMALIZE);
        }
    }
}


void Draw::sphere(
    const Sphere&       sphere,
    RenderDevice*       renderDevice,
    const Color4&       solidColor,
    const Color4&       wireColor) {

    if (solidColor.a > 0) {
        renderDevice->pushState();

            int numPasses = 1;

            if (solidColor.a < 1) {
                numPasses = 2;
                renderDevice->setCullFace(RenderDevice::CULL_FRONT);
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                renderDevice->setDepthWrite(false);
            } else {
                renderDevice->setCullFace(RenderDevice::CULL_BACK);
            }

            if (wireColor.a > 0) {
                renderDevice->setPolygonOffset(3);
            }

            for (int k = 0; k < numPasses; ++k) {
                sphereSection(sphere, renderDevice, solidColor, true, true);
                renderDevice->setCullFace(RenderDevice::CULL_BACK);
            }
        renderDevice->popState();
    }

    if (wireColor.a > 0) {
        wireSphereSection(sphere, renderDevice, wireColor, true, true);
    }
}


void Draw::fullScreenImage(const GImage& im, RenderDevice* renderDevice) {
    debugAssert( im.channels() == 3 || im.channels() == 4 );
    renderDevice->push2D();
        glPixelZoom((float)renderDevice->width() / (float)im.width(), 
                   -(float)renderDevice->height() / (float)im.height());
        glRasterPos4d(0.0, 0.0, 0.0, 1.0);

        glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glDrawPixels(im.width(), im.height(), (im.channels() == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*)im.byte());

        glPopClientAttrib();
    renderDevice->pop2D();
}


void Draw::rect2D(
    const Rect2D& rect,
    RenderDevice* rd,
    const Color4& color,
    const Vector2& texCoord0,
    const Vector2& texCoord1,
    const Vector2& texCoord2,
    const Vector2& texCoord3,
    const Vector2& texCoord4,
    const Vector2& texCoord5,
    const Vector2& texCoord6,
    const Vector2& texCoord7) { 
    Draw::rect2D(rect, rd, color,
        Rect2D::xywh(0,0,texCoord0.x, texCoord0.y),
        Rect2D::xywh(0,0,texCoord1.x, texCoord1.y),
        Rect2D::xywh(0,0,texCoord2.x, texCoord2.y),
        Rect2D::xywh(0,0,texCoord3.x, texCoord3.y),
        Rect2D::xywh(0,0,texCoord4.x, texCoord4.y),
        Rect2D::xywh(0,0,texCoord5.x, texCoord5.y),
        Rect2D::xywh(0,0,texCoord6.x, texCoord6.y),
        Rect2D::xywh(0,0,texCoord7.x, texCoord7.y));
}


void Draw::rect2D(
    const Rect2D& rect,
    RenderDevice* rd,
    const Color4& color,
    const Rect2D& texCoord0,
    const Rect2D& texCoord1,
    const Rect2D& texCoord2,
    const Rect2D& texCoord3,
    const Rect2D& texCoord4,
    const Rect2D& texCoord5,
    const Rect2D& texCoord6,
    const Rect2D& texCoord7) {

    const Rect2D* tx[8];
    tx[0] = &texCoord0;
    tx[1] = &texCoord1;
    tx[2] = &texCoord2;
    tx[3] = &texCoord3;
    tx[4] = &texCoord4;
    tx[5] = &texCoord5;
    tx[6] = &texCoord6;
    tx[7] = &texCoord7;

    int N = iMin(8, GLCaps::numTextureCoords());

    rd->pushState();
    {
        rd->setColor(color);
        debugAssertGLOk();
        rd->beginPrimitive(PrimitiveType::QUADS);
        {
            for (int i = 0; i < 4; ++i) {
                for (int t = 0; t < N; ++t) {
                    rd->setTexCoord(t, tx[t]->corner(i));
                }
                rd->sendVertex(rect.corner(i));
            }
        }
        rd->endPrimitive();
    }
    rd->popState();
}


void Draw::fastRect2D(
    const Rect2D&       rect,
    RenderDevice*       rd,
    const Color4&       color) {

    rd->setColor(color);
    // Use begin primitive in case there are any 
    // lazy state changes pending.
    rd->beginPrimitive(PrimitiveType::QUADS);
        
        glTexCoord2f(0, 0);
        glVertex2f(rect.x0(), rect.y0());

        glTexCoord2f(0, 1);
        glVertex2f(rect.x0(), rect.y1());

        glTexCoord2f(1, 1);
        glVertex2f(rect.x1(), rect.y1());

        glTexCoord2f(1, 0);
        glVertex2f(rect.x1(), rect.y0());

    rd->endPrimitive();

    // Record the cost of the raw OpenGL calls
    rd->minGLStateChange(8);
}



void Draw::rect2DBorder(
    const class Rect2D& rect,
    RenderDevice* rd,
    const Color4& color,
    float outerBorder,
    float innerBorder) {


    //
    //
    //   **************************************
    //   **                                  **
    //   * **                              ** *
    //   *   ******************************   *
    //   *   *                            *   *
    //
    //
    const Rect2D outer = rect.border(outerBorder);
    const Rect2D inner = rect.border(-innerBorder); 

    rd->pushState();
    rd->setColor(color);
    rd->beginPrimitive(PrimitiveType::QUAD_STRIP);

    for (int i = 0; i < 5; ++i) {
        int j = i % 4;
        rd->sendVertex(outer.corner(j));
        rd->sendVertex(inner.corner(j));
    }

    rd->endPrimitive();
    rd->popState();
}

void sendFrustumGeometry(const GCamera::Frustum& frustum, RenderDevice* rd, bool lines) {
    if (! lines) {
        rd->beginPrimitive(PrimitiveType::QUADS);
    }
    for (int f = 0; f < frustum.faceArray.size(); ++f) {
        if (lines) {
            rd->beginPrimitive(PrimitiveType::LINE_STRIP);
        }
        const GCamera::Frustum::Face& face = frustum.faceArray[f];
        rd->setNormal(face.plane.normal());
        for (int v = 0; v < 4; ++v) {
            rd->sendVertex(frustum.vertexPos[face.vertexIndex[v]]);
        }
        if (lines) {
            rd->sendVertex(frustum.vertexPos[face.vertexIndex[0]]);
            rd->endPrimitive();
        }
    }
    if (! lines) {
        rd->endPrimitive();
    }
}


void Draw::frustum(const GCamera::Frustum& frustum, RenderDevice* rd,
                   const Color4& solidColor, const Color4& wireColor) {
    rd->pushState();

    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

    if (wireColor.a > 0) {
        rd->setColor(wireColor);
        rd->setLineWidth(2);
        sendFrustumGeometry(frustum, rd, true);
    }

    if (solidColor.a > 0) {
        rd->setCullFace(RenderDevice::CULL_FRONT);
        rd->setColor(solidColor);
        if (solidColor.a < 1) {
            rd->setDepthWrite(false);
        }
        rd->enableTwoSidedLighting();
        for (int i = 0; i < 2; ++i) {
            sendFrustumGeometry(frustum, rd, false);
            rd->setCullFace(RenderDevice::CULL_BACK);
        }
    }

    rd->popState();
}


}

