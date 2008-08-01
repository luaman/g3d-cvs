/** 
 */

#ifndef G3D_VIDEOOUTPUT_H
#define G3D_VIDEOOUTPUT_H

// includes
#include <string>
#include "G3D/g3dmath.h"
#include "G3D/GImage.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Texture.h"

// forward declarations for ffmpeg
struct AVOutputFormat;
struct AVFormatContext;
struct AVStream;
struct AVFrame;

namespace G3D {

/** video output */
class VideoOutput : public ReferenceCountedObject {

public:
    enum CodecID {
        CODEC_ID_NONE,

        CODEC_ID_MPEG1VIDEO,
        CODEC_ID_MPEG2VIDEO, ///< preferred ID for MPEG-1/2 video decoding
        CODEC_ID_MPEG2VIDEO_XVMC,
        CODEC_ID_H261,
        CODEC_ID_H263,
        CODEC_ID_RV10,
        CODEC_ID_RV20,
        CODEC_ID_MJPEG,
        CODEC_ID_MJPEGB,
        CODEC_ID_LJPEG,
        CODEC_ID_SP5X,
        CODEC_ID_JPEGLS,
        CODEC_ID_MPEG4,
        CODEC_ID_RAWVIDEO,
        CODEC_ID_MSMPEG4V1,
        CODEC_ID_MSMPEG4V2,
        CODEC_ID_MSMPEG4V3,
        CODEC_ID_WMV1,
        CODEC_ID_WMV2,
        CODEC_ID_H263P,
        CODEC_ID_H263I,
        CODEC_ID_FLV1,
        CODEC_ID_SVQ1,
        CODEC_ID_SVQ3,
        CODEC_ID_DVVIDEO,
        CODEC_ID_HUFFYUV,
        CODEC_ID_CYUV,
        CODEC_ID_H264,
        CODEC_ID_INDEO3,
        CODEC_ID_VP3,
        CODEC_ID_THEORA,
        CODEC_ID_ASV1,
        CODEC_ID_ASV2,
        CODEC_ID_FFV1,
        CODEC_ID_4XM,
        CODEC_ID_VCR1,
        CODEC_ID_CLJR,
        CODEC_ID_MDEC,
        CODEC_ID_ROQ,
        CODEC_ID_INTERPLAY_VIDEO,
        CODEC_ID_XAN_WC3,
        CODEC_ID_XAN_WC4,
        CODEC_ID_RPZA,
        CODEC_ID_CINEPAK,
        CODEC_ID_WS_VQA,
        CODEC_ID_MSRLE,
        CODEC_ID_MSVIDEO1,
        CODEC_ID_IDCIN,
        CODEC_ID_8BPS,
        CODEC_ID_SMC,
        CODEC_ID_FLIC,
        CODEC_ID_TRUEMOTION1,
        CODEC_ID_VMDVIDEO,
        CODEC_ID_MSZH,
        CODEC_ID_ZLIB,
        CODEC_ID_QTRLE,
        CODEC_ID_SNOW,
        CODEC_ID_TSCC,
        CODEC_ID_ULTI,
        CODEC_ID_QDRAW,
        CODEC_ID_VIXL,
        CODEC_ID_QPEG,
        CODEC_ID_XVID,
        CODEC_ID_PNG,
        CODEC_ID_PPM,
        CODEC_ID_PBM,
        CODEC_ID_PGM,
        CODEC_ID_PGMYUV,
        CODEC_ID_PAM,
        CODEC_ID_FFVHUFF,
        CODEC_ID_RV30,
        CODEC_ID_RV40,
        CODEC_ID_VC1,
        CODEC_ID_WMV3,
        CODEC_ID_LOCO,
        CODEC_ID_WNV1,
        CODEC_ID_AASC,
        CODEC_ID_INDEO2,
        CODEC_ID_FRAPS,
        CODEC_ID_TRUEMOTION2,
        CODEC_ID_BMP,
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
        CODEC_ID_TIERTEXSEQVIDEO,
        CODEC_ID_TIFF,
        CODEC_ID_GIF,
        CODEC_ID_FFH264,
        CODEC_ID_DXA,
        CODEC_ID_DNXHD,
        CODEC_ID_THP,
        CODEC_ID_SGI,
        CODEC_ID_C93,
        CODEC_ID_BETHSOFTVID,
        CODEC_ID_PTX,
        CODEC_ID_TXD,
        CODEC_ID_VP6A,
        CODEC_ID_AMV,
        CODEC_ID_VB,
        CODEC_ID_PCX,
        CODEC_ID_SUNRAST,
        CODEC_ID_INDEO4,
        CODEC_ID_INDEO5,
        CODEC_ID_MIMIC,
        CODEC_ID_RL2,
        CODEC_ID_8SVX_EXP,
        CODEC_ID_8SVX_FIB,
        CODEC_ID_ESCAPE124,
        CODEC_ID_DIRAC,
        CODEC_ID_BFI
    };

