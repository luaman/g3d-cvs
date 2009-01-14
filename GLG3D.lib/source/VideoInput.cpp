/** 
 @file VideoInput.cpp
 @author Corey Taylor
 */

#include "G3D/platform.h"
#include "G3D/fileutils.h"
#include "GLG3D/VideoInput.h"
#include "GLG3D/Texture.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include <errno.h>
}

namespace G3D {

VideoInput::Ref VideoInput::fromFile(const std::string& filename, const Settings& settings) {
    Ref vi = new VideoInput;

    try {
        vi->initialize(filename, settings);
    } catch (const std::string& s) {
        // TODO: Throw the exception
        debugAssertM(false, s);(void)s;
        vi = NULL;
    }
    return vi;
}


VideoInput::VideoInput() : 
    m_currentTime(0.0f),
    m_currentIndex(0),
    m_finished(false),
    m_quitThread(false),
    m_clearBuffersAndSeek(false),
    m_seekTimestamp(-1),
    m_lastTimestamp(-1),
    m_avFormatContext(NULL),
    m_avCodecContext(NULL),
    m_avVideoCodec(NULL),
    m_avVideoStreamIdx(-1) {

}

VideoInput::~VideoInput() {
    // shutdown decoding thread
    if (m_decodingThread.notNull() && !m_decodingThread->completed()) {
        m_quitThread = true;
        m_decodingThread->waitForCompletion();
    }

    // shutdown ffmpeg
    avcodec_close(m_avCodecContext);
    av_close_input_file(m_avFormatContext);

    // clear decoding buffers
    while (m_emptyBuffers.length() > 0) {
        Buffer* buffer = m_emptyBuffers.dequeue();
        av_free(buffer->m_frame->data[0]);
        av_free(buffer->m_frame);
        delete buffer;
    }

    while (m_decodedBuffers.length() > 0) {
        Buffer* buffer = m_decodedBuffers.dequeue();
        av_free(buffer->m_frame->data[0]);
        av_free(buffer->m_frame);
        delete buffer;
    }
}

static const char* ffmpegError(int code);

void VideoInput::initialize(const std::string& filename, const Settings& settings) {
    // helper for exiting VideoInput construction (exceptions caught by static ref creator)
    #define throwException(exp, msg) if (!(exp)) { throw std::string(msg); }

    // initialize list of available muxers/demuxers and codecs in ffmpeg
    avcodec_register_all();
    av_register_all();

    m_filename = filename;
    m_settings = settings;

    int avRet = av_open_input_file(&m_avFormatContext, filename.c_str(), NULL, 0, NULL);
    throwException(avRet >= 0, ffmpegError(avRet));

    // find and use the first video stream by default
    // this may need to be expanded to configure or accommodate multiple streams in a file
    av_find_stream_info(m_avFormatContext);

    for (int streamIdx = 0; streamIdx < (int)m_avFormatContext->nb_streams; ++streamIdx) {
        if (m_avFormatContext->streams[streamIdx]->codec->codec_type == CODEC_TYPE_VIDEO) {
            m_avCodecContext = m_avFormatContext->streams[streamIdx]->codec;
            m_avVideoStreamIdx = streamIdx;
            break;
        }
    }

    // We load on the assumption that this is a video file
    throwException(m_avCodecContext != NULL, ("Unable to initialize FFmpeg."));

    // Find the video codec
    m_avVideoCodec = avcodec_find_decoder(m_avCodecContext->codec_id);
    throwException(m_avVideoCodec, ("Unable to initialize FFmpeg."));

    // Initialize the codecs
    avRet = avcodec_open(m_avCodecContext, m_avVideoCodec);
    throwException(avRet >= 0, ("Unable to initialize FFmpeg."));

    // Create array of buffers for decoding
    int bufferSize = avpicture_get_size(PIX_FMT_RGB24, m_avCodecContext->width, m_avCodecContext->height);

    for (int bufferIdx = 0; bufferIdx < m_settings.numBuffers; ++bufferIdx) {
        Buffer* buffer = new Buffer;

        buffer->m_frame = avcodec_alloc_frame();
        
        // Allocate the rgb buffer
        uint8* rgbBuffer = static_cast<uint8*>(av_malloc(bufferSize));

        // Initialize the frame
        avpicture_fill(reinterpret_cast<AVPicture*>(buffer->m_frame), rgbBuffer, PIX_FMT_RGB24, m_avCodecContext->width, m_avCodecContext->height);

        // add to queue of empty frames
        m_emptyBuffers.enqueue(buffer);
    }

    // everything is setup and ready to be decoded
    m_decodingThread = GThread::create("VideoInput::m_bufferThread", VideoInput::decodingThreadProc, this);
    m_decodingThread->start();
}

bool VideoInput::readNext(RealTime timeStep, Texture::Ref& frame) {
    GMutexLock m(&m_bufferMutex);

    m_currentTime += timeStep;

    bool readAfterSeek = (m_seekTimestamp != -1);

    bool frameUpdated = false;
    if ((m_decodedBuffers.length() > 0) && 
        (readAfterSeek || (m_decodedBuffers[0]->m_pos <= m_currentTime))) {

        Buffer* buffer = m_decodedBuffers.dequeue();

        // reset seek
        if (readAfterSeek) {
            m_seekTimestamp = -1;
        }

        // increment current playback index
        ++m_currentIndex;

        // adjust current playback position to the time of the frame
        m_currentTime = buffer->m_pos;

        // check if the texture is re-usable and create a new one if not
        if (frame.notNull() && frame->width() == width() && frame->height() == height()) {

            // update existing texture
            glBindTexture(frame->openGLTextureTarget(), frame->openGLID());
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            
            glTexImage2D(frame->openGLTextureTarget(), 0, frame->format()->openGLFormat, frame->width(), frame->height(), 0,
                TextureFormat::RGB8()->openGLBaseFormat, TextureFormat::RGB8()->openGLDataFormat, buffer->m_frame->data[0]);
            
            glBindTexture(frame->openGLTextureTarget(), NULL);

            // make sure this renders correctly since we didn't create the texture
            frame->invertY = false;
        } else {
            // clear existing texture
            frame = NULL;

            // create new texture with right dimentions
            frame = Texture::fromMemory("VideoInput frame", buffer->m_frame->data[0], TextureFormat::RGB8(), width(), height(), 1, TextureFormat::AUTO(), Texture::DIM_2D_NPOT, Texture::Settings::video());
        }

        m_emptyBuffers.enqueue(buffer);
        frameUpdated = true;

        // check if video is finished
        if ((m_decodedBuffers.length() == 0) && m_decodingThread->completed()) {
            m_finished = true;
        }
    }

    return frameUpdated;
}

bool VideoInput::readNext(RealTime timeStep, GImage& frame) {
    GMutexLock m(&m_bufferMutex);

    m_currentTime += timeStep;

    bool readAfterSeek = (m_seekTimestamp != -1);

    bool frameUpdated = false;
    if ((m_decodedBuffers.length() > 0) && 
        (readAfterSeek || (m_decodedBuffers[0]->m_pos <= m_currentTime))) {

        Buffer* buffer = m_decodedBuffers.dequeue();

        // reset seek
        if (readAfterSeek) {
            m_seekTimestamp = -1;
        }

        // increment current playback index
        ++m_currentIndex;

        // adjust current playback position to the time of the frame
        m_currentTime = buffer->m_pos;

        // create 3-channel GImage (RGB8)
        frame.resize(width(), height(), 3);

        // copy frame
        memcpy(frame.byte(), buffer->m_frame->data[0], (width() * height() * 3));

        m_emptyBuffers.enqueue(buffer);
        frameUpdated = true;

        // check if video is finished
        if ((m_decodedBuffers.length() == 0) && m_decodingThread->completed()) {
            m_finished = true;
        }
    }

    return frameUpdated;
}

bool VideoInput::readNext(RealTime timeStep, Image3uint8::Ref& frame) {
    GMutexLock m(&m_bufferMutex);

    m_currentTime += timeStep;

    bool readAfterSeek = (m_seekTimestamp != -1);

    bool frameUpdated = false;
    if ((m_decodedBuffers.length() > 0) && 
        (readAfterSeek || (m_decodedBuffers[0]->m_pos <= m_currentTime))) {

        Buffer* buffer = m_decodedBuffers.dequeue();

        // reset seek
        if (readAfterSeek) {
            m_seekTimestamp = -1;
        }

        // increment current playback index
        ++m_currentIndex;

        // adjust current playback position to the time of the frame
        m_currentTime = buffer->m_pos;

        // clear existing image
        frame = NULL;
        
        // create new image
        frame = Image3uint8::fromArray(reinterpret_cast<Color3uint8*>(buffer->m_frame->data[0]), width(), height());

        m_emptyBuffers.enqueue(buffer);
        frameUpdated = true;

        // check if video is finished
        if ((m_decodedBuffers.length() == 0) && m_decodingThread->completed()) {
            m_finished = true;
        }
    }

    return frameUpdated;
}

bool VideoInput::readNext(RealTime timeStep, Image3::Ref& frame) {
    GMutexLock m(&m_bufferMutex);

    m_currentTime += timeStep;

    bool readAfterSeek = (m_seekTimestamp != -1);

    bool frameUpdated = false;
    if ((m_decodedBuffers.length() > 0) && 
        (readAfterSeek || (m_decodedBuffers[0]->m_pos <= m_currentTime))) {

        Buffer* buffer = m_decodedBuffers.dequeue();

        // reset seek
        if (readAfterSeek) {
            m_seekTimestamp = -1;
        }

        // increment current playback index
        ++m_currentIndex;

        // adjust current playback position to the time of the frame
        m_currentTime = buffer->m_pos;

        // clear existing image
        frame = NULL;
        
        // create new image
        frame = Image3::fromArray(reinterpret_cast<Color3uint8*>(buffer->m_frame->data[0]), width(), height());

        m_emptyBuffers.enqueue(buffer);
        frameUpdated = true;

        // check if video is finished
        if ((m_decodedBuffers.length() == 0) && m_decodingThread->completed()) {
            m_finished = true;
        }
    }

    return frameUpdated;
}

bool VideoInput::readFromPos(RealTime pos, Texture::Ref& frame) {
    // find the closest index to seek to
    return readFromIndex(iFloor(pos * fps()), frame);
}

bool VideoInput::readFromPos(RealTime pos, GImage& frame) {
    // find the closest index to seek to
    return readFromIndex(iFloor(pos * fps()), frame);
}

bool VideoInput::readFromPos(RealTime pos, Image3uint8::Ref& frame) {
    // find the closest index to seek to
    return readFromIndex(iFloor(pos * fps()), frame);
}

bool VideoInput::readFromPos(RealTime pos, Image3::Ref& frame) {
    // find the closest index to seek to
    return readFromIndex(iFloor(pos * fps()), frame);
}

bool VideoInput::readFromIndex(int index, Texture::Ref& frame) {
    setIndex(index);

    // wait for seek to complete
    while (!m_decodingThread->completed() && m_clearBuffersAndSeek) {
        System::sleep(0.005);
    }

    bool foundFrame = false;

    // wait for a new frame after seek and read it
    while(!m_decodingThread->completed() && !foundFrame) {

        // check for frame
        m_bufferMutex.lock();
        foundFrame = (m_decodedBuffers.length() > 0);
        m_bufferMutex.unlock();

        if (foundFrame) {
            // read new frame
            debugAssert(readNext(0.0, frame));
        } else {
            // let decode run more
            System::sleep(0.005);
        }
    }

    // invalidate video if seek failed
    if (!foundFrame) {
        m_finished = true;
    }

    return foundFrame;
}

bool VideoInput::readFromIndex(int index, GImage& frame) {
    setIndex(index);

    // wait for seek to complete
    while (!m_decodingThread->completed() && m_clearBuffersAndSeek) {
        System::sleep(0.005);
    }

    bool foundFrame = false;

    // wait for a new frame after seek and read it
    while(!m_decodingThread->completed() && !foundFrame) {

        // check for frame
        m_bufferMutex.lock();
        foundFrame = (m_decodedBuffers.length() > 0);
        m_bufferMutex.unlock();

        if (foundFrame) {
            // read new frame
            readNext(0.0, frame);
        } else {
            // let decode run more
            System::sleep(0.005);
        }
    }

    // invalidate video if seek failed
    if (!foundFrame) {
        m_finished = true;
    }

    return foundFrame;
}

bool VideoInput::readFromIndex(int index, Image3uint8::Ref& frame) {
    setIndex(index);

    // wait for seek to complete
    while (!m_decodingThread->completed() && m_clearBuffersAndSeek) {
        System::sleep(0.005);
    }

    bool foundFrame = false;

    // wait for a new frame after seek and read it
    while(!m_decodingThread->completed() && !foundFrame) {

        // check for frame
        m_bufferMutex.lock();
        foundFrame = (m_decodedBuffers.length() > 0);
        m_bufferMutex.unlock();

        if (foundFrame) {
            // read new frame
            readNext(0.0, frame);
        } else {
            // let decode run more
            System::sleep(0.005);
        }
    }

    // invalidate video if seek failed
    if (!foundFrame) {
        m_finished = true;
    }

    return foundFrame;
}

bool VideoInput::readFromIndex(int index, Image3::Ref& frame) {
    setIndex(index);

    // wait for seek to complete
    while (!m_decodingThread->completed() && m_clearBuffersAndSeek) {
        System::sleep(0.005);
    }

    bool foundFrame = false;

    // wait for a new frame after seek and read it
    while(!m_decodingThread->completed() && !foundFrame) {

        // check for frame
        m_bufferMutex.lock();
        foundFrame = (m_decodedBuffers.length() > 0);
        m_bufferMutex.unlock();

        if (foundFrame) {
            // read new frame
            readNext(0.0, frame);
        } else {
            // let decode run more
            System::sleep(0.005);
        }
    }

    // invalidate video if seek failed
    if (!foundFrame) {
        m_finished = true;
    }

    return foundFrame;
}

void VideoInput::setTimePosition(RealTime pos) {
    // find the closest index to seek to
    setIndex(iFloor(pos * fps()));
}

void VideoInput::setIndex(int index) {
    m_currentIndex = index;

    m_currentTime = index / fps();

    // calculate timestamp in stream time base units
    m_seekTimestamp = static_cast<int64>(G3D::fuzzyEpsilon + m_currentTime / av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->time_base)) + m_avFormatContext->streams[m_avVideoStreamIdx]->start_time;

