/** 
 */

#ifndef G3D_VIDEOINPUT_H
#define G3D_VIDEOINPUT_H

// includes
#include <string>
#include "G3D/ReferenceCount.h"
#include "G3D/Queue.h"
#include "G3D/GThread.h"
#include "G3D/GImage.h"
#include "GLG3D/Texture.h"

// forward declarations for ffmpeg
struct AVFrame;
struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;

namespace G3D {


/** video input */
class VideoInput : public ReferenceCountedObject {

public:
    struct Settings
    {
    public:
        Settings(): numBuffers(2) {}

        /** Number of asynchronous buffers to allocate */
        int     numBuffers;
    };

    typedef ReferenceCountedPointer<VideoInput> Ref;

    static Ref  fromFile(const std::string& filename, const Settings& settings);

    ~VideoInput();

    /** Gets the next frame if available and returns false if not. */
    bool        readNext(RealTime timeStep, Texture::Ref& frame);
    bool        readNext(RealTime timeStep, GImage& frame);
    bool        readNext(RealTime timeStep, Image3uint8::Ref& frame);
    bool        readNext(RealTime timeStep, Image3::Ref& frame);

    /* Gets the frame at playback position pos and returns false if pos is invalid */
    bool        readFromPos(RealTime pos, Texture::Ref& frame);
    bool        readFromPos(RealTime pos, GImage& frame);
    bool        readFromPos(RealTime pos, Image3uint8::Ref& frame);
    bool        readFromPos(RealTime pos, Image3::Ref& frame);

    /* Gets the frame at frame index and returns false if index is invalid */
    bool        readFromIndex(int index, Texture::Ref& frame);
    bool        readFromIndex(int index, GImage& frame);
    bool        readFromIndex(int index, Image3uint8::Ref& frame);
    bool        readFromIndex(int index, Image3::Ref& frame);

    /** Seek to playback position */
    void        setTimePosition(RealTime pos);
    void        setFrameIndex(int index);

    /** Seek ahead in playback position */
    void        skipTime(RealTime length);
    void        skipFrames(RealTime length);

    int         width() const;

    int         height() const;

    float       fps() const;

    /** Length of video in seconds */
    RealTime    length() const;

    /** Current playback position in seconds */
    RealTime    pos() const         { return m_currentTime; }

    bool        finished() const    { return m_finished; }

private:
    VideoInput();

    void initialize(const std::string& filename, const Settings& settings);

    static void decodingThreadProc(void* param);

    Settings            m_settings;
    std::string         m_filename;

    RealTime            m_currentTime;
    volatile bool       m_finished;

    struct Buffer
    {
        AVFrame* m_frame;
        RealTime m_pos;
    };

    Queue<Buffer*>      m_decodedBuffers;
    Queue<Buffer*>      m_emptyBuffers;
    GMutex              m_bufferMutex;

    GThreadRef          m_decodingThread;

    volatile bool       m_clearBuffersAndSeek;
    int64               m_seekTimestamp;

    // ffmpeg management
    AVFormatContext*    m_avFormatContext;
    AVCodecContext*     m_avCodecContext;
    AVCodec*            m_avVideoCodec;
    int                 m_avVideoStreamIdx;
};

} // namespace G3D

#endif // G3D_VIDEOINPUT_H
