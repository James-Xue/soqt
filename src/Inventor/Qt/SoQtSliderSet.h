/**************************************************************************\
 *
 *  Copyright (C) 1998-1999 by Systems in Motion.  All rights reserved.
 *
 *  This file is part of the Coin library.
 *
 *  This file may be distributed under the terms of the Q Public License
 *  as defined by Troll Tech AS of Norway and appearing in the file
 *  LICENSE.QPL included in the packaging of this file.
 *
 *  If you want to use Coin in applications not covered by licenses
 *  compatible with the QPL, you can contact SIM to aquire a
 *  Professional Edition license for Coin.
 *
 *  Systems in Motion AS, Prof. Brochs gate 6, N-7030 Trondheim, NORWAY
 *  http://www.sim.no/ sales@sim.no Voice: +47 22114160 Fax: +47 67172912
 *
\**************************************************************************/

//  $Id$

#ifndef SOQT_SLIDERSET_H
#define SOQT_SLIDERSET_H

#include <Inventor/Qt/SoQtSliderSetBase.h>

class SOQT_DLL_EXPORT SoQtSliderSet : public SoQtSliderSetBase
{
  typedef SoQtSliderSetBase inherited;

  Q_OBJECT

public:

protected:
  SoQtSliderSet(QWidget * parent,
                const char * name,
                SbBool buildInsideParent,
                SoNode * editnode);
  ~SoQtSliderSet();
};

#endif // ! SOQT_SLIDERSET_H
