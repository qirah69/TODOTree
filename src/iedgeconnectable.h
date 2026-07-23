#pragma once

class EdgeItem;

// Lets EdgeItem notify a connected node when the edge is destroyed,
// without EdgeItem needing to know about NodeItem (which would create
// a circular #include between edgeitem.h and nodeitem.h).
class IEdgeConnectable {
public:
  virtual ~IEdgeConnectable() = default;
  virtual void addEdge(EdgeItem *edge) = 0;
  virtual void removeEdge(EdgeItem *edge) = 0;
};
