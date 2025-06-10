#ifndef OK_ITEM_GROUP_HPP
#define OK_ITEM_GROUP_HPP

#include "../core/object.hpp"
#include "item.hpp"
#include <cstddef>
#include <string>
#include <vector>

/**
 * @brief Class representing a group of OkItems that can be managed and rendered
 *        as a single unit. Items can be tagged for selective visibility
 * control.
 */
class OkItemGroup : public OkObject {
private:
  // Structure to hold an item with its associated tags
  struct OkTaggedItem {
    OkItem                  *item;
    std::vector<std::string> tags;

    OkTaggedItem(OkItem *itm, const std::vector<std::string> &itemTags = {})
        : item(itm), tags(itemTags) {}
  };

  std::vector<OkTaggedItem> items;

protected:
  void drawSelf() override;
  void stepSelf(float dt) override;
  void updateTransformSelf() override;

public:
  // Constructors
  explicit OkItemGroup(const std::string &name);
  ~OkItemGroup();

  // Delete copy constructor and assignment
  OkItemGroup(const OkItemGroup &)            = delete;
  OkItemGroup &operator=(const OkItemGroup &) = delete;

  // Item management
  void addItem(OkItem *item, const std::vector<std::string> &tags = {});
  void addItem(OkItem *item, const std::string &tag);
  void removeItem(OkItem *item);
  void removeItemByIndex(int index);
  void clearItems();

  // Tag management
  void addTagToItem(int itemIndex, const std::string &tag);
  void addTagToItem(OkItem *item, const std::string &tag);
  void removeTagFromItem(int itemIndex, const std::string &tag);
  void removeTagFromItem(OkItem *item, const std::string &tag);
  void setItemTags(int itemIndex, const std::vector<std::string> &tags);
  void setItemTags(OkItem *item, const std::vector<std::string> &tags);

  // Query methods
  size_t                   getItemCount() const { return items.size(); }
  OkItem                  *getItem(int index) const;
  int                      getItemIndex(OkItem *item) const;
  std::vector<OkItem *>    getAllItems() const;
  std::vector<std::string> getItemTags(int itemIndex) const;
  std::vector<std::string> getItemTags(OkItem *item) const;
  std::vector<std::string> getAllTags() const;
  std::vector<OkItem *>    getItemsWithTag(const std::string &tag) const;
  std::vector<int>         getItemIndicesWithTag(const std::string &tag) const;

  // Statistics
  int getItemCountWithTag(const std::string &tag) const;

  // Bulk operations on all items
  void setWireframe(bool wireframe);
  void setVisible(bool visible);
  void setDrawOriginAxisForAll(bool drawAxis);
};

#endif  // OK_ITEM_GROUP_HPP
