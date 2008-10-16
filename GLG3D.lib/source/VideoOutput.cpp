/** 
 @file VideoOutput.cpp
 @author Corey Taylor
 */

#include "G3D/platform.h"
#include <cstdio>
#include "GLG3D/VideoOutput.h"
#include "GLG3D/RenderDevice.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
}

namespace G3D {

void VideoOutput::getSupportedCodecs(Array<std::string>& list) {
    Array<CodecID> codec;
    getSupportedCodecs(codec);
    list.resize(codec.size());
    for (int i = 0; i < codec.size(); ++i) {
        list[i] = toString(codec[i]);
    }
}


void VideoOutput::getSupportedCodecs(Array<CodecID>& list) {
    for (int i = CODEC_ID_NONE; i < CODEC_ID_LAST; ++i) {
        CodecID c = (CodecID)i;
        if (supports(c)) {
            list.append(c);
        }
    }
}


bool VideoOutput::supports(CodecID c) {
    // initialize list of available muxers/demuxers and codecs in ffmpeg
    // TODO: is it ok to execute this multiple times?
    avcodec_register_all();
    av_register_all();

    AVCodec* codec = avcodec_find_encoder(static_cast< ::CodecID>(c));
    return codec != NULL;
}


const char* VideoOutput::toString(CodecID c) {
    switch (c) {
    case CODEC_ID_MPEG1VIDEO: return "MPEG1";
    case CODEC_ID_MPEG2VIDEO: return "MPEG2";
    case CODEC_ID_MPEG2VIDEO_XVMC: return "MPEG2_XVMC";
    case CODEC_ID_H261: return "H.261";
    case CODEC_ID_H263: return "H.263";
    case CODEC_ID_RV10: return "RV10";
    case CODEC_ID_RV20: return "RV20";
    case CODEC_ID_MJPEG: return "MJPEG";
    case CODEC_ID_MJPEGB: return "MJPEGB";
    case CODEC_ID_LJPEG: return "LJPEG";
    case CODEC_ID_SP5X: return "SP5X";
    case CODEC_ID_JPEGLS: return "JPEGLS";
    case CODEC_ID_MPEG4: return "MPEG4";
    case CODEC_ID_RAWVIDEO: return "Raw Video";
    case CODEC_ID_MSMPEG4V1: return "MS MPEG v1";
    case CODEC_ID_MSMPEG4V2: return "MS MPEG v2";
    case CODEC_ID_MSMPEG4V3: return "MS MPEG v3";
    case CODEC_ID_WMV1: return "WMV1";
    case CODEC_ID_WMV2: return "WMV2";
    case CODEC_ID_H263P: return "H.263P";
    case CODEC_ID_H263I: return "H.263I";
    case CODEC_ID_FLV1: return "FLV1";
    case CODEC_ID_SVQ1: return "SVQ1";
    case CODEC_ID_SVQ3: return "SVQ3";
    case CODEC_ID_DVVIDEO: return "DV";
    case CODEC_ID_HUFFYUV: return "HuffYUV";
    case CODEC_ID_CYUV: return "CYUV";
    case CODEC_ID_H264: return "H.264";
    case CODEC_ID_INDEO3: return "Indeo3";
    case CODEC_ID_VP3: return "VP3";
    case CODEC_ID_THEORA: return "Theora";
    case CODEC_ID_ASV1: return "ASV1";
    case CODEC_ID_ASV2: return "ASV2";
    case CODEC_ID_FFV1: return "FFV1";
    case CODEC_ID_4XM: return "4XM";
    case CODEC_ID_VCR1: return "VCR1";
    case CODEC_ID_CLJR: return "CLJR";
    case CODEC_ID_MDEC: return "MDEC";
    case CODEC_ID_ROQ: return "Roq";
    case CODEC_ID_INTERPLAY_VIDEO: return "Interplay";
    case CODEC_ID_XAN_WC3: return "XAN_WC3";
    case CODEC_ID_XAN_WC4: return "XAN_WC4";
    case CODEC_ID_RPZA: return "RPZA";
    case CODEC_ID_CINEPAK: return "Cinepak";
    case CODEC_ID_WS_VQA: return "WS_VQA";
    case CODEC_ID_MSRLE: return "MS RLE";
    case CODEC_ID_MSVIDEO1: return "MS Video1";
    case CODEC_ID_IDCIN: return "IDCIN";
    case CODEC_ID_8BPS: return "8BPS";
    case CODEC_ID_SMC: return "SMC";
    case CODEC_ID_FLIC: return "FLIC";
    case CODEC_ID_TRUEMOTION1: return "TrueMotion1";
    case CODEC_ID_VMDVIDEO: return "VMD Video";
    case CODEC_ID_MSZH: return "MS ZH";
    case CODEC_ID_ZLIB: return "zlib";
    case CODEC_ID_QTRLE: return "QT RLE";
    case CODEC_ID_SNOW: return "Snow";
    case CODEC_ID_TSCC : return "TSCC";
    case CODEC_ID_ULTI: return "ULTI";
    case CODEC_ID_QDRAW: return "QDRAW";
    case CODEC_ID_VIXL: return "VIXL";
    case CODEC_ID_QPEG: return "QPEG";
    case CODEC_ID_XVID: return "XVID";
    case CODEC_ID_PNG: return "PNG";
    case CODEC_ID_PPM: return "PPM";
    case CODEC_ID_PBM: return "PBM";
    case CODEC_ID_PGM: return "PGM";
    case CODEC_ID_PGMYUV: return "PGM YUV";
    case CODEC_ID_PAM: return "PAM";
    case CODEC_ID_FFVHUFF: return "FFV Huff";
    case CODEC_ID_RV30: return "RV30";
    case CODEC_ID_RV40: return "RV40";
    case CODEC_ID_VC1: return "VC 1";
    case CODEC_ID_WMV3: return "WMV 3";
    case CODEC_ID_LOCO: return "LOCO";
    case CODEC_ID_WNV1: return "WNV1";
    case CODEC_ID_AASC: return "AASC";
    case CODEC_ID_INDEO2: return "Indeo 2";
    case CODEC_ID_FRAPS: return "Fraps";
    case CODEC_ID_TRUEMOTION2: return "TrueMotion 2";
    case CODEC_ID_BMP: return "BMP";
        /*
        CODEC_ID_CSCD,
        CODEC_ID_MMVIDEO,
        CODEC_ID_ZMBV,
        CODEC_ID_AVS,
        CODEC_ID_SMACKVIDEO,
        CODEC_ID_NUV,
        CODEC_ID_KMVC,
        CODEC_ID_FLASHSV,
        CODEC_ID_CAVS,
        CODEC_ID_JPEG2000,
        CODEC_ID_VMNC,
        CODEC_ID_VP5,
        CODEC_ID_VP6,
        CODEC_ID_VP6F,
        CODEC_ID_TARGA,
        CODEC_ID_DSICINVIDEO,
        CODEC_ID_TIERTEXSEQVIDEO,*/
    case CODEC_ID_TIFF: return "TIFF";
    case CODEC_ID_GIF: return "GIF";
    case CODEC_ID_FFH264: return "FF H.264";
    case CODEC_ID_DXA: return "DXA";
    case CODEC_ID_DNXHD: return "DNX HD";
    case CODEC_ID_THP: return "THP";
    case CODEC_ID_SGI: return "SGI";
    case CODEC_ID_C93: return "C93";
    case CODEC_ID_BETHSOFTVID: return "BethSoftVid";
    case CODEC_ID_PTX: return "PTX";
    case CODEC_ID_TXD: return "TXD";
    case CODEC_ID_VP6A: return "VP6A";
    case CODEC_ID_AMV: return "AMV";
    case CODEC_ID_VB: return "VB";
    case CODEC_ID_PCX: return "PCX";
    case CODEC_ID_SUNRAST: return "Sun Raster";
    case CODEC_ID_INDEO4: return "Indeo 4";
    case CODEC_ID_INDEO5: return "Indeo 5";
    case CODEC_ID_MIMIC: return "Mimic";
    case CODEC_ID_RL2: return "RL 2";
    case CODEC_ID_8SVX_EXP: return "8SVX EXP";
    case CODEC_ID_8SVX_FIB: return "8SVX FIB";
    case CODEC_ID_ESCAPE124: return "Escape 124";
    case CODEC_ID_DIRAC: return "Dirac";
    case CODEC_ID_BFI: return "BFI";
    default:
        return "Unknown";
    }
}


VideoOutput::Settings::Settings(CodecID c, int w, int h, float f, int fourcc) :
    codec(c),
    fps(f),
    width(w),
    height(h),
    bitrate(0),
    customFOURCC(fourcc) {

    // just initializes the values so the optional entries aren't used
    raw.format = PIX_FMT_NONE;
    mpeg.bframes = 0;
    mpeg.gop = 0;
}


VideoOutput::Settings VideoOutput::Settings::rawAVI(int width, int height, float fps) {
    Settings s(CODEC_ID_RAWVIDEO, width, height, fps);

    // uncompressed avi files use BGR not RGB
    s.raw.format = PIX_FMT_BGR24;

    return s;
}


VideoOutput::Settings VideoOutput::Settings::MPEG4(int width, int height, float fps, int fourCC) {
    Settings s(CODEC_ID_MPEG4, width, height, fps, fourCC);
    
    // About 3 MB / min for 640 * 480 gives decent quality at a
    // reasonable file size.
    s.bitrate = (3000000 * 8 / 60) * (width * height) / (640 * 480);

    return s;
}

VideoOutput::Ref VideoOutput::create(const std::string& filename, const Settings& settings) {
    VideoOutput* vo = new VideoOutput;

    // todo: should the exception still be thrown when we're creating a reference?
    try {
        vo->initialize(filename, settings);
    } catch (const std::string& s) {
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

    debugAssert(settings.width > 0);
    debugAssert(settings.height > 0);
    debugAssert(settings.fps > 0);

    // initialize list of available muxers/demuxers and codecs in ffmpeg
    avcodec_register_all();
    av_register_all();

    m_filename = filename;
    m_settings = settings;

    // see if ffmpeg can support this muxer and setup output format
    m_avOutputFormat = guess_format(NULL, filename.c_str(), NULL);
    throwException(m_avOutputFormat, ("Error initializing ffmpeg in guess_format."));

    // set the codec id
    m_avOutputFormat->video_codec = static_cast< ::CodecID>(m_settings.codec);

    // create format context which controls writing the file
    m_avFormatContext = av_alloc_format_context();
    throwException(m_avFormatContext, ("Error initializing ffmpeg in av_alloc_format_context."));

    // attach format to context
    m_avFormatContext->oformat = m_avOutputFormat;
    strncpy(m_avFormatContext->filename, filename.c_str(), sizeof(m_avFormatContext->filename));

    // add video stream 0
    m_avStream = av_new_stream(m_avFormatContext, 0);
    throwException(m_avStream, ("Error initializing ffmpeg in av_new_stream."));

    // setup video stream
    m_avStream->codec->codec_id     = m_avOutputFormat->video_codec;
    m_avStream->codec->codec_type   = CODEC_TYPE_VIDEO;

    // find and open required codec
    AVCodec* codec = avcodec_find_encoder(m_avStream->codec->codec_id);
    throwException(codec, 
                   format("Could not find an %s (%d) encoder on this machine.",
                          toString(static_cast<CodecID>(m_avStream->codec->codec_id)),
                          m_avStream->codec->codec_id));

    // finish setting codec parameters
    m_avStream->codec->bit_rate     = m_settings.bitrate;
    m_avStream->codec->time_base.den = iRound(m_settings.fps);
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
    throwException(avRet >= 0, ("Error initializing ffmpeg in av_set_parameters"));

    avRet = avcodec_open(m_avStream->codec, codec);
    throwException(avRet >= 0, ("Error initializing ffmpeg in avcodec_open"));

    // create encoding buffer - just allocate largest possible for now (3 channels)
    m_avEncodingBufferSize = iMax(m_settings.width * m_settings.height * 3, 512 * 1024);
    m_avEncodingBuffer = static_cast<uint8*>(av_malloc(m_avEncodingBufferSize));

    // create buffer to hold converted input frame if the codec needs a conversion
    int inputBufferSize = avpicture_get_size(m_avStream->codec->pix_fmt, m_settings.width, m_settings.height);

    m_avInputBuffer = static_cast<uint8*>(av_malloc(inputBufferSize));
    throwException(m_avInputBuffer, ("Error initializing ffmpeg in av_malloc"));

    m_avInputFrame = avcodec_alloc_frame();
    throwException(m_avInputFrame, ("Error initializing ffmpeg in avcodec_alloc_frame"));
    avpicture_fill(reinterpret_cast<AVPicture*>(m_avInputFrame), m_avInputBuffer, m_avStream->codec->pix_fmt, m_settings.width, m_settings.height);

    // open output file for writing using ffmpeg
    avRet = url_fopen(&m_avFormatContext->pb, filename.c_str(), URL_WRONLY);
    throwException(avRet >= 0, ("Error opening ffmpeg video file with url_fopen"));

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


void VideoOutput::append(RenderDevice* rd) {
    rd->screenshotPic(m_temp);
    append(m_temp);
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
    debugAssert(frame.width == m_settings.width);
    debugAssert(frame.height == m_settings.height);
    encodeAndWriteFrame(const_cast<uint8*>(frame.byte()), PIX_FMT_RGB24);
}

void VideoOutput::append(const Image1uint8::Ref& frame) {
    debugAssert(frame->width() == m_settings.width);
    debugAssert(frame->height() == m_settings.height);
    encodeAndWriteFrame(reinterpret_cast<uint8*>(frame->getCArray()), PIX_FMT_GRAY8);
}

void VideoOutput::append(const Image3uint8::Ref& frame) {
    debugAssert(frame->width() == m_settings.width);
    debugAssert(frame->height() == m_settings.height);
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


void VideoOutput::encodeAndWriteFrame(uint8* frame, PixelFormat frameFormat) {
    if (static_cast< ::PixelFormat>(frameFormat) != m_avStream->codec->pix_fmt) {
        // convert to required input format
        AVFrame* convFrame = avcodec_alloc_frame();
        throwException(convFrame, ("Unable to add frame."));

        avpicture_fill(reinterpret_cast<AVPicture*>(convFrame), 
                       reinterpret_cast<uint8_t*>(frame),
                       frameFormat, 
                       m_settings.width, 
                       m_settings.height);

        int convertRet = img_convert(reinterpret_cast<AVPicture*>(m_avInputFrame), 
                                     m_avStream->codec->pix_fmt, 
                                     reinterpret_cast<AVPicture*>(convFrame),
                                     frameFormat, 
                                     m_settings.width, 
                                     m_settings.height);

        av_free(convFrame);

        throwException(convertRet >= 0, ("Unable to add frame of this format."));

    } else {

        avpicture_fill(reinterpret_cast<AVPicture*>(m_avInputFrame), 
                       reinterpret_cast<uint8_t*>(frame), 
                       frameFormat, 
                       m_settings.width, 
                       m_settings.height);
    }

    // encode frame
    int encodeSize = avcodec_encode_video(m_avStream->codec, 
                                          m_avEncodingBuffer, 
                                          m_avEncodingBufferSize,
                                          m_avInputFrame);

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

#       ifdef _MSC_VER
            _unlink(m_filename.c_str());
#       else
            unlink(m_filename.c_str());
#       endif //_MSVC_VER
    }
}

} // namespace G3D

