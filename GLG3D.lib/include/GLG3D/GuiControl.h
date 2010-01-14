/**
 @file GLG3D/GuiControl.h

 @created 2006-05-01
 @edited  2009-09-08

 G3D Library http://g3d.sf.net
 Copyright 2000-2010, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GuiControl_h
#define G3D_GuiControl_h

#include <string>
#include "GLG3D/GuiTheme.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/Widget.h"

namespace G3D {

// forward declare heavily dependent classes
class RenderDevice;
class GuiWindow;
class GuiContainer;


/** Base class for all controls. */
class GuiControl {
    friend class GuiWindow;
    friend class GuiPane;
    friend class GuiContainer;
protected:

    enum {
        LEFT_CAPTION_SIZE = 80,
        TOP_CAPTION_SIZE = 20
    };

    /** Interface to hide the default Callback implementation from programmers using it. */
    class CallbackInterface {
    public:
        /** Execute the callback */
        virtual void execute() = 0;
        inline virtual ~CallbackInterface() {};
        
        /** Used by Callback's copy constructor */
        virtual CallbackInterface* clone() = 0;
    };
    

    /** Callback for functions. */
    class FunctionCallback : public CallbackInterface {
    private:
        void (*m_callback)();
    public:
        FunctionCallback(
                         void (*callback)()) : m_callback(callback) {
        }
        
        virtual void execute() {
            (*m_callback)();
        }
        
        virtual CallbackInterface* clone() {
            return new FunctionCallback(m_callback);
        }
    };
    
    
    /** Callback for classes. */
    template<class Class>
    class MethodCallback : public CallbackInterface {
    private:
        Class*		m_object;
        void (Class::*m_callback)();
    public:
        MethodCallback(
                       Class* object,
                       void (Class::*callback)()) : m_object(object), m_callback(callback) {}
        
        virtual void execute() {
            debugAssertGLOk();
            (m_object->*m_callback)();
            debugAssertGLOk();
        }
        
        virtual CallbackInterface* clone() {
            return new MethodCallback<Class>(m_object, m_callback);
        }
    };
    

    /** Callback for reference counted objects */
    template<class Class>
    class MethodRefCallback : public CallbackInterface {
    private:
        ReferenceCountedPointer<Class> m_object;
        void (Class::*m_callback)();
    public:
        MethodRefCallback(
                          const ReferenceCountedPointer<Class>& object,
                          void (Class::*callback)()) : m_object(object), m_callback(callback) {}
        
        virtual void execute() {
            (m_object.pointer()->*m_callback)();
        }
        
        virtual CallbackInterface* clone() {
            return new MethodRefCallback<Class>(m_object, m_callback);
        }
    };

public:

    /** Base class for GuiButton pre-event handlers. You may subclass this and override execute or
        simply use one of the provided constructors. */
    class Callback {
    private:
        
        CallbackInterface*		m_internal;
        
    public:
        
        inline Callback() : m_internal(NULL) {}
        
        /** Create a callback from a function, e.g., <code>Callback( &printWarning )</code> */
        inline Callback(void (*function)()) : m_internal(new FunctionCallback(function)) {}
        
        /** Create a callback from a class and method of no arguments, e.g.,
            <CODE> App* app = ...; Callback( app, &App::endProgram ); </CODE>.
            If the method is defined on a base class and not
            overriden in the derived class, you must cast the pointer:
            <CODE> Callback(static_cast<Base*>(ptr), &Base::method); </CODE>
         */
        template<class Class>
        inline Callback(
			Class* const object,
			void (Class::*method)()) : m_internal(new MethodCallback<Class>(object, method)) {}
        
        /** Create a callback from a reference counted class and method of no arguments, 
            e.g., \code PlayerRef player = ...; 
            Callback( player, &Player::jump ) \endcode */
        template<class Class>
        inline Callback(
			const ReferenceCountedPointer<Class>& object,
			void (Class::*method)()) : m_internal(new MethodRefCallback<Class>(object, method)) {}
        
        /** Execute the callback.*/
        virtual void execute() {
            if (m_internal) {
                m_internal->execute();
            }
        }
        
