#ifndef OK_GUI_LAYER_HPP
#define OK_GUI_LAYER_HPP

#include <string>
#include <vector>

class OkObject;

/**
 * @brief One GUI depth layer: a named, ordered list of elements drawn by the
 *        GUI pass. Layers are rendered from the LOWEST order to the highest
 *        (far to near): a higher order paints on top. Within a layer, items
 *        draw in insertion order. Everything sits at the same real Z; depth
 *        is paint order.
 *
 *        The layer OWNS the items added to it and deletes them when
 *        destroyed (or when removeItem is called).
 */
class OkGuiLayer {
public:
  OkGuiLayer(const std::string &name, int order);
  ~OkGuiLayer();

  // Prevent copies (items are owned raw pointers).
  OkGuiLayer(const OkGuiLayer &)            = delete;
  OkGuiLayer &operator=(const OkGuiLayer &) = delete;

  const std::string &getName() const { return _name; }

  int  getOrder() const { return _order; }
  void setOrder(int order) { _order = order; }

  bool getVisible() const { return _visible; }
  void setVisible(bool visible) { _visible = visible; }

  // Add an element (takes ownership; any OkObject: items, graphs,
  // composite elements). Returns the same pointer for chaining.
  OkObject *addItem(OkObject *item);

  // Remove and DELETE the element. True if it was found.
  bool removeItem(OkObject *item);

  int getItemCount() const { return (int)_items.size(); }

  // Find an owned element by its object name (null when missing).
  OkObject *getItemByName(const std::string &name);

  // Draw every visible item in insertion order. Called by the GUI pass.
  void draw();

private:
  std::string           _name;
  int                   _order;
  bool                  _visible;
  std::vector<OkObject *> _items;  // owned
};

#endif
