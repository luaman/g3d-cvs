/**
  @file Profiler.h
  @author Morgan McGuire, http://graphics.cs.williams.edu
  
 @created 2009-01-01
 @edited  2009-11-05

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
*/
#ifndef G3D_Profiler_h
#define G3D_Profiler_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "GLG3D/glheaders.h"

namespace G3D {

/** 
    \brief Measures execution time on the CPU and GPU of parts of a program.

    Requires EXT_timer_query OpenGL extension (GLEW_EXT_timer_query is true 
    when this is available.)

    Not threadsafe (timing on multiple threads would not be meaningful anyway),
    although beginCPU() and beginGFX() can be executed on different threads 
    simultaneously.
    
    The GPU timers are called "GFX" and not "GPU" to make them differ by
    more than a few pixels of a font from "CPU" when reading them in the
    code.

    \beta

 */
class Profiler {
public:
    class Task {
        friend class Profiler;
        friend class TaskList;
    private:
        /** For sorting purposes */
        std::string                     m_lowerName;
        std::string                     m_name;
        RealTime                        m_time;
        uint64                          m_frameNum;

        inline Task(const std::string& name, const std::string& L) : 
            m_lowerName(L), m_name(name), m_time(0), m_frameNum(0) {}

    public:

        inline Task() : m_time(0), m_frameNum(0) {}

        /** Name of this task.*/
        inline const std::string& name() const {
            return m_name;
        }

        /** Time this task took, in seconds. */
        inline RealTime time() const {
            return m_time;
        }

        /** Last frame in which this was measured.*/
        inline RealTime frameNum() const {
            return m_frameNum;
        }
    };

protected:

    class Pair {
    public:
        std::string                     name;
        GLint                           query;
        inline Pair() {}
        inline Pair(const std::string& n, GLint q) : name(n), query(q) {}
    };   

    /** Sorted by name */
    class TaskList {
    private:
        friend class Profiler;

        Array<Task>                     m_data;

        /** Return the index at which name exists (or where it should be 
         inserted BEFORE if it does not exist) */
        int find(const std::string& name) const;

    public:

        /** Returns a reference to the task with this name, allocating it if 
            necessary.  Old references will be void after this is called.*/
        Task& operator[](const std::string& name);

        /** Returns true if a task by this name exists with this frame number.*/
        bool contains(const std::string& name, uint64 frame) const;

        /** Erase all data */
        void clear();
    };

    /** Updated on every call to nextFrame() to ensure */
    uint64                              m_frameNum;

    /** Current CPU timer's name. Empty when there is none.*/
    std::string                         m_currentCPU;

    RealTime                            m_currentGPUTime;

    /** Current cpu tasks */
    TaskList                            m_cpuTask;

    Array<Task>                         m_oldCPUTask;

    /** Current GPU timer's name. Empty when there is none.*/
    std::string                         m_currentGFX;

    /** GPU query objects available for use.*/
    Array<GLuint>                       m_queryFreelist;

    /** Queries that have been issued and are waiting nextFrame() for reading.*/
    Array<Pair>                         m_pendingQueries;

    TaskList                            m_gfxTask;
    Array<Task>                         m_oldGFXTask;

    bool                                m_enabled;

public:

    Profiler();

    virtual ~Profiler();

    /** Reads the GFX timers.  Call this after swapBuffers() 
        to ensure that all GFX timers have completed. 

        Invoking nextFrame may stall the GPU and CPU by blocking in
        the method, causing your net frame time to appear to increase.
        This is (correctly) not reflected in the values returned by
        cpuTime and gfxTime.
    */
    void nextFrame();

    /** When disabled no profiling occurs (i.e., beginCPU and beginGFX
        do nothing).  Since profiling can affect performance
        (nextFrame() may block), top framerate should be measured with
        profiling disabled. */
    inline bool enabled() const {
        return m_enabled;
    }

    /** \copydoc enabled() */
    void setEnabled(bool e);

    /** Get timing information for the CPU tasks from the previous frame */
    const Array<Task>& cpuTasks() const {
        return m_oldCPUTask;
    }

    /** Get timing information for the GFX tasks from the previous frame */
    const Array<Task>& gfxTasks() const {
        return m_oldGFXTask;
    }

    /** Returns timing information for one task in seconds, NaN if not found */
    float cpuTime(const std::string& taskName) const;

    /** Returns timing information for one task in seconds, NaN if not found */
    float gfxTime(const std::string& taskName) const;

    /** \brief The number of the previous frame that was
        measured. cpuTasks() and gfxTasks() for which the frame is not
        previousFrameNum() were not recently measured.*/
    inline int previousFrameNum() const {
        return m_frameNum - 1;
    }

    /** Begins a new GPU timer.  This measures the elapsed time on the GPU from when
        this call enters the GPU stream (e.g., it may be delayed until the GPU is
        available for new instructions) until the corresponding endGFX() call
        exits the GPU stream.
        
        GFX calls may not be nested, even between instances of Profiler, due to an
        OpenGL limitation.
     */
    void beginGFX(const std::string& name);
    void endGFX();

    /** Wipes the names of old tasks.*/
    void clear();

    /** Begin a CPU-side timer.
        CPU timers may not be nested.
     */
    void beginCPU(const std::string& name);

    /** Ends the next CPU timer on the stack */
    void endCPU();
};

} // namespace G3D 

#endif