    // tell decoding thread to clear buffers and start at this position
    m_clearBuffersAndSeek = true;
}

void VideoInput::skipTime(RealTime length) {
    setTimePosition(m_currentTime + length);
}

void VideoInput::skipFrames(int length) {
    setIndex(m_currentIndex + length);
}

int VideoInput::width() const {
    return m_avCodecContext->width;
}

int VideoInput::height() const {
    return m_avCodecContext->height;
}

RealTime VideoInput::fps() const {
    // return FFmpeg's calculated base frame rate
    return av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->r_frame_rate);
}

RealTime VideoInput::length() const {
    // return duration in seconds calculated from stream duration in FFmpeg's stream time base
    return m_avFormatContext->streams[m_avVideoStreamIdx]->duration * av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->time_base);
}

RealTime VideoInput::pos() const {
    return m_currentTime;
}

int VideoInput::numFrames() const {
    return static_cast<int>(length() * fps());
}

int VideoInput::index() const {
    return m_currentIndex;
}

void VideoInput::decodingThreadProc(void* param) {

    VideoInput* vi = reinterpret_cast<VideoInput*>(param);

    // allocate avframe to hold decoded frame
    AVFrame* decodingFrame = avcodec_alloc_frame();
    debugAssert(decodingFrame);

    Buffer* emptyBuffer = NULL;

    AVPacket packet;
    bool useExistingSeekPacket = false;

    while (!vi->m_quitThread) {

        // seek to frame if requested
        if (vi->m_clearBuffersAndSeek) {
            seekToTimestamp(vi, decodingFrame, &packet, useExistingSeekPacket);
            vi->m_clearBuffersAndSeek = false;
        }

        // get next available empty buffer
        if (emptyBuffer == NULL) {
            // yield while no available buffers
            System::sleep(0.005f);

            // check for a new buffer
            GMutexLock lock(&vi->m_bufferMutex);
            if (vi->m_emptyBuffers.length() > 0) {
                emptyBuffer = vi->m_emptyBuffers.dequeue();
            }
        }

        if (emptyBuffer && !vi->m_quitThread) {

            // decode next frame
            if (!useExistingSeekPacket) {
                // exit thread if video is complete (or errors)
                if (av_read_frame(vi->m_avFormatContext, &packet) != 0) {
                    vi->m_quitThread = true;
                }
            }

            // reeset now that we are decoding the frame and not waiting on a free buffer
            useExistingSeekPacket = false;

            // ignore frames other than our video frame
            if (packet.stream_index == vi->m_avVideoStreamIdx) {

                // decode the frame
                int completedFrame = 0;
                avcodec_decode_video(vi->m_avCodecContext, decodingFrame, &completedFrame, packet.data, packet.size);

                // we have a valid frame, let's use it!
                if (completedFrame != 0) {

                    // Convert the image from its native format to RGB
                    img_convert((AVPicture*)emptyBuffer->m_frame, PIX_FMT_RGB24, (AVPicture*)decodingFrame, vi->m_avCodecContext->pix_fmt, vi->m_avCodecContext->width, vi->m_avCodecContext->height);

                    // calculate start time based off of presentation time stamp
                    emptyBuffer->m_pos = (packet.pts - vi->m_avFormatContext->streams[vi->m_avVideoStreamIdx]->start_time) * av_q2d(vi->m_avCodecContext->time_base);

                    // store original time stamp of frame
                    emptyBuffer->m_timestamp = packet.pts;

                    // set last decoded timestamp
                    vi->m_lastTimestamp = packet.pts;

                    // add frame to decoded queue
                    vi->m_bufferMutex.lock();
                    vi->m_decodedBuffers.enqueue(emptyBuffer);

                    // get new buffer if available since we have the lock
                    if (vi->m_emptyBuffers.length() > 0) {
                        emptyBuffer = vi->m_emptyBuffers.dequeue();
                    } else {
                        emptyBuffer = NULL;
                    }
                    vi->m_bufferMutex.unlock();
                }
            }
        }          

        // always clean up packaet allocated during av_read_frame
        if (packet.data != NULL) {
            av_free_packet(&packet);
        }
    }

    // free codec decoding frame
    av_free(decodingFrame);
}




