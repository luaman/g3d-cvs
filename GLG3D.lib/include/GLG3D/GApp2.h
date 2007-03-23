/**
 @file GApp2.h
 
 @maintainer Morgan McGuire, morgan@graphics3d.com

 @created 2003-11-03
 @edited  2007-03-18
 */

#ifndef G3D_GAPP2_H
#define G3D_GAPP2_H

#include "G3D/Stopwatch.h"
#include "GLG3D/GFont.h"
#include "G3D/GCamera.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/NetworkDevice.h"
#include "GLG3D/GWindow.h"
#include "GLG3D/GModule.h"

namespace G3D {

class RenderDevice;
class UserInput;

/**
  See @link guideapp @endlink for a discussion of GApp and GApplet. 

  For each frame, the GApp2 has several tasks that can be implemented by overriding
  base class methods.  The use of cooperative, round-robbin scheduling avoids the need
  for threads in most applications.  These tasks are:

  <ul>
   <li> doGraphics(G3D::RenderDevice*)
   <li> doUserInput(G3D::UserInput*)
   <li> doLogic()
   <li> doNetwork()
   <li> doSimulation(G3D::RealTime, G3D::SimTime)
   <li> doWait(G3D::RealTime cumulativeTime, G3D::RealTime desiredCumulativeTime)
  </ul>


  To invoke a GApplet and let it control the main loop, call
  run().  To control the main loop explicitly, invoke beginRun on
  initialization, call oneFrame() from the main loop, and call endRun on cleanup.
 
 */
class GApp2 {
public:
    friend class GWindow;

    class Settings {
    public:
        GWindow::Settings       window;

	    /**
	       If "<AUTO>", G3D will search for the standard
	       data files.  It is recommended that you override this
	       default and set dataDir to a directory relative
	       to your executable (e.g. "./data/")
	       so your programs can be distributed to users who
	       do not have full the G3D data directory.
	    */
	    std::string             dataDir;
    	
	    /**
	       Can be relative to the G3D data directory (e.g. "font/dominant.fnt")
	       or relative to the current directory.
	       Default is "console-small.fnt"
	    */
	    std::string             debugFontName;
    	
	    std::string             logFilename;
    	
	    /** 
	        When true, GAapp ensures that g3d-license.txt exists in the current
	        directory.  That file is written from the return value of G3D::license() */
	    bool                    writeLicenseFile;
    	    	
	    Settings() : dataDir("<AUTO>"), debugFontName("console-small.fnt"), 
	        logFilename("log.txt"), writeLicenseFile(true) {
	    }
    };

private:

    /** Called from init. */
    void loadFont(const std::string& fontName);

    GWindow*                _window;
    bool                    _hasUserCreatedWindow;

protected:

    Stopwatch               m_graphicsWatch;
    Stopwatch               m_logicWatch;
    Stopwatch               m_networkWatch;
    Stopwatch               m_userInputWatch;
    Stopwatch               m_simulationWatch;
    Stopwatch               m_waitWatch;

    GModuleManagerRef       m_moduleManager;

    bool                    m_endProgram;
    int                     m_exitCode;

    /**
     Strings that have been printed with debugPrint.
     */
    Array<std::string>      debugText;

public:

    const Stopwatch& graphicsWatch() const {
        return m_graphicsWatch;
    }

    const Stopwatch& waitWatch() const {
        return m_waitWatch;
    }

    const Stopwatch& logicWatch() const {
        return m_logicWatch;
    }

    const Stopwatch& networkWatch() const {
        return m_networkWatch;
    }

    const Stopwatch& userInputWatch() const {
        return m_userInputWatch;
    }

    const Stopwatch& simulationWatch() const {
        return m_simulationWatch;
    }

    /** Initialized to GApp::Settings::dataDir, or if that is "<AUTO>", 
        to System::demoFindData(). To make your program
        distributable, override the default 
        and copy all data files you need to a local directory.
        Recommended setting is "data/" or "./", depending on where
        you put your data relative to the executable.

        Your data directory must contain the default debugging font, 
        "console-small.fnt", unless you change it.
    */
    std::string             dataDir;
    Log*                    debugLog;
    RenderDevice*           renderDevice;
    NetworkDevice*          networkDevice;

    /**
     NULL if not loaded
     */
    GFontRef                debugFont;
    UserInput*              userInput;

