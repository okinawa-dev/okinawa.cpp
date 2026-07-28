---
title: Config
section: Reference
nav_order: 11
---

# Config

`OkConfig` is a typed, global key/value store for engine and application settings. It is a singleton with static accessors, seeded with defaults at startup (for example window size and graphics flags). Values are looked up by string key and grouped by type: int, float and bool maps are kept separate.

## OkConfig methods

| Method | Purpose |
| --- | --- |
| `static void setInt(const std::string &key, int value)` | Set an int value. |
| `static void setFloat(const std::string &key, float value)` | Set a float value. |
| `static void setBool(const std::string &key, bool value)` | Set a bool value. |
| `static int getInt(const std::string &key)` | Read an int value. |
| `static float getFloat(const std::string &key)` | Read a float value. |
| `static bool getBool(const std::string &key)` | Read a bool value. |
| `static void reset()` | Restore every value to its default. |
| `static std::vector<std::string> getKeysWithPrefix(prefix)` | Every key starting with a prefix, across the four typed maps, sorted (powers the console's prefix-aware `get`). |
| `static bool hasKey(const std::string &key)` | Whether the key exists in any typed map. |
| `static std::string getValueAsString(const std::string &key)` | The value formatted as a string, whatever its type (`"<unset>"` when missing). |

Keys are plain strings, so applications can store their own settings alongside the engine's (for example a `"viewer.debug-gizmos-visible"` flag). `reset()` is mainly useful to isolate global state between tests. The GUI keys (`gui.grid.size`, `gui.scale`, `gui.fov`, `gui.debug.grid`) are documented in the [GUI reference](gui.html#configuration-keys); every key is also reachable at runtime through the console's `set`/`get`.

## Example

```cpp
int width  = OkConfig::getInt("window.width");
int height = OkConfig::getInt("window.height");

OkConfig::setBool("graphics.drawCameras", false);
bool drawCameras = OkConfig::getBool("graphics.drawCameras");
```