    enum PixelFormat {
        PIX_FMT_NONE= -1,
        PIX_FMT_YUV420P,   ///< Planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
        PIX_FMT_YUYV422,   ///< Packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
        PIX_FMT_RGB24,     ///< Packed RGB 8:8:8, 24bpp, RGBRGB...
        PIX_FMT_BGR24,     ///< Packed RGB 8:8:8, 24bpp, BGRBGR...
        PIX_FMT_YUV422P,   ///< Planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
        PIX_FMT_YUV444P,   ///< Planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
        PIX_FMT_RGB32,     ///< Packed RGB 8:8:8, 32bpp, (msb)8A 8R 8G 8B(lsb), in cpu endianness
        PIX_FMT_YUV410P,   ///< Planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
        PIX_FMT_YUV411P,   ///< Planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
        PIX_FMT_RGB565,    ///< Packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), in cpu endianness
        PIX_FMT_RGB555,    ///< Packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), in cpu endianness most significant bit to 0
        PIX_FMT_GRAY8,     ///<        Y        ,  8bpp
        PIX_FMT_MONOWHITE, ///<        Y        ,  1bpp, 0 is white, 1 is black
        PIX_FMT_MONOBLACK, ///<        Y        ,  1bpp, 0 is black, 1 is white
        PIX_FMT_PAL8,      ///< 8 bit with PIX_FMT_RGB32 palette
        PIX_FMT_YUVJ420P,  ///< Planar YUV 4:2:0, 12bpp, full scale (jpeg)
        PIX_FMT_YUVJ422P,  ///< Planar YUV 4:2:2, 16bpp, full scale (jpeg)
        PIX_FMT_YUVJ444P,  ///< Planar YUV 4:4:4, 24bpp, full scale (jpeg)
        PIX_FMT_XVMC_MPEG2_MC,///< XVideo Motion Acceleration via common packet passing(xvmc_render.h)
        PIX_FMT_XVMC_MPEG2_IDCT,
        PIX_FMT_UYVY422,   ///< Packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
        PIX_FMT_UYYVYY411, ///< Packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
        PIX_FMT_BGR32,     ///< Packed RGB 8:8:8, 32bpp, (msb)8A 8B 8G 8R(lsb), in cpu endianness
        PIX_FMT_BGR565,    ///< Packed RGB 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), in cpu endianness
        PIX_FMT_BGR555,    ///< Packed RGB 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), in cpu endianness most significant bit to 1
        PIX_FMT_BGR8,      ///< Packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
        PIX_FMT_BGR4,      ///< Packed RGB 1:2:1,  4bpp, (msb)1B 2G 1R(lsb)
        PIX_FMT_BGR4_BYTE, ///< Packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
        PIX_FMT_RGB8,      ///< Packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
        PIX_FMT_RGB4,      ///< Packed RGB 1:2:1,  4bpp, (msb)2R 3G 3B(lsb)
        PIX_FMT_RGB4_BYTE, ///< Packed RGB 1:2:1,  8bpp, (msb)2R 3G 3B(lsb)
        PIX_FMT_NV12,      ///< Planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 for UV
        PIX_FMT_NV21,      ///< as above, but U and V bytes are swapped

        PIX_FMT_RGB32_1,   ///< Packed RGB 8:8:8, 32bpp, (msb)8R 8G 8B 8A(lsb), in cpu endianness
        PIX_FMT_BGR32_1,   ///< Packed RGB 8:8:8, 32bpp, (msb)8B 8G 8R 8A(lsb), in cpu endianness

        PIX_FMT_GRAY16BE,  ///<        Y        , 16bpp, big-endian
        PIX_FMT_GRAY16LE,  ///<        Y        , 16bpp, little-endian
        PIX_FMT_YUV440P,   ///< Planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
        PIX_FMT_YUVJ440P,  ///< Planar YUV 4:4:0 full scale (jpeg)
        PIX_FMT_YUVA420P,  ///< Planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
        PIX_FMT_NB,        ///< number of pixel formats, DO NOT USE THIS if you want to link with shared libav* because the number of formats might differ between versions
    };

    struct Settings
    {
    public:
        Settings();

        /** ffmpeg codec id */
        CodecID         codec;

        /** Frames per second the video should be encoded as */
        int             fps;

        /** Frame width */
        int             width;

        /** Frame height */
        int             height;

        /** Stream avarage bitrate if needed */
        int             bitrate;

        /** Custom fourcc if the automatic fourcc for a codec needs to be changed
            (eg. 'XVID' vs. 'FMP4' default) */
        int             customFOURCC;

        struct
        {
            /** Uncompressed pixel format used with raw codec */
            PixelFormat format;
        } raw;

        struct
        {
            /** B-Frames if needed (0 means unused) */
            int         bframes;

            /** GOP (Group of Pixtures) size if needed (0 means unused) */
            int         gop;
        } mpeg;

        /** Settings that can be used to when writing an uncompressed avi video (with BGR pixel format output) */
        static Settings uncompressedAVI();

        /** Settings that can be used when writing an mpeg4 avi video using ffmpeg's native implementation */
        static Settings nativeMPEG4();
    };

    typedef ReferenceCountedPointer<VideoOutput> Ref;
    
    static Ref create(const std::string& filename, const Settings& settings);

    ~VideoOutput();

    void append(const Texture::Ref& frame); 
    void append(const GImage& frame); 
    void append(const Image1uint8::Ref& frame); 
    void append(const Image3uint8::Ref& frame); 
    void append(const Image4uint8::Ref& frame); 
    void append(const Image1::Ref& frame); 
    void append(const Image3::Ref& frame); 
    void append(const Image4::Ref& frame); 
    void append(uint8* frame, PixelFormat frameFormat); 

    /** Aborts writing video file and ends encoding */
    void abort();

    /** Finishes writing video file and ends encoding */
    void commit();

    bool finished()       { return m_isFinished; }

private:
    VideoOutput();

    void initialize(const std::string& filename, const Settings& settings);
    void encodeAndWriteFrame(uint8* frame, PixelFormat frameFormat);

    Settings    m_settings;
    std::string m_filename;

    bool        m_isInitialized;
    bool        m_isFinished;

    // ffmpeg management
    AVOutputFormat*     m_avOutputFormat;
    AVFormatContext*    m_avFormatContext;
    AVStream*           m_avStream;

    uint8*              m_avInputBuffer;
    AVFrame*            m_avInputFrame;

    uint8*              m_avEncodingBuffer;
    int                 m_avEncodingBufferSize;
};

} // namespace G3D

#endif // G3D_VIDEOOUTPUT_H