/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2003 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  ("GPL") version 2 as published by the Free Software Foundation.
 *  See the file LICENSE.GPL at the root directory of this source
 *  distribution for additional information about the GNU GPL.
 *
 *  For using Coin with software that can not be combined with the GNU
 *  GPL, and for taking advantage of the additional benefits of our
 *  support services, please contact Systems in Motion about acquiring
 *  a Coin Professional Edition License.
 *
 *  See <URL:http://www.coin3d.org> for more information.
 *
 *  Systems in Motion, Teknobyen, Abels Gate 5, 7030 Trondheim, NORWAY.
 *  <URL:http://www.sim.no>.
 *
\**************************************************************************/

#include <assert.h>
#include <qpoint.h>
#include <qpainter.h>
#include <qcursor.h>
#include <Inventor/SbLinear.h>
#include "ColorCurve.h"
#include "CurveView.h"

CurveView::CurveView(SoQtCurveWidget::Mode mode,
                     QCanvas * canvas,
                     QWidget * parent,
                     const char * name,
                     WFlags flags)
                     
 : QCanvasView(canvas, parent, name, flags), ptsize(3)
{
  this->colormode = mode;
  this->canvas = canvas;

  this->initColorCurves();
  this->hideUnselected();
  
  this->curvemode = CurveView::SMOOTH;

  this->mousepressed = FALSE;
  this->initGrid();
  this->initCanvasCurve();
  this->viewport()->setMouseTracking(TRUE);
}

CurveView::~CurveView()
{
  for (int i = 0; i < this->colormode; i++) {
    delete this->colorcurves[i];
  }
}

void
CurveView::initColorCurves()
{
  for (int i = 0; i < this->colormode; i++) {
    this->colorindex = i;
    ColorCurve::CurveType type = ColorCurve::LINEAR;
    if (((this->colormode == SoQtCurveWidget::LUMINANCE_ALPHA) && (i == 1)) ||
        ((this->colormode == SoQtCurveWidget::RGBA) && (i == 3))){
      type = ColorCurve::CONSTANT;
    }

    this->colorcurves.append(new ColorCurve(type));
    this->canvasctrlpts.append(this->newCanvasCtrlPtList());
  }
  this->colorindex = 0;
}

void 
CurveView::initCanvasCurve()
{
  const uint8_t * curvepts = this->colorcurves[this->colorindex]->getColorMap();

  for (int i = 2; i < this->colorcurves[this->colorindex]->getNumPoints(); i+=2) {
    QCanvasLine * line = new QCanvasLine(this->canvas);
    line->setPoints(i-2, 255-curvepts[i-2], i, 255-curvepts[i]);
    line->setZ(1); // to make the curve be drawn on top of the grid
    line->show();
    this->curvesegments.append(line);
  }
}

void CurveView::initGrid()
{
  int step = 256/4;
  QPen pen(Qt::gray);

  for (int i = step; i < 255; i+=step) {
    QCanvasLine * line = new QCanvasLine(this->canvas);
    line->setPoints(i, 0, i, 255);
    line->setPen(pen);
    line->show();
    this->grid.append(line);
  }

  for (int j = step; j < 255; j+=step) {
    QCanvasLine * line = new QCanvasLine(this->canvas);
    line->setPoints(0, j, 255, j);
    line->setPen(pen);
    line->show();
    this->grid.append(line);
  }  
}

void CurveView::contentsMousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton) {
    this->mousepressed = TRUE;
  }
  QPoint p = inverseWorldMatrix().map(e->pos());
  this->movingstart = p;
  this->lastpos = p;
  QCanvasItemList list = this->canvas->collisions(p);

  if (this->curvemode == CurveView::SMOOTH) {

    QCanvasItemList::Iterator it = list.begin();

    if ((it != list.end()) && ((*it)->rtti() == QCanvasRectangle::RTTI)) {
      if (e->button() == Qt::LeftButton) {
        this->movingitem = (*it);
      } else {
        if (this->canvasctrlpts[this->colorindex].size() > 2) {
          delete (*it);
          this->canvasctrlpts[this->colorindex].remove(*it);
        }
      }
    } else {
      if (e->button() == Qt::LeftButton) {
        QCanvasRectangle * ctrlpt = this->newControlPoint(p.x(), p.y());
        this->movingitem = ctrlpt;
        this->canvasctrlpts[this->colorindex].append(ctrlpt);
      }
    }
    this->updateCurve();
  }

  this->canvas->update();
}

