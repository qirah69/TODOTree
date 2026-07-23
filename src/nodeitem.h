#pragma once
#include "edgeitem.h"
#include "iedgeconnectable.h"
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QRandomGenerator>
#include <QStyleOptionGraphicsItem>
#include <algorithm>
#include <functional>

class NodeItem : public QGraphicsItem, public IEdgeConnectable {
private:
  QString nodeName;
  QList<EdgeItem *> edges; // Tracks all edges connected to this node 🔗
  bool manuallyCompleted = false; // Only meaningful for leaf nodes
  qreal jitter = 0.0; // Fixed per-node random offset (see verticalJitter())

  static constexpr qreal checkboxSize = 14.0;
  static constexpr qreal checkboxMargin = 8.0;
  static constexpr qreal jitterRange = 20.0; // +/- px

public:
  // Set by the application (e.g. in main.cpp) to something like
  // AutoLayout::layoutTree. NodeItem doesn't know AutoLayout exists —
  // this indirection is what avoids a circular #include between
  // nodeitem.h and autolayout.h.
  static inline std::function<void(NodeItem *)> onTreeChanged = nullptr;

  NodeItem(const QString &name, QGraphicsItem *parent = nullptr)
      : QGraphicsItem(parent), nodeName(name) {
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemIsFocusable);
    jitter = QRandomGenerator::global()->bounded(2.0 * jitterRange) -
             jitterRange;
  }

  // A fixed random vertical offset assigned once at construction, so
  // AutoLayout can space nodes a little unevenly without them jumping
  // around every time the tree re-lays itself out.
  qreal verticalJitter() const { return jitter; }

  void addEdge(EdgeItem *edge) override { edges.append(edge); }

  void removeEdge(EdgeItem *edge) override { edges.removeAll(edge); }

  const QList<EdgeItem *> &connectedEdges() const { return edges; }

  // Every node this one points to via an outgoing edge.
  QList<NodeItem *> children() const {
    QList<NodeItem *> result;
    for (EdgeItem *edge : edges) {
      if (edge->sourceNode() == this) {
        if (auto *child = dynamic_cast<NodeItem *>(edge->destNode()))
          result.append(child);
      }
    }
    return result;
  }

  // The node this one is a child of, or nullptr if this is the root.
  NodeItem *parentNode() const {
    for (EdgeItem *edge : edges) {
      if (edge->destNode() == this) {
        return dynamic_cast<NodeItem *>(edge->sourceNode());
      }
    }
    return nullptr;
  }

  bool isLeaf() const { return children().isEmpty(); }

  ~NodeItem() override {
    // Copy the list first: deleting an edge mutates `edges` as a side
    // effect (via removeEdge), so iterating the original list is unsafe.
    QList<EdgeItem *> edgesToDelete = edges;

    for (EdgeItem *edge : edgesToDelete) {
      if (edge->sourceNode() == this) {
        // We're the parent side: delete the edge and cascade-delete the
        // whole child subtree.
        NodeItem *childNode = dynamic_cast<NodeItem *>(edge->destNode());
        delete edge;
        if (childNode) {
          delete childNode;
        }
      } else {
        // We're the child side (this is our link to our parent): just
        // detach the edge. Deleting the parent here would be wrong.
        delete edge;
      }
    }
  }

  void setNodeName(const QString &name) {
    nodeName = name;
    update();
  }

  QString name() const { return nodeName; }

  // Leaf nodes: whatever the checkbox says. Everything else: true only if
  // every child is complete. Never stored for non-leaves, always derived,
  // so it can't drift out of sync with the children underneath it.
  bool isComplete() const {
    QList<NodeItem *> kids = children();
    if (kids.isEmpty())
      return manuallyCompleted;

    for (NodeItem *child : kids) {
      if (!child->isComplete())
        return false;
    }
    return true;
  }

  bool isManuallyCompleted() const { return manuallyCompleted; }

  // Used by TreePersistence when loading — restores the raw checkbox
  // state without going through the click-handling/repaint logic below.
  void setManuallyCompleted(bool value) {
    manuallyCompleted = value;
    update();
  }

  // Toggles the checkbox (leaf nodes only) and repaints every ancestor,
  // since their computed isComplete() may have just changed.
  void toggleComplete() {
    manuallyCompleted = !manuallyCompleted;
    update();
    for (NodeItem *p = parentNode(); p; p = p->parentNode()) {
      p->update();
    }
  }

  QRectF checkboxRect() const {
    qreal h = boundingRect().height();
    return QRectF(checkboxMargin, (h - checkboxSize) / 2.0, checkboxSize,
                  checkboxSize);
  }

  QRectF boundingRect() const override {
    QFont font;
    QFontMetrics metrics(font);

    // Measure text bounded by a maximum width of 200px
    QRect textRect =
        metrics.boundingRect(QRect(0, 0, 200, 0), Qt::TextWordWrap, nodeName);

    // Leaf nodes reserve extra space on the left for the checkbox.
    qreal checkboxSpace = isLeaf() ? (checkboxSize + checkboxMargin) : 0.0;

    // Ensure a minimum width of 100px, plus 20px padding
    int width = std::max(100, textRect.width()) + 20 + checkboxSpace;
    int height = std::max(textRect.height() + 20,
                          int(checkboxSize + 2 * checkboxMargin));

    return QRectF(0, 0, width, height);
  }

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    bool complete = isComplete();

    if (isSelected()) {
      painter->setPen(QPen(Qt::cyan, 2));
    } else {
      painter->setPen(QPen(Qt::white, 1));
    }

    painter->setBrush(complete ? QColor(40, 90, 55) : QColor(50, 50, 50));
    painter->drawRect(boundingRect());

    QRectF textArea = boundingRect();

    if (isLeaf()) {
      QRectF cb = checkboxRect();
      painter->setPen(QPen(Qt::white, 1));
      painter->setBrush(Qt::NoBrush);
      painter->drawRect(cb);
      if (manuallyCompleted) {
        painter->drawLine(cb.topLeft(), cb.bottomRight());
        painter->drawLine(cb.topRight(), cb.bottomLeft());
      }
      textArea.setLeft(cb.right() + checkboxMargin);
    }

    // Combine alignment and text wrapping flags 🚩
    painter->drawText(textArea, Qt::AlignCenter | Qt::TextWordWrap, nodeName);
  }

  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override {
    Q_UNUSED(event);
    NodeItem *newNode = new NodeItem("New Node");
    scene()->addItem(newNode);
    EdgeItem *newEdge =
        new EdgeItem(this, newNode); // registers itself with both nodes
    scene()->addItem(newEdge);

    // Re-layout the whole tree so existing siblings shift to make room,
    // instead of the new node landing on top of whatever's already there.
    if (onTreeChanged) {
      NodeItem *root = this;
      while (NodeItem *p = root->parentNode())
        root = p;
      onTreeChanged(root);
    }
  }

  void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
    if (isLeaf() && checkboxRect().contains(event->pos())) {
      toggleComplete();
      return; // consume the click — don't also start a drag/selection
    }
    setFocus(); // Focus this item for keyboard events like F2 ⌨️
    QGraphicsItem::mousePressEvent(
        event); // Call base implementation to keep selection/dragging working
  }

  void keyPressEvent(QKeyEvent *event) override {
    if (event->key() == Qt::Key_F2) {
      bool ok;
      QString newName =
          QInputDialog::getText(nullptr,           // Parent widget
                                 "Rename Node",     // Dialog title
                                 "Enter new name:", // Label text
                                 QLineEdit::Normal, // Input mode
                                 nodeName, // Default text (current name)
                                 &ok // Stores whether the user clicked 'OK'
          );
      if (ok && !newName.isEmpty()) {
        setNodeName(newName);
      }
    } else if (event->key() == Qt::Key_Delete ||
               event->key() == Qt::Key_Backspace) {
      delete this; // Triggers ~NodeItem(), which cleans up edges and child
                   // nodes recursively 🗑️
      return;      // Exit immediately so Qt doesn't process events on a deleted
                   // item!
    } else {
      QGraphicsItem::keyPressEvent(event); // Call base implementation
    }
  }
};
