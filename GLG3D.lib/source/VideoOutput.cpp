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

// conversion routine helper
static PixelFormat ConvertImageFormatToPixelFormat(const ImageFormat* format);

VideoOutput::Settings::Settings(CodecID c, int w, int h, float f, int fourcc) :
    codec(c),
    fps(f),
    width(w),
    height(h),
    bitrate(0),
    customFOURCC(fourcc) {

    // just initializes the values so the optional entries aren't used
    raw.format = NULL;
    raw.invert = false;
    mpeg.bframes = 0;
    mpeg.gop = 12;  // The default
}


VideoOutput::Settings VideoOutput::Settings::rawAVI(int width, int height, float fps) {
    Settings s(CODEC_ID_RAWVIDEO, width, height, fps);

    // uncompressed avi files use BGR not RGB
    s.raw.format = ImageFormat::BGR8();
    s.raw.invert = true;
    s.extension   = "avi";
    s.description = "Uncompressed AVI (.avi)";

    return s;
}


VideoOutput::Settings VideoOutput::Settings::WMV(int width, int height, float fps) {
    Settings s(CODEC_ID_WMV2, width, height, fps);

    s.extension   = "wmv";
    s.description = "Windows Media Video 2 (.wmv)";
    s.bitrate = iRound(3000000.0 * ((double)s.width * s.height) / (640 * 480));

    return s;
}


VideoOutput::Settings VideoOutput::Settings::CinepakAVI(int width, int height, float fps) {
    Settings s(CODEC_ID_CINEPAK, width, height, fps);

    s.extension   = "avi";
    s.description = "Cinepak AVI (.avi)";
    s.bitrate = iRound(2000000.0 * ((double)s.width * s.height) / (640 * 480));

    return s;
}


VideoOutput::Settings VideoOutput::Settings::MPEG4(int width, int height, float fps) {
    Settings s(CODEC_ID_MPEG4, width, height, fps, XVID_FOURCC);
    
    // About 1500 kb/s for 640 * 480 gives high quality at a
    // reasonable file size. 
    s.bitrate = iRound(1500000.0 * ((double)s.width * s.height) / (640 * 480));

    s.extension   = "mp4";
    s.description = "MPEG-4/H.264 (.mp4)";

    return s;
}


