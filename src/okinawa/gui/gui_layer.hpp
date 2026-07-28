#ifndef OK_GUI_LAYER_HPP
#define OK_GUI_LAYER_HPP

#include <string>
#include <vector>

class OkItem;

/**
 * @brief One GUI depth layer: a named, ordered list of OkItems drawn by the
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

  // Add an item (takes ownership). Returns the same pointer for chaining.
  OkItem *addItem(OkItem *item);

  // Remove and DELETE the item. True if it was found.
  bool removeItem(OkItem *item);

  int getItemCount() const { return (int)_items.size(); }

  // Draw every visible item in insertion order. Called by the GUI pass.
  void draw();

private:
  std::string           _name;
  int                   _order;
  bool                  _visible;
  std::vector<OkItem *> _items;  // owned
};

#endif
