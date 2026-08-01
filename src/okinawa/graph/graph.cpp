#include "graph.hpp"

#include "../core/gl_config.hpp"  // GL enums + glPointSize
#include "../item/item.hpp"

#include <cmath>
#include <vector>

OkGraph::OkGraph(const std::string &name) : OkObject(name) {
  _edgesItem    = nullptr;
  _nodesItem    = nullptr;
  _edgeColor[0] = 1.0f;
  _edgeColor[1] = 1.0f;
  _edgeColor[2] = 1.0f;
  _nodeColor[0] = 1.0f;
  _nodeColor[1] = 1.0f;
  _nodeColor[2] = 1.0f;
  _showEdges    = true;
  _showNodes    = false;
  _nodeSize     = 4.0f;
  _mode         = RENDER_LINES;
  _edgeWidth    = 2.0f;
  _nodeMarker   = 3.0f;
}

OkGraph::~OkGraph() {
  destroyItems();
}

void OkGraph::destroyItems() {
  if (_edgesItem) {
    _edgesItem->detachFromParent();
    delete _edgesItem;
    _edgesItem = nullptr;
  }
  if (_nodesItem) {
    _nodesItem->detachFromParent();
    delete _nodesItem;
    _nodesItem = nullptr;
  }
}

int OkGraph::addNode(const OkPoint &position) {
  _nodes.push_back(position);
  return static_cast<int>(_nodes.size()) - 1;
}

void OkGraph::addEdge(int a, int b) {
  Edge e;
  e.a = a;
  e.b = b;
  _edges.push_back(e);
}

void OkGraph::clear() {
  _nodes.clear();
  _edges.clear();
}

void OkGraph::setEdgeColor(float r, float g, float b) {
  _edgeColor[0] = r;
  _edgeColor[1] = g;
  _edgeColor[2] = b;
  if (_edgesItem) {
    _edgesItem->setFillColor(r, g, b);
    _edgesItem->setUnlit(true);  // debug layer: exact colour
    _edgesItem->setWireframeColor(r * 0.35f, g * 0.35f, b * 0.35f);
  }
}

void OkGraph::setNodeColor(float r, float g, float b) {
  _nodeColor[0] = r;
  _nodeColor[1] = g;
  _nodeColor[2] = b;
  if (_nodesItem) {
    _nodesItem->setFillColor(r, g, b);
    _nodesItem->setUnlit(true);  // debug layer: exact colour
    _nodesItem->setWireframeColor(r * 0.35f, g * 0.35f, b * 0.35f);
  }
}

void OkGraph::setShowEdges(bool show) {
  _showEdges = show;
  if (_edgesItem) {
    _edgesItem->setVisible(show);
  }
}

void OkGraph::setShowNodes(bool show) {
  _showNodes = show;
  if (_nodesItem) {
    _nodesItem->setVisible(show);
  }
}

void OkGraph::rebuild() {
  destroyItems();
  if (_nodes.empty()) {
    return;
  }
  if (_mode == RENDER_POLYGONS) {
    buildPolygons();
  } else {
    buildLines();
  }
}

void OkGraph::buildLines() {
  // Shared vertex buffer: position + a dummy UV (OkItem's vertex stride is 5).
  std::vector<float> verts;
  verts.reserve(_nodes.size() * 5);
  for (std::size_t i = 0; i < _nodes.size(); i++) {
    verts.push_back(_nodes[i].x());
    verts.push_back(_nodes[i].y());
    verts.push_back(_nodes[i].z());
    verts.push_back(0.0f);
    verts.push_back(0.0f);
  }

  // Edges as a line list.
  if (!_edges.empty()) {
    std::vector<unsigned int> edgeIdx;
    edgeIdx.reserve(_edges.size() * 2);
    for (std::size_t i = 0; i < _edges.size(); i++) {
      edgeIdx.push_back(static_cast<unsigned int>(_edges[i].a));
      edgeIdx.push_back(static_cast<unsigned int>(_edges[i].b));
    }
    _edgesItem = new OkItem(getName() + "_edges", verts.data(),
                            static_cast<long>(verts.size()), edgeIdx.data(),
                            static_cast<long>(edgeIdx.size()));
    _edgesItem->setDrawMode(GL_LINES);
    _edgesItem->setFillColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
    _edgesItem->setUnlit(true);  // debug layer: exact colour
    _edgesItem->setVisible(_showEdges);
    attach(_edgesItem);
  }

  // Nodes as a point list.
  std::vector<unsigned int> nodeIdx;
  nodeIdx.reserve(_nodes.size());
  for (std::size_t i = 0; i < _nodes.size(); i++) {
    nodeIdx.push_back(static_cast<unsigned int>(i));
  }
  _nodesItem = new OkItem(getName() + "_nodes", verts.data(),
                          static_cast<long>(verts.size()), nodeIdx.data(),
                          static_cast<long>(nodeIdx.size()));
  _nodesItem->setDrawMode(GL_POINTS);
  _nodesItem->setFillColor(_nodeColor[0], _nodeColor[1], _nodeColor[2]);
    _nodesItem->setUnlit(true);  // debug layer: exact colour
  _nodesItem->setVisible(_showNodes);
  attach(_nodesItem);
}