void
CurveView::contentsMouseReleaseEvent(QMouseEvent * e)
{
  this->mousepressed = FALSE;
  this->colorcurves[this->colorindex]->notify();
}

void 
CurveView::contentsMouseMoveEvent(QMouseEvent* e)
{
	QPoint p = inverseWorldMatrix().map(e->pos());

  if (this->curvemode == CurveView::SMOOTH) {
    // change the cursor if it is over a control point
    QCanvasItemList list = this->canvas->collisions(p);
    QCanvasItemList::Iterator it = list.begin();

    if ((it != list.end()) && ((*it)->rtti() == QCanvasRectangle::RTTI)) {
      this->setCursor(QCursor::SizeAllCursor);
    } else {
      this->setCursor(QCursor::ArrowCursor);
    }
  
    if (this->movingitem && this->mousepressed) {
      int x = p.x();
      int y = p.y();
    
      if (x > 255 - this->ptsize) x = 255 - this->ptsize;
      if (y > 255 - this->ptsize) y = 255 - this->ptsize;
      if (x < this->ptsize) x = this->ptsize;
      if (y < this->ptsize) y = this->ptsize;

      this->movingitem->moveBy(x - movingstart.x(), y - movingstart.y());
	    this->movingstart = QPoint(x, y);
      this->updateCurve();
    }
  } else {
    if (this->mousepressed) {
      int lastx = lastpos.x();
      int lasty = lastpos.y();
      int currentx = p.x();
      int currenty = p.y();
      if ((lastx >= 0) && (lastx <= 255) && (currentx >= 0) && (currentx <= 255)) {

        if (currentx < lastx) { // swap
          currentx ^= lastx ^= currentx ^= lastx;
          currenty ^= lasty ^= currenty ^= lasty;
        }
        float x0 = float(lastx);
        float y0 = float(lasty);
        float x1 = float(currentx);
        float y1 = float(currenty);
        float dx = x1 - x0;

        // fill in the points between p and lastpos,
        // linearly interpolate the position
        for (int i = lastx; i < currentx; i++) {
          float w0 = (x1 - float(i)) / dx;
          float w1 = (float(i) - x0) / dx;
          int y = int(w0 * y0 + w1 * y1 + 0.5f);
          if (y > 255) y = 255;
          if (y < 0) y = 0;
          this->colorcurves[this->colorindex]->setColorMapping(i, 255-y);
        }
        this->lastpos = p;
        this->updateCurve();
      }
    }
  }
  this->canvas->update();
}

void 
CurveView::drawContents(QPainter * p, int cx, int cy, int cw, int ch)
{
  QCanvasItemList::Iterator it;
  
    // draw the grid in the background
  it = this->grid.begin();
  for (; it != this->grid.end(); it++) {
    (*it)->draw(*p);
  }
  // draw the curve
  it = this->curvesegments.begin();
  for (; it != this->curvesegments.end(); it++) {
    (*it)->draw(*p);
  }
  // draw the control points
  it = this->canvasctrlpts[this->colorindex].begin();
  for (; it != this->canvasctrlpts[this->colorindex].end(); ++it) {
    if (this->curvemode == CurveView::SMOOTH) {
      (*it)->draw(*p);
    }
  }
}

void 
CurveView::hideUnselected()
{
  QCanvasItemList::Iterator it;

  for (int i = 0; i < this->colormode; i++) {
    it = this->canvasctrlpts[i].begin();
    for (; it != this->canvasctrlpts[i].end(); ++it) {
      if ((this->colorindex == i) && (this->curvemode != CurveView::FREE)) {
        (*it)->show();
      } else {
        (*it)->hide();
      }
    }
  }
}