        /** Assignment */
        inline Callback& operator=(const Callback& c) {
            delete m_internal;
            if (c.m_internal != NULL) {
                m_internal = c.m_internal->clone();
            } else {
                m_internal = NULL;
            }
            return this[0];
        }
        
        /** Copy constructor */
        Callback(const Callback& c) : m_internal(NULL) {
            this[0] = c;
        }
        
        inline virtual ~Callback() {
            delete m_internal;
        };
    };
    
protected:
    
    /** Sent events should appear to be from this object, which is
       usually "this".  Other controls can set the event source
       to create compound controls that seem atomic from the outside. */
    GuiControl*       m_eventSource;

    bool              m_enabled;

    /** The window that ultimately contains this control */
    GuiWindow*        m_gui;

    /** Parent pane */
    GuiContainer*     m_parent;

    /** Rect bounds used for rendering and layout.
        Relative to the enclosing pane's clientRect. */
    Rect2D            m_rect;

    /** Rect bounds used for mouse actions.  Updated by setRect.*/
    Rect2D            m_clickRect;

    GuiText           m_caption;

    /** For classes that have a caption, this is the size reserved for it.*/
    float             m_captionSize;

    bool              m_visible;

    GuiControl(GuiWindow* gui, const GuiText& text = "");
    GuiControl(GuiContainer* parent, const GuiText& text = "");

    /** Fires an action event */
    void fireEvent(GEventType type);    

public:

    virtual ~GuiControl() {}

    bool enabled() const;
    bool mouseOver() const;
    bool visible() const;
    void setVisible(bool b);
    bool focused() const;
    virtual void setCaption(const GuiText& caption);

    /** Grab or release keyboard focus */
    void setFocused(bool b);
    virtual void setEnabled(bool e);

    /** For controls that have a caption outside the bounds of the control,
        this is the size reserved for the caption. The caption width defaults
        to CAPTION_LEFT_WIDTH, CAPTION_RIGHT_WIDTH, or CAPTION_TOP_HEIGHT, depending 
        on the control type, if the initial caption is not "" (even if it is " ") 
        and 0 if the initial caption is "". */
    float captionSize() const;
    virtual void setCaptionSize(float c);
    const GuiText& caption() const;
    const Rect2D& rect() const;

    /** Get the window containing this control. */
    GuiWindow* window() const;

    /** If you explicitly change the rectangle of a control, the
        containing pane may clip its borders.  Call pack() on the
        containing pane (or window) to resize that container
        appropriately.*/
    virtual void setRect(const Rect2D& rect);
    void setSize(const Vector2& v);
    void setSize(float x, float y);
    void setPosition(const Vector2& v);
    void setPosition(float x, float y);
    void setWidth(float w);
    void setHeight(float h);

    /** If these two controls have the same parent, move this one 
        immediately to the right of the argument*/
    void moveRightOf(const GuiControl* control);
    void moveBy(const Vector2& delta);
    void moveBy(float dx, float dy);    

    GuiThemeRef theme() const;

    /** Return true if this is in tool button style */
    virtual bool toolStyle() const { 
        return false;
    }

    /** Default caption size for this control. */
    virtual float defaultCaptionSize() const {
        return LEFT_CAPTION_SIZE;
    }

    /**
     Only methods on @a skin may be called from this method by default.  To make arbitrary 
     RenderDevice calls, wrap them in GuiTheme::pauseRendering ... GuiTheme::resumeRendering.
     */
    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const = 0;

    /** Used by GuiContainers */
    const Rect2D& clickRect() const {
        return m_clickRect;
    }

    /** Returns the coordinates of \a v, which is in the coordinate system of this object,
       relative to the OSWindow on which it will be rendered. */
    Vector2 toOSWindowCoords(const Vector2& v) const;

    /** Transforms \a v from OS window coordinates to this control's coordinates */
    Vector2 fromOSWindowCoords(const Vector2& v) const;

    Rect2D toOSWindowCoords(const Rect2D& r) const {
        return Rect2D::xywh(toOSWindowCoords(r.x0y0()), r.wh());
    }
protected:

    /** Events are only delivered to a control when the control that
        control has the key focus (which is transferred during a mouse
        down) */
    virtual bool onEvent(const GEvent& event) { (void)event; return false; }

};

}

#endif
