/**
 @file Widget.h

 @maintainer Morgan McGuire, morgan3d@users.sourceforge.net

 @created 2006-04-22
 @edited  2009-12-28
*/

#ifndef GLG3D_Widget_h
#define GLG3D_Widget_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Surface.h"
#include "GLG3D/OSWindow.h"

namespace G3D {

class RenderDevice;
class UserInput;

typedef ReferenceCountedPointer<class Widget> WidgetRef;
typedef ReferenceCountedPointer<class WidgetManager> WidgetManagerRef;

/**
 Interface for 2D or 3D objects that experience standard virtual world
 events and are rendered.

 Widget is an interface for GUI-like objects.  You could think of it
 as a bare-bones scene graph.

 Widgets are objects like the G3D::FirstPersonController,
 G3D::GConsole, and debug text overlay that need to receive almost the
 same set of events (onXXX methods) as G3D::GApp and that you would
 like to be called from the corresponding methods of a G3D::GApp.
 They are a way to break large pieces of functionality for UI and
 debugging off so that they can be mixed and matched.

 Widget inherits G3D::Surface2D because it is often convenient to
 implement a 2D widget whose onPose method adds itself to the
 rendering array rather than using a proxy object.
 */
class Widget : public Surface2D {
protected:

    /** The manager, set by setManager().  This cannot be a reference
        counted pointer because that would create a cycle between the
        Widget and its manager. */
    WidgetManager* m_manager;

    Widget() : m_manager(NULL) {}

public:

    typedef ReferenceCountedPointer<class Widget> Ref;

    /** 
     Appends a posed model for this object to the array, if it has a
     graphic representation.  The posed model appended is allowed to
     reference the agent and is allowed to mutate if the agent is
     mutated.
     */
    virtual void onPose(
        Array<Surface::Ref>& posedArray,
        Array<Surface2D::Ref>& posed2DArray) {
        (void)posedArray;
        (void)posed2DArray;
    }

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
        (void)rdt;
        (void)sdt;
        (void)idt;
    }

    /** Called by the WidgetManager when this module is added to it.  The argument may be NULL */
    virtual void setManager(WidgetManager* m) {
        m_manager = m;
    }

    /** Fire an event on the containing window */
    virtual void fireEvent(const GEvent& event);

    /** Returning true consumes the event and prevents other GModules
        from seeing it.  Motion events (GEventType::MOUSEMOTION,
        GEventType::JOYHATMOTION, JGEventType::OYBALLMOTION, and
        GEventType::JOYAXISMOTION) cannot be cancelled.
     */
    virtual bool onEvent(const GEvent& event) { (void)event; return false;}

    virtual void onUserInput(UserInput* ui) { (void)ui;}

    virtual void onNetwork() {}

    virtual void onAI() {}

    /** Returns the operating system window that is currently
        rendering this Widget. */
    virtual OSWindow* window() const;

    /** Inherited from Surface2D */
    virtual void render(RenderDevice* rd) const {(void)rd; }

    /** Inherited from Surface2D */
    virtual Rect2D bounds() const {
        return AABox2D(-Vector2::inf(), Vector2::inf());
    }

    /** Inherited from Surface2D */
    virtual float depth() const { return 0.5f; }
};



/**
 Manages a group of G3D::Widget.  This is used internally by G3D::GApp
 to process its modules.  It also enables use of Widgets without the
 G3D::GApp infrastructure.  Most users do not need to use this class.

 You can use G3D::Widget without this class.
 */
class WidgetManager : public Widget {
public:

    typedef ReferenceCountedPointer<class WidgetManager> Ref;

private:
    
    /** Events are delivered in decreasing index order, except
        rendering, which is processed in increasing order.  */
    Array<Widget::Ref>   m_moduleArray;

    bool                 m_locked;

    /** The widget that will receive events first. This is usually but
        not always the top Widget in m_moduleArray. */
    Widget::Ref          m_focusedModule;

    WidgetManager();

