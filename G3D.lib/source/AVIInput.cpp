/**

*/

/* includes */
#include <limits>

#include "G3D/AVIInput.h"
#include "G3D/fileutils.h"

namespace G3D {

#define G3D_FOURCC(a, b, c, d) ((d << 24) | (c << 16) | (b << 8) | a)

const uint32 FOURCC_RIFF = G3D_FOURCC('R','I','F','F');
const uint32 FOURCC_AVI  = G3D_FOURCC('A','V','I',' ');
const uint32 FOURCC_LIST = G3D_FOURCC('L','I','S','T');
const uint32 FOURCC_HDRL = G3D_FOURCC('h','d','r','l');
const uint32 FOURCC_AVIH = G3D_FOURCC('a','v','i','h');
const uint32 FOURCC_STRL = G3D_FOURCC('s','t','r','l');
const uint32 FOURCC_STRH = G3D_FOURCC('s','t','r','h');
const uint32 FOURCC_STRF = G3D_FOURCC('s','t','r','f');
const uint32 FOURCC_MOVI = G3D_FOURCC('m','o','v','i');
const uint32 FOURCC_REC  = G3D_FOURCC('r','e','c',' ');

const uint32 FOURCC_JUNK = G3D_FOURCC('J','U','N','K');

const uint32 FOURCC_AUDS = G3D_FOURCC('a','u','d','s');
const uint32 FOURCC_VIDS = G3D_FOURCC('v','i','d','s');

const uint32 FOURCC_INVALID = 0;

/* implementation */

AVIInput::AVIInput() :
    m_input(NULL),
    m_fourccVideoStream(FOURCC_INVALID),
    m_fourccAudioStream(FOURCC_INVALID),
    m_currentFrameTime(0.0f) {

    m_videoFrame.streamType = STREAM_VIDEO;
    m_videoFrame.frameData = NULL;
    m_videoFrame.frameSize = 0;

    m_audioFrame.streamType = STREAM_AUDIO;
    m_audioFrame.frameData = NULL;
    m_audioFrame.frameSize = 0;
    
    m_currentList.fourcc = FOURCC_INVALID;
    m_currentChunk.fourcc = FOURCC_INVALID;
}

AVIInput::~AVIInput() {
    if (m_input) {
        delete m_input;
    }

    delete reinterpret_cast<uint8*>(m_videoFrame.frameData);
    delete reinterpret_cast<uint8*>(m_audioFrame.frameData);
}

AVIInputRef AVIInput::fromFile(const std::string& filename) {
    debugAssert(fileExists(filename));

    AVIInputRef ai = new AVIInput();
    AVIInput& aviInput = (*ai);

    aviInput.m_input = new BinaryInput(filename, G3D_LITTLE_ENDIAN, false);

    // currently AVI files support 32-bit chunks only
    // there will be 1 list containing all of the data chunks
    debugAssert(aviInput.m_input->getLength() <= std::numeric_limits<uint32>::max());

    if ((aviInput.m_input->getLength() == 0) || (aviInput.m_input->getLength() > std::numeric_limits<uint32>::max()))
    {
        // couldn't load file
        // set invalid file flag and let the assert above provide more info
        aviInput.m_aviInfo.invalidFile = true;
    }
    else
    {
        aviInput.readHeaders();
    }

    if (aviInput.m_aviInfo.invalidFile) {
        return NULL;
    } else {
        return ai;
    }
}

bool AVIInput::isFrameAvailable(double realTimeStep) {
    // update time-in-frame and check if a new frame
    // should be available based on frame rate

    bool frameAvailable = false;
    if (m_currentFrameTime == 0) {
        frameAvailable = true;
    }

    m_currentFrameTime += realTimeStep;

    if ( !m_aviInfo.completed && !m_aviInfo.invalidFile &&
         (m_currentFrameTime >= (1.0f / m_aviInfo.frameRate)) ) {
        frameAvailable = true;
    }

    return frameAvailable;
}

void AVIInput::ignoreStream(STREAM_TYPE streamType) {
    debugAssertM(false, "ignoreStream() not supported");
    switch (streamType) {
    case STREAM_VIDEO:
        m_aviInfo.ignoringVideo = true;
        break;

    case STREAM_AUDIO:
        m_aviInfo.ignoringAudio = true;
        break;

    default:
        debugAssertM(false, "Attempting to ignore invalid AVI stream");
        break;
    }
}

AVIInput::FrameInfo AVIInput::nextFrame() {
    // this only returns video frames right now
    // any audio chunks are read in and thrown away
    m_aviInfo.currentFrame += 1;
    m_currentFrameTime = 0.0f;

    readVideoFrame();
    return m_videoFrame;
}

void AVIInput::readHeaders() {
    struct MainHeader {
        uint32 microsecondsPerFrame;
        uint32 maxBytesPerSecond;
        uint32 paddingGranularity;
        uint32 flags;
        uint32 numFrames;
        uint32 interleaveInitFrames;
        uint32 numStreams;
        uint32 suggestedBufferSize;
        uint32 videoFrameWidth;
        uint32 videoFrameHeight;
        uint32 reserved[4];
    };

    struct StreamHeader {
         uint32 fourccStream;
         uint32 fourccHandler;
         uint32 flags;
         uint16 priority;
         uint16 language;
         uint32 initialFrames;
         uint32 scale;
         uint32 rate;
         uint32 start;
         uint32 length;
         uint32 suggestedBufferSize;
         uint32 quality;
         uint32 sampleSize;
         int16  frameLeft;
         int16  frameTop;
         int16  frameRight;
         int16  frameBottom;
     };

    struct BitmapInfoHeader { 
        uint32 size; 
        int32  width;
        int32  height; 
        uint16 numPlanes; 
        uint16 numBitsPerPixel; 
        uint32 compression; 
        uint32 imageSize; 
        int32  xresPixelsPerMeter; 
        int32  yresPixelsPerMeter; 
        uint32 numColorIndicesUsed; 
        uint32 numColorIndicesRequired; 
    }; 

    struct WaveFormatEx { 
        uint16 formatTag; 
        uint16 numChannels; 
        uint32 samplesPerSecond; 
        uint32 avgBytesPerSecond;; 
        uint16 blockAlign; 
        uint16 bitsPerSample; 
        uint16 extraSizeAfterHeader; 
    };

    MainHeader mainHeader;

    // validate RIFF chunk
    if (m_input->readUInt32() != FOURCC_RIFF) {
        m_aviInfo.invalidFile = true;
    }
    else {
        // size in RIFF doesn't contain fourcc and size field
        uint32 totalSize = m_input->readUInt32() + 8;
        uint32 fileType = m_input->readUInt32();

        if ((static_cast<int64>(totalSize) > m_input->size()) ||
            (fileType != FOURCC_AVI)) {
            m_aviInfo.invalidFile = true;
        }
    }

    if (!m_aviInfo.invalidFile) {
        // start headers LIST
        startNextChunk();

        debugAssert(m_currentList.fourcc == FOURCC_HDRL);
        debugAssert(m_currentChunk.fourcc == FOURCC_AVIH);

        // read in avih chunk
        mainHeader.microsecondsPerFrame = m_input->readUInt32();
        mainHeader.maxBytesPerSecond = m_input->readUInt32();
        mainHeader.paddingGranularity = m_input->readUInt32();
        mainHeader.flags = m_input->readUInt32();
        mainHeader.numFrames = m_input->readUInt32();
        mainHeader.interleaveInitFrames = m_input->readUInt32();
        mainHeader.numStreams = m_input->readUInt32();
        mainHeader.suggestedBufferSize = m_input->readUInt32();
        mainHeader.videoFrameWidth = m_input->readUInt32();
        mainHeader.videoFrameHeight = m_input->readUInt32();
        m_input->readBytes(mainHeader.reserved, sizeof(mainHeader.reserved));

        finishChunk();

        // read in all strl lists and find audio and video streams
        startNextChunk();

        debugAssert(m_currentList.fourcc == FOURCC_STRL);
        debugAssert(m_currentChunk.fourcc == FOURCC_STRH);

        StreamHeader        streamHeader;
        BitmapInfoHeader    bitmapHeader;
        //WaveFormatEx        waveFormat;

        uint32 streamIdx = '0';

        do {
            // read in stream header chunk
            streamHeader.fourccStream = m_input->readUInt32();
            streamHeader.fourccHandler = m_input->readUInt32();
            streamHeader.flags = m_input->readUInt32();
            streamHeader.priority = m_input->readUInt16();
            streamHeader.language = m_input->readUInt16();
            streamHeader.initialFrames = m_input->readUInt32();
            streamHeader.scale = m_input->readUInt32();
            streamHeader.rate = m_input->readUInt32();
            streamHeader.start = m_input->readUInt32();
            streamHeader.length = m_input->readUInt32();
            streamHeader.suggestedBufferSize = m_input->readUInt32();
            streamHeader.quality = m_input->readUInt32();
            streamHeader.sampleSize = m_input->readUInt32();
            streamHeader.frameLeft = m_input->readInt16();
            streamHeader.frameTop = m_input->readInt16();
            streamHeader.frameRight = m_input->readInt16();
            streamHeader.frameBottom = m_input->readInt16();

            finishChunk();
            startNextChunk();

            debugAssert(m_currentChunk.fourcc == FOURCC_STRF);

            char fourccname[5] = "";
            System::memcpy(fourccname, &streamHeader.fourccStream, 4);

            // read in stream format chunk
            if (!m_aviInfo.hasVideoStream && (streamHeader.fourccStream == FOURCC_VIDS)) {
                m_aviInfo.hasVideoStream = true;
                // create 0Xdb fourcc where X is stream #
                m_fourccVideoStream = 'd' << 24 | 'b' << 16;
                m_fourccVideoStream |= streamIdx << 8 | '0';

                m_aviInfo.frameRate = static_cast<float>(streamHeader.rate) / static_cast<float>(streamHeader.scale);
                m_aviInfo.numFrames = streamHeader.length;

                bitmapHeader.size = m_input->readUInt32();
                bitmapHeader.width = m_input->readInt32();
                bitmapHeader.height = m_input->readInt32();
                bitmapHeader.numPlanes = m_input->readUInt16();
                bitmapHeader.numBitsPerPixel = m_input->readUInt16();
                bitmapHeader.compression = m_input->readUInt32();
                bitmapHeader.imageSize = m_input->readUInt32();
                bitmapHeader.xresPixelsPerMeter = m_input->readInt32();
                bitmapHeader.yresPixelsPerMeter = m_input->readInt32();
                bitmapHeader.numColorIndicesUsed = m_input->readUInt32();
                bitmapHeader.numColorIndicesRequired = m_input->readUInt32();

                if ((bitmapHeader.compression != 0) || (bitmapHeader.numBitsPerPixel != 24)) {
                    m_aviInfo.invalidFile = true;
                } else {
                    m_videoFrame.frameData = new uint8[streamHeader.suggestedBufferSize];
                    m_videoFrame.frameSize = streamHeader.suggestedBufferSize;

                    m_aviInfo.width = (streamHeader.frameRight - streamHeader.frameLeft);
                    m_aviInfo.height = (streamHeader.frameBottom - streamHeader.frameTop);
                }

            } else if (! m_aviInfo.hasAudioStream && (streamHeader.fourccStream == FOURCC_AUDS)) {
                m_aviInfo.hasAudioStream = true;

                /* TODO
                waveFormat.formatTag = m_input->readUInt16();
                waveFormat.numChannels = m_input->readUInt16();
                waveFormat.samplesPerSecond = m_input->readUInt32();
                waveFormat.avgBytesPerSecond = m_input->readUInt32();
                waveFormat.blockAlign = m_input->readUInt16();
                waveFormat.extraSizeAfterHeader = m_input->readUInt16();
                waveFormat.extraSizeAfterHeader = m_input->readUInt16();
                */
            }

            if (!m_aviInfo.invalidFile) {
                m_input->skip(m_currentChunk.size - (m_input->getPosition() - m_currentChunk.startPos));

                // eat the rest of the stream chunks in this list
                while (!finishChunk()) {
                    startNextChunk();
                    m_input->skip(m_currentChunk.size);
                }

                startNextChunk();
            }

            streamIdx += 1;
        } while (!m_aviInfo.invalidFile && (m_currentList.fourcc != FOURCC_MOVI));

        // videos should have a supported stream
        if (!m_aviInfo.hasVideoStream) {
            m_aviInfo.invalidFile = true;
        }
    }
}

void AVIInput::readVideoFrame() {
    while (m_currentChunk.fourcc != m_fourccVideoStream) {
        finishChunk();
        startNextChunk();
    }

    m_videoFrame.frameSize = m_currentChunk.size;
    m_input->readBytes(m_videoFrame.frameData, m_currentChunk.size);
    
    if (!finishChunk() && (m_aviInfo.currentFrame < m_aviInfo.numFrames)) {
        startNextChunk();
    } else {
        m_aviInfo.completed = true;
    }
}

void AVIInput::readAudioFrame() {
    debugAssert(false);
}

bool AVIInput::startNextChunk() {
    RIFFChunk rchunk;

    rchunk.fourcc = m_input->readUInt32();
    rchunk.size = m_input->readUInt32();

    // have to assume int64 -> uint32 is valid since AVI RIFF file format only uses 4-byte integers
    rchunk.startPos = static_cast<uint32>(m_input->getPosition());

    bool listStarted = false;

    if (rchunk.fourcc == FOURCC_LIST) {
        rchunk.fourcc = m_input->readUInt32();

        m_currentList = rchunk;
        m_lists.pushBack(m_currentList);

        // recursively call until a non-list chunk is found
        // there shouldn't be more than 1 or 2 nested list headers
        startNextChunk();

        listStarted = true;
    } else {
        // eat padding chunks
        if (rchunk.fourcc == FOURCC_JUNK) {
            m_input->skip(rchunk.size);
            finishChunk();
            startNextChunk();
        } else {
            m_currentChunk = rchunk;
        }
    }

    return listStarted;
}

bool AVIInput::finishChunk() {
    // chunks are WORD aligned and any padding is not included in the chunk size so adjust here
    if (m_input->getPosition() & 0x01) {
        m_input->skip(1);
    }

    if (m_currentList.fourcc != FOURCC_INVALID) {
        // assert that we haven't read past the end of the list or that the list header isn't incorrect
        debugAssert(m_input->getPosition() <= (m_currentList.startPos + m_currentList.size));
    }

    bool listFinished = false;

    while (m_currentList.fourcc != FOURCC_INVALID && (m_currentList.startPos + m_currentList.size <= m_input->getPosition())) {
        if (m_input->getPosition() == (m_currentList.startPos + m_currentList.size)) {
            // the end of a list, to update current list to the previous list
            if (m_lists.length() > 0) {
                m_currentList = m_lists.popBack();
            } else {
                m_currentList.fourcc = FOURCC_INVALID;
            }
            listFinished = true;
        } else if (m_input->getPosition() > (m_currentList.startPos + m_currentList.size)) {
            // stop processing immediately after an invalid chunk or list is found
            m_aviInfo.invalidFile = true;
        }
    }

    return listFinished;
}


}
