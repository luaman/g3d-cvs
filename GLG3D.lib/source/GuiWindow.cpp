#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/UserInput.h"

namespace G3D {

GuiWindow::Ref GuiWindow::create(const GuiText& label, const Rect2D& rect, const GuiSkinRef& skin, Style style, CloseAction close) {
    return new GuiWindow(label, rect, skin, style, close);
}


GuiWindow::GuiWindow(const GuiText& text, const Rect2D& rect, GuiSkinRef skin, Style style, CloseAction close) 
    : m_text(text), m_rect(rect), m_visible(true), 
      m_style(style), m_closeAction(close), skin(skin), 
      inDrag(false),
      mouseOverGuiControl(NULL), 
      keyFocusGuiControl(NULL),
      m_focused(false),
      m_mouseVisible(false) {

    setRect(rect);
    posed = new Posed(this);
    m_rootPane = new GuiPane(this, NULL, "", clientRect() - clientRect().x0y0(), GuiPane::NO_FRAME_STYLE);
}


GuiWindow::~GuiWindow() {
    delete m_rootPane;
}


void GuiWindow::setRect(const Rect2D& r) {
    m_rect = r;

    switch (m_style) {
    case TOOL_FRAME_STYLE:
        m_clientRect = skin->toolWindowToClientBounds(m_rect);
        break;

    case FRAME_STYLE:
        m_clientRect = skin->windowToClientBounds(m_rect);
        break;
   
    case NO_FRAME_STYLE:
        m_clientRect = m_rect;
        break;
    }
}


void GuiWindow::onUserInput(UserInput* ui) {
    // Not in focus if the mouse is invisible
    m_mouseVisible = (ui->window()->mouseHideCount() <= 0);

    m_focused =
        m_visible &&
        (m_manager->focusedModule().pointer() == this) &&
        m_mouseVisible;

    if (! focused()) {
        return;
    }

    Vector2 mouse = ui->mouseXY();
    mouseOverGuiControl = NULL;

    if (inDrag) {
        setRect(dragOriginalRect + mouse - dragStart);
        return;
    }

    m_closeButton.mouseOver = false;
    if (m_rect.contains(mouse)) {
        // The mouse is over this window, update the mouseOver control
        
        if (m_closeAction != NO_CLOSE) {
            switch (m_style) {
            case TOOL_FRAME_STYLE:
                m_closeButton.mouseOver = skin->toolWindowToCloseButtonBounds(m_rect).contains(mouse);
                break;
                
            case FRAME_STYLE:
                {
                    const Rect2D& buttonBounds = skin->windowToCloseButtonBounds(m_rect);
                    m_closeButton.mouseOver = buttonBounds.contains(mouse);
                }
                break;

            default:;
            }
        }


        mouse -= m_clientRect.x0y0();
        m_rootPane->findControlUnderMouse(mouse, mouseOverGuiControl);
    }
}


void GuiWindow::getPosedModel(Array<PosedModelRef>& posedArray, Array<PosedModel2DRef>& posed2DArray) {
    if (m_visible) {
        posed2DArray.append(posed);
    }
}

static bool isMouseEvent(const GEvent& e) {
    return (e.type == GEventType::MOUSE_MOTION) ||
        (e.type == GEventType::MOUSE_BUTTON_DOWN) ||
        (e.type == GEventType::MOUSE_BUTTON_UP);
}

static GEvent makeRelative(const GEvent& e, const Vector2& clientOrigin) {
    GEvent out(e);

    switch (e.type) {
    case GEventType::MOUSE_MOTION:
        out.motion.x -= clientOrigin.x;
        out.motion.y -= clientOrigin.y;
        break;

    case GEventType::MOUSE_BUTTON_DOWN:
    case GEventType::MOUSE_BUTTON_UP:
        out.button.x -= clientOrigin.x;
        out.button.y -= clientOrigin.y;        
        break;
    }

    return out;
}


bool GuiWindow::onEvent(const GEvent &event) {
    if (! m_mouseVisible || ! m_visible) {
        // Can't be using the GuiWindow if the mouse isn't visible or the gui isn't visible
        return false;
    }

    bool consumed = false;

    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {
        // Mouse down; change the focus
        Vector2 mouse(event.button.x, event.button.y);

        if (! m_rect.contains(mouse)) {
            // The click was not on this object.  Lost focus if we have it
            if (focused()) {
                m_manager->setDefocusedModule(this);
            }
            return false;
        }

        if (! focused()) {
            // Set focus
            m_manager->setFocusedModule(this);
            m_focused = true;

            // Most windowing systems do not allow the original click
            // to reach a control if it was consumed on focusing the
            // window.  However, we deliver events because for most 3D
            // programs the multiple windows are probably acting like
            // tool windows and should not require multiple clicks for
            // selection.
        }

        Rect2D titleRect;
        Rect2D closeRect;
        switch (m_style) {
        case TOOL_FRAME_STYLE:
            titleRect = skin->toolWindowToTitleBounds(m_rect);
            closeRect = skin->toolWindowToCloseButtonBounds(m_rect);
            break;

        case FRAME_STYLE:
            titleRect = skin->windowToTitleBounds(m_rect);
            closeRect = skin->windowToCloseButtonBounds(m_rect);
            break;

        case NO_FRAME_STYLE:
            titleRect = Rect2D::xywh(m_rect.x0y0(), Vector2(m_rect.width(), 0));
            break;
        }

        if ((m_closeAction != NO_CLOSE) && closeRect.contains(mouse)) {
            close();
            return true;
        }

        if (titleRect.contains(mouse)) {
            inDrag = true;
            dragStart = mouse;
            dragOriginalRect = m_rect;
            return true;

        } else {

            mouse -= m_clientRect.x0y0();

            keyFocusGuiControl = NULL;
            m_rootPane->findControlUnderMouse(mouse, keyFocusGuiControl);
        }
    } else if (event.type == GEventType::MOUSE_BUTTON_UP) {
        if (inDrag) {
            inDrag = false;
            return true;
        }
    }

    if (! focused()) {
        return consumed;
    }
    
    if (keyFocusGuiControl != NULL) {
        // Deliver event to the control

        if (isMouseEvent(event)) {

            // Make the event relative by accumulating all of the transformations
            Vector2 origin = m_clientRect.x0y0();
            GuiPane* p = keyFocusGuiControl->m_parent;
            while (p != NULL) {
                origin += p->m_clientRect.x0y0();
                p = p->m_parent;
            }

            consumed = consumed || keyFocusGuiControl->onEvent(makeRelative(event, origin));

        } else {
            consumed = consumed || keyFocusGuiControl->onEvent(event);
        }
    }

    return consumed;
}

void GuiWindow::close() {
    switch (m_closeAction) {
    case NO_CLOSE:
      debugAssertM(false, "Internal Error");
      break;
    
    case HIDE_ON_CLOSE:
        setVisible(false);
        // Intentionally fall through

    case IGNORE_CLOSE:
        {
            GEvent e;
            e.guiClose.type = GEventType::GUI_CLOSE;
            e.guiClose.window = this;
            fireEvent(e);
        }
        break;
    
    case REMOVE_ON_CLOSE:
        {
            GEvent e;
            e.guiClose.type = GEventType::GUI_CLOSE;
            e.guiClose.window = NULL;
            fireEvent(e);
        }
        break;
    }
}


void GuiWindow::render(RenderDevice* rd) {
    // Offset by the window bounds 
    
    skin->beginRendering(rd);
    {
        bool hasClose = m_closeAction != NO_CLOSE;

        switch (m_style) {
        case TOOL_FRAME_STYLE:
            skin->renderToolWindow(m_rect, focused(), hasClose, m_closeButton.down, m_closeButton.mouseOver, m_text);
            break;

        case FRAME_STYLE:
            skin->renderWindow(m_rect, focused(),  hasClose, m_closeButton.down, m_closeButton.mouseOver, m_text);
            break;

        case NO_FRAME_STYLE:
            debugAssertM(m_closeAction == NO_CLOSE, "Windows without frames cannot have a close button.");
            break;
        }
        
        skin->pushClientRect(m_clientRect);
            m_rootPane->render(rd, skin);
        skin->popClientRect();
    }
    skin->endRendering();
    
}

///////////////////////////////////////////////////////////////////////


GuiWindow::Posed::Posed(GuiWindow* gui) : gui(gui) {}

Rect2D GuiWindow::Posed::bounds () const {
    return gui->m_rect;
}

float GuiWindow::Posed::depth () const {
    return 0;
}

void GuiWindow::Posed::render (RenderDevice *rd) const {
    gui->render(rd);
}

}
