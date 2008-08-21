/** 
 @file VideoOutput.cpp
 @author Corey Taylor
 */

#include "G3D/platform.h"
#include <cstdio>
#include "GLG3D/VideoOutput.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
}

namespace G3D {

VideoOutput::Settings::Settings() :
    codec(CODEC_ID_NONE),
    fps(0),
    width(0),
    height(0),
    bitrate(0),
    customFOURCC(0) {

    // just initializes the values so the optional entries aren't used
    raw.format = PIX_FMT_NONE;
    mpeg.bframes = 0;
    mpeg.gop = 0;
}

VideoOutput::Settings VideoOutput::Settings::uncompressedAVI() {
    Settings s;

    // uncompressed avi files use BGR not RGB
    s.codec = CODEC_ID_RAWVIDEO;
    s.raw.format = PIX_FMT_BGR24;

    return s;
}

VideoOutput::Settings VideoOutput::Settings::ffmpegMPEG4() {
    Settings s;

    // native ffmpeg iso mpeg4 implementation
    s.codec = CODEC_ID_MPEG4;
    
    // this is just a default and should be overriden by user, we don't have width/height to pre-calculate
    s.bitrate = 1024 * 512;

    // the 'XVID' fourcc code will make this a more compatible stream
    // but still using ffmpeg's native encoder
    s.customFOURCC = ('X' << 24) | ('V' << 16) | ('I' << 8) | ('D');

    return s;
}

VideoOutput::Ref VideoOutput::create(const std::string& filename, const Settings& settings) {
    VideoOutput* vo = new VideoOutput;

    // todo: should the exception still be thrown when we're creating a reference?
    try {
        vo->initialize(filename, settings);
    } catch (std::string s) {
        debugAssertM(false, s);

        delete vo;
        vo = NULL;
    }

    return Ref(vo);
}

VideoOutput::VideoOutput() :
    m_isInitialized(false),
    m_isFinished(false),
    m_avOutputFormat(NULL),
    m_avFormatContext(NULL),
    m_avStream(NULL),
    m_avInputBuffer(NULL),
    m_avInputFrame(NULL),
    m_avEncodingBuffer(NULL),
    m_avEncodingBufferSize(0)
{
}

VideoOutput::~VideoOutput() {
    if (!m_isFinished && m_isInitialized) {
        abort();
    }

    if (m_avInputBuffer) {
        av_free(m_avInputBuffer);
        m_avInputBuffer = NULL;
    }

    if (m_avInputFrame) {
        av_free(m_avInputFrame);
        m_avInputFrame = NULL;
    }

    if (m_avEncodingBuffer) {
        av_free(m_avEncodingBuffer);
        m_avEncodingBuffer = NULL;
    }

    if (m_avStream->codec) {
        avcodec_close(m_avStream->codec);
    }

    if (m_avStream) {
        av_free(m_avStream);
        m_avStream = NULL;
    }

    if (m_avFormatContext) {
        av_free(m_avFormatContext);
        m_avFormatContext = NULL;
    }
}

void VideoOutput::initialize(const std::string& filename, const Settings& settings) {
    // helper for exiting VideoOutput construction (exceptions caught by static ref creator)
    #define throwException(exp, msg) if (!(exp)) { throw std::string(msg); }

    // initialize list of available muxers/demuxers and codecs in ffmpeg
    avcodec_register_all();
    av_register_all();

    m_filename = filename;
    m_settings = settings;

    // see if ffmpeg can support this muxer and setup output format
    m_avOutputFormat = guess_format(NULL, filename.c_str(), NULL);
    throwException(m_avOutputFormat, ("Error initializing ffmpeg."));

    // set the codec id
    m_avOutputFormat->video_codec = static_cast< ::CodecID>(m_settings.codec);

    // create format context which controls writing the file
    m_avFormatContext = av_alloc_format_context();
    throwException(m_avFormatContext, ("Error initializing ffmpeg."));

    // attach format to context
    m_avFormatContext->oformat = m_avOutputFormat;
    strncpy(m_avFormatContext->filename, filename.c_str(), sizeof(m_avFormatContext->filename));

    // add video stream 0
    m_avStream = av_new_stream(m_avFormatContext, 0);
    throwException(m_avStream, ("Error initializing ffmpeg."));

    // setup video stream
    m_avStream->codec->codec_id     = m_avOutputFormat->video_codec;
    m_avStream->codec->codec_type   = CODEC_TYPE_VIDEO;

    // find and open required codec
    AVCodec* codec = avcodec_find_encoder(m_avStream->codec->codec_id);
    throwException(codec, ("Error initializing ffmpeg."));

    // finish setting codec parameters
    m_avStream->codec->bit_rate     = m_settings.bitrate;
    m_avStream->codec->time_base.den = m_settings.fps;
    m_avStream->codec->time_base.num = 1;
    m_avStream->codec->width        = m_settings.width;
    m_avStream->codec->height       = m_settings.height;

    // set codec input format
    if (m_settings.codec == CODEC_ID_RAWVIDEO) {
        m_avStream->codec->pix_fmt = static_cast< ::PixelFormat>(m_settings.raw.format);
    } else {
        m_avStream->codec->pix_fmt = codec->pix_fmts[0];
    }

    // set the fourcc
    if (m_settings.customFOURCC != 0) {
        m_avStream->codec->codec_tag  = m_settings.customFOURCC;
    }

    // set bframes and gop
    m_avStream->codec->max_b_frames = m_settings.mpeg.bframes;
    m_avStream->codec->gop_size = m_settings.mpeg.gop;

    // some formats want stream headers to be separate
    if (m_avOutputFormat->flags & AVFMT_GLOBALHEADER) {
        m_avStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    // set null parameters as this call is required
    int avRet = av_set_parameters(m_avFormatContext, NULL);
    throwException(avRet >= 0, ("Error initializing ffmpeg."));

    avRet = avcodec_open(m_avStream->codec, codec);
    throwException(avRet >= 0, ("Error initializing ffmpeg."));

    // create encoding buffer - just allocate largest possible for now (3 channels)
    m_avEncodingBufferSize = iMax(m_settings.width * m_settings.height * 3, 512 * 1024);
    m_avEncodingBuffer = static_cast<uint8*>(av_malloc(m_avEncodingBufferSize));

    // create buffer to hold converted input frame if the codec needs a conversion
    int inputBufferSize = avpicture_get_size(m_avStream->codec->pix_fmt, m_settings.width, m_settings.height);

    m_avInputBuffer = static_cast<uint8*>(av_malloc(inputBufferSize));
    throwException(m_avInputBuffer, ("Error initializing ffmpeg."));

    m_avInputFrame = avcodec_alloc_frame();
    throwException(m_avInputFrame, ("Error initializing ffmpeg."));
    avpicture_fill(reinterpret_cast<AVPicture*>(m_avInputFrame), m_avInputBuffer, m_avStream->codec->pix_fmt, m_settings.width, m_settings.height);

    // open output file for writing using ffmpeg
    avRet = url_fopen(&m_avFormatContext->pb, filename.c_str(), URL_WRONLY);
    throwException(avRet >= 0, ("Error opening ffmpeg video file."));

    // start the stream
    avRet = av_write_header(m_avFormatContext);

    // make sure the file is closed on error
    if (avRet < 0) {
        // abort closes and removes the output file
        abort();
        throwException(false, ("Error initializing and writing ffmpeg video file."));
    }

    m_isInitialized = true;
}

void VideoOutput::append(const Texture::Ref& frame) {

    // Only support formats available to GImage and ffmpeg and that are convertable
    const TextureFormat* format = TextureFormat::RGB8();

    // todo: should this be pre-allocated after first call?
    GImage img;
    frame->getImage(img, format);

    // todo: this can be slow flipping in-place - good candidate for simd optimization with 2nd buffer?
    if (frame->invertY) {
        GImage::flipRGBVertical(img.byte(), img.byte(), img.width, img.height);
    }

    append(img);
}

void VideoOutput::append(const GImage& frame) {
    throwException(frame.channels == 3, ("Appending 4-channel Gimage is not currently supported"));
    encodeAndWriteFrame(const_cast<uint8*>(frame.byte()), PIX_FMT_RGB24);
}

void VideoOutput::append(const Image1uint8::Ref& frame) {
    encodeAndWriteFrame(reinterpret_cast<uint8*>(frame->getCArray()), PIX_FMT_GRAY8);
}

void VideoOutput::append(const Image3uint8::Ref& frame) {
    encodeAndWriteFrame(reinterpret_cast<uint8*>(frame->getCArray()), PIX_FMT_RGB24);
}

void VideoOutput::append(const Image4uint8::Ref& frame) {
    // not currently supported
    throwException(false, ("Appending Image4uint8 frame is not currently supported"));
}

void VideoOutput::append(const Image1::Ref& frame) {
    // not currently supported
    throwException(false, ("Appending Image1 frame is not currently supported"));
}

void VideoOutput::append(const Image3::Ref& frame) {
    // not currently supported
    throwException(false, ("Appending Image3 frame is not currently supported"));
}

void VideoOutput::append(const Image4::Ref& frame) {
    // not currently supported
    throwException(false, ("Appending Image4 frame is not currently supported"));
}

void VideoOutput::append(uint8* frame, PixelFormat frameFormat) {
    encodeAndWriteFrame(frame, frameFormat);
}

void VideoOutput::encodeAndWriteFrame(uint8* frame, PixelFormat frameFormat)
{
    if (static_cast< ::PixelFormat>(frameFormat) != m_avStream->codec->pix_fmt) {
        // convert to required input format
        AVFrame* convFrame = avcodec_alloc_frame();
        throwException(convFrame, ("Unable to add frame."));

        avpicture_fill(reinterpret_cast<AVPicture*>(convFrame), reinterpret_cast<uint8_t*>(frame), frameFormat, m_settings.width, m_settings.height);
        int convertRet = img_convert(reinterpret_cast<AVPicture*>(m_avInputFrame), m_avStream->codec->pix_fmt, reinterpret_cast<AVPicture*>(convFrame), frameFormat, m_settings.width, m_settings.height);

        av_free(convFrame);

        throwException(convertRet >= 0, ("Unable to add frame of this format."));
    } else {
        avpicture_fill(reinterpret_cast<AVPicture*>(m_avInputFrame), reinterpret_cast<uint8_t*>(frame), frameFormat, m_settings.width, m_settings.height);
    }

    // encode frame
    int encodeSize = avcodec_encode_video(m_avStream->codec, m_avEncodingBuffer, m_avEncodingBufferSize, m_avInputFrame);

    // write the frame
    if (encodeSize > 0) {
        AVPacket packet;
        av_init_packet(&packet);

        packet.pts          = av_rescale_q(m_avStream->codec->coded_frame->pts, m_avStream->codec->time_base, m_avStream->time_base);
        packet.stream_index = m_avStream->index;
        packet.data         = m_avEncodingBuffer;
        packet.size         = encodeSize;

        if(m_avStream->codec->coded_frame->key_frame) {
            packet.flags |= PKT_FLAG_KEY;
        }

        av_write_frame(m_avFormatContext, &packet);
    }
}

void VideoOutput::commit() {
    m_isFinished = true;

    if (m_isInitialized) {
        // write the trailer to create a valid file
        av_write_trailer(m_avFormatContext);

        url_fclose(m_avFormatContext->pb);
    }
}

void VideoOutput::abort() {
    m_isFinished = true;

    if (m_avFormatContext && m_avFormatContext->pb) {
        url_fclose(m_avFormatContext->pb);
        m_avFormatContext->pb = NULL;

#ifdef _MSC_VER
        _unlink(m_filename.c_str());
#else
        unlink(m_filename.c_str());
#endif //_MSVC_VER
    }
}

} // namespace G3D