    /** Invoke to true to end the program at the end of the next event loop. */
    virtual void exit(int code = 0);

    /**
     A default camera that is driven by the defaultController.
     */
    GCamera		            defaultCamera;

    /**
     Allows first person (Quake game-style) control
     using the arrow keys or W,A,S,D and the mouse.
     */
    FirstPersonManipulatorRef defaultController;

    inline GWindow* window() const {
        return _window;
    }

    /**
     When true, debugPrintf prints to the screen.
     (default is true)
     */
    bool                    showDebugText;

    /**
     When true an GKey::ESCAPE keydown event
     quits the program.
     (default is true)
     */
    bool                    quitOnEscape;

    /**
     When true GKey::TAB keydown deactivates
     the camera and restores the mouse cursor.
     (default is true)
     */
    bool                    tabSwitchCamera;

    /**
     When true,   renderDebugInfo prints the frame rate and
     other data to the screen.
     */
    bool                    showRenderingStats;

    /**
     When true and the window is resizable, automatically
     responds to SDL_RESIZE events by notifying the
     RenderDevice that the window has been resized and
     resetting the viewport to full screen.
     (default is true)
     */
    bool                    autoResize;

    /**
      When true, there is an assertion failure if an exception is 
      thrown.

      Default is true.
      */
    bool                    catchCommonExceptions;

    /**
     If debugShowText is true, prints to an on-screen buffer that
     is cleared every frame.
     */
    virtual void debugPrintf(const char* fmt ...);

    /**
     Called from GApplet::run immediately after doGraphics to render
     the debugging text.  Does nothing if debugMode is false.  It
     is not usually necessary to override this method.
     */
    virtual void renderDebugInfo();

    /**
     @param window If null, a SDLWindow will be created for you. This
         argument is useful for substituting a different window
         system (e.g. GlutWindow)
     */
    GApp2(const Settings& options = Settings(), GWindow* window = NULL);

    virtual ~GApp2();

    /**
     Call this to run the app.  Subclasses should override onInit(), not run.
     */
    int run();

private:
    /** Used by doSimulation for elapsed time. */
    RealTime            now, lastTime;

    /** Used by doWait for elapsed time. */
    RealTime            lastWaitTime;

    float               m_desiredFrameRate;
    double              m_simTimeRate;
    RealTime            m_realTime;
    SimTime             m_simTime;
    SimTime             m_idealSimTime;

private:

    /** Helper for run() that actually starts the program loop. Called from run(). */
    void onRun();

    /**
       Initializes state at the beginning of onRun, including calling onCleanup.
    */
    void beginRun();

    /**
       Cleans up at the end of onRun, including calling onCleanup.
     */
    void endRun();

    /** 
      A single frame of rendering, simulation, AI, events, networking,
      etc.  Invokes the onXXX methods. 
    */
    void oneFrame();

public:

    /**
     Installs a module.  Actual insertion may be delayed until the next frame.

     Modules can be installed either on a GApp or a GApplet.  Those installed
     on the GApp will be run by the GApplet every frame in addition to its own.
     
     By default events have NORMAL_PRIORITY. HIGH_PRIORITY is only intended
     for debugging modules that need to temporarily superceed other components.

     All modules within the same priority class receive events
     with undetermined priority.  All high priority modules
     receive events before.
     
     Event priority is important because a module can consume
     events, preventing other modules from observing them.
     For example, a debugging console might run at high priority 
     so that it can consume arrow keys before they reach a normal priority
     first-person controller.

     @beta
     */
    virtual void addModule(const GModuleRef& module, GModuleManager::EventPriority p = GModuleManager::NORMAL_PRIORITY);


    /**
     The actual removal of the module may be delayed until the next frame.
     @beta
     */
    virtual void removeModule(const GModuleRef& module);

    /** Amount of time that passes in simTime for every second of realTime.
        e.g., 1.0 == real-time, 2.0 == fast, 0.5 == slow, 0.0 = stop time.
        Default is 1.0.
        */
    inline double simTimeRate() const {
        return m_simTimeRate;
    }

    virtual void setSimTimeRate(double s) {
        m_simTimeRate = s;
    }

    /** Accumulated wall-clock time since init was called on this applet. 
        Since this time is accumulated, it may drift from the true
        wall-clock obtained by System::time().*/
    inline RealTime realTime() const {
        return m_realTime;
    }

