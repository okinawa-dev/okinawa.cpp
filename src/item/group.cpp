#include "group.hpp"
#include "../utils/logger.hpp"
#include <algorithm>

/**
 * @brief Create a new item group with the given name.
 * @param name The name of the item group.
 */
OkItemGroup::OkItemGroup(const std::string &name) : OkObject(name) {
  OkLogger::info("ItemGroup :: Creating item group " + name);
}

/**
 * @brief Destructor for the OkItemGroup class.
 *        Note: Items are not deleted as they may be shared or managed
 * elsewhere.
 */
OkItemGroup::~OkItemGroup() {
  // Items are not deleted here as they may be managed elsewhere
  clearItems();
}

/**
 * @brief Add an item to the group with optional tags.
 * @param item The item to add.
 * @param tags Vector of tags to associate with the item.
 */
void OkItemGroup::addItem(OkItem *item, const std::vector<std::string> &tags) {
  if (!item) {
    OkLogger::error("ItemGroup :: Cannot add null item to group");
    return;
  }

  // Check if item already exists
  for (size_t i = 0; i < items.size(); i++) {
    if (items[i].item == item) {
      OkLogger::warning("ItemGroup :: Item already exists in group");
      return;
    }
  }

  items.push_back(OkTaggedItem(item, tags));

  OkLogger::info("ItemGroup :: Added item to group with " +
                 std::to_string(tags.size()) + " tags");
}

/**
 * @brief Add an item to the group with a single tag.
 * @param item The item to add.
 * @param tag Single tag to associate with the item.
 */
void OkItemGroup::addItem(OkItem *item, const std::string &tag) {
  std::vector<std::string> tags;
  if (!tag.empty()) {
    tags.push_back(tag);
  }
  addItem(item, tags);
}

/**
 * @brief Remove an item from the group.
 * @param item The item to remove.
 */
void OkItemGroup::removeItem(OkItem *item) {
  for (size_t i = 0; i < items.size(); i++) {
    if (items[i].item == item) {
      removeItemByIndex(static_cast<int>(i));
      return;
    }
  }
  OkLogger::warning("ItemGroup :: Item not found in group");
}

/**
 * @brief Remove an item from the group by index.
 * @param index The index of the item to remove.
 */
void OkItemGroup::removeItemByIndex(int index) {
  if (index < 0 || index >= static_cast<int>(items.size())) {
    OkLogger::error("ItemGroup :: Invalid item index " + std::to_string(index));
    return;
  }

  items.erase(items.begin() + index);

  OkLogger::info("ItemGroup :: Removed item at index " + std::to_string(index));
}

/**
 * @brief Clear all items from the group.
 */
void OkItemGroup::clearItems() {
  items.clear();
}

/**
 * @brief Add a tag to an item by index.
 * @param itemIndex The index of the item.
 * @param tag The tag to add.
 */
void OkItemGroup::addTagToItem(int itemIndex, const std::string &tag) {
  if (itemIndex < 0 || itemIndex >= static_cast<int>(items.size())) {
    OkLogger::error("ItemGroup :: Invalid item index " +
                    std::to_string(itemIndex));
    return;
  }

  items[itemIndex].tags.push_back(tag);
}

/**
 * @brief Add a tag to an item by pointer.
 * @param item The item to add the tag to.
 * @param tag The tag to add.
 */
void OkItemGroup::addTagToItem(OkItem *item, const std::string &tag) {
  int index = getItemIndex(item);
  if (index >= 0) {
    addTagToItem(index, tag);
  }
}

/**
 * @brief Remove a tag from an item by index.
 * @param itemIndex The index of the item.
 * @param tag The tag to remove.
 */
void OkItemGroup::removeTagFromItem(int itemIndex, const std::string &tag) {
  if (itemIndex < 0 || itemIndex >= static_cast<int>(items.size())) {
    OkLogger::error("ItemGroup :: Invalid item index " +
                    std::to_string(itemIndex));
    return;
  }

  std::vector<std::string> &tags = items[itemIndex].tags;
  tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
}

/**
 * @brief Remove a tag from an item by pointer.
 * @param item The item to remove the tag from.
 * @param tag The tag to remove.
 */
void OkItemGroup::removeTagFromItem(OkItem *item, const std::string &tag) {
  int index = getItemIndex(item);
  if (index >= 0) {
    removeTagFromItem(index, tag);
  }
}

/**
 * @brief Set all tags for an item by index.
 * @param itemIndex The index of the item.
 * @param tags The vector of tags to assign.
 */
void OkItemGroup::setItemTags(int                             itemIndex,
                              const std::vector<std::string> &tags) {
  if (itemIndex < 0 || itemIndex >= static_cast<int>(items.size())) {
    OkLogger::error("ItemGroup :: Invalid item index " +
                    std::to_string(itemIndex));
    return;
  }

  items[itemIndex].tags = tags;
}

/**
 * @brief Set all tags for an item by pointer.
 * @param item The item to set tags for.
 * @param tags The vector of tags to assign.
 */
void OkItemGroup::setItemTags(OkItem                         *item,
                              const std::vector<std::string> &tags) {
  int index = getItemIndex(item);
  if (index >= 0) {
    setItemTags(index, tags);
  }
}

/**
 * @brief Get an item by index.
 * @param index The index of the item.
 * @return Pointer to the item, or nullptr if index is invalid.
 */
OkItem *OkItemGroup::getItem(int index) const {
  if (index < 0 || index >= static_cast<int>(items.size())) {
    return nullptr;
  }
  return items[index].item;
}

/**
 * @brief Get all items in the group.
 * @return Vector of all item pointers in the group.
 */