    /** Manager events that have been delayed by a lock.  Not related
        to GEvent in any way. */
    class DelayedEvent {
    public:
        enum Type {REMOVE_ALL, REMOVE, ADD, SET_FOCUS, SET_FOCUS_AND_MOVE_TO_FRONT, SET_DEFOCUS, MOVE_TO_BACK};

        Type        type;
        Widget::Ref module;

        DelayedEvent(Type type = ADD, const Widget::Ref& module = NULL) : type(type), module(module) {}
    };
    
    /** To be processed in endLock */
    Array<DelayedEvent> m_delayedEvent;

    OSWindow*  m_window;

public:

    /** @param window The window that generates events for this manager.*/
    static WidgetManager::Ref create(OSWindow* window);

    /** 
      Between beginLock and endLock, add and remove operations are
      delayed so that iteration is safe.  Locks may not be executed
      recursively; only one level of locking is allowed.  If using with
      G3D::GApp, allow the G3D::GApp to perform the locking for you.
      */
    void beginLock();

    void endLock();

    /** Widgets currently executing. Note that some widgets may have already been added
        but if the WidgetManager is locked they will not appear in this array yet.*/
    inline const Array<Widget::Ref>& widgetArray() const {
        return m_moduleArray;
    }

    /** Pushes this widget to the back of the z order.  This window will render first and receive events last.
        This is the opposite of focussing a window.
     */
    void moveWidgetToBack(const Widget::Ref& widget);

    /** At most one widget has focus at a time.  May be NULL. */
    Widget::Ref focusedWidget() const;

    /** The module must have already been added.  This module will be moved to
        the top of the priority list (i.e., it will receive events first).
        You can pass NULL.

        If you change the focus during a lock, the actual focus change
        will not take effect until the lock is released.

        Setting the focus automatically brings a module to the front of the 
        event processing list unless \a bringToFront is false
        */
    void setFocusedWidget(const Widget::Ref& m, bool bringToFront = true);

    /** Removes focus from this module if it had focus, otherwise does
        nothing.  The widget will move down one z order in focus.  See
        moveWidgetToBack() to push it all of the way to the back. */
    void defocusWidget(const Widget::Ref& m);

    /** If a lock is in effect, the add may be delayed until the
        unlock.

        Priorities should generally not be used; they are largely for
        supporting debugging components at HIGH_PRIORITY that
        intercept events before they can hit the regular
        infrastructure.
      */
    void add(const Widget::Ref& m);

    /**
       If a lock is in effect the remove will be delayed until the unlock.
     */
    void remove(const Widget::Ref& m);

    /** If this widget is currently visible (note: as of the last lock) */
    bool contains(const Widget::Ref& m) const;

    /**
     Removes all.
     */
    void clear();

    /** Number of installed modules */
    int size() const;

    /** Queues an event on the window associated with this manager. */
    void fireEvent(const GEvent& event);

    /** @deprecated
        Runs the event handles of each manager interlaced, as if all
        the modules from b were in a.*/
    static bool onEvent(const GEvent& event, 
                        WidgetManager::Ref& a, 
                        WidgetManager::Ref& b);

    static bool onEvent(const GEvent& event,
                        WidgetManager::Ref& a);

    /** Returns a module by index number.  The highest index is the one that receives events first.*/
    const Widget::Ref& operator[](int i) const;

    /** Calls onPose on all children.*/
    virtual void onPose(
        Array<Surface::Ref>& posedArray, 
        Array<Surface2DRef>& posed2DArray);

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    virtual bool onEvent(const GEvent& event);

    virtual void onUserInput(UserInput* ui);

    virtual void onNetwork();

    virtual void onAI();

    virtual OSWindow* window() const;
};


/**
 Exports a coordinate frame, typically in response to user input.
 Examples:
 G3D::ThirdPersonManipulator,
 G3D::FirstPersonManipulator
 */
class Manipulator : public Widget {
public:
    typedef ReferenceCountedPointer<class Manipulator> Ref;

    virtual void getFrame(CoordinateFrame& c) const = 0;

    virtual CoordinateFrame frame() const = 0;
};


} // G3D

#endif