void
CurveView::resetActive() 
{
  this->colorcurves[this->colorindex]->resetCtrlPts();
  // QCanvasItemList::clear() only removes the items from the list, 
  // but they need to be deleted also.
  QCanvasItemList::Iterator it = this->canvasctrlpts[this->colorindex].begin();
  for (; it != this->canvasctrlpts[this->colorindex].end(); it++) {
    delete (*it);
  }
  this->canvasctrlpts[this->colorindex].clear();
  this->canvasctrlpts[this->colorindex] = this->newCanvasCtrlPtList();

  this->curvemode = CurveView::SMOOTH;

  this->updateCurve();
  this->canvas->update();
  emit this->curveChanged();

}

QCanvasItemList
CurveView::newCanvasCtrlPtList()
{
  QCanvasItemList list;
  int numpts = this->colorcurves[this->colorindex]->getNumCtrlPoints();
  const SbVec3f * ctrlpts = this->colorcurves[this->colorindex]->getCtrlPoints();

  for (int i = 0; i < numpts; i++) {
    list.append(this->newControlPoint(ctrlpts[i][0], 255.0f - ctrlpts[i][1]));
  }
  return list;
}

QCanvasRectangle *
CurveView::newControlPoint(int x, int y)
{
  QCanvasRectangle * rect
    = new QCanvasRectangle(x-this->ptsize, y-this->ptsize,
                           this->ptsize*2, this->ptsize*2,
                           this->canvas);
  rect->setZ(2); // the control points will be on top of the curve
  rect->show();
  return rect;
}

void 
CurveView::updateCurve()
{
  QCanvasItemList::iterator it;

  int i = 0;
  if (this->curvemode == CurveView::SMOOTH) { 
    QCanvasItemList list = this->canvasctrlpts[this->colorindex];
    QCanvasItemList sortedlist;
    
    // Sort the list of control points
    while ((it = list.begin()) != list.end()) {
      QCanvasRectangle * smallest =
        (QCanvasRectangle *) this->smallestItem(&list);
      smallest->setBrush(Qt::black);
      sortedlist.append(smallest);
      list.remove(smallest);
    }
    
    int numpts = sortedlist.size();
    SbVec3f * ctrlpts = new SbVec3f[numpts];

    for (it =  sortedlist.begin(); it != sortedlist.end(); it++) {
        ctrlpts[i++] = SbVec3f((*it)->x() + this->ptsize, 255.0f - (*it)->y() - this->ptsize, 0.0f);
    }
    this->colorcurves[this->colorindex]->setControlPoints(ctrlpts, numpts);
    delete [] ctrlpts;
  }

  i = 2;
  const uint8_t * curvepts = this->colorcurves[this->colorindex]->getColorMap();
  it = this->curvesegments.begin();
  for (; it != this->curvesegments.end(); it++) {
    QCanvasLine* line = (QCanvasLine*)(*it);
    line->setPoints(i-2, 255-curvepts[i-2], i, 255-curvepts[i]);
    i+=2;
  }    
  emit this->curveChanged();
}

QCanvasItem * 
CurveView::smallestItem(QCanvasItemList * list)
{
  QCanvasItemList::Iterator it = list->begin();
  QCanvasItem * smallest = (*it);

  it++;
  for (; it != list->end(); ++it) {
    if ((*it)->x() < smallest->x()) {
      smallest = (*it);
    }
  }
  int x = smallest->x();
  return smallest;
}

void
CurveView::setMode(SoQtCurveWidget::Mode mode)
{
  for (int i = 0; i < this->colormode; i++) {
    delete this->colorcurves[i];
    QCanvasItemList::Iterator it = this->canvasctrlpts[i].begin();
    for (; it != this->canvasctrlpts[i].end(); it++) {
      delete *it;
    }
  }
  this->colorcurves.clear();
  this->canvasctrlpts.clear();

  this->colormode = mode;
  this->initColorCurves();
  this->hideUnselected();
}

