/**************************************************************************\
 *
 *  This file is part of the Coin family of 3D visualization libraries.
 *  Copyright (C) 1998-2002 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and / or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.  See the
 *  file LICENSE.GPL at the root directory of this source distribution
 *  for more details.
 *
 *  If you desire to use this library in software that is incompatible
 *  with the GNU GPL, and / or you would like to take advantage of the
 *  additional benefits with regard to our support services, please
 *  contact Systems in Motion about acquiring a Coin Professional
 *  Edition License.  See <URL:http://www.coin3d.org> for more
 *  information.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  <URL:http://www.sim.no>, <mailto:support@sim.no>
 *
\**************************************************************************/

#if HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <qevent.h>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/Qt/devices/SoQtMouse.h>
#include <Inventor/Qt/devices/SoGuiMouseP.h>

// *************************************************************************

#ifndef DOXYGEN_SKIP_THIS

class SoQtMouseP : public SoGuiMouseP {
public:
  SoQtMouseP(SoQtMouse * p) : SoGuiMouseP(p) { }
};

#endif // !DOXYGEN_SKIP_THIS

// *************************************************************************

SoQtMouse::SoQtMouse(int mask)
{
  PRIVATE(this) = new SoQtMouseP(this);
  PRIVATE(this)->eventmask = mask;
}

SoQtMouse::~SoQtMouse()
{
  delete PRIVATE(this);
}

// *************************************************************************

void
SoQtMouse::enable(QWidget * widget, SoQtEventHandler * handler, void * closure)
{
  // FIXME: should add some magic here so Qt events are actually
  // enabled or disabled for the widget in question. 20020625 mortene.
}

void
SoQtMouse::disable(QWidget * widget, SoQtEventHandler * handler, void * closure)
{
  // FIXME: should add some magic here so Qt events are actually
  // enabled or disabled for the widget in question. 20020625 mortene.
}

// *************************************************************************

const SoEvent *
SoQtMouse::translateEvent(QEvent * event)
{
  SoEvent * conv = NULL;

  QWheelEvent * wheelevent =
    (event->type() == QEvent::Wheel) ? (QWheelEvent *)event : NULL;

  QMouseEvent * mouseevent =
    ((event->type() == QEvent::MouseButtonDblClick) ||
     (event->type() == QEvent::MouseButtonPress) ||
     (event->type() == QEvent::MouseButtonRelease) ||
     (event->type() == QEvent::MouseMove)) ?
    (QMouseEvent *)event : NULL;

  if (!wheelevent && !mouseevent) return NULL;

  // Convert wheel mouse events to Coin SoMouseButtonEvents.
  //
  // FIXME: should consider adding an SoMouseWheel event to Coin?
  // 20020821 mortene. (idea mentioned by Florian Link on
  // coin-discuss.)

#ifdef HAVE_SOMOUSEBUTTONEVENT_BUTTON5
  if (wheelevent) {
    if (wheelevent->delta() > 0)
      PRIVATE(this)->buttonevent->setButton(SoMouseButtonEvent::BUTTON4);
    else if (wheelevent->delta() < 0)
      PRIVATE(this)->buttonevent->setButton(SoMouseButtonEvent::BUTTON5);
#if SOQT_DEBUG
    else {
      SoDebugError::postInfo("SoQtMouse::translateEvent",
                             "event, but no movement");
    }
#endif // SOQT_DEBUG
    PRIVATE(this)->buttonevent->setState(SoButtonEvent::DOWN);
    conv = PRIVATE(this)->buttonevent;
  }
#endif // HAVE_SOMOUSEBUTTONEVENT_BUTTON5

  // Check for mousebutton press/release. Note that mousebutton
  // doubleclick events are handled by converting them to two
  // press/release events. In other words: it's the user's
  // responsibility to translate pairs of singleclicks to
  // doubleclicks, if doubleclicks have a special meaning in the
  // application.

  // Qt actually sends this series of events upon dblclick:
  // QEvent::MouseButtonPress, QEvent::MouseButtonRelease,
  // QEvent::MouseButtonDblClick, QEvent::MouseButtonRelease.
  //
  // This was reported to Troll Tech as a possible bug, but was
  // confirmed by TT support to be the intended behavior.

  if (((event->type() == QEvent::MouseButtonDblClick) ||
       (event->type() == QEvent::MouseButtonPress) ||
       (event->type() == QEvent::MouseButtonRelease)) &&
      (PRIVATE(this)->eventmask & (BUTTON_PRESS | BUTTON_RELEASE))) {

    // Which button?
    switch (mouseevent->button()) {
    case Qt::LeftButton:
      PRIVATE(this)->buttonevent->setButton(SoMouseButtonEvent::BUTTON1);
// Emulate right mouse button on Mac OS X platform by ctrl-click.
#ifdef Q_WS_MAC
// Since Qt version 3.1.x, Qt::ControlButton is mapped to the "Apple"
// key, not the "ctrl" key. (According to qt-support, this is intended
// and not a bug.) The standard way of emulating right-click in the Mac 
// world is still ctrl-click, though... *sigh* 
#if QT_VERSION < 0x030100
      if (mouseevent->state() & Qt::ControlButton)
        PRIVATE(this)->buttonevent->setButton(SoMouseButtonEvent::BUTTON2);
#else
      if (mouseevent->state() & Qt::MetaButton) 
        PRIVATE(this)->buttonevent->setButton(SoMouseButtonEvent::BUTTON2);
#endif
#endif // Q_WS_MAC
      break;
    case Qt::RightButton:
      PRIVATE(this)->buttonevent->setButton(SoMouseButtonEvent::BUTTON2);
      break;
    case Qt::MidButton:
      PRIVATE(this)->buttonevent->setButton(SoMouseButtonEvent::BUTTON3);
      break;
    default:
      assert(0 && "no such SoQtMouse button");
      break;
    }

    // Press or release?
    if (mouseevent->button() & mouseevent->state())
      PRIVATE(this)->buttonevent->setState(SoButtonEvent::UP);
    else
      PRIVATE(this)->buttonevent->setState(SoButtonEvent::DOWN);

    conv = PRIVATE(this)->buttonevent;
  }


  // Check for mouse movement.
  if ((event->type() == QEvent::MouseMove) &&
      // FIXME: is this correct? BUTTON_MOTION means "motion with
      // buttons down". 20020625 mortene.
      (PRIVATE(this)->eventmask & (POINTER_MOTION | BUTTON_MOTION))) {
    conv = PRIVATE(this)->locationevent;
  }


  // Common settings for SoEvent superclass.
  if (conv) {
    // Modifiers
    if (mouseevent) {
      conv->setShiftDown(mouseevent->state() & Qt::ShiftButton);
      conv->setCtrlDown(mouseevent->state() & Qt::ControlButton);
      conv->setAltDown(mouseevent->state() & Qt::AltButton);
      this->setEventPosition(conv, mouseevent->x(), mouseevent->y());
    }
    else { // wheelevent
      conv->setShiftDown(wheelevent->state() & Qt::ShiftButton);
      conv->setCtrlDown(wheelevent->state() & Qt::ControlButton);
      conv->setAltDown(wheelevent->state() & Qt::AltButton);
      this->setEventPosition(conv, wheelevent->x(), wheelevent->y());
    }

    // FIXME: should be time of Qt event. 19990211 mortene.
    conv->setTime(SbTime::getTimeOfDay());
  }

  return conv;
}

// *************************************************************************