VideoOutput::Ref VideoOutput::create(const std::string& filename, const Settings& settings) {
    VideoOutput* vo = new VideoOutput;

    try {
        vo->initialize(filename, settings);
    } catch (const std::string& s) {
        (void)s;
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
    av_register_all();

    m_filename = filename;
    m_settings = settings;

    // see if ffmpeg can support this muxer and setup output format
    m_avOutputFormat = guess_format(NULL, filename.c_str(), NULL);
    throwException(m_avOutputFormat, ("Error initializing FFmpeg in guess_format."));

    // set the codec id
    m_avOutputFormat->video_codec = static_cast< ::CodecID>(m_settings.codec);

    // create format context which controls writing the file
    m_avFormatContext = av_alloc_format_context();
    throwException(m_avFormatContext, ("Error initializing FFmpeg in av_alloc_format_context."));

    // attach format to context
    m_avFormatContext->oformat = m_avOutputFormat;
    strncpy(m_avFormatContext->filename, filename.c_str(), sizeof(m_avFormatContext->filename));

    // add video stream 0
    m_avStream = av_new_stream(m_avFormatContext, 0);
    throwException(m_avStream, ("Error initializing FFmpeg in av_new_stream."));

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
    m_avStream->codec->time_base.den = iRound(m_settings.fps * 100.0f);
    m_avStream->codec->time_base.num = 100;
    m_avStream->codec->width        = m_settings.width;
    m_avStream->codec->height       = m_settings.height;

    // set codec input format
    if (m_settings.codec == CODEC_ID_RAWVIDEO) {
        m_avStream->codec->pix_fmt = ConvertImageFormatToPixelFormat(m_settings.raw.format);
        throwException(m_avStream->codec->pix_fmt != PIX_FMT_NONE, ("Error initializing FFmpeg setting raw video input format."));
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

    // TODO: What is this?
    m_avStream->quality = 100.0f;

    // set null parameters as this call is required
    int avRet = av_set_parameters(m_avFormatContext, NULL);
    throwException(avRet >= 0, ("Error initializing FFmpeg in av_set_parameters"));

    avRet = avcodec_open(m_avStream->codec, codec);
    throwException(avRet >= 0, ("Error initializing FFmpeg in avcodec_open"));

    // create encoding buffer - just allocate largest possible for now (3 channels)
    m_avEncodingBufferSize = iMax(m_settings.width * m_settings.height * 3, 512 * 1024);
    m_avEncodingBuffer = static_cast<uint8*>(av_malloc(m_avEncodingBufferSize));

    // create buffer to hold converted input frame if the codec needs a conversion
    int inputBufferSize = avpicture_get_size(m_avStream->codec->pix_fmt, m_settings.width, m_settings.height);

    m_avInputBuffer = static_cast<uint8*>(av_malloc(inputBufferSize));
    throwException(m_avInputBuffer, ("Error initializing FFmpeg in av_malloc"));

    m_avInputFrame = avcodec_alloc_frame();
    throwException(m_avInputFrame, ("Error initializing FFmpeg in avcodec_alloc_frame"));
    avpicture_fill(reinterpret_cast<AVPicture*>(m_avInputFrame), m_avInputBuffer, m_avStream->codec->pix_fmt, m_settings.width, m_settings.height);

    // open output file for writing using ffmpeg
    avRet = url_fopen(&m_avFormatContext->pb, filename.c_str(), URL_WRONLY);
    throwException(avRet >= 0, ("Error opening FFmpeg video file with url_fopen"));

    // start the stream
    avRet = av_write_header(m_avFormatContext);

    // make sure the file is closed on error
    if (avRet < 0) {
        // abort closes and removes the output file
        abort();
        throwException(false, ("Error initializing and writing FFmpeg video file."));
    }

    m_isInitialized = true;
}


void VideoOutput::append(RenderDevice* rd, bool backbuffer) {
    debugAssert(rd->width() == m_settings.width);
    debugAssert(rd->height() == m_settings.height);

    RenderDevice::ReadBuffer old = rd->readBuffer();
    if (backbuffer) {
        rd->setReadBuffer(RenderDevice::READ_BACK);
    } else {
        rd->setReadBuffer(RenderDevice::READ_FRONT);
    }
    rd->screenshotPic(m_temp, false, false);
    rd->setReadBuffer(old);
    encodeFrame(const_cast<uint8*>(m_temp.byte()), ImageFormat::RGB8(), true);
}


void VideoOutput::append(const Texture::Ref& frame) {
    debugAssert(frame->width() == m_settings.width);
    debugAssert(frame->height() == m_settings.height);

    frame->getImage(m_temp, TextureFormat::RGB8());
    encodeFrame(const_cast<uint8*>(m_temp.byte()), ImageFormat::RGB8(), frame->invertY);
}


void VideoOutput::append(const GImage& frame) {
    debugAssert(frame.width() == m_settings.width);
    debugAssert(frame.height() == m_settings.height);
    encodeFrame(const_cast<uint8*>(frame.byte()), (frame.channels() == 3) ? ImageFormat::RGB8() : ImageFormat::RGBA8());
}


void VideoOutput::append(const Image1uint8::Ref& frame) {
    debugAssert(frame->width() == m_settings.width);
    debugAssert(frame->height() == m_settings.height);
    encodeFrame(reinterpret_cast<uint8*>(frame->getCArray()), ImageFormat::L8());
}


void VideoOutput::append(const Image3uint8::Ref& frame) {
    debugAssert(frame->width() == m_settings.width);
    debugAssert(frame->height() == m_settings.height);
    encodeFrame(reinterpret_cast<uint8*>(frame->getCArray()), ImageFormat::RGB8());
}

void VideoOutput::append(const Image4uint8::Ref& frame) {
    debugAssert(frame->width() == m_settings.width);
    debugAssert(frame->height() == m_settings.height);
    encodeFrame(reinterpret_cast<uint8*>(frame->getCArray()), ImageFormat::RGBA8());
}

void VideoOutput::append(const Image1::Ref& frame) {
    debugAssert(frame->width() == m_settings.width);
    debugAssert(frame->height() == m_settings.height);
    encodeFrame(reinterpret_cast<uint8*>(frame->getCArray()), ImageFormat::L32F());
}

void VideoOutput::append(const Image3::Ref& frame) {
    debugAssert(frame->width() == m_settings.width);
    debugAssert(frame->height() == m_settings.height);
    encodeFrame(reinterpret_cast<uint8*>(frame->getCArray()), ImageFormat::RGB32F());
}


void VideoOutput::append(const Image4::Ref& frame) {
    debugAssert(frame->width() == m_settings.width);
    debugAssert(frame->height() == m_settings.height);
    encodeFrame(reinterpret_cast<uint8*>(frame->getCArray()), ImageFormat::RGBA32F());
}


void VideoOutput::encodeFrame(uint8* frame, const ImageFormat* format, bool invertY) {
    alwaysAssertM(m_isInitialized, "VideoOutput was not initialized before call to encodeAndWriteFrame.");
    alwaysAssertM(! m_isFinished, "Cannot call VideoOutput::append() after commit() or abort().");

    convertFrame(frame, format, invertY);

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

// this routine does not support any planar input formats
void VideoOutput::convertFrame(uint8* frame, const ImageFormat* format, bool invertY) {

    // invert the frame if required or not already inverted
    bool invertRequired = (m_settings.codec == CODEC_ID_RAWVIDEO) && (m_settings.raw.invert != invertY);
    invertRequired      = invertRequired || ((m_settings.codec != CODEC_ID_RAWVIDEO) && invertY);

    // go through an intermediate conversion if required
    PixelFormat matchingPixelFormat = ConvertImageFormatToPixelFormat(format);
    bool intermediateRequired       = (matchingPixelFormat == PIX_FMT_NONE);

    if (invertRequired && !intermediateRequired) {

        throwException(format->colorSpace == ImageFormat::COLOR_SPACE_RGB, "Flipping is not implemented for this pixel format.");

        // borrowed for GImage and expanded to work for any rgb colorspace format with no padding
        uint8* temp = (uint8*)System::malloc(m_settings.width * format->cpuBitsPerPixel / 8);
        throwException(temp != NULL, "Out of memory."); 

        int oneRow = m_settings.width * format->cpuBitsPerPixel / 8;
        int N = m_settings.height / 2;

        // if height is an odd value, don't swap odd middle row
        for (int i = 0; i < N; ++i) {
            int topOff = i * oneRow;
            int botOff = (m_settings.height - i - 1) * oneRow;
            System::memcpy(temp,           frame + topOff, oneRow);
            System::memcpy(frame + topOff, frame + botOff, oneRow);
            System::memcpy(frame + botOff, temp,           oneRow);
        }
        System::free(temp);

    } else if (intermediateRequired) {

        // make sure the temporary buffer is big enough
        m_tempBuffer.resize(m_settings.width * m_settings.height * ImageFormat::RGB8()->cpuBitsPerPixel, false);

        Array<const void*> inputBuffers;
        inputBuffers.append(frame);

        Array<void*> outputBuffers;
        outputBuffers.append(m_tempBuffer.getCArray());

        bool converted = ImageFormat::convert(inputBuffers, 
                                              m_settings.width, 
                                              m_settings.height, 
                                              format, 0, 
                                              outputBuffers, 
                                              ImageFormat::RGB8(), 0, 
                                              invertRequired);

        throwException(converted, "Unable to add frame due to unsupported conversion of formats.");

        // set the output buffer as "frame" for input to final format
        frame = m_tempBuffer.getCArray();
        matchingPixelFormat = PIX_FMT_RGB24;

    }
    
    if (matchingPixelFormat != m_avStream->codec->pix_fmt) {
        // finally convert to the format the encoder expects
        AVFrame* convFrame = avcodec_alloc_frame();
        throwException(convFrame, "Unable to append frame because in "
                                  "VideoOutput, avcodec_alloc_frame returned NULL");

        avpicture_fill(reinterpret_cast<AVPicture*>(convFrame), 
                       frame,
                       matchingPixelFormat,
                       m_settings.width, 
                       m_settings.height);

        int convertRet = img_convert(reinterpret_cast<AVPicture*>(m_avInputFrame), 
                                     m_avStream->codec->pix_fmt, 
                                     reinterpret_cast<AVPicture*>(convFrame),
                                     matchingPixelFormat, 
                                     m_settings.width, 
                                     m_settings.height);

        av_free(convFrame);
        throwException(convertRet >= 0, ("Unable to add frame of this format."));

    } else {
        // otherwise just setup the input frame without conversion
        avpicture_fill(reinterpret_cast<AVPicture*>(m_avInputFrame), 
                       frame,
                       matchingPixelFormat,
                       m_settings.width, 
                       m_settings.height);
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

// conversion routine helper
static PixelFormat ConvertImageFormatToPixelFormat(const ImageFormat* format) {
    switch (format->code) {
        // return valid types that match original
        case ImageFormat::CODE_RGB8:
            return PIX_FMT_RGB24;
        case ImageFormat::CODE_RGBA8:
            return PIX_FMT_RGB32_1;
        case ImageFormat::CODE_BGR8:
            return PIX_FMT_BGR24;

        case ImageFormat::CODE_YUV420_PLANAR:
            return PIX_FMT_YUV420P;

        case ImageFormat::CODE_L8:
        case ImageFormat::CODE_A8:
            return PIX_FMT_GRAY8;
        default:
            return PIX_FMT_NONE;
    }
}

} // namespace G3D

