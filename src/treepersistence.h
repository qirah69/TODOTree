#pragma once
#include "edgeitem.h"
#include "nodeitem.h"
#include <QFile>
#include <QGraphicsScene>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

class TreePersistence {
public:
  static bool saveToFile(QGraphicsScene *scene, const QString &filePath) {
    QJsonArray nodesArray;
    QJsonArray edgesArray;
    QHash<NodeItem *, int> nodeIds;

    // Pass 1: give every node a fresh sequential id and record its data.
    // IDs only need to be consistent within this one file, not across saves.
    int nextId = 0;
    for (QGraphicsItem *item : scene->items()) {
      if (auto *node = dynamic_cast<NodeItem *>(item)) {
        nodeIds[node] = nextId;

        QJsonObject nodeObj;
        nodeObj["id"] = nextId;
        nodeObj["name"] = node->name();
        nodeObj["x"] = node->pos().x();
        nodeObj["y"] = node->pos().y();
        nodeObj["completed"] = node->isManuallyCompleted();
        nodesArray.append(nodeObj);

        nextId++;
      }
    }

    // Pass 2: write edges using the id map built above.
    for (QGraphicsItem *item : scene->items()) {
      if (auto *edge = dynamic_cast<EdgeItem *>(item)) {
        auto *src = dynamic_cast<NodeItem *>(edge->sourceNode());
        auto *dst = dynamic_cast<NodeItem *>(edge->destNode());
        if (!src || !dst)
          continue; // half-deleted / dangling edge, skip it

        QJsonObject edgeObj;
        edgeObj["source"] = nodeIds.value(src, -1);
        edgeObj["dest"] = nodeIds.value(dst, -1);
        edgesArray.append(edgeObj);
      }
    }

    QJsonObject root;
    root["nodes"] = nodesArray;
    root["edges"] = edgesArray;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
      return false;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
  }

  static bool loadFromFile(QGraphicsScene *scene, const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
      return false;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
      return false;

    QJsonObject root = doc.object();
    QJsonArray nodesArray = root.value("nodes").toArray();
    QJsonArray edgesArray = root.value("edges").toArray();

    scene->clear(); // wipe whatever's currently displayed

    QHash<int, NodeItem *> idToNode;

    // Pass 1: create every node before touching edges — an edge in the
    // array can reference a node that hasn't been parsed yet.
    for (const QJsonValue &val : nodesArray) {
      QJsonObject obj = val.toObject();
      int id = obj.value("id").toInt();
      QString name = obj.value("name").toString();
      qreal x = obj.value("x").toDouble();
      qreal y = obj.value("y").toDouble();

      auto *node = new NodeItem(name);
      node->setPos(x, y);
      node->setManuallyCompleted(obj.value("completed").toBool());
      scene->addItem(node);
      idToNode[id] = node;
    }

    // Pass 2: now every node exists, so it's safe to wire up edges.
    for (const QJsonValue &val : edgesArray) {
      QJsonObject obj = val.toObject();
      int sourceId = obj.value("source").toInt();
      int destId = obj.value("dest").toInt();

      NodeItem *sourceNode = idToNode.value(sourceId, nullptr);
      NodeItem *destNode = idToNode.value(destId, nullptr);
      if (!sourceNode || !destNode)
        continue; // malformed file: reference to a missing node, skip it

      auto *edge = new EdgeItem(sourceNode, destNode); // self-registers
      scene->addItem(edge);
    }

    return true;
  }
};
