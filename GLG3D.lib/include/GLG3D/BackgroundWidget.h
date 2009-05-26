/**
  @file BackgroundWidget.h
 
  @maintainer Morgan McGuire, morgan3d@users.sf.net
 
  @created 2007-08-16
  @edited  2007-08-16

  Copyright 2000-2007, Morgan McGuire.
  All rights reserved.
 */

#ifndef G3D_BACKGROUNDWIDGET_H
#define G3D_BACKGROUNDWIDGET_H

#include "G3D/platform.h"
#include "G3D/Rect2D.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Widget.h"

namespace G3D {

typedef ReferenceCountedPointer<class BackgroundWidget> BackgroundWidgetRef;

/**
   A full-screen texture that sits behind everything else in the scene.
 */
class BackgroundWidget : public Widget {
protected:

    /**
       A full-screen texture that sits behind everything else in the scene.
     */
    class Posed : public Surface2D {
    public:

        TextureRef    texture;

        virtual Rect2D bounds () const {
            // Grows to the size of any screen
            return Rect2D::xywh(0, 0, finf(), finf());
        }

        virtual float depth() const {
            return finf();
        }

        virtual void render (RenderDevice *rd) const;
    };

    typedef ReferenceCountedPointer<Posed> PosedRef;

    TextureRef          m_texture;
    PosedRef            m_posed;     

    BackgroundWidget();

public:

    static BackgroundWidgetRef fromTexture(TextureRef t);

    static BackgroundWidgetRef create(TextureRef t = NULL) {
        return fromTexture(NULL);
    }

    static BackgroundWidgetRef fromFile(const std::string& textureFilename);

    TextureRef texture() const {
        return m_texture;
    }

    void setTexture(const std::string& textureFilename);

    void setTexture(TextureRef t) {
        m_texture = t;
    }

    virtual bool onEvent (const GEvent& event) { (void)event;return false; }

    virtual void onAI () {}

    virtual void onNetwork () {}

    virtual void onPose (Array< Surface::Ref > &posedArray, Array< Surface2DRef > &posed2DArray) {
        (void)posedArray;
        m_posed->texture = m_texture;
        posed2DArray.append(m_posed);
    }

    virtual void onSimulation (RealTime rdt, SimTime sdt, SimTime idt) {
        (void)idt;(void)sdt;(void)rdt;
    }

    virtual void onUserInput (UserInput* ui) {
        (void)ui;
    }
};

}

#endif
