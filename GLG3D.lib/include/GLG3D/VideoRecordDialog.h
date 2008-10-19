/**
  @file VideoRecordDialog.h

  @maintainer Morgan McGuire, morgan@cs.williams.edu

  @created 2008-10-14
  @edited  2008-10-14
*/
#ifndef G3D_VideoRecordDialog_h
#define G3D_VideoRecordDialog_h

#include "G3D/platform.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiNumberBox.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/VideoOutput.h"

namespace G3D {

/**
   @brief A dialog that allows the user to launch recording of the
   on-screen image to a movie.

   The playback rate is the frames-per-second value to be stored in
   the movie file.  The record rate 1/G3D::GApp::simTimeStep.

   Set enabled to false to prevent hot-key handling.
 */
class VideoRecordDialog : public GuiWindow {
public:

    typedef ReferenceCountedPointer<class VideoRecordDialog> Ref;

protected:
    GApp*                        m_app;

    /** For drawing messages on the screen */
    GFontRef                     m_font;

    Array<VideoOutput::Settings> m_settingsTemplate;

    /** Parallel array to m_settingsTemplate of the descriptions for
        use with a drop-down list. */
    Array<std::string>           m_formatList;

    /** Index into m_settingsTemplate and m_formatList */
    int                          m_templateIndex;

    std::string                  m_filename;

    float                        m_playbackFPS;
    float                        m_recordFPS;

    bool                         m_halfSize;
    bool                         m_enableMotionBlur;
    int                          m_motionBlurFrames;

    /** Recording modifies the GApp::simTimeStep; this is the old value */
    float                        m_oldSimTimeStep;
    float                        m_oldDesiredFrameRate;
    
    /** Motion blur frames */
    GuiNumberBox<int>*           m_framesBox;

    /**
       When false, the screen is captured at the beginning of 
       Posed2DModel rendering from the back buffer, which may 
       slow down rendering.
       
       When true, the screen is captured from the the previous 
       frame, which will not introduce latency into rendering.
    */
    bool                         m_captureGUI;

    /** Draw a software cursor on the frame after capture, since the
     hardware cursor will not be visible.*/
    bool                         m_showCursor;

    GuiButton*                   m_recordButton;

    /** Key to start/stop recording even when the GUI is not
        visible.
      */
    GKey                         m_hotKey;
    GKeyMod                      m_hotKeyMod;

    /** Hotkey + mod as a human readable string */
    std::string                  m_hotKeyString;
    
    /** 
        Inserts itself into the bottom of the Posed2D model drawing
        list to call recordFrame so that the rest of the GUI is not
        yet visible.
     */
    class Recorder : public PosedModel2D {
    public:
        VideoRecordDialog*       dialog;

        virtual Rect2D bounds () const {
            return Rect2D::xywh(0,0,1,1);
        }
            
        virtual float depth () const {
            // Lowest possible depth
            return inf();
        }

        virtual void render (RenderDevice *rd) const;
    };
    typedef ReferenceCountedPointer<Recorder> RecorderRef;

    RecorderRef                  m_recorder;

    /** Non-NULL while recording */
    VideoOutput::Ref             m_video;

    VideoRecordDialog(const GuiThemeRef& theme, GApp* app);

public:

    /**
       @param app If not NULL, the VideoRecordDialog will set the app's
       simTimeStep.
     */
    static Ref create(const GuiThemeRef& theme, GApp* app = NULL);
    static Ref create(GApp* app);

    /** Automatically invoked when the record button is pressed. */
    void startRecording();
    void recordFrame(RenderDevice* rd);
    void stopRecording();

    virtual void onPose (Array<PosedModelRef> &posedArray, Array< PosedModel2DRef > &posed2DArray);

    virtual void onLogic();
    virtual bool onEvent(const GEvent& event);
};

}

#endif