void OkGraph::buildPolygons() {
  const float eps = 1e-4f;

  // Edges as filled ribbons (a quad per edge, the width laid out in XZ).
  if (!_edges.empty()) {
    std::vector<float>        ev;
    std::vector<unsigned int> ei;
    float                     hw = _edgeWidth * 0.5f;
    for (std::size_t i = 0; i < _edges.size(); i++) {
      const OkPoint &pa = _nodes[_edges[i].a];
      const OkPoint &pb = _nodes[_edges[i].b];
      float          dx = pb.x() - pa.x();
      float          dz = pb.z() - pa.z();
      float          ln = std::sqrt(dx * dx + dz * dz);
      if (ln < eps) {
        continue;
      }
      float          nx   = -dz / ln * hw;
      float          nz   = dx / ln * hw;
      unsigned int   base = static_cast<unsigned int>(ev.size() / 5);
      float          quad[4][3] = {{pa.x() + nx, pa.y(), pa.z() + nz},
                                   {pa.x() - nx, pa.y(), pa.z() - nz},
                                   {pb.x() - nx, pb.y(), pb.z() - nz},
                                   {pb.x() + nx, pb.y(), pb.z() + nz}};
      for (int k = 0; k < 4; k++) {
        ev.push_back(quad[k][0]);
        ev.push_back(quad[k][1]);
        ev.push_back(quad[k][2]);
        ev.push_back(0.0f);
        ev.push_back(0.0f);
      }
      ei.push_back(base);
      ei.push_back(base + 1);
      ei.push_back(base + 2);
      ei.push_back(base);
      ei.push_back(base + 2);
      ei.push_back(base + 3);
    }
    if (!ei.empty()) {
      _edgesItem = new OkItem(getName() + "_edges", ev.data(),
                              static_cast<long>(ev.size()), ei.data(),
                              static_cast<long>(ei.size()));
      _edgesItem->setFillColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
    _edgesItem->setUnlit(true);  // debug layer: exact colour
      _edgesItem->setWireframe(true);  // outline over the fill
      _edgesItem->setWireframeColor(_edgeColor[0] * 0.35f, _edgeColor[1] * 0.35f,
                                    _edgeColor[2] * 0.35f);
      _edgesItem->setVisible(_showEdges);
      attach(_edgesItem);
    }
  }

  // Nodes as filled quads centred on each node (laid out in XZ).
  std::vector<float>        nv;
  std::vector<unsigned int> ni;
  float                     hm = _nodeMarker * 0.5f;
  for (std::size_t i = 0; i < _nodes.size(); i++) {
    const OkPoint &p    = _nodes[i];
    unsigned int   base = static_cast<unsigned int>(nv.size() / 5);
    float          quad[4][3] = {{p.x() - hm, p.y(), p.z() - hm},
                                 {p.x() + hm, p.y(), p.z() - hm},
                                 {p.x() + hm, p.y(), p.z() + hm},
                                 {p.x() - hm, p.y(), p.z() + hm}};
    for (int k = 0; k < 4; k++) {
      nv.push_back(quad[k][0]);
      nv.push_back(quad[k][1]);
      nv.push_back(quad[k][2]);
      nv.push_back(0.0f);
      nv.push_back(0.0f);
    }
    ni.push_back(base);
    ni.push_back(base + 1);
    ni.push_back(base + 2);
    ni.push_back(base);
    ni.push_back(base + 2);
    ni.push_back(base + 3);
  }
  _nodesItem = new OkItem(getName() + "_nodes", nv.data(),
                          static_cast<long>(nv.size()), ni.data(),
                          static_cast<long>(ni.size()));
  _nodesItem->setFillColor(_nodeColor[0], _nodeColor[1], _nodeColor[2]);
    _nodesItem->setUnlit(true);  // debug layer: exact colour
  _nodesItem->setWireframe(true);  // outline over the fill
  _nodesItem->setWireframeColor(_nodeColor[0] * 0.35f, _nodeColor[1] * 0.35f,
                                _nodeColor[2] * 0.35f);
  _nodesItem->setVisible(_showNodes);
  attach(_nodesItem);
}

void OkGraph::drawSelf() {
  // Children (the nodes item) draw right after this; set their point size here.
  glPointSize(_nodeSize);
}
