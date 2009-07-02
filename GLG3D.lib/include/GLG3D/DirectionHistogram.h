/**
  @file DirectionHistogram.h
  @maintainer Morgan McGuire, morgan@cs.williams.edu
  @created 2009-03-25
  @edited  2009-03-25
*/

#ifndef GLG3D_DirectionHistogram_h
#define GLG3D_DirectionHistogram_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/GThread.h"
#include "G3D/Vector3.h"
#include "G3D/Color3.h"
#include "G3D/Color4.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/VertexRange.h"

namespace G3D {

class RenderDevice;
    
/** \brief A histogram on the surface of a sphere.
    Useful for visualizing BSDFs. 
    
    Requires <code>sphere.ifs</code> to be in the current 
    directory or a location that System::findDataFile can 
    find it.

    The histogram drawn is a smoothing of the actual distribution
    by a \f$ \cos^{sharp} \f$ filter to ensure that it is not 
    undersampled by the underlying histogram mesh and buckets.

    Storage size is constant in the amount of data.  Input is 
    immediately inserted into a bucket and then discarded.
  */
class DirectionHistogram {
private:

    class WorkerThread : public GThread {
    private:
        DirectionHistogram*     hist;
        int                     startIndex;
        int                     stopIndex;
        const Array<Vector3>&   vector;
        const Array<float>&     weight;

    public:
        WorkerThread(DirectionHistogram* h, int start, int stop, 
            const Array<Vector3>& vector, const Array<float>& weight);
        void threadMain();
    };

    friend class WorkerThread;

    /** Vertices of the visualization mesh, on the unit sphere. */
    Array<Vector3>      m_meshVertex;

    /** Indices into meshVertex of the trilist for the visualization mesh. */
    Array<int>          m_meshIndex;

    /** Histogram buckets.  These are the scales of the corresponding meshVertex.*/
    Array<float>        m_bucket;

    VertexRange                 m_gpuMeshIndex;
    VertexRange                 m_gpuMeshVertex;

    /** True when the VertexRange needs to be recomputed */
    bool                m_dirty;

    float               m_sharp;

    /** Total weight: \f$ \sum bucket[i] \f$ */
    float               m_totalWeight;

    /** Dot product that is so low that it won't materially affect the 
        distribution and can be ignored for filtering purposes.*/
    float               m_cutoff;

    /** Volume of a tetrahedron whose 4th vertex is at the origin.  
        The vertices are assumed to be
        in ccw order.*/
    static float tetrahedronVolume(const Vector3& v0, const Vector3& v1, const Vector3& v2);

    /** Compute the total volume of the distribution */
    float totalVolume() const;

    void sendGeometry(RenderDevice* rd);

    /** Assumes vector has unit length.

       @param startIndex Inclusive
       @param stopIndex  Inclusive
    */
    void insert(const Vector3& vector, float weight, int startIndex, int stopIndex);

public:

    /** @param sharp Sharpness of the smoothing filter.  Recommended range 
         is 30 (very smooth) - 220 (very sharp).  Smoother filters are needed
         when taking very few samples.
         
       \param hemiAxis if nonzero, the distribution is only recorded for the hemisphere 
        about this axis.*/
    DirectionHistogram(float sharp = 30.0f, const Vector3& hemiAxis = Vector3::zero());

    /** Discard all data */
    void reset();

    /**
     \brief Insert a new data point into the set.
      Only the direction of @a vector matters; it will be normalized.
     */
    void insert(const Vector3& vector, float weight = 1.0f);

    /**
     \brief Insert many new data points.

     This method uses multiple threads to efficiently insert the data.
     */
    void insert(const Array<Vector3>& vector, const Array<float>& weight = Array<float>());

    /** Draw a wireframe of the distribution.  Renders with approximately constant volume. */
    void render(
        class RenderDevice* rd, 
        const Color3& solidColor = Color3::white(), 
        const Color4& lineColor = Color3::black());

};

} // G3D

#endif
