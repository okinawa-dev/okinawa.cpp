#include "gui_layer.hpp"
#include "../item/item.hpp"

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
 * @brief Add an item to the layer (the layer takes ownership).
 */
OkItem *OkGuiLayer::addItem(OkItem *item) {
  if (item != nullptr) {
    _items.push_back(item);
  }
  return item;
}

/**
 * @brief Remove and delete an item. Returns true if it belonged to the layer.
 */
bool OkGuiLayer::removeItem(OkItem *item) {
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
 * @brief Draw every item in insertion order (painter's order within the
 *        layer). Visibility of individual items is honoured by OkItem.
 */
void OkGuiLayer::draw() {
  if (!_visible) {
    return;
  }
  for (std::size_t i = 0; i < _items.size(); i++) {
    _items[i]->draw();
  }
}
