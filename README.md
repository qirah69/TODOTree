# 🌳 TODOTree

> An interactive, visual task-decomposition engine built with C++ and Qt Graphics View Framework. Break down complex problems into manageable sub-tasks using hierarchical tree graphs.

![C++](https://img.shields.io/badge/C%2B%2B-17%2F20-blue.svg)
![Qt Framework](https://img.shields.io/badge/Qt-6.x%20%7C%205.15-green.svg)
![Build](https://img.shields.io/badge/CMake-3.16%2B-orange.svg)
![License](https://img.shields.io/badge/License-MIT-brightgreen.svg)

---

## 🌟 Key Features

* **⚡ Interactive Task Nodes**:
  * **Dynamic Box Sizing**: Nodes automatically calculate bounding rectangles based on text length and word wrapping.
  * **Inline Editing**: Press `F2` while focusing a node to rename it via a native dialog.
  * **Sub-task Spawning**: Double-click any node to instantly spawn a connected child node.

* **🎯 Smart Edge Docking**:
  * Connectors intelligently calculate ray-box intersections (`borderPoint`), snapping line endpoints directly to node boundaries rather than bleeding into the center.

* **📐 Post-Order Auto-Layout Engine**:
  * Automatic recursive spatial positioning prevents sibling overlap while keeping parent nodes centered above their subtrees.
  * Organic visual feel with non-repetitive vertical jitter.

* **✅ Dynamic Progress Cascading**:
  * **Leaf Completion**: Manual checkbox toggle for leaf tasks.
  * **Hierarchical State**: Parent task completion status is derived dynamically—a parent completes **only** when all underlying sub-tasks are completed.

* **🧹 Memory-Safe Cascading Deletion**:
  * Top-down recursive cleanup: Deleting a node recursively purges its children and associated edge connectors.
  * Decoupled architecture using `IEdgeConnectable` to eliminate dangling pointers and circular `#include` headers.

* **💾 JSON Scene Persistence**:
  * Save and load tree structures, layouts, titles, and completion states seamlessly using `Ctrl+S` and `Ctrl+O`.

---

## ⌨️ Controls & Shortcuts

| Action | Shortcut / Gesture |
| :--- | :--- |
| **Spawn Sub-Task** | `Double-Click` on a Node |
| **Rename Node** | Focus Node + `F2` |
| **Delete Subtree** | Focus Node + `Delete` / `Backspace` |
| **Toggle Leaf Task** | `Left-Click` Checkbox |
| **Save Diagram** | `Ctrl + S` |
| **Load Diagram** | `Ctrl + O` |
| **Pan & Focus** | Click and Drag Items |

---

## 🏗️ Architecture Overview

```text
                +-------------------+
                | IEdgeConnectable |
                +---------+---------+
                          ^
                          | (implements)
  +--------------+  1   * |       +--------------+
  |   EdgeItem   |<-------+------>|   NodeItem   |
  +--------------+                +--------------+
         |                                | (triggers via function ptr)
         v                                v
+------------------+            +------------------+
| TreePersistence  |            |    AutoLayout    |
+------------------+            +------------------+
