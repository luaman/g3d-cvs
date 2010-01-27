/**
  @file Profiler.cpp
  @author Morgan McGuire, http://graphics.cs.williams.edu

 @created 2009-01-01
 @edited  2009-11-05

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/Profiler.h"
#include "G3D/stringutils.h"

namespace G3D {

Profiler::Profiler() : m_frameNum(1), m_enabled(true) {
}


Profiler::~Profiler() {
    // Wait for all queries to complete
    nextFrame();
    
    // Delete all queries
    if (m_queryFreelist.size() > 0) {
        glDeleteQueries(m_queryFreelist.size(), m_queryFreelist.getCArray());
        m_queryFreelist.clear();
    }
}


void Profiler::setEnabled(bool e) {
    m_enabled = e;
}


void Profiler::nextFrame() {
    // Wait for all queries to complete

    if (GLEW_EXT_timer_query && m_enabled) {
        for (int i = 0; i < m_pendingQueries.size(); ++i) {
            // Block until available
            GLint available = 0;
            
            const GLint id = m_pendingQueries[i].query;
            const std::string& name = m_pendingQueries[i].name;

            do {
                glGetQueryObjectiv(id, GL_QUERY_RESULT_AVAILABLE, &available);
            } while (available == 0);

            // Get the time, in nanoseconds
            GLuint64EXT nsElapsed = 0;
            glGetQueryObjectui64vEXT(id, GL_QUERY_RESULT, &nsElapsed);

            Task& t = m_gfxTask[name];

            t.m_time = nsElapsed * 1e-9;
            t.m_frameNum = m_frameNum;

            // Put the query object back on the GPU freelist
            m_queryFreelist.append(id);
        }
    }

    m_oldCPUTask = m_cpuTask.m_data;
    m_oldGFXTask = m_gfxTask.m_data;

    m_pendingQueries.fastClear();

    // Advance the frame counter
    ++m_frameNum;
}


void Profiler::beginGFX(const std::string& name) {
    if (! GLEW_EXT_timer_query || ! m_enabled) {
        return;
    }
    alwaysAssertM(m_currentGFX == "", "There is already a GFX task named " + m_currentCPU + " pending.");
    alwaysAssertM(! m_gfxTask.contains(name, m_frameNum), "A GFX task named " + name +
        " was already timed this frame.");

    if (m_queryFreelist.size() == 0) {
        // Allocate some more query objects
        const int N = 10;
        m_queryFreelist.resize(N);
        glGenQueries(N, m_queryFreelist.getCArray());
    }

    m_currentGFX = name;
    GLint query = m_queryFreelist.pop();
    m_pendingQueries.append(Pair(name, query));

    glBeginQuery(GL_TIME_ELAPSED_EXT, query);
}


void Profiler::endGFX() {
    if (! GLEW_EXT_timer_query || ! m_enabled) {
        return;
    }

    alwaysAssertM(m_currentGFX != "", "No GFX profile pending");
    glEndQuery(GL_TIME_ELAPSED_EXT);
    m_gfxTask[m_currentGFX].m_frameNum = m_frameNum;
    m_currentGFX = "";

    // The actual time will be updated in nextFrame()
}


void Profiler::clear() {
    m_gfxTask.clear();
    m_cpuTask.clear();
}


void Profiler::beginCPU(const std::string& name) {
    if (! m_enabled) {
        return;
    }
    alwaysAssertM(m_currentCPU == "", "There is already a CPU task named " + 
        m_currentCPU + " pending.");
    alwaysAssertM(! m_cpuTask.contains(name, m_frameNum), 
        "A CPU task named " + name + " was already timed this frame.");

    m_currentCPU = name;
    m_currentGPUTime = System::time();
}


void Profiler::endCPU() {
    if (! m_enabled) {
        return;
    }
    double elapsed = System::time() - m_currentGPUTime;
    alwaysAssertM(m_currentCPU != "", "No CPU profile pending");

    Task& t = m_cpuTask[m_currentCPU];

    t.m_time = elapsed;
    t.m_frameNum = m_frameNum;

    m_currentCPU = "";
}


float Profiler::cpuTime(const std::string& taskName) const {
    for (int i = 0; i < m_oldCPUTask.size(); ++i) {
        if (taskName == m_oldCPUTask[i].name()) {
            return m_oldCPUTask[i].time();
        }
    }
    return nan();
}


float Profiler::gfxTime(const std::string& taskName) const {
    for (int i = 0; i < m_oldGFXTask.size(); ++i) {
        if (taskName == m_oldGFXTask[i].name()) {
            return m_oldGFXTask[i].time();
        }
    }
    return (float)nan();
}
///////////////////////////////////////////////////////////////////////////////////

Profiler::Task& Profiler::TaskList::operator[](const std::string& name) {
    const std::string lowerName = toLower(name);
    int i = find(lowerName);

    if ((i == m_data.size()) || 
        (m_data[i].m_lowerName != lowerName)) {
        // Does not yet exist
        m_data.insert(i, Task(name, lowerName));
    }

    return m_data[i];
}


int Profiler::TaskList::find(const std::string& lowerName) const {
    debugAssert(lowerName != "");
    // Binary search 

    // highest index known to contain a string "less than or equal to" name
    int lo = 0;

    // highest index known to contain a string "greater than " name
    int hi = m_data.size();

    if (hi == 0) {
        // Only one element
        return 0;
    }

    // Make sure we aren't past the high end
    int c = lowerName.compare(m_data[hi - 1].m_lowerName);
    if (c > 0) {
        // Off the top end
        return hi;
    } else if (c == 0) {
        // At the high end
        return hi - 1;
    }
    // Must be lower than the high end

    while (hi > lo) {
        int mid = (hi + lo) / 2;
        c = lowerName.compare(m_data[mid].m_lowerName);

        if (c > 0) {
            // data[mid] < name < hi
            lo = mid + 1;
        } else if (c < 0) {
            // lo <= name < data[mid]
            hi = mid;
        } else {
            // name == mid
            return mid;
        }
    }

    return lo;
}


bool Profiler::TaskList::contains(const std::string& name, uint64 frame) const {
    const std::string& lowerName = toLower(name);
    int i = find(lowerName);
    return (i < m_data.size() && (m_data[i].m_lowerName == lowerName) && (m_data[i].m_frameNum == frame));
}


void Profiler::TaskList::clear() {
    m_data.clear();
}

} // namespace G3D
