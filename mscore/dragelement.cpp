//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "log.h"

#include "scoreview.h"
#include "libmscore/score.h"
#include "musescore.h"
#include "libmscore/staff.h"
#include "libmscore/utils.h"
#include "libmscore/undo.h"
#include "libmscore/part.h"
#include "tourhandler.h"

namespace Ms {

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void ScoreView::startDrag()
      {
      editData.grips = 0;
      editData.clearData();
      editData.normalizedStartMove = editData.startMove - editData.element->offset();

      _score->startCmd();

      for (Element* e : _score->selection().elements())
            e->startDrag(editData);

      _score->selection().lock("drag");
      }

//---------------------------------------------------------
//   doDragElement
//---------------------------------------------------------

void ScoreView::doDragElement(QMouseEvent* ev)
      {
      const QPointF logicalPos = toLogical(ev->pos());
      QPointF delta = logicalPos - editData.normalizedStartMove;
      QPointF evtDelta = logicalPos - editData.pos;

      TourHandler::startTour("autoplace-tour");

      QPointF pt(delta);
      if (qApp->keyboardModifiers() == Qt::ShiftModifier) {
            pt.setX(editData.delta.x());
            evtDelta.setX(0.0);
            }
      else if (qApp->keyboardModifiers() == Qt::ControlModifier) {
            pt.setY(editData.delta.y());
            evtDelta.setY(0.0);
            }

      editData.lastPos = editData.pos;
      editData.hRaster = mscore->hRaster();
      editData.vRaster = mscore->vRaster();
      editData.delta   = pt;
      editData.moveDelta = pt + (editData.normalizedStartMove - editData.startMove); // TODO: restructure
      editData.evtDelta = evtDelta;
      editData.pos     = logicalPos;

      const Selection& sel = _score->selection();
      const bool filterType = sel.isRange();
      const ElementType type = editData.element->type();

      for (Element* e : sel.elements()) {
            if (filterType && type != e->type())
                  continue;
            _score->addRefresh(e->drag(editData));
            }

      Element* e = _score->getSelectedElement();
      if (e) {
            if (_score->playNote()) {
                  mscore->play(e);
                  _score->setPlayNote(false);
                  }
            _score->update();

            QVector<QLineF> anchorLines = e->dragAnchorLines();
            const QPointF pageOffset(e->findAncestor(ElementType::PAGE)->pos());

            if (!anchorLines.isEmpty()) {
                  for (QLineF& l : anchorLines)
                        l.translate(pageOffset);
                  setDropAnchorLines(anchorLines);
                  }
            else
                  setDropTarget(0); // this also resets dropAnchor
            }
      updateGrips();
      _score->update();
      }

//---------------------------------------------------------
//   endDrag
//---------------------------------------------------------

void ScoreView::endDrag()
      {
      for (Element* e : _score->selection().elements()) {
            e->endDrag(editData);
            e->triggerLayout();
            }

      _score->selection().unlock("drag");
      setDropTarget(0); // this also resets dropAnchor
      _score->endCmd();
      updateGrips();
      if (editData.element->normalModeEditBehavior() == Element::EditBehavior::Edit && _score->selection().element() == editData.element)
            startEdit(/* editMode */ false);
      }
}

