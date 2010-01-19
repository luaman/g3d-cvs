/** 
    @file VideoOutput.h
 */

#ifndef G3D_VIDEOOUTPUT_H
#define G3D_VIDEOOUTPUT_H

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

/** 
 @brief Saves video to disk in a variety of popular formats, including AVI and MPEG. 

 @beta
 */
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
        CODEC_ID_MPEG4,  // Uses xvid.org's encoding algorithm for MPEG4
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
        CODEC_ID_H264,   // Uses libx264 encoding algorithm for MPEG4
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
        CODEC_ID_BFI,
        CODEC_ID_LAST
    };

    /** Constants for customFOURCC */
    enum {
        /** FFmpeg broadly compatible format for MPG4  */
        XVID_FOURCC = ('X' << 24) | ('V' << 16) | ('I' << 8) | ('D'),

        /** FFmpeg internal codec specific format for MPG4  */
        FMP4_FOURCC = ('F' << 24) | ('M' << 16) | ('P' << 8) | ('4')
    };

    class Settings {
    public:
        
        /** FFmpeg codec id */
        CodecID         codec;

        /** Frames per second the video should be encoded as */
        float           fps;

        /** Frame width */
        int             width;

        /** Frame height */
        int             height;

        /** Stream avarage bits per second if needed by the codec.*/
        int             bitrate;

        /** Custom fourcc if the automatic fourcc for a codec needs to be changed
            (eg. 'XVID' vs. 'FMP4' default)
            (0 means not set) */
        int             customFOURCC;

        struct
        {
            /** Uncompressed pixel format used with raw codec */
            const ImageFormat*  format;

            /** True if the input images must be inverted before encoding */
            bool                invert;
        } raw;

        struct
        {
            /** Max number of B-Frames if needed by the codec */
            int         bframes;

            /** GOP (Group of Pixtures) size if needed by the codec */
            int         gop;
        } mpeg;

        /** For Settings created by the static factory methods, the
            file extension (without the period) recommended for this
            kind of file. */
        std::string      extension;

        /** For Settings created by the static factory methods, the a
            brief human-readable description, suitable for use in a
            drop-down box for end users. */
        std::string      description;

        /** Defaults to MPEG-4 */
        Settings(CodecID codec = CODEC_ID_MPEG4, int width = 640, int height = 480, 
                 float fps = 30.0f, int customFourCC = 0);

        /** Settings that can be used to when writing an uncompressed
            AVI video (with BGR pixel format output). 

            This preserves full quality. It can be played on most
            computers.
        */
        static Settings rawAVI(int width, int height, float fps = 30.0f);

        /** Vendor-independent industry standard, also known as
            H.264. 

            This is the most advanced widely supported format and
            provides a good blend of quality and size.

            The default customFourCC of XVID uses the Xvid.org
            implementation, which is available for all G3D platforms.
            This is for encoding only; it has no impact on playback.
        */
        static Settings MPEG4(int width, int height, float fps = 30.0f);
        static Settings HQ_MPEG4(int width, int height, float fps = 30.0f);
        
        /** Windows Media Video 2 (WMV) format, which is supported by
            Microsoft's Media Player distributed with Windows.  This
            is the best-supported format and codec for Windows.*/
        static Settings WMV(int width, int height, float fps = 30.0f);

        /** 
            AVI file using Cinepak compression, an older but widely supported
            format for providing good compatibility and size but poor quality.

            Wikipedia describes this format as: "Cinepak is a video
            codec developed by SuperMatch, a division of SuperMac
            Technologies, and released in 1992 as part of Apple
            Computer's QuickTime video suite. It was designed to
            encode 320x240 resolution video at 1x (150 kbyte/s) CD-ROM
            transfer rates. The codec was ported to the Microsoft
            Windows platform in 1993."
            */
        static Settings CinepakAVI(int width, int height, float fps = 30.0f);
    };
protected:

    VideoOutput();

    void initialize(const std::string& filename, const Settings& settings);
    void encodeFrame(uint8* frame, const ImageFormat* format, bool invertY = false);
    void convertFrame(uint8* frame, const ImageFormat* format, bool invertY);

    Settings            m_settings;
    std::string         m_filename;

    bool                m_isInitialized;
    bool                m_isFinished;

    // ffmpeg management
    AVOutputFormat*     m_avOutputFormat;
    AVFormatContext*    m_avFormatContext;
    AVStream*           m_avStream;

    uint8*              m_avInputBuffer;
    AVFrame*            m_avInputFrame;

    uint8*              m_avEncodingBuffer;
    int                 m_avEncodingBufferSize;

    /** Used by append(RenderDevice) to hold the read-back frame.*/
    GImage              m_temp;

    /** Used by convertImage() to hold temp frame. */
    Array<uint8>        m_tempBuffer;

public:

    typedef ReferenceCountedPointer<VideoOutput> Ref;

    /**
       Video files have a file format and a codec.  VideoOutput
       chooses the file format based on the filename's extension
       (e.g., .avi creates an AVI file) and the codec based on Settings::codec
     */
    static Ref create(const std::string& filename, const Settings& settings);

    /** Tests each codec for whether it is supported on this operating system. */
    static void getSupportedCodecs(Array<CodecID>& c);
    static void getSupportedCodecs(Array<std::string>& c);

    /** Returns true if this operating system/G3D build supports this codec. */
    static bool supports(CodecID c);

    /** Returns a human readable name for the codec. */
    static const char* toString(CodecID c);

    ~VideoOutput();

    const std::string& filename() const { return m_filename; }

    void append(const Texture::Ref& frame); 

    void append(const GImage& frame); 

    /** @brief Append the current frame on the RenderDevice to this
        video.  


        @param useBackBuffer If true, read from the back
        buffer (the current frame) instead of the front buffer.
     */
    void append(class RenderDevice* rd, bool useBackBuffer = false); 

    /** Reserved for future use */
    void append(const Image1uint8::Ref& frame); 

    void append(const Image3uint8::Ref& frame); 

    /** Reserved for future use */
    void append(const Image4uint8::Ref& frame); 

    /** Reserved for future use */
    void append(const Image1::Ref& frame);

    void append(const Image3::Ref& frame);

    /** Reserved for future use */ 
    void append(const Image4::Ref& frame); 

    /** Aborts writing video file and ends encoding */
    void abort();

    /** Finishes writing video file and ends encoding */
    void commit();

    bool finished()       { return m_isFinished; }

};

} // namespace G3D

#endif // G3D_VIDEOOUTPUT_H
