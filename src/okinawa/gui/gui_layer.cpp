#include "gui_layer.hpp"
#include "../core/object.hpp"

OkGuiLayer::OkGuiLayer(const std::string &name, int order) {
  _name    = name;
  _order   = order;
  _visible = true;
}

OkGuiLayer::~OkGuiLayer() {
  for (std::size_t i = 0; i < _items.size(); i++) {
    delete _items[i];
  }
  _items.clear();
}

/**
 * @brief Add an element to the layer (the layer takes ownership).
 */
OkObject *OkGuiLayer::addItem(OkObject *item) {
  if (item != nullptr) {
    _items.push_back(item);
  }
  return item;
}

/**
 * @brief Remove and delete an element. True if it belonged to the layer.
 */
bool OkGuiLayer::removeItem(OkObject *item) {
  for (std::size_t i = 0; i < _items.size(); i++) {
    if (_items[i] == item) {
      delete _items[i];
      _items.erase(_items.begin() + (long)i);
      return true;
    }
  }
  return false;
}

/**
 * @brief Find an owned element by its object name.
 */
OkObject *OkGuiLayer::getItemByName(const std::string &name) {
  for (std::size_t i = 0; i < _items.size(); i++) {
    if (_items[i]->getName() == name) {
      return _items[i];
    }
  }
  return nullptr;
}

/**
 * @brief Draw every element in insertion order (painter's order within
 *        the layer). Per-element visibility is honoured by the elements.
 */
void OkGuiLayer::draw() {
  if (!_visible) {
    return;
  }
  for (std::size_t i = 0; i < _items.size(); i++) {
    _items[i]->draw();
  }
}
