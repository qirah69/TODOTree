#include "autolayout.h"
#include "nodeitem.h"
#include "treepersistence.h"
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QShortcut>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // Whenever a node creates a child (double-click), it'll call this so
  // the whole tree re-lays itself out. NodeItem itself has no idea
  // AutoLayout exists — this line is the only place that connects them.
  NodeItem::onTreeChanged = &AutoLayout::layoutTree;

  // 1. Create Scene
  auto *scene = new QGraphicsScene();
  scene->setBackgroundBrush(QColor(30, 30, 30)); // Dark background
  scene->setSceneRect(0, 0, 1920, 1080); // Set scene rect to allow panning

  // 2. Add sample node
  scene->addItem(new NodeItem("Root Node"));
  // 3. Create View
  QGraphicsView view(scene);
  view.setWindowTitle("Task Tree Node Engine - Step 1");

  // -------------------------------------------------------------
  // FIX FOR GHOST TRAILS / TEARING:
  // -------------------------------------------------------------
  // Force Qt to redraw the full window during fast mouse movements
  view.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

  // Enable Antialiasing for smooth renders
  view.setRenderHint(QPainter::Antialiasing);
  view.setRenderHint(QPainter::SmoothPixmapTransform);
  // -------------------------------------------------------------

  // -------------------------------------------------------------
  // SAVE / LOAD (Ctrl+S / Ctrl+O)
  // -------------------------------------------------------------
  QShortcut *saveShortcut = new QShortcut(QKeySequence::Save, &view);
  QObject::connect(saveShortcut, &QShortcut::activated, [scene, &view]() {
    QString filePath = QFileDialog::getSaveFileName(
        &view, "Save Task Tree", QString(), "JSON Files (*.json)");
    if (filePath.isEmpty())
      return; // user hit Cancel

    if (!filePath.endsWith(".json", Qt::CaseInsensitive))
      filePath += ".json";

    if (!TreePersistence::saveToFile(scene, filePath)) {
      qWarning() << "Failed to save to" << filePath;
    }
  });

  QShortcut *loadShortcut = new QShortcut(QKeySequence::Open, &view);
  QObject::connect(loadShortcut, &QShortcut::activated, [scene, &view]() {
    QString filePath = QFileDialog::getOpenFileName(
        &view, "Load Task Tree", QString(), "JSON Files (*.json)");
    if (filePath.isEmpty())
      return; // user hit Cancel

    if (!TreePersistence::loadFromFile(scene, filePath)) {
      qWarning() << "Failed to load from" << filePath;
    }
  });
  // -------------------------------------------------------------

  view.show();
  return app.exec();
}
