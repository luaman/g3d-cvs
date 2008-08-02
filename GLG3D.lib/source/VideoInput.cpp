/** 
 @file VideoInput.cpp
 @author Corey Taylor
 */

#include "G3D/platform.h"
#include "GLG3D/VideoInput.h"
#include "GLG3D/Texture.h"
#include "G3D/fileutils.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include <errno.h>
}

namespace G3D {

VideoInput::Ref VideoInput::fromFile(const std::string& filename, const Settings& settings) {
    Ref vi = new VideoInput;

    debugAssert(fileExists(filename, false)); // TODO: make exception

    try {
        vi->initialize(filename, settings);
    } catch (const std::string& s) {
        // TODO: Throw the exception
        debugAssertM(false, s);(foid)s;
        vi = NULL;
    }
    return vi;
}


VideoInput::VideoInput() : 
    m_currentTime(0.0f),
    m_finished(false),
    m_clearBuffersAndSeek(false),
    m_seekTimestamp(0),
    m_avFormatContext(NULL),
    m_avCodecContext(NULL),
    m_avVideoCodec(NULL),
    m_avVideoStreamIdx(-1) {

}

VideoInput::~VideoInput() {
    m_finished = true;

    if (m_decodingThread.notNull() && !m_decodingThread->completed()) {
        m_decodingThread->waitForCompletion();
    }

    // shutdown ffmpeg
    avcodec_close(m_avCodecContext);
    av_close_input_file(m_avFormatContext);

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
        return "Not implemented in FFMPEG";

    default:
        return "Unrecognized error code.";
    }
}

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
    throwException(m_avCodecContext != NULL, ("Unable to initialize ffmpeg."));

    // Find the video codec
    m_avVideoCodec = avcodec_find_decoder(m_avCodecContext->codec_id);
    throwException(m_avVideoCodec, ("Unable to initialize ffmpeg."));

    // Initialize the codecs
    avRet = avcodec_open(m_avCodecContext, m_avVideoCodec);
    throwException(avRet >= 0, ("Unable to initialize ffmpeg."));

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

    bool frameUpdated = false;
    if ((m_decodedBuffers.length() > 0) && (m_decodedBuffers[0]->m_pos <= m_currentTime)) {
        Buffer* buffer = m_decodedBuffers.dequeue();

        // clear exiting texture
        frame = NULL;

        // create new texture
        frame = Texture::fromMemory("VideoInput frame", buffer->m_frame->data[0], TextureFormat::RGB8(), width(), height(), 1, TextureFormat::AUTO(), Texture::DIM_2D_NPOT, Texture::Settings::video());

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

    bool frameUpdated = false;
    if ((m_decodedBuffers.length() > 0) && (m_decodedBuffers[0]->m_pos <= m_currentTime)) {
        Buffer* buffer = m_decodedBuffers.dequeue();

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

    bool frameUpdated = false;
    if ((m_decodedBuffers.length() > 0) && (m_decodedBuffers[0]->m_pos <= m_currentTime)) {
        Buffer* buffer = m_decodedBuffers.dequeue();

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

    bool frameUpdated = false;
    if ((m_decodedBuffers.length() > 0) && (m_decodedBuffers[0]->m_pos <= m_currentTime)) {
        Buffer* buffer = m_decodedBuffers.dequeue();

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
    setTimePosition(pos);

    bool foundFrame = false;
    while (!m_decodingThread->completed()) {
        System::sleep(0.005);

        m_bufferMutex.lock();
        if (m_decodedBuffers.length() > 0) {
            // adjust current playback position to the time of keyframe found
            m_currentTime = m_decodedBuffers[0]->m_pos;

            m_bufferMutex.unlock();

            readNext(0.0, frame);

            foundFrame = true;

            break;
        } else {
            m_bufferMutex.unlock();
        }
    }

    // invalidate video if seek failed
    if (!foundFrame) {
        m_finished = true;
    }

    return foundFrame;
}

bool VideoInput::readFromPos(RealTime pos, GImage& frame) {
    setTimePosition(pos);

    bool foundFrame = false;
    while (!m_decodingThread->completed()) {
        System::sleep(0.005);

        m_bufferMutex.lock();
        if (m_decodedBuffers.length() > 0) {
            // adjust current playback position to the time of keyframe found
            m_currentTime = m_decodedBuffers[0]->m_pos;

            m_bufferMutex.unlock();

            readNext(0.0, frame);

            foundFrame = true;

            break;
        } else {
            m_bufferMutex.unlock();
        }
    }

    // invalidate video if seek failed
    if (!foundFrame) {
        m_finished = true;
    }

    return foundFrame;
}

bool VideoInput::readFromPos(RealTime pos, Image3uint8::Ref& frame) {
    setTimePosition(pos);

    bool foundFrame = false;
    while (!m_decodingThread->completed()) {
        System::sleep(0.005);

        m_bufferMutex.lock();
        if (m_decodedBuffers.length() > 0) {
            // adjust current playback position to the time of keyframe found
            m_currentTime = m_decodedBuffers[0]->m_pos;

            m_bufferMutex.unlock();

            readNext(0.0, frame);

            foundFrame = true;

            break;
        } else {
            m_bufferMutex.unlock();
        }
    }

    // invalidate video if seek failed
    if (!foundFrame) {
        m_finished = true;
    }

    return foundFrame;
}

bool VideoInput::readFromPos(RealTime pos, Image3::Ref& frame) {
    setTimePosition(pos);

    bool foundFrame = false;
    while (!m_decodingThread->completed()) {
        System::sleep(0.005);

        m_bufferMutex.lock();
        if (m_decodedBuffers.length() > 0) {
            // adjust current playback position to the time of keyframe found
            m_currentTime = m_decodedBuffers[0]->m_pos;

            m_bufferMutex.unlock();

            readNext(0.0, frame);

            foundFrame = true;

            break;
        } else {
            m_bufferMutex.unlock();
        }
    }

    // invalidate video if seek failed
    if (!foundFrame) {
        m_finished = true;
    }

    return foundFrame;
}

bool VideoInput::readFromIndex(int index, Texture::Ref& frame) {
    return readFromPos(index * av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->r_frame_rate), frame);
}

bool VideoInput::readFromIndex(int index, GImage& frame) {
    return readFromPos(index * av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->r_frame_rate), frame);
}

bool VideoInput::readFromIndex(int index, Image3uint8::Ref& frame) {
    return readFromPos(index * av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->r_frame_rate), frame);
}