void 
CurveView::changeColorMode(int mode)
{
  if (mode != this->colorindex) {
    this->colorindex = mode;
    this->hideUnselected();
    this->updateCurve();
    this->canvas->update();
  }
}

void
CurveView::changeCurveMode(int cmode)
{
  if (cmode != this->curvemode) {
    this->curvemode = (CurveType) cmode;
    if (this->curvemode == CurveView::SMOOTH) {
      this->interpolateFromColors();
    }
    this->hideUnselected();
    this->updateCurve();
    this->canvas->update();
  }
}

void
CurveView::interpolateFromColors()
{
  for (int i = 0; i < this->colormode; i++) {
    this->colorcurves[i]->interpolateColorMapping();
    const SbVec3f * ctrlpts = this->colorcurves[i]->getCtrlPoints();

    QCanvasItemList::Iterator it = this->canvasctrlpts[i].begin();
    for (; it != this->canvasctrlpts[i].end(); it++) {
      delete (*it);
    }
    this->canvasctrlpts[i].clear();
    for (int j = 0; j < this->colorcurves[i]->getNumCtrlPoints(); j++) {
      this->canvasctrlpts[i].append(this->newControlPoint(ctrlpts[j][0], 255.0f-ctrlpts[j][1]));
    }
  }
}

void
CurveView::setConstantValue(int value)
{
  this->colorcurves[this->colorindex]->fill(value);
  this->updateCurve();
  this->canvas->update();
}

void 
CurveView::getColors(uint8_t * colors, int num) const
{
  uint8_t * clrs = new uint8_t[num];
  for (int i = 0; i < this->colormode; i++) {
    this->colorcurves[i]->getColors(clrs, num);
    for (int j = 0; j < num; j++) {
      colors[j*(this->colormode) + i] = clrs[j];
    }
  }
  delete [] clrs;
}

void 
CurveView::setColors(uint8_t * colors, int num)
{
  uint8_t * clrs = new uint8_t[num];
  for (int i = 0; i < colormode; i++) {
    for (int j = 0; j < num; j++) {
      clrs[j] = colors[j*colormode + i];
    }
    this->colorcurves[i]->setColors(clrs, num);
  }
  delete [] clrs;
  this->changeCurveMode(CurveView::FREE);
}

QPixmap
CurveView::getPixmap(int width, int height) const
{
  QImage img(width, height, 32);
  QPixmap pm;
  if (this->colormode < 3) {
    const uint8_t * colors = this->colorcurves[0]->getColorMap();
    pm = this->makePixmap(width, height, colors, colors, colors, width);
  } else {
    const uint8_t * red = this->colorcurves[0]->getColorMap();
    const uint8_t * green = this->colorcurves[1]->getColorMap();
    const uint8_t * blue = this->colorcurves[2]->getColorMap();
    pm = this->makePixmap(width, height, red, green, blue, width);
  }
  return pm;
}

QPixmap
CurveView::makePixmap(int w, int h, const uint8_t * r, const uint8_t * g, const uint8_t * b, int num) const
{
  // use an image since it is optimized for direct pixel access
  QImage img(w, h, 32);
  for (int i = 0; i < w; i++) {
    int original = (int) ((float(i) / float(w)) * 255.0f);
    for (int j = 0; j < h/2; j++) {
      img.setPixel(i, j, qRgb(r[i], g[i], b[i]));
    }
    // we'll split the image in half, making the bottom
    // half show the original color mapping for comparison
    for (j = h/2; j < h; j++) {
      img.setPixel(i, j, qRgb(original, original, original));
    }
  }
  return QPixmap(img);
}

QPixmap
CurveView::getGradient(int width, int height) const
{
  QImage img(width, height, 32);

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      img.setPixel(j, i, qRgb(255-i, 255-i, 255-i));
    }
  }
  return QPixmap(img);
}

void
CurveView::setCallBack(ColorCurve::ChangeCB * cb, void * userData)
{
  for (int i = 0; i < this->colormode; i++) {
    this->colorcurves[i]->setChangeCallBack(cb, userData);
  }
}
