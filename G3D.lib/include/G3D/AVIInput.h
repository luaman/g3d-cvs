/**

*/

#ifndef G3D_AVIINPUT_H
#define G3D_AVIINPUT_H

/* includes */
#include "G3D/ReferenceCount.h"
#include "G3D/BinaryInput.h"
#include "G3D/Queue.h"

namespace G3D {

/* forward declarations */
/* globals declarations */

/* implementation */
typedef ReferenceCountedPointer<class AVIInput> AVIInputRef;

/**
  AVI file reader
*/
class AVIInput : public ReferenceCountedObject {
public:
    enum STREAM_TYPE { STREAM_VIDEO, STREAM_AUDIO };
    struct AVIInfo {
        inline AVIInfo();

        uint32  width;
        uint32  height;

        float   frameRate;
        uint32  numFrames;
        uint32  currentFrame;

        bool    hasVideoStream;
        bool    hasAudioStream;

        bool    ignoringVideo;
        bool    ignoringAudio;

        bool    completed;
        bool    invalidFile;
    };

    struct FrameInfo {
        STREAM_TYPE streamType;
        uint32      frameSize;
        void*       frameData;
    };

    ~AVIInput();

    static AVIInputRef fromFile(const std::string& filename);

    bool isFrameAvailable(double realTimeStep);
    void ignoreStream(STREAM_TYPE streamType);

    FrameInfo nextFrame();

    const AVIInfo& currentInfo() const  { return m_aviInfo; }
private:
    AVIInput();

    // Not implemented
    AVIInput(const AVIInput&);
    AVIInput& operator=(const AVIInput&);

    void readHeaders();
    void readVideoFrame();
    void readAudioFrame();
    bool startNextChunk();
    bool finishChunk();

    AVIInfo         m_aviInfo;

    BinaryInput*    m_input;

    uint32          m_fourccVideoStream;
    uint32          m_fourccAudioStream;

    float           m_currentFrameTime;

    FrameInfo       m_videoFrame;
    FrameInfo       m_audioFrame;

    struct RIFFChunk {
        uint32 fourcc;
        uint32 size;
        uint32 startPos;
    };

    Queue<RIFFChunk> m_lists;

    RIFFChunk m_currentList;
    RIFFChunk m_currentChunk;
};


AVIInput::AVIInfo::AVIInfo() :
    frameRate(0.0f),
    numFrames(0),
    currentFrame(0),
    hasVideoStream(false),
    hasAudioStream(false),
    ignoringVideo(false),
    ignoringAudio(false),
    completed(false),
    invalidFile(false) {
}

}

#endif //G3D_AVIINPUT_H
