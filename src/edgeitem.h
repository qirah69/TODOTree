#pragma once
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <algorithm>
#include <cmath>
#include <limits>

#include "iedgeconnectable.h"

class EdgeItem : public QGraphicsItem {
private:
  QGraphicsItem *source;
  QGraphicsItem *dest;

  // Given a rect (in some item's local coords) and a target point (in that
  // same local coords), returns where the ray from rect.center() to target
  // crosses the rect's border. This is what makes edges "dock" at the node
  // boundary instead of terminating at the center.
  static QPointF borderPoint(const QRectF &rect, const QPointF &target) {
    const QPointF center = rect.center();
    const qreal dx = target.x() - center.x();
    const qreal dy = target.y() - center.y();

    if (dx == 0.0 && dy == 0.0)
      return center;

    const qreal halfW = rect.width() / 2.0;
    const qreal halfH = rect.height() / 2.0;
    const qreal inf = std::numeric_limits<qreal>::max();

    const qreal tx = (dx != 0.0) ? halfW / std::abs(dx) : inf;
    const qreal ty = (dy != 0.0) ? halfH / std::abs(dy) : inf;
    const qreal t = std::min(tx, ty);

    return QPointF(center.x() + dx * t, center.y() + dy * t);
  }

  // Returns the docking points for the current source/dest, in this edge's
  // own local coordinate system. Caller must have already checked
  // source/dest are non-null.
  QLineF dockedLine() const {
    const QPointF sourceCenterScene =
        source->mapToScene(source->boundingRect().center());
    const QPointF destCenterScene =
        dest->mapToScene(dest->boundingRect().center());

    const QPointF destCenterInSourceLocal =
        source->mapFromScene(destCenterScene);
    const QPointF sourceEdgeLocal =
        borderPoint(source->boundingRect(), destCenterInSourceLocal);
    const QPointF sourceEdgeScene = source->mapToScene(sourceEdgeLocal);

    const QPointF sourceCenterInDestLocal =
        dest->mapFromScene(sourceCenterScene);
    const QPointF destEdgeLocal =
        borderPoint(dest->boundingRect(), sourceCenterInDestLocal);
    const QPointF destEdgeScene = dest->mapToScene(destEdgeLocal);

    return QLineF(mapFromScene(sourceEdgeScene), mapFromScene(destEdgeScene));
  }

public:
  EdgeItem(QGraphicsItem *sourceNode, QGraphicsItem *destNode,
           QGraphicsItem *parent = nullptr)
      : QGraphicsItem(parent), source(sourceNode), dest(destNode) {
    setZValue(-1); // Ensure edges are drawn behind nodes

    // Register this edge with both endpoints automatically, so callers
    // can't forget to (previously required two manual addEdge() calls).
    if (auto *s = dynamic_cast<IEdgeConnectable *>(source))
      s->addEdge(this);
    if (auto *d = dynamic_cast<IEdgeConnectable *>(dest))
      d->addEdge(this);
  }

  void removeSource() { source = nullptr; }
  void removeDest() { dest = nullptr; }

  ~EdgeItem() override {
    // Tell whichever endpoints are still alive to forget about this edge,
    // so they don't keep a dangling pointer to it in their edge lists.
    if (source) {
      if (auto *connectable = dynamic_cast<IEdgeConnectable *>(source))
        connectable->removeEdge(this);
    }
    if (dest) {
      if (auto *connectable = dynamic_cast<IEdgeConnectable *>(dest))
        connectable->removeEdge(this);
    }
  }

  QRectF boundingRect() const override {
    if (!source || !dest)
      return QRectF();
    // Must match what paint() actually draws, or the bounding rect won't
    // contain the line (causing redraw/clipping glitches). A little padding
    // covers the pen width.
    QLineF line = dockedLine();
    return QRectF(line.p1(), line.p2()).normalized().adjusted(-2, -2, 2, 2);
  }

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (!source || !dest)
      return;

    painter->setPen(QPen(Qt::white, 1));
    painter->drawLine(dockedLine());
  }

  void deleteEdge() {
    delete this; // QGraphicsItem's destructor auto-removes it from the scene
  }

  QGraphicsItem *sourceNode() const { return source; }
  QGraphicsItem *destNode() const { return dest; }
};