// decoding thread helpers
void VideoInput::seekToTimestamp(VideoInput* vi, AVFrame* decodingFrame, AVPacket* packet, bool& validPacket) {
    // maximum number of frames to decode before seeking (1 second)
    const int64 MAX_DECODE_FRAMES = iRound(vi->fps());

    GMutexLock lock(&vi->m_bufferMutex);

    // remove frames before target timestamp
    while ((vi->m_decodedBuffers.length() > 0)) {
        if ((vi->m_decodedBuffers[0]->m_timestamp != vi->m_seekTimestamp)) {
            vi->m_emptyBuffers.enqueue(vi->m_decodedBuffers.dequeue());
        } else {
            // don't remove buffers past desired frame!
            break;
        }
    }

    // will be set below if valid
    validPacket = false;

    if (vi->m_decodedBuffers.length() == 0) {
        // TODO - try to use av_index_search_timestamp() to calculate if a seek will just cause a key frame reset

        // don't need to seek if we are close enough to just decode
        int64 seekDiff = vi->m_seekTimestamp - vi->m_lastTimestamp;

        if ((seekDiff <= 0) || (seekDiff > MAX_DECODE_FRAMES)) {
            // flush FFmpeg decode buffers for seek
            avcodec_flush_buffers(vi->m_avCodecContext);

            int seekRet = av_seek_frame(vi->m_avFormatContext, vi->m_avVideoStreamIdx, vi->m_seekTimestamp, AVSEEK_FLAG_BACKWARD);
            debugAssert(seekRet >= 0);(void)seekRet;
        }

        // read frames up to desired frame since can only seek to a key frame
        do {
            int readRet = av_read_frame(vi->m_avFormatContext, packet);
            debugAssert(readRet >= 0);

            if ((readRet >= 0) && (packet->stream_index == vi->m_avVideoStreamIdx)) {

                // if checking the seek find that we're at the frame we want, then use it
                // otherwise quit seeking and the next decoded frame will be the target frame
                if (packet->pts >= vi->m_seekTimestamp) {
                    validPacket = true;
                } else {
                    int completedFrame = 0;
                    avcodec_decode_video(vi->m_avCodecContext, decodingFrame, &completedFrame, packet->data, packet->size);
                    debugAssert(completedFrame);
                }
            }

            // only delete the packet if we're reading past it, otherwise save for decoder
            if (packet->pts < (vi->m_seekTimestamp)) {
                av_free_packet(packet);
            }

        } while (packet->pts < vi->m_seekTimestamp);    
    }
}


// static helpers
static const char* ffmpegError(int code) {
    switch (iAbs(code)) {
    case AVERROR_UNKNOWN:
        return "Unknown error";
    
    case AVERROR_IO:
        return "I/O error";

    case AVERROR_NUMEXPECTED:
        return "Number syntax expected in filename.";

        // gives the compilation error, "case 22 already used."
//    case AVERROR_INVALIDDATA:
//        return "Invalid data found.";

    case AVERROR_NOMEM:
        return "Not enough memory.";

    case AVERROR_NOFMT:
        return "Unknown format";

    case AVERROR_NOTSUPP:
        return "Operation not supported.";

    case AVERROR_NOENT:
        return "No such file or directory.";

    case AVERROR_PATCHWELCOME:
        return "Not implemented in FFmpeg";

    default:
        return "Unrecognized error code.";
    }
}

} // namespace G3D

