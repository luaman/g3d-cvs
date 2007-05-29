/**
 @file GModule.h

 @maintainer Morgan McGuire, morgan3d@users.sourceforge.net


 @created 2006-04-22
 @edited  2007-05-30
*/

#ifndef GLG3D_GMODULE_H
#define GLG3D_GMODULE_H

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/PosedModel.h"
#include "GLG3D/GWindow.h"

namespace G3D {

class RenderDevice;
class UserInput;

typedef ReferenceCountedPointer<class GModule> GModuleRef;
typedef ReferenceCountedPointer<class GModuleManager> GModuleManagerRef;

/**
 Interface for 2D or 3D objects that experience standard
 virtual world events and are rendered.

 GModule is an interface for "widget"-like objects.  You could think
 of it as a bare-bones scene graph.

 Modules are objects like the G3D::FirstPersonController,
 G3D::GConsole, and debug text overlay that need to receive almost the
 same set of events (onXXX methods) as GApp2 and that you would like
 to be called from the corresponding methods of a GApp2.  They are a
 way to break large pieces of functionality for UI and debugging off
 so that they can be mixed and matched.

 @beta
 */
class GModule : public ReferenceCountedObject {
protected:

    /** The manager, set by setManager().
        This cannot be a reference counted pointer because that would create a cycle
        between the GModule and its manager.
     */
    GModuleManager* m_manager;

    GModule() : m_manager(NULL) {}

public:

    typedef ReferenceCountedPointer<class GModule> Ref;

    /** 
     Appends a posed model for this object to the array, if it has a
     graphic representation.  The posed model appended is allowed to
     reference the agent and is allowed to mutate if the agent is
     mutated.
     */
    virtual void getPosedModel(
        Array<PosedModel::Ref>& posedArray,
        Array<PosedModel2DRef>& posed2DArray) = 0;

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) = 0;

    /** Called by the GModuleManager when this module is added to it.  The argument may be NULL */
    virtual void setManager(GModuleManager* m) {
        m_manager = m;
    }

    /** Returning true consumes the event and prevents other GModules
        from seeing it.  Motion events (GEventType::MOUSEMOTION,
        GEventType::JOYHATMOTION, JGEventType::OYBALLMOTION, and
        GEventType::JOYAXISMOTION) cannot be cancelled.
     */
    virtual bool onEvent(const GEvent& event) = 0;

    virtual void onUserInput(UserInput* ui) = 0;

    virtual void onNetwork() = 0;

    virtual void onLogic() = 0;
};



/**
 Manages a group of GModules.  This is used internally by G3D::GApp2
 to process its modules.  It also enables use of GModules without
 the GApp2 infrastructure.  Most users do not need to use this class.

 You can use GModules without this class.
 */
class GModuleManager : public GModule {
public:
    typedef ReferenceCountedPointer<class GModuleManager> Ref;

private:
    
    /** 
        Events are delivered in decreasing index order, except
        rendering, which is processed in increasing order.
     */
    Array<GModuleRef>   m_moduleArray;

    bool                m_locked;

    GModuleRef          m_focusedModule;

    GModuleManager();

    /** Events that have been delayed by a lock */
    class DelayedEvent {
    public:
        enum Type {REMOVE_ALL, REMOVE, ADD, SET_FOCUS, SET_DEFOCUS};
        Type type;
        GModuleRef module;

        DelayedEvent(Type type = ADD, const GModuleRef& module = NULL) : type(type), module(module) {}
    };
    
    /** To be processed in endLock */
    Array<DelayedEvent> m_delayedEvent;

public:

    static GModuleManagerRef create();

    /** 
      Between beginLock and endLock, add and remove operations are
      delayed so that iteration is safe.  Locks may not be executed
      recursively; only one level of locking is allowed.
      */
    void beginLock();

    void endLock();

    /** 
        At most one module has focus at a time.  May be NULL.
     */
    GModuleRef focusedModule() const;

    /** The module must have already been added.  This module will be moved to
        the top of the priority list (i.e., it will receive events first).
        You can pass NULL.

        If you change the focus during a lock, the actual focus change
        will not take effect until the lock is released.

        Setting the focus automatically brings a module to the front of the event processing list.
        */
    void setFocusedModule(const GModuleRef& m);

    /** Removes focus from this module if it had focus, otherwise does nothing */
    void setDefocusedModule(const GModuleRef& m);

    /** 
        If a lock is in effect, the add may be delayed until the
        unlock.

        Priorities should generally not be used; they are largely for
        supporting debugging components at HIGH_PRIORITY that
        intercept events before they can hit the regular
        infrastructure.
      */
    void add(const GModuleRef& m);

    /**
       If a lock is in effect the remove will be delayed until the unlock.
     */
    void remove(const GModuleRef& m);

    /**
     Removes all.
     */
    void clear();

    /** Number of installed modules */
    int size() const;

    /** @deprecated
        Runs the event handles of each manager interlaced, as if all
        the modules from b were in a.*/
    static bool onEvent(const GEvent& event, 
                        GModuleManagerRef& a, 
                        GModuleManagerRef& b);

    static bool onEvent(const GEvent& event,
                        GModuleManagerRef& a);

    /** Returns a module by index number.  The highest index is the one that receives events first.*/
    const GModuleRef& operator[](int i) const;

    /** Calls getPosedModel on all children.*/
    virtual void getPosedModel(
        Array<PosedModel::Ref>& posedArray, 
        Array<PosedModel2DRef>& posed2DArray);

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    virtual bool onEvent(const GEvent& event);

    virtual void onUserInput(UserInput* ui);

    virtual void onNetwork();

    virtual void onLogic();
};


/**
 Exports a coordinate frame, typically in response to user input.
 Examples:
 G3D::ThirdPersonManipulator,
 G3D::FirstPersonManipulator
 */
class Manipulator : public GModule {
public:
    typedef ReferenceCountedPointer<class Manipulator> Ref;

    virtual void getFrame(CoordinateFrame& c) const = 0;

    virtual CoordinateFrame frame() const = 0;
};


} // G3D

#endif