bool VideoInput::readFromIndex(int index, Image3::Ref& frame) {
    return readFromPos(index * av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->r_frame_rate), frame);
}

void VideoInput::setTimePosition(RealTime pos) {
    // calculate timestamp in stream time base units
    m_seekTimestamp = static_cast<int64>(pos / av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->time_base)) + m_avFormatContext->streams[m_avVideoStreamIdx]->start_time;

    // tell decoding thread to clear buffers and start at this position
    m_clearBuffersAndSeek = true;

    m_currentTime = pos;
}

void VideoInput::setFrameIndex(int index) {
    setTimePosition(index * av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->r_frame_rate));
}

void VideoInput::skipTime(RealTime length) {
    setTimePosition(m_currentTime + length);
}

void VideoInput::skipFrames(int length) {
    setTimePosition(m_currentTime + (length * av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->r_frame_rate)));
}

int VideoInput::width() const {
    return m_avCodecContext->width;
}

int VideoInput::height() const {
    return m_avCodecContext->height;
}

float VideoInput::fps() const {
    return static_cast<float>(av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->r_frame_rate));
}

RealTime VideoInput::length() const {
    return m_avFormatContext->streams[m_avVideoStreamIdx]->duration * av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->time_base);
}

void VideoInput::decodingThreadProc(void* param) {

    VideoInput* vi = reinterpret_cast<VideoInput*>(param);

    // allocate avframe to hold decoded frame
    AVFrame* decodingFrame = avcodec_alloc_frame();
    debugAssert(decodingFrame);

    Buffer* emptyBuffer = NULL;

    bool completed = false;
    while (!vi->m_finished && !completed) {

        // packet used during seeking and decoding
        AVPacket packet;
        memset(&packet, 0, sizeof(packet));

        bool useSeekDecodedFrame = false;

        // check if a seek was requested
        if (vi->m_clearBuffersAndSeek) {
            GMutexLock lock(&vi->m_bufferMutex);

            while (vi->m_decodedBuffers.length() > 0) {
                vi->m_emptyBuffers.enqueue(vi->m_decodedBuffers.dequeue());
            }

            // flush codec
            avcodec_flush_buffers(vi->m_avCodecContext);

            // seek to timestamp
            int seekRet = av_seek_frame(vi->m_avFormatContext, vi->m_avVideoStreamIdx, vi->m_seekTimestamp, AVSEEK_FLAG_BACKWARD);
            debugAssert(seekRet >= 0);(void)seekRet;

            // todo: shutdown on seekRet error?

            // read frames up to desired frame since can only seek to a key frame
            do {

                int readRet = av_read_frame(vi->m_avFormatContext, &packet);
                debugAssert(readRet >= 0);

                if ((readRet >= 0) && packet.stream_index == vi->m_avVideoStreamIdx) {
                    int completedFrame = 0;
                    avcodec_decode_video(vi->m_avCodecContext, decodingFrame, &completedFrame, packet.data, packet.size);
                    debugAssert(completedFrame);

                    // if checking the seek find that we're at the frame we want, then use it
                    // otherwise quit seeking and the next decoded frame will be the target frame
                    if (packet.pts >= vi->m_seekTimestamp) {
                        useSeekDecodedFrame = true;
                    }
                }

                // only delete the packet if we're reading past it, otherwise save for decoder
                if (packet.pts < (vi->m_seekTimestamp)) {
                    av_free_packet(&packet);
                }
            } while (packet.pts < vi->m_seekTimestamp);

            // reset seek flag
            vi->m_clearBuffersAndSeek = false;
        }

        // wait until buffer is available
        while (!emptyBuffer && !vi->m_finished) {
            System::sleep(0.005f);

            GMutexLock lock(&vi->m_bufferMutex);
            if (vi->m_emptyBuffers.length() > 0) {
                emptyBuffer = vi->m_emptyBuffers.dequeue();
            }
        }

        // read in a full frame and decode
        bool frameRead = false;
        while (!frameRead && !vi->m_finished) {

            // if packet.data is still valid from seek, use that packet
            if (useSeekDecodedFrame || (av_read_frame(vi->m_avFormatContext, &packet) >= 0)) {

                // ignore frames other than our video frame
                if (packet.stream_index == vi->m_avVideoStreamIdx) {

                    // decode video frame
                    int completedFrame = 0;
                    if (useSeekDecodedFrame) {
                        completedFrame = 1;
                    } else {
                        avcodec_decode_video(vi->m_avCodecContext, decodingFrame, &completedFrame, packet.data, packet.size);
                    }
              
                    // for some reason completed frame is an int
                    if (completedFrame != 0) {
                        // Convert the image from its native format to RGB
                        img_convert((AVPicture*)emptyBuffer->m_frame, PIX_FMT_RGB24, (AVPicture*)decodingFrame, vi->m_avCodecContext->pix_fmt, vi->m_avCodecContext->width, vi->m_avCodecContext->height);

                        // calculate start time based off of presentation time stamp
                        // might need to check for valid pts and use dts
                        emptyBuffer->m_pos = (packet.pts - vi->m_avFormatContext->streams[vi->m_avVideoStreamIdx]->start_time) * av_q2d(vi->m_avCodecContext->time_base);

                        // add frame to decoded queue
                        {
                            GMutexLock lock(&vi->m_bufferMutex);                        
                            vi->m_decodedBuffers.enqueue(emptyBuffer);

                            if (vi->m_emptyBuffers.length() > 0) {
                                emptyBuffer = vi->m_emptyBuffers.dequeue();
                            } else {
                                emptyBuffer = NULL;
                            }
                        }

                        // current frame is finished
                        frameRead = true;
                    }
                }

                // clean up packaet allocated during frame decoding
                av_free_packet(&packet);
            } else {
                // no more frames to decode so break out of this loop
                completed = true;
                frameRead = true;
            }
        }

        // make sure the packet isn't leaked if decoding is canceled mid-frame somehow
        if (packet.data != NULL) {
            av_free_packet(&packet);
        }
    }

    // free codec decoding frame
    av_free(decodingFrame);
}

} // namespace G3D