std::vector<OkItem *> OkItemGroup::getAllItems() const {
  std::vector<OkItem *> allItems;
  allItems.reserve(items.size());

  for (size_t i = 0; i < items.size(); i++) {
    allItems.push_back(items[i].item);
  }

  return allItems;
}

/**
 * @brief Get the index of an item.
 * @param item The item to find.
 * @return The index of the item, or -1 if not found.
 */
int OkItemGroup::getItemIndex(OkItem *item) const {
  for (size_t i = 0; i < items.size(); i++) {
    if (items[i].item == item) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

/**
 * @brief Get tags for an item by index.
 * @param itemIndex The index of the item.
 * @return Vector of tags for the item.
 */
std::vector<std::string> OkItemGroup::getItemTags(int itemIndex) const {
  if (itemIndex < 0 || itemIndex >= static_cast<int>(items.size())) {
    return std::vector<std::string>();
  }
  return items[itemIndex].tags;
}

/**
 * @brief Get tags for an item by pointer.
 * @param item The item to get tags for.
 * @return Vector of tags for the item.
 */
std::vector<std::string> OkItemGroup::getItemTags(OkItem *item) const {
  int index = getItemIndex(item);
  return (index >= 0) ? getItemTags(index) : std::vector<std::string>();
}

/**
 * @brief Get all unique tags used in the group.
 * @return Vector of all unique tags.
 */
std::vector<std::string> OkItemGroup::getAllTags() const {
  std::vector<std::string> allTags;
  for (size_t i = 0; i < items.size(); i++) {
    const std::vector<std::string> &itemTags = items[i].tags;
    for (size_t j = 0; j < itemTags.size(); j++) {
      // Check if tag already exists in allTags
      bool found = false;
      for (size_t k = 0; k < allTags.size(); k++) {
        if (allTags[k] == itemTags[j]) {
          found = true;
          break;
        }
      }
      if (!found) {
        allTags.push_back(itemTags[j]);
      }
    }
  }
  return allTags;
}

/**
 * @brief Get all items that have a specific tag.
 * @param tag The tag to search for.
 * @return Vector of items with the specified tag.
 */
std::vector<OkItem *>
OkItemGroup::getItemsWithTag(const std::string &tag) const {
  std::vector<OkItem *> result;
  for (size_t i = 0; i < items.size(); i++) {
    const std::vector<std::string> &itemTags = items[i].tags;
    for (size_t j = 0; j < itemTags.size(); j++) {
      if (itemTags[j] == tag) {
        result.push_back(items[i].item);
        break;  // Found the tag, no need to check other tags for this item
      }
    }
  }
  return result;
}

/**
 * @brief Get indices of all items that have a specific tag.
 * @param tag The tag to search for.
 * @return Vector of item indices with the specified tag.
 */
std::vector<int>
OkItemGroup::getItemIndicesWithTag(const std::string &tag) const {
  std::vector<int> result;
  for (size_t i = 0; i < items.size(); i++) {
    const std::vector<std::string> &itemTags = items[i].tags;
    for (size_t j = 0; j < itemTags.size(); j++) {
      if (itemTags[j] == tag) {
        result.push_back(static_cast<int>(i));
        break;  // Found the tag, no need to check other tags for this item
      }
    }
  }
  return result;
}

/**
 * @brief Get the number of items with a specific tag.
 * @param tag The tag to count.
 * @return Number of items with the specified tag.
 */
int OkItemGroup::getItemCountWithTag(const std::string &tag) const {
  int count = 0;
  for (size_t i = 0; i < items.size(); i++) {
    const std::vector<std::string> &itemTags = items[i].tags;
    for (size_t j = 0; j < itemTags.size(); j++) {
      if (itemTags[j] == tag) {
        count++;
        break;  // Found the tag, no need to check other tags for this item
      }
    }
  }
  return count;
}

/**
 * @brief Update method called each frame.
 * @param dt Delta time since last update.
 */
void OkItemGroup::stepSelf(float dt) {
  // Update all items in the group
  for (size_t i = 0; i < items.size(); i++) {
    if (items[i].item) {
      items[i].item->step(dt);
    }
  }
}

/**
 * @brief Render method called each frame.
 */
void OkItemGroup::drawSelf() {
  // Draw all items
  for (size_t i = 0; i < items.size(); i++) {
    if (items[i].item) {
      items[i].item->draw();
    }
  }
}

/**
 * @brief Update transform for this group.
 *        The group itself doesn't have geometry, so this is mainly for
 * hierarchy.
 */
void OkItemGroup::updateTransformSelf() {
  // Group doesn't have its own geometry, but this is needed for the hierarchy
  // Individual items maintain their own transforms
}

/**
 * @brief Set wireframe mode for all items in the group.
 * @param wireframe True to enable wireframe, false to disable.
 */
void OkItemGroup::setWireframe(bool wireframe) {
  for (size_t i = 0; i < items.size(); i++) {
    if (items[i].item) {
      items[i].item->setWireframe(wireframe);
    }
  }
}

/**
 * @brief Set visibility for all items in the group.
 * @param visible True to make items visible, false to hide them.
 */
void OkItemGroup::setVisible(bool visible) {
  for (size_t i = 0; i < items.size(); i++) {
    if (items[i].item) {
      items[i].item->setVisible(visible);
    }
  }
}

/**
 * @brief Set draw origin axis for all items in the group.
 * @param drawAxis True to enable axis drawing, false to disable.
 */
void OkItemGroup::setDrawOriginAxisForAll(bool drawAxis) {
  for (size_t i = 0; i < items.size(); i++) {
    if (items[i].item) {
      items[i].item->setDrawOriginAxis(drawAxis);
    }
  }
}
