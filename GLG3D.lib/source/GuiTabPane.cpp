/**
 @file GuiTabPane.cpp

 @created 2010-03-01
 @edited  2010-03-01

 Copyright 2000-2010, Morgan McGuire, http://graphics.cs.williams.edu
 All rights reserved.
*/
#include "GLG3D/GuiTabPane.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

// How far the buttons overlap the content
static const float OVERLAP = 10;

GuiTabPane::GuiTabPane(GuiWindow* window, const Pointer<int>& index) : 
    GuiContainer(window, ""), m_internalIndex(0), m_indexPtr(index) {

    if (m_indexPtr.isNull()) {
        m_indexPtr = &m_internalIndex;
    }

    m_tabButtonPane = new GuiPane(this, "", Rect2D::xywh(0,0,0,0), GuiTheme::NO_PANE_STYLE);
    m_viewPane      = new GuiPane(this, "", Rect2D::xywh(0,0,0,0), GuiTheme::ORNATE_PANE_STYLE);
}


void GuiTabPane::setRect(const Rect2D& rect) {
    m_rect = rect;
    float y = m_tabButtonPane->rect().height() - OVERLAP;
    m_viewPane->setRect(Rect2D::xywh(0, y, rect.width(), rect.height() - y));
    m_clientRect = m_viewPane->rect() + rect.x0y0();
}

void GuiTabPane::findControlUnderMouse(Vector2 mouse, GuiControl*& control) const {
    m_viewPane->findControlUnderMouse(mouse, control);
    m_tabButtonPane->findControlUnderMouse(mouse, control);
}


GuiPane* GuiTabPane::addTabPane(const GuiText& label, int id) {
    if (id == -1) {
        id = m_contentPaneArray.size();
    }

    debugAssertM(! m_contentIDArray.contains(id), format("id %d already in use", id));

    GuiPane* p = m_viewPane->addPane("", GuiTheme::NO_PANE_STYLE);
    GuiRadioButton* b = m_tabButtonPane->addRadioButton(label, id, m_indexPtr, GuiTheme::TOOL_RADIO_BUTTON_STYLE);
    (void)b;

    m_contentIDArray.append(id);
    m_contentPaneArray.append(p);
    p->setVisible(*m_indexPtr == id);

    m_tabButtonPane->pack();

    // TODO: adjust layout?

    return p;
}


void GuiTabPane::render(RenderDevice* rd, const GuiTheme::Ref& theme) const {
    // Make the active tab visible
    for (int i = 0; i < m_contentPaneArray.size(); ++i) {
        m_contentPaneArray[i]->setVisible(*m_indexPtr == m_contentIDArray[i]);
    }

    // Update layout
    m_tabButtonPane->setPosition(Vector2((rect().width() - m_tabButtonPane->rect().width()) * 0.5f, 0));

    m_viewPane->setPosition(Vector2(0, m_tabButtonPane->rect().height() - OVERLAP));

    // TODO: Offset
    m_viewPane->render(rd, theme);
    m_tabButtonPane->render(rd, theme);
}

}
