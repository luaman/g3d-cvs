/**
 \file MD2Model.cpp

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2003-08-07
 \edited  2010-04-19
 */

#include "G3D/platform.h"
#if defined(G3D_WIN32)
#    include <xmmintrin.h>
#endif

#include "G3D/Log.h"
#include "G3D/FileSystem.h"
#include "G3D/BinaryInput.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/MD2Model.h"
#include "GLG3D/VertexRange.h"
#include "GLG3D/SuperSurface.h"

namespace G3D {


MD2Model2::Specification::Specification() : scale(1.0f) {}

MD2Model2::Specification::Specification(const std::string& trisFilename) 
: filename(trisFilename), scale(1.0f) {
    // TODO: try and locate appropriate materials and weapons
}

MD2Model2::Specification::Specification(const Any& any) {
    if (any.type() == Any::STRING) {
        *this = Specification(any.resolveStringAsFilename());
        return;
    }

    any.verifyName("MD2Model::Specification");
    *this = Specification();
    for (Table<std::string, Any>::Iterator it = any.table().begin(); it.hasMore(); ++it) {
        const std::string& key = toLower(it->key);
        if (key == "filename") {
            filename = it->value.resolveStringAsFilename();
        } else if (key == "material") {
            material = Material::create(it->value);
        } else if (key == "scale") {
            scale = it->value;
        } else if (key == "weaponfilename") {
            weaponFilename = it->value.resolveStringAsFilename();
        } else if (key == "weaponmaterial") {
            weaponMaterial = Material::create(it->value);
        } else {
            it->value.verify(false, "Unknown key: " + it->key);
        }
    }
}


MD2Model2::Ref MD2Model2::create(const Specification& s) {
    MD2Model2::Ref m = new MD2Model2();

    Part::Specification ps;
    ps.filename = s.filename;
    ps.material = s.material;
    ps.scale = s.scale;
    m->m_part.append(Part::create(ps));

    if (! s.weaponFilename.empty()) {
        ps.filename = s.weaponFilename;
        ps.material = s.weaponMaterial;
        m->m_part.append(Part::create(ps));
    }

    m->m_name = FilePath::base(FilePath::parent(FileSystem::resolve(s.filename)));

    m->m_numTriangles = 0;
    for (int p = 0; p < m->m_part.size(); ++p) {
        m->m_numTriangles += m->m_part[p]->indexArray.size() / 3;
    }

    return m;
}


void MD2Model2::pose(Array<Surface::Ref>& surfaceArray, const CFrame& rootFrame, const MD2Model::Pose& pose) {
    for (int p = 0; p < m_part.size(); ++p) {
        m_part[p]->pose(surfaceArray, rootFrame, pose);
    }
}

///////////////////////////////////////////////////////

MD2Model::Specification::Specification(const Any& any) {
    any.verifyName("MD2Model::Specification");
    *this = Specification();
    for (Table<std::string, Any>::Iterator it = any.table().begin(); it.hasMore(); ++it) {
        const std::string& key = toLower(it->key);
        if (key == "filename") {
            filename = it->value.resolveStringAsFilename();
        } else if (key == "material") {
            material = Material::create(it->value);
        } else if (key == "scale") {
            scale = it->value;
        } else {
            it->value.verify(false, "Unknown key: " + it->key);
        }
    }
}



MD2Model*           MD2Model::interpolatedModel      = NULL;
MD2Model::Pose      MD2Model::interpolatedPose;
VertexBuffer::Ref   MD2Model::varArea[MD2Model::NUM_VAR_AREAS];
int                 MD2Model::nextVarArea            = MD2Model::NONE_ALLOCATED;
const GameTime      MD2Model::PRE_BLEND_TIME         = 1.0 / 8.0;
const float         MD2Model::hangTimePct            = 0.1f;

MD2Model::PackedGeometry::PackedGeometry() {
    vertexArray.clearAndSetMemoryManager(AlignedMemoryManager::create());
    normalArray.clearAndSetMemoryManager(AlignedMemoryManager::create());
}

const MD2Model::MD2AnimInfo MD2Model::animationTable[MD2Model::MAX_ANIMATIONS] = 
{
    // first, last, fps
    {   0,  39,  9, true },    // STAND
    {  40,  45, 10, true },    // RUN
    {  46,  53, 10, false },   // ATTACK
    {  54,  57,  7, false },   // PAIN_A
    {  58,  61,  7, false },   // PAIN_B
    {  62,  65,  7, false },   // PAIN_C
    {  66,  71,  7, false },   // JUMP_DOWN
    {  72,  83,  7, false },   // FLIP
    {  84,  94,  7, false },   // SALUTE
    {  95, 111, 10, false },   // FALLBACK
    { 112, 122,  7, false },   // WAVE
    { 123, 134,  6, false },   // POINT
    { 135, 153, 10, true },    // CROUCH_STAND
    { 154, 159,  7, true },    // CROUCH_WALK
    { 160, 168, 10, false },   // CROUCH_ATTACK
    { 169, 172,  7, false },   // CROUCH_PAIN
    { 173, 177,  5, false },   // CROUCH_DEATH
    { 178, 183,  7, false },   // DEATH_FALLBACK
    { 184, 189,  7, false },   // DEATH_FALLFORWARD
    { 190, 197,  7, false },   // DEATH_FALLBACKSLOW
    // JUMP is not in the table; it is handled specially
};

MD2Model::Ref MD2Model::create(const Specification& spec) {
    MD2Model* model = new MD2Model();
    model->load(spec.filename, spec.scale);
    model->m_material = spec.material;

    return model;
}


MD2Model::Ref MD2Model::fromFile(const std::string& filename, const std::string& diffuseFilename, float s) {
    MD2Model* model = new MD2Model();
    model->load(filename, s);
    Material::Specification mat;
    mat.setLambertian(diffuseFilename);
    model->m_material = Material::create(mat);

    return model;
}


bool MD2Model::Pose::operator==(const MD2Model::Pose& other) const {
    return (animation == other.animation) && fuzzyEq(time, other.time);
}


const Array<MeshAlg::Face>& MD2Model::faces() const {
	return faceArray;
}


const Array<MeshAlg::Edge>& MD2Model::edges() const {
	return edgeArray;
}


const Array<MeshAlg::Vertex>& MD2Model::vertices() const {
	return vertexArray;
}


const Array<MeshAlg::Face>& MD2Model::weldedFaces() const {
	return weldedFaceArray;
}


const Array<MeshAlg::Edge>& MD2Model::weldedEdges() const {
	return weldedEdgeArray;
}


const Array<MeshAlg::Vertex>& MD2Model::weldedVertices() const {
	return weldedVertexArray;
}


void MD2Model::computeFrameNumbers(const MD2Model::Pose& pose, int& kf0, int& kf1, float& alpha) {

    if (pose.time < 0.0) {
        Animation a = pose.animation;
        
        if (pose.animation == JUMP) {
            a = JUMP_UP;
        } else {
        }

        debugAssert(iAbs(a) < MAX_ANIMATIONS);

        if (a >= 0) {
            kf1 = animationTable[a].first;
        } else {
            kf1 = animationTable[-a].last;
        }

        alpha = clamp(1 + (pose.time / PRE_BLEND_TIME), 0, 1);

        if ((pose.preFrameNumber >= 0) && (pose.preFrameNumber < 197)) {
            kf0 = pose.preFrameNumber;
        } else {
            // Illegal pose number; just hold the first frame
            kf0 = kf1;
        }

        return;
    }


    debugAssert(pose.time >= 0.0);

    // Assume time is positive
    if (pose.animation == JUMP) {
        // Jump is special because it is two animations pasted together

        // Time to jump up (== time to jump down)
        GameTime upTime = animationLength(JUMP_UP);

        // Make the time on the right interval
        GameTime time = iWrap(iRound(pose.time * 1000), iRound(1000 * upTime * (2 + hangTimePct))) / 1000.0;

        if (time < upTime) {
            // Jump up
            computeFrameNumbers(Pose(JUMP_UP, time), kf0, kf1, alpha);
        } else if (time < upTime * (1 + hangTimePct)) {
            // Hold at the peak
            computeFrameNumbers(Pose(JUMP_UP, upTime), kf0, kf1, alpha);
        } else {
            // Jump down
            computeFrameNumbers(Pose(JUMP_DOWN, time - upTime * (1 + hangTimePct)), kf0, kf1, alpha);
        }

        return;
    }

    Animation a = (Animation)iAbs(pose.animation);
    debugAssert(a < MAX_ANIMATIONS);

    // Figure out how far between frames we are
    const MD2AnimInfo& anim = animationTable[a];
    GameTime time = pose.time;
    GameTime len = animationLength(a);

    if (pose.animation < 0) {
        // Run the animation backwards
        time = len - time;
    }

    int totalFrames = anim.last - anim.first + 1;

    // Number of frames into the animation
    float frames = time * anim.fps;

    int iframes   = iFloor(frames);

    if (anim.loops) {
        kf0   = anim.first + iWrap(iframes, totalFrames);
        kf1   = anim.first + iWrap((iframes + 1), totalFrames);
        alpha = frames - iframes;
    } else {
        kf0   = anim.first + iMin(iframes, totalFrames - 1);
        kf1   = anim.first + iMin((iframes + 1), totalFrames - 1);
        alpha = frames - iframes;
    }
}

void MD2Model::Pose::onSimulation(GameTime dt, const Action& a) {

    Pose currentPose = *this;

    time += dt;

    if (animationDeath(animation)) {
        // Can't recover from a death pose
        return;
    }

    if (a.death1 || a.death2 || a.death3) {
        // Death interrupts anything
        preFrameNumber = getFrameNumber(currentPose);
        time           = -PRE_BLEND_TIME;
        if (a.crouching) {
            animation  = CROUCH_DEATH;
        } else if (a.death1) {
            animation  = DEATH_FALLBACK;
        } else if (a.death2) {
            animation  = DEATH_FALLFORWARD;
        } else if (a.death3) {
            animation  = DEATH_FALLBACKSLOW;
        }
        return;
    }

    if ((a.pain1 || a.pain2 || a.pain3) && ! animationPain(animation)) {
        // Pain interrupts anything but death
        preFrameNumber = getFrameNumber(currentPose);
        time           = -PRE_BLEND_TIME;
        if (a.crouching) {
            animation  = CROUCH_PAIN;
        } else if (a.pain1) {
            animation  = PAIN_A;
        } else if (a.pain2) {
            animation  = PAIN_B;
        } else if (a.pain3) {
            animation  = PAIN_C;
        }
        return;
    }

    // End of an animation
    if (! animationLoops(animation) &&
        (time >= animationLength(animation))) {
        animation = STAND;
    }


    // Run
    if (a.movingForward) {
        if ((! animationRunForward(animation)) && animationInterruptible(animation)) {
            // Start running
            animation = RUN;
        }
    } else if (a.movingBackward) {
        if ((! animationRunBackward(animation)) && animationInterruptible(animation)) {
            // Start running
            animation = RUN_BACKWARD;
        }
    } else {
        if (animationRun(animation)) {
            // Stop running
            animation = STAND;
        }
    }

    if (animationInterruptible(animation)) {

        if (a.attack) {
            animation = ATTACK;
        } else if (a.jump && ! animationJump(animation)) {
            animation = JUMP;
        } else if (a.flip) {
            animation = FLIP;
        } else if (a.salute) {
            animation = SALUTE;
        } else if (a.fallback) {
            animation = FALLBACK;
        } else if (a.wave) {
            animation = WAVE;
        } else if (a.point) {
            animation = POINT;
        }
    }

    // Crouch
    if (a.crouching) {
        // Move to a crouch if necessary
        switch (animation) {
        case STAND:
            animation = CROUCH_STAND;
            break;

        case RUN:
            animation = CROUCH_WALK;
            break;

        case RUN_BACKWARD:
            animation = CROUCH_WALK_BACKWARD;
            break;

        case ATTACK:
            animation = CROUCH_ATTACK;
            break;

        default:;
        // We don't allow crouch during pain or other actions
        }
    } else {

        // Move to a crouch if necessary
        switch (animation) {
        case CROUCH_STAND:
            animation = STAND;
            break;

        case CROUCH_WALK:
            animation = RUN;
            break;

        case CROUCH_WALK_BACKWARD:
            animation = RUN_BACKWARD;
            break;

        case CROUCH_ATTACK:
            animation = ATTACK;
            break;

        default:;
        // We don't allow standing up during pain or other actions
        }
    }

    // Blend in old animation
    if (animation != currentPose.animation) {
        preFrameNumber = getFrameNumber(currentPose);
        time           = -PRE_BLEND_TIME;
    }
}


bool MD2Model::Pose::completelyDead() const {
    return animationDeath(animation) && (time > animationLength(animation));
}


bool MD2Model::animationRun(Animation a) {
    return (iAbs(a) == RUN) || (iAbs(a) == CROUCH_WALK);
}


bool MD2Model::animationRunForward(Animation a) {
    return (a == RUN) || (a == CROUCH_WALK);
}


bool MD2Model::animationRunBackward(Animation a) {
    return (a == RUN_BACKWARD) || (a == CROUCH_WALK_BACKWARD);
}


bool MD2Model::animationStand(Animation a) {
    return (a == STAND) || (a == CROUCH_STAND);
}


bool MD2Model::animationAttack(Animation a) {
    return (a == ATTACK) || (a == CROUCH_ATTACK);
}


bool MD2Model::animationJump(Animation a) {
    return (a == JUMP) || (a == JUMP_UP) || (a == JUMP_DOWN);
}


bool MD2Model::animationInterruptible(Animation a) {
    return (! animationAttack(a)) && (! animationDeath(a)) && (! animationPain(a));
}


bool MD2Model::animationPain(Animation a) {
    return (a == CROUCH_PAIN) || ((a >= PAIN_A) && (a <= PAIN_C));
}


bool MD2Model::animationCrouch(Animation a) {
    return (a >= CROUCH_STAND) && (a <= CROUCH_DEATH);
}


bool MD2Model::animationDeath(Animation a) {
    return (a >= CROUCH_DEATH) && (a <= DEATH_FALLBACKSLOW);
}


bool MD2Model::animationLoops(Animation a) {
    if (a == JUMP) {
        return false;
    }

    a = (Animation)iAbs(a);
    debugAssert(a < MAX_ANIMATIONS);
    return animationTable[a].loops;
}


GameTime MD2Model::animationLength(Animation a) {
    if (a == JUMP) {
        return animationLength(JUMP_DOWN) * (2 + hangTimePct);
    }

    a = (Animation)iAbs(a);
    debugAssert(a < MAX_ANIMATIONS);

    const MD2AnimInfo& info = animationTable[a];

    if (info.loops) {
        return (info.last - info.first + 1) / (float) info.fps;
    } else {
        return (info.last - info.first) / (float) info.fps;
    }
}


int MD2Model::getFrameNumber(const Pose& pose) {
    // Return the frame we're about to go to.
    int kf0, kf1;
    float alpha;
    computeFrameNumbers(pose, kf0, kf1, alpha);
    return kf1;
}


static const uint32 maxVARVerts = 1600; 

void MD2Model::allocateVertexArrays(RenderDevice* renderDevice) {
    (void)renderDevice;
    size_t size = maxVARVerts * (24 + sizeof(Vector3) * 2 + sizeof(Vector2));

    for (int i = 0; i < NUM_VAR_AREAS; ++i) {
        varArea[i] = VertexBuffer::create(size);
    }

    if (varArea[NUM_VAR_AREAS - 1].isNull()) {
        nextVarArea = NONE_ALLOCATED;
        Log::common()->println("\n*******\nCould not allocate vertex arrays.");
        return;
    }

    nextVarArea = 0;
}


void MD2Model::pose(Array<Surface::Ref>& surfaceArray, const CoordinateFrame& cframe, const Pose& pose) {

    // Keep a back pointer so that the index array can't be deleted
    SuperSurface::Ref surface = SuperSurface::create(name(), cframe, SuperSurface::GPUGeom::create(), 
                                                     SuperSurface::CPUGeom(), this);

    // Use the internal storage of the surface
    SuperSurface::CPUGeom& cpuGeom = surface->cpuGeom();

    // Need an empty array for the tangents
    const static Array<Vector4> packedTangentArray;
    cpuGeom.index         = &indexArray;
    cpuGeom.geometry      = &surface->internalGeometry();
    cpuGeom.packedTangent = &packedTangentArray;
    cpuGeom.texCoord0     = &_texCoordArray;

    getGeometry(pose, *const_cast<MeshAlg::Geometry*>(cpuGeom.geometry));
    
    // Upload data to the GPU
    SuperSurface::GPUGeom::Ref gpuGeom = surface->gpuGeom();
    cpuGeom.copyVertexDataToGPU(gpuGeom->vertex, gpuGeom->normal, gpuGeom->packedTangent, 
                                gpuGeom->texCoord0, VertexBuffer::WRITE_EVERY_FRAME);

    gpuGeom->index = indexVAR;

    // TODO: this isn't conservative when blending between animations; we should
    // take the max of both
    gpuGeom->boxBounds = animationBoundingBox[iAbs(pose.animation)];
    gpuGeom->sphereBounds = animationBoundingSphere[iAbs(pose.animation)];

    gpuGeom->material = m_material;

    surfaceArray.append(surface);
}


void MD2Model::render(RenderDevice* renderDevice, const Pose& pose) {
    sendGeometry(renderDevice, pose);    
}


void MD2Model::debugRenderWireframe(RenderDevice* renderDevice, const Pose& pose) {
    getGeometry(pose, interpolatedFrame);

    renderDevice->pushState();
        renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        renderDevice->setPolygonOffset(-0.1f);
        renderDevice->setColor(Color3::black());
        
        renderDevice->beginPrimitive(PrimitiveType::TRIANGLES);
        for (int i = 0; i < indexArray.size(); ++i) {
            renderDevice->sendVertex(interpolatedFrame.vertexArray[indexArray[i]]);
        }
        renderDevice->endPrimitive();

        renderDevice->setPolygonOffset(0.0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    renderDevice->popState();
}

 
size_t MD2Model::mainMemorySize() const {

    size_t frameSize   = keyFrame.size() * (sizeof(PackedGeometry)  + (sizeof(Vector3) + sizeof(uint8)) * keyFrame[0].vertexArray.size());
    size_t indexSize   = indexArray.size() * sizeof(int);
    size_t faceSize    = faceArray.size() * sizeof(MeshAlg::Face);
    size_t texSize     = _texCoordArray.size() * sizeof(Vector2);
    size_t valentSize  = vertexArray.size() * sizeof(Array<MeshAlg::Vertex>);
    for (int i = 0; i < vertexArray.size(); ++i) {
        valentSize += vertexArray[i].faceIndex.size() * sizeof(int);
        valentSize += vertexArray[i].edgeIndex.size() * sizeof(int);
    }

    size_t primitiveSize  = primitiveArray.size() * sizeof(Primitive);
    for (int i = 0; i < primitiveArray.size(); ++i) {
        primitiveSize += primitiveArray[i].pvertexArray.size() * sizeof(Primitive::PVertex);
    }

    size_t edgeSize    = edgeArray.size() * sizeof(MeshAlg::Edge);

    return sizeof(MD2Model) + frameSize + indexSize + faceSize + valentSize + primitiveSize + texSize + edgeSize;
}


MeshAlg::Geometry MD2Model::interpolatedFrame;

#ifdef _MSC_VER
    // Supress warnings about mutating ebx and ebp; we 
    // push and pop them during the asm code of getGeometry
    // to get 2 extra registers
    #pragma warning( push )
    #pragma warning( disable : 4731 )
#endif

void MD2Model::getGeometry(const Pose& pose, MeshAlg::Geometry& out) const {
    
    const int numVertices = keyFrame[0].vertexArray.size();

    AlignedMemoryManager::Ref mm = AlignedMemoryManager::create();

    if ((out.vertexArray.memoryManager() != mm) ||
        (out.normalArray.memoryManager() != mm)) {
        out.vertexArray.clearAndSetMemoryManager(AlignedMemoryManager::create());
        out.normalArray.clearAndSetMemoryManager(AlignedMemoryManager::create());
    }

    out.vertexArray.resize(numVertices, DONT_SHRINK_UNDERLYING_ARRAY);
    out.normalArray.resize(numVertices, DONT_SHRINK_UNDERLYING_ARRAY);

    if ((interpolatedModel == this) && (pose == interpolatedPose)) {
        // We're being asked to recompute a pose we have cached.
        if (&out != &interpolatedFrame) {
            // Copy from the cache
            System::memcpy(out.vertexArray.getCArray(), interpolatedFrame.vertexArray.getCArray(), sizeof(Vector3) * interpolatedFrame.vertexArray.size());
            System::memcpy(out.normalArray.getCArray(), interpolatedFrame.normalArray.getCArray(), sizeof(Vector3) * interpolatedFrame.vertexArray.size());
        }

        return;
    }

    if (&out == &interpolatedFrame) {
        // Make a note about what the cache contains
        interpolatedPose  = pose;
        interpolatedModel = const_cast<MD2Model*>(this);
    }

    float alpha;
    int i0, i1;

    computeFrameNumbers(pose, i0, i1, alpha);

    if ((i0 >= keyFrame.size()) || (i1 >= keyFrame.size())) {
        // This animation is not supported by this model.
        i0 = 0;
        i1 = 0;
        alpha = 0;
    }

    const PackedGeometry& frame0 = keyFrame[i0];
    const PackedGeometry& frame1 = keyFrame[i1];

    bool useSSE = false;

    #ifdef G3D_WIN32
        #pragma message("Port MD2Model::GetGeometry SIMD to all platforms")
        useSSE = true;
    #endif

    if (! useSSE) {
        const Vector3*  v0 = frame0.vertexArray.getCArray();
        const Vector3*  v1 = frame1.vertexArray.getCArray();
    
        const uint8*    n0 = frame0.normalArray.getCArray();
        const uint8*    n1 = frame1.normalArray.getCArray();

        Vector3*        vI = out.vertexArray.getCArray();
        Vector3*        nI = out.normalArray.getCArray();

        for (int v = numVertices - 1; v >= 0; --v) {
            vI[v] = v0[v].lerp(v1[v], alpha);
            nI[v] = normalTable[n0[v]].lerp(normalTable[n1[v]], alpha);
        }

        return;
    }


    // Put this code inside an ifdef because it won't compile
    // on non-SSE compilers otherwise.
    #if defined(G3D_WIN32)
        // The high performance path interpolates between two arrays of floats.
        // We first convert the vertices and normals into this format.

        // n0 and v0 are first moved to the interpolated frame then
        // lerped in place

        const int numFloats = numVertices * 3;

        Array<Vector3> normal1;
        normal1.clearAndSetMemoryManager(mm);
        normal1.resize(numVertices, DONT_SHRINK_UNDERLYING_ARRAY);

        {
            // Expand the normals out
            const uint8* n0Index = frame0.normalArray.getCArray();
            const uint8* n1Index = frame1.normalArray.getCArray();

            Vector3* n0Vector = out.normalArray.getCArray();
            Vector3* n1Vector = normal1.getCArray();

            for (int i = numVertices - 1; i >= 0; --i) {
                n0Vector[i] = normalTable[n0Index[i]];
                n1Vector[i] = normalTable[n1Index[i]];
            }
        }

        const float* v0 = reinterpret_cast<const float*>(frame0.vertexArray.getCArray());
        const float* v1 = reinterpret_cast<const float*>(frame1.vertexArray.getCArray());

        const float* n1 = reinterpret_cast<const float*>(normal1.getCArray());

        // Interpolated values
        float* vI = reinterpret_cast<float*>(out.vertexArray.getCArray());
        float* nI = reinterpret_cast<float*>(out.normalArray.getCArray());

        // Copy over vertices so we can mutate them in place
        System::memcpy(vI, v0, numFloats * sizeof(float));

        alwaysAssertM(((size_t)v1 % 16) == 0, "SEE array not aligned to 16-bytes");
        alwaysAssertM(((size_t)vI % 16) == 0, "SEE array not aligned to 16-bytes");
        alwaysAssertM(((size_t)n1 % 16) == 0, "SEE array not aligned to 16-bytes");
        alwaysAssertM(((size_t)nI % 16) == 0, "SEE array not aligned to 16-bytes");

        const int num128 = numFloats / sizeof(float);

        debugAssert(frame1.vertexArray.size() * 3 == numFloats);
        debugAssert(normal1.size() * 3 == numFloats);

        // Spread alpha across a vector
        const __m128 alpha128 = _mm_set_ps(alpha, alpha, alpha, alpha);

        // Our goal:
        //   vI = vI + (v1 - vI) * alpha
        //   nI = nI + (n1 - nI) * alpha
        __asm {
            mov     ecx,  num128
            movaps  xmm7, alpha128

            // Set up source and destination registers
            mov     edi, vI
            mov     esi, v1

            mov     edx, nI
            mov     eax, n1

        beginLoop:
            // Load vI and nI
            movaps  xmm0, [edi]
            movaps  xmm2, [edx]

            // Load v1 and n1
            movaps  xmm1, [esi]
            movaps  xmm3, [eax]

            // Compute (v1 - v0) and (n1 - n0)
            subps   xmm1, xmm0
            subps   xmm3, xmm2

            // Multiply each by alpha
            mulps   xmm1, xmm7
            mulps   xmm3, xmm7

            // Add the totals to v0 and n0
            addps   xmm0, xmm1
            addps   xmm2, xmm3

            // Store the results back in vI and nI
            movaps  [edi], xmm0
            movaps  [edx], xmm2

            // Increment all pointers
            add     edi, 16
            add     esi, 16
            add     edx, 16
            add     eax, 16

            dec     ecx
            jnz     beginLoop
        }
       
        // The last few floats may have been missed by the previous loop.
        for (int i = num128 * 4; i < numFloats; ++i) {
            vI[i] = vI[i] + (v1[i] - vI[i]) * alpha;
            nI[i] = nI[i] + (n1[i] - nI[i]) * alpha;
        }

    #endif
}

#ifdef _MSC_VER
    #pragma warning( pop )
#endif



void MD2Model::sendGeometry(RenderDevice* renderDevice, const Pose& pose) const {
    getGeometry(pose, interpolatedFrame);

    bool tooBig = ((int)maxVARVerts < keyFrame[0].vertexArray.size());
    bool useVAR = (nextVarArea != NONE_ALLOCATED) && ! tooBig;
   
    if (! useVAR && ! tooBig) {
        // Try to allocate some memory
        const_cast<MD2Model*>(this)->allocateVertexArrays(renderDevice);
        useVAR = (nextVarArea != NONE_ALLOCATED);
    }

    SuperSurface::CPUGeom cpuGeom(&indexArray, &interpolatedFrame, &_texCoordArray);

    if (useVAR) {

        // Upload the arrays
        debugAssert(! varArea[nextVarArea].isNull());
        varArea[nextVarArea]->reset();

        debugAssert(! varArea[nextVarArea].isNull());
        VertexRange varTexCoord(*cpuGeom.texCoord0, varArea[nextVarArea]);
        VertexRange varNormal  (cpuGeom.geometry->normalArray,   varArea[nextVarArea]);
        VertexRange varVertex  (cpuGeom.geometry->vertexArray,   varArea[nextVarArea]);
        
        renderDevice->beginIndexedPrimitives();
            renderDevice->setTexCoordArray(0, varTexCoord);
            renderDevice->setNormalArray(varNormal);
            renderDevice->setVertexArray(varVertex);
            renderDevice->sendIndices(PrimitiveType::TRIANGLES, *cpuGeom.index);
        renderDevice->endIndexedPrimitives();
        
        nextVarArea = (nextVarArea + 1) % NUM_VAR_AREAS;

    } else {

        // No VertexRange available; use the default rendering path
        // Quake's triangles are backwards of OpenGL in our coordinate
        // system, so cull front faces instead of back faces.
        glFrontFace(GL_CW);

        for (int p = 0; p < primitiveArray.size(); ++p) {

            const Primitive&          primitive = primitiveArray[p];
            const int                 n         = primitive.pvertexArray.size();
            const Primitive::PVertex* pvertex   = primitive.pvertexArray.getCArray();

            renderDevice->beginPrimitive(primitive.type);

                const Vector3* normal = cpuGeom.geometry->normalArray.getCArray();
                const Vector3* vertex = cpuGeom.geometry->vertexArray.getCArray();

                for (int i = 0; i < n; ++i) {
                    const int v = pvertex[i].index;
                    renderDevice->setTexCoord(0, pvertex[i].texCoord);
                    renderDevice->setNormal(normal[v]);
                    renderDevice->sendVertex(vertex[v]);
                }
            renderDevice->endPrimitive();
        }
        glFrontFace(GL_CCW);
    }
}


Texture::Ref MD2Model::textureFromFile(const std::string& filename) {
    Texture::Settings settings;
    settings.wrapMode = WrapMode::CLAMP;

    Texture::Preprocess preprocess;
    preprocess.modulate = Color4::one() * 2.0f;

    return Texture::fromFile(filename, ImageFormat::AUTO(), Texture::DIM_2D, settings, preprocess);
}

}