    virtual void setRealTime(RealTime r) {
        m_realTime = r;
    }

    /** In-simulation time since init was called on this applet.  
        Takes into account simTimeSpeed.  Automatically incremented
        after doSimulation.
      */
    inline SimTime simTime() const {
        return m_simTime;
    }

    virtual void setIdealSimTime(SimTime s) {
        m_idealSimTime = s;
    }

    /**
       Simulation time that is always advanced by precisely the
       desiredFrameDuration * simTimeRate, regardless of the 
       actual frame duration.
     */
    inline SimTime idealSimTime() const {
        return m_idealSimTime;
    }

    virtual void setSimTime(SimTime s) {
        m_simTime = s;
    }

    /** Change to invoke frame limiting via doWait.
        Defaults to inf() */
    virtual void setDesiredFrameRate(float fps) {
        debugAssert(fps > 0);
        m_desiredFrameRate = fps;
    }

    float desiredFrameRate() const {
        return m_desiredFrameRate;
    }

    RealTime desiredFrameDuration() const {
        return 1.0 / m_desiredFrameRate;
    }

protected:

    /** Default implementation poses the managed modules from the GApplet and GApp.*/
    virtual void getPosedModel(
        Array<PosedModelRef>& posedArray, 
        Array<PosedModel2DRef>& posed2DArray);


    /**
     Override this with your simulation code.
     Called from GApp::run.
        
     Default implementation does nothing.

     simTime(), idealSimTime() and realTime() are incremented after
     doSimulation is called, so at the beginning of call the current
     time is the end of the previous frame.

     @param rdt Elapsed real-world time since the last call to doSimulation.

     @param sdt Elapsed sim-world time since the last call to
     doSimulation, computed by multiplying the wall-clock time by the
     simulation time rate.

     @param idt Elapsed ideal sim-world time.  Use this for perfectly
     reproducible timing results.  Ideal time always advances by the
     desiredFrameDuration * simTimeRate, no matter how much wall-clock
     time has elapsed.
     */
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
        (void)idt;
        (void)rdt;
		(void)sdt;
    }

    /**
     Rendering callback.

     Override and implement.  The debugCamera's projection and object to world
     matrices are set by default; you can set other cameras as desired. 
     RenderDevice::beginFrame and endFrame are called for you.
     
     Use getPosedModel() to obtain the installed GModules to be rendered.

	 See <A HREF="../demos/main.cpp">demos/main.cpp</A> for an example of
	 overriding lights.
     */
    virtual void onGraphics(RenderDevice* rd);


    /**
     For a networked app, override this to implement your
     network message polling.
     */
    virtual void onNetwork() {}

    /**
      Task to be used for frame rate limiting.  

      Overriding onWait is not recommended.

      Frame rate limiting is useful
      to avoid overloading a maching that is running background tasks and
      for situations where fixed time steps are needed for simulation and there
      is no reason to render faster.

      Default implementation System::sleep()s until cumulativeTime + time
      in wait is greater than or equal to @a frameDuration = 1 / desiredFrameRate.
      */
    virtual void onWait(RealTime cumulativeTime, RealTime frameDuration);


    /**
     Update any state you need to here.  This is a good place for
     AI code, for example.  Called after network and user input,
     before simulation.
     */
    virtual void onLogic() {}

    /**
     Invoked every time run is called.  Default implementation
     resets timers and simTimeRate.

     Sublcasses should invoke GApplet::init to reset the timers.
     */
    virtual void onInit() {
        m_simTime     = 0;
        m_realTime    = 0;
        m_simTimeRate = 1.0;
        lastWaitTime  = System::time();
    }

    /**
     Invoked at the end of every run call.  Default implementation
     does nothing.
     */
    virtual void onCleanup() {}
    
    /**
     It is recommended to override onUserInput() instead of this method.

     Override if you need to explicitly handle events in the order
     they appear.
     
     Note that the userInput contains a record of all
     keys pressed/held, mouse, and joystick state, so 
     you do not have to override this method to handle
     basic input events.

     Return true if the event has been consumed (i.e., no-one else including GApp should process it further).

     The default implementation does nothing.
     */
    virtual bool onEvent(const GEvent& event);


    /**
     Routine for processing user input from the previous frame.
	 Default handles ESC.
     */
    virtual void onUserInput(class UserInput* userInput);
};

}

#endif
