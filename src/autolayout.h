#pragma once
#include "nodeitem.h"
#include <QList>
#include <QPointF>

class AutoLayout {
public:
  static constexpr qreal horizontalSpacing = 140.0;
  static constexpr qreal verticalSpacing = 120.0;

  // Lays out the whole tree rooted at `root`, positioning every node so
  // siblings don't overlap and parents sit centered above their children.
  static void layoutTree(NodeItem *root) {
    if (!root)
      return;
    int nextLeafSlot = 0;
    layoutSubtree(root, 0, nextLeafSlot);
  }

private:
  // Positions `node` (and recursively, everything below it), returns the x
  // it was assigned. `depth` sets y. `nextLeafSlot` is shared across the
  // whole walk so leaves never collide.
  static qreal layoutSubtree(NodeItem *node, int depth, int &nextLeafSlot) {
    QList<NodeItem *> children = node->children();

    qreal x;
    if (children.isEmpty()) {
      // Base case: leaf claims the next free horizontal slot.
      x = nextLeafSlot * horizontalSpacing;
      nextLeafSlot++;
    } else {
      // Recursive case: children first (post-order), then center over them.
      qreal sum = 0;
      for (NodeItem *child : children) {
        sum += layoutSubtree(child, depth + 1, nextLeafSlot);
      }
      x = sum / children.size();
    }

    qreal y = depth * verticalSpacing + node->verticalJitter();
    node->setPos(x, y);
    return x;
  }
};
