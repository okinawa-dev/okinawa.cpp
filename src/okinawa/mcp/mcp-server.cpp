// The whole implementation is gated on OKINAWA_WITH_MCP (resolved by
// mcp-config.hpp from NDEBUG / the xmake "mcp" option). When the server is not
// compiled in, this is an empty translation unit, so no MCP/HTTP code (or its
// header-only dependencies) ends up in the binary.
#include "../math/frustum.hpp"
#include "mcp-config.hpp"
#ifdef OKINAWA_WITH_MCP

#include "mcp-server.hpp"

#include "../avatar/avatar.hpp"
#include "../config/config.hpp"
#include "../core/core.hpp"  // OkCore + OpenGL / GLFW headers
#include "../core/object.hpp"
#include "../gui/console.hpp"
#include "../gui/stats.hpp"
#include "../handlers/scenes.hpp"
#include "../input/input.hpp"
#include "../input/keys.hpp"
#include "../item/item.hpp"
#include "../math/point.hpp"
#include "../math/rotation.hpp"
#include "../scene/scene.hpp"
#include "../utils/logger.hpp"

#include <array>
#include <httplib.h>
#include <nlohmann/json.hpp>

// stb_image_write provides the PNG encoder. The read side
// (STB_IMAGE_IMPLEMENTATION) is defined elsewhere (item/texture.cpp); the
// write side uses a different macro and header, so there is no conflict.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <cctype>
#include <chrono>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef __APPLE__
#include <mach/mach.h>
#endif

using nlohmann::json;

namespace {

  const double kPi          = 3.14159265358979323846;
  const double kHalfTurnDeg = 180.0;
  // Bytes per mebibyte, for the resident-memory report.
  const double kBytesPerMiB = 1024.0 * 1024.0;
  // How long a request waits for the render loop to pick it up. Long
  // enough for a stalled frame, short enough that a wedged app answers
  // instead of hanging the client.
  const int kLoopWaitSeconds = 5;
  // Default hold for an injected key, and the milliseconds-per-second
  // divisor the durations are expressed in.
  const double kDefaultHoldMs = 120.0;
  const double kMsPerSecond   = 1000.0;
  // Slack added to the hold before the pose is read, so the loop has
  // rendered at least one frame past the release.
  const long kPoseSettleMs = 40;
  // Mean this far above the median means long frames are dragging it:
  // that is what a hitch looks like in a summary.
  const float kHitchRatio = 1.15f;
  // JSON-RPC 2.0 error codes and the HTTP status a notification gets.
  const int kJsonRpcParseError     = -32700;
  const int kJsonRpcMethodNotFound = -32601;
  const int kHttpAccepted          = 202;
  // Frame-rate smoothing: one part of the new reading to nine of the
  // running value, so a single slow frame does not swing the report.
  const double      kFpsSmoothing    = 0.9;
  const char *const kProtocolVersion = "2024-11-05";
  const char *const kServerName      = "okinawa";
  const char *const kServerVersion   = "0.1.0";

  double radToDeg(double r) {
    return r * kHalfTurnDeg / kPi;
  }

  // Standard base64 encoder (no external dependency).
  std::string base64Encode(const std::vector<unsigned char> &data) {
    static const std::string table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    // Base64 packs three bytes into four six-bit symbols. The shifts pull
    // each symbol out of the 24-bit group, most significant first.
    const unsigned int kSixBits    = 0x3F;
    const unsigned int kShiftByte0 = 16;
    const unsigned int kShiftByte1 = 8;
    const unsigned int kShiftSym0  = 18;
    const unsigned int kShiftSym1  = 12;
    const unsigned int kShiftSym2  = 6;
    std::string        out;
    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i + 2 < data.size()) {
      unsigned int n =
          (data[i] << kShiftByte0) | (data[i + 1] << kShiftByte1) | data[i + 2];
      out.push_back(table[(n >> kShiftSym0) & kSixBits]);
      out.push_back(table[(n >> kShiftSym1) & kSixBits]);
      out.push_back(table[(n >> kShiftSym2) & kSixBits]);
      out.push_back(table[n & kSixBits]);
      i += 3;
    }
    size_t remaining = data.size() - i;
    if (remaining == 1) {
      unsigned int n = data[i] << kShiftByte0;
      out.push_back(table[(n >> kShiftSym0) & kSixBits]);
      out.push_back(table[(n >> kShiftSym1) & kSixBits]);
      out.push_back('=');
      out.push_back('=');
    } else if (remaining == 2) {
      unsigned int n = (data[i] << kShiftByte0) | (data[i + 1] << kShiftByte1);
      out.push_back(table[(n >> kShiftSym0) & kSixBits]);
      out.push_back(table[(n >> kShiftSym1) & kSixBits]);
      out.push_back(table[(n >> kShiftSym2) & kSixBits]);
      out.push_back('=');
    }
    return out;
  }

  void stbWriteToVector(void *context, void *data, int size) {
    std::vector<unsigned char> *out =
        static_cast<std::vector<unsigned char> *>(context);
    unsigned char *bytes = static_cast<unsigned char *>(data);
    out->insert(out->end(), bytes, bytes + size);
  }

  // Map a key name (e.g. "W", "space", "1", "up") to an OkKey.
  OkKey okKeyFromName(const std::string &name) {
    if (name.empty()) {
      return OK_KEY_UNKNOWN;
    }
    std::string s;
    for (size_t i = 0; i < name.size(); i++) {
      s += static_cast<char>(std::toupper(static_cast<unsigned char>(name[i])));
    }
    if (s.size() == 1) {
      char c = s[0];
      if (c >= 'A' && c <= 'Z') {
        return static_cast<OkKey>(OK_KEY_A + (c - 'A'));
      }
      if (c >= '0' && c <= '9') {
        return static_cast<OkKey>(OK_KEY_0 + (c - '0'));
      }
    }
    if (s == "SPACE")
      return OK_KEY_SPACE;
    if (s == "UP")
      return OK_KEY_UP;
    if (s == "DOWN")
      return OK_KEY_DOWN;
    if (s == "LEFT")
      return OK_KEY_LEFT;
    if (s == "RIGHT")
      return OK_KEY_RIGHT;
    if (s == "ESCAPE" || s == "ESC")
      return OK_KEY_ESCAPE;
    if (s == "ENTER" || s == "RETURN")
      return OK_KEY_ENTER;
    if (s == "TAB")
      return OK_KEY_TAB;
    if (s == "GRAVE" || s == "BACKTICK")
      return OK_KEY_GRAVE_ACCENT;
    if (s == "PERIOD" || s == ".")
      return OK_KEY_PERIOD;
    if (s == "MINUS" || s == "-")
      return OK_KEY_MINUS;
    if (s == "BACKSPACE")
      return OK_KEY_BACKSPACE;
    return OK_KEY_UNKNOWN;
  }

  // Resident set size in MB (macOS), or -1 if unavailable.
  double residentMb() {
#ifdef __APPLE__
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t      count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info),
                  &count) == KERN_SUCCESS) {
      return static_cast<double>(info.resident_size) / kBytesPerMiB;
    }
#endif
    return -1.0;
  }

  // Read the active camera's pose. Must be called on the engine loop thread.
  json cameraPoseJson() {
    json      p;
    OkCamera *cam = OkCore::getCamera();
    if (cam == nullptr) {
      p["error"] = "no active camera";
      return p;
    }
    OkPoint    pos    = cam->getPosition();
    OkRotation rot    = cam->getRotation();
    p["camera_index"] = OkCore::getCurrentCameraIndex();
    p["position"]     = {{"x", pos.x()}, {"y", pos.y()}, {"z", pos.z()}};
    p["rotation_deg"] = {{"pitch", radToDeg(rot.getPitch())},
                         {"yaw", radToDeg(rot.getYaw())},
                         {"roll", radToDeg(rot.getRoll())}};
    return p;
  }

  // Whether the person at the window can drive the app, and for how much
  // longer it is being ignored. Loop thread.
  json inputStateJson() {
    double left = OkCore::userInputBlockedFor();
    json   r;
    r["user_input_enabled"] = left == 0.0;
    if (left > 0.0) {
      r["blocked_for_seconds"] = left;
    } else if (left < 0.0) {
      // No deadline: the launch flag, or a deliberate zero.
      r["blocked_for_seconds"] = nullptr;
    }
    r["release"] = "ctrl+shift+escape";
    return r;
  }

  // The six numbers the `view` tool takes and reproduces: the avatar position
  // (x,y,z) and the orbit camera angle (yaw_deg, pitch_deg, distance). Pass
  // this object straight back to `view` to restore the exact viewpoint. Loop
  // thread.
  json viewJson() {
    json      v;
    OkAvatar *avatar = OkCore::getActiveAvatar();
    OkObject *obj    = avatar ? avatar->getControlledObject() : nullptr;
    if (obj != nullptr) {
      OkPoint p = obj->getPosition();
      v["x"]    = p.x();
      v["y"]    = p.y();
      v["z"]    = p.z();
    }
    // Reflect the active camera (identified by name): orbit cameras report
    // yaw/pitch/distance, overhead/fixed ones report their view distance.
    OkCamera *cam = OkCore::getCamera();
    if (cam != nullptr) {
      v["camera"] = cam->getName();
      if (cam->isOrbit()) {
        v["yaw_deg"]   = cam->orbitYawDeg();
        v["pitch_deg"] = cam->orbitPitchDeg();
        v["distance"]  = cam->orbitDistance();
      } else {
        v["distance"] = cam->viewDistance();
      }
    }
    return v;
  }

  // MCP tool-result builders.
  json textContent(const std::string &t) {
    return json{{"type", "text"}, {"text", t}};
  }
  json textResult(const std::string &t) {
    return json{{"content", json::array({textContent(t)})}, {"isError", false}};
  }
  json errorResult(const std::string &t) {
    return json{{"content", json::array({textContent(t)})}, {"isError", true}};
  }
  json imageResult(const std::string &base64Png) {
    json image;
    image["type"]     = "image";
    image["data"]     = base64Png;
    image["mimeType"] = "image/png";
    return json{{"content", json::array({image})}, {"isError", false}};
  }

}  // namespace

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

struct OkMcpServer::Impl {
  int                               port;
  httplib::Server                   server;
  std::thread                       thread;
  std::mutex                        queueMutex;
  std::deque<std::function<void()>> queue;

  // Measured frame rate, updated each frame in drainCommands.
  double lastFrameTime = 0.0;
  double fps           = 0.0;

  // Run fn on the engine loop thread (where the GL context lives) and return
  // its JSON result. Blocks the calling (HTTP) thread until the next frame
  // executes the work, or times out.
  json runOnLoop(const std::function<json()> &fn) {
    std::shared_ptr<std::promise<json>> promise =
        std::make_shared<std::promise<json>>();
    std::future<json> future = promise->get_future();
    {
      std::scoped_lock lock(queueMutex);
      queue.emplace_back([promise, fn]() {
        json result;
        try {
          result = fn();
        } catch (...) {
          result          = json::object();
          result["error"] = "exception on loop thread";
        }
        promise->set_value(result);
      });
    }
    if (future.wait_for(std::chrono::seconds(kLoopWaitSeconds)) !=
        std::future_status::ready) {
      json r;
      r["error"] = "timed out (is the app rendering?)";
      return r;
    }
    return future.get();
  }

  // Capture the current framebuffer into a PNG byte buffer (on the loop
  // thread). Returns true on success and fills width/height.
  bool capturePng(const std::shared_ptr<std::vector<unsigned char>> &outPng,
                  int &widthOut, int &heightOut) {
    std::shared_ptr<std::pair<int, int>> wh =
        std::make_shared<std::pair<int, int>>(0, 0);
    json meta = runOnLoop([outPng, wh]() -> json {
      GLFWwindow *window = OkCore::getWindow();
      if (window == nullptr) {
        return json{{"ok", false}};
      }
      int width  = 0;
      int height = 0;
      glfwGetFramebufferSize(window, &width, &height);
      if (width <= 0 || height <= 0) {
        return json{{"ok", false}};
      }
      int                        stride = width * 4;
      std::vector<unsigned char> pixels(static_cast<size_t>(stride) * height);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glReadBuffer(GL_BACK);
      glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                   pixels.data());
      // OpenGL origin is bottom-left; PNG expects top-left, so flip rows.
      std::vector<unsigned char> flipped(static_cast<size_t>(stride) * height);
      for (int y = 0; y < height; y++) {
        memcpy(&flipped[static_cast<size_t>(stride) * (height - 1 - y)],
               &pixels[static_cast<size_t>(stride) * y], stride);
      }
      outPng->clear();
      int rc = stbi_write_png_to_func(stbWriteToVector, outPng.get(), width,
                                      height, 4, flipped.data(), stride);
      if (rc == 0 || outPng->empty()) {
        return json{{"ok", false}};
      }
      wh->first  = width;
      wh->second = height;
      return json{{"ok", true}};
    });
    widthOut  = wh->first;
    heightOut = wh->second;
    return meta.value("ok", false);
  }

  // The catalogue of tools, for tools/list.
  static json toolList() {
    json tools = json::array();

    json viewFrame;
    viewFrame["name"] = "view_frame";
    viewFrame["description"] =
        "Capture the current rendered frame and return it as a PNG image, so "
        "the agent can visually inspect what is on screen.";
    viewFrame["inputSchema"] = {{"type", "object"},
                                {"properties", json::object()},
                                {"additionalProperties", false}};
    tools.push_back(viewFrame);

    json screenshot;
    screenshot["name"] = "screenshot";
    screenshot["description"] =
        "Capture the current frame and write it to a PNG file on disk (for a "
        "human to open). Returns the file path.";
    screenshot["inputSchema"] = {
        {"type", "object"},
        {"properties",
         {{"path",
           {{"type", "string"},
            {"description", "Output file path (default: okinawa-screenshot.png "
                            "in the working dir)."}}}}},
        {"additionalProperties", false}};
    tools.push_back(screenshot);

    json pressKey;
    pressKey["name"] = "press_key";
    pressKey["description"] =
        "Hold a key for a duration (movement integrates over frames). W/A/S/D "
        "move, SPACE/T/R/F are actions, 1-9 switch camera, arrows turn. "
        "Returns the resulting camera pose.";
    pressKey["inputSchema"] = {
        {"type", "object"},
        {"properties",
         {{"key",
           {{"type", "string"},
            {"description", "Key name, e.g. W, A, S, D, SPACE, R, 1, UP."}}},
          {"duration_ms",
           {{"type", "number"},
            {"description", "How long to hold the key (default 120)."}}}}},
        {"required", json::array({"key"})},
        {"additionalProperties", false}};
    tools.push_back(pressKey);

    json pressKeys;
    pressKeys["name"] = "press_keys";
    pressKeys["description"] =
        "Hold several keys at once for a duration (e.g. W and D for diagonal "
        "movement). Returns the resulting camera pose.";
    pressKeys["inputSchema"] = {
        {"type", "object"},
        {"properties",
         {{"keys", {{"type", "array"}, {"items", {{"type", "string"}}}}},
          {"duration_ms", {{"type", "number"}}}}},
        {"required", json::array({"keys"})},
        {"additionalProperties", false}};
    tools.push_back(pressKeys);

    json view;
    view["name"] = "view";
    view["description"] =
        "THE camera tool -- set the whole viewpoint in one call. Optionally "
        "activates a camera BY NAME (`camera`; get_state lists the registered "
        "names), then places the avatar at x,y,z and drives the active camera: "
        "orbit cameras take yaw_deg/pitch_deg/distance (pitch negative looks "
        "DOWN; ~-89 = top-down); overhead/fixed cameras take just distance "
        "(their height). It never force-switches cameras on its own. All "
        "fields optional; an omitted field keeps its current value. Persistent "
        "(survives input, so the user takes over in the same view). get_state "
        "returns the same values (with the active camera name) under `view` -- "
        "reproduce any viewpoint by passing them straight back. Returns the "
        "resulting view.";
    view["inputSchema"] = {
        {"type", "object"},
        {"properties",
         {{"camera",
           {{"type", "string"},
            {"description",
             "Camera to activate and drive, by registered name (see "
             "get_state.cameras). Omitted = keep the active camera."}}},
          {"x", {{"type", "number"}}},
          {"y", {{"type", "number"}}},
          {"z", {{"type", "number"}}},
          {"yaw_deg", {{"type", "number"}}},
          {"pitch_deg",
           {{"type", "number"},
            {"description", "Tilt; negative looks down, ~-89 is top-down."}}},
          {"distance",
           {{"type", "number"},
            {"description", "Camera distance back / height, in metres."}}}}},
        {"additionalProperties", false}};
    tools.push_back(view);

    json setVis;
    setVis["name"] = "set_item_visible";
    setVis["description"] =
        "Show/hide scene items by name, to isolate geometry. If prefix=true it "
        "applies to every item whose name starts with `name` (e.g. 'tree_' to "
        "hide every tree at once, or 'tree_oak_' to narrow it further); "
        "otherwise it toggles the single item with that exact name. Returns "
        "how many items changed.";
    setVis["inputSchema"] = {
        {"type", "object"},
        {"properties",
         {{"name",
           {{"type", "string"},
            {"description", "Item name or, with prefix=true, a name prefix."}}},
          {"visible", {{"type", "boolean"}}},
          {"prefix",
           {{"type", "boolean"},
            {"description", "Match all names starting with `name` (default "
                            "false = exact)."}}}}},
        {"required", json::array({"name", "visible"})},
        {"additionalProperties", false}};
    tools.push_back(setVis);

    json perf;
    perf["name"] = "get_performance";
    perf["description"] =
        "Return the recorded frame-time SERIES, not a single reading: count, "
        "min/max/mean/median in milliseconds, the equivalent fps, and "
        "optionally the raw samples (oldest first). A one-off fps value cannot "
        "tell a real change from a lucky frame, so use this to compare builds "
        "or viewpoints. Also returns `draw_ms`, the CPU time spent issuing the "
        "frame's draws, measured before the swap -- where the platform "
        "enforces vsync (macOS does) every frame with budget to spare reads as "
        "exactly one refresh interval, so frame_ms cannot tell whether a "
        "change cost anything and draw_ms is the number to compare. Samples "
        "are collected whether or not the stats panel is visible.";
    perf["inputSchema"] = {
        {"type", "object"},
        {"properties",
         {{"samples",
           {{"type", "boolean"},
            {"description",
             "Include the raw per-frame milliseconds (default false)."}}}}},
        {"additionalProperties", false}};
    tools.push_back(perf);

    json config;
    config["name"] = "config";
    config["description"] =
        "Read or write an engine config key at runtime -- the same keys the "
        "console's set/get reach (shadows.*, render.*, graphics.*, and any the "
        "application registers). With no `value` it reads; with one it writes, "
        "converting to the key's existing type. Pass no `key` at all to list "
        "every key matching `prefix`, with its current value. Meant for "
        "bisecting a visual problem: flip one setting, capture, flip it back, "
        "without typing into the console a character at a time.";
    config["inputSchema"] = {
        {"type", "object"},
        {"properties",
         {{"key",
           {{"type", "string"},
            {"description", "Config key, e.g. shadows.cascades."}}},
          {"value",
           {{"type", "string"}, {"description", "New value; omit to read."}}},
          {"prefix",
           {{"type", "string"},
            {"description", "List every key starting with this instead."}}}}},
        {"additionalProperties", false}};
    tools.push_back(config);

    json console;
    console["name"] = "console";
    console["description"] =
        "Run one console command line, exactly as if it had been typed into "
        "the drop-down console and submitted, and return what it printed. The "
        "console does not need to be open and its open state is left as it was "
        "found. This is the way to drive the console from an agent: press_key "
        "holds one key per call, so `hide sidewalks` that way is fifteen round "
        "trips. Pass no `line` to list the registered command names.";
    console["inputSchema"] = {
        {"type", "object"},
        {"properties",
         {{"line",
           {{"type", "string"},
            {"description", "Command line, e.g. \"hide sidewalks\" or \"set "
                            "shadows.enabled 0\"."}}}}},
        {"additionalProperties", false}};
    tools.push_back(console);

    json quit;
    quit["name"]        = "quit";
    quit["description"] = "Close the application, the same way its window's "
                          "close button does: the loop stops, the application "
                          "gets its chance to save whatever it keeps between "
                          "sessions, and the process ends. The reply is sent "
                          "before the shutdown starts, and this connection is "
                          "the last one this server will answer.";
    quit["inputSchema"] = {{"type", "object"},
                           {"properties", json::object()},
                           {"additionalProperties", false}};
    tools.push_back(quit);

    json input;
    input["name"] = "input";
    input["description"] =
        "Read or set whether the PERSON at the window can drive the app with "
        "their own keyboard and mouse. With no arguments it reports. "
        "`enabled: false` hands control to this agent alone, so a stray key "
        "cannot move the view under a measurement; `enabled: true` gives it "
        "straight back. A block EXPIRES on its own after `seconds` (default "
        "300, clamped to 1..3600), and the person can always lift it at the "
        "window with ctrl+shift+escape -- an agent must never leave somebody "
        "locked out of their own window. While blocked, the app says so on "
        "screen. Injected input (press_key, view) is unaffected either way.";
    input["inputSchema"] = {
        {"type", "object"},
        {"properties",
         {{"enabled",
           {{"type", "boolean"},
            {"description",
             "false to take the keyboard, true to give it back"}}},
          {"seconds",
           {{"type", "number"},
            {"description",
             "how long a block lasts before it lifts itself; 0 means no "
             "deadline, which only the launch flag should want"}}}}},
        {"additionalProperties", false}};
    tools.push_back(input);

    json getState;
    getState["name"] = "get_state";
    getState["description"] =
        "Return numeric runtime state: active camera pose, fps, scene object "
        "count, window size and resident memory.";
    getState["inputSchema"] = {{"type", "object"},
                               {"properties", json::object()},
                               {"additionalProperties", false}};
    tools.push_back(getState);

    return tools;
  }

  /**
   * @brief Refuses a call carrying an argument the tool does not have.
   *
   * Every schema here says `additionalProperties: false`, and until this
   * ran nothing enforced it: an unknown argument was dropped and the
   * tool went ahead with what was left. That is the worst way to be
   * wrong, because the answer looks like success. A `console` call whose
   * argument is misspelled becomes a `console` call with no argument at
   * all, which is a request for the list of command names -- so it
   * returns a tidy list, the command never runs, and the caller has no
   * way to tell. It cost a session before anyone noticed the app was not
   * doing as it was told.
   *
   * @return an error result naming the argument and listing the ones the
   *         tool does take, or a null json when the call is fine.
   */
  json rejectUnknownArgs(const std::string &name, const json &args) {
    if (!args.is_object() || args.empty()) {
      return json();
    }
    json list = toolList();
    for (size_t i = 0; i < list.size(); i++) {
      if (list[i].value("name", std::string()) != name) {
        continue;
      }
      if (!list[i].contains("inputSchema") ||
          !list[i]["inputSchema"].contains("properties")) {
        return json();
      }
      const json &props = list[i]["inputSchema"]["properties"];
      std::string unknown;
      for (json::const_iterator it = args.begin(); it != args.end(); ++it) {
        if (!props.contains(it.key())) {
          if (!unknown.empty()) {
            unknown += ", ";
          }
          unknown += it.key();
        }
      }
      if (unknown.empty()) {
        return json();
      }
      std::string taken;
      for (json::const_iterator it = props.begin(); it != props.end(); ++it) {
        if (!taken.empty()) {
          taken += ", ";
        }
        taken += it.key();
      }
      if (taken.empty()) {
        taken = "(none)";
      }
      return errorResult(name + ": no such argument: " + unknown +
                         ". It takes: " + taken);
    }
    return json();
  }

  // Dispatch a tools/call by name. Returns the MCP result object.
  json callTool(const std::string &name, const json &args) {
    json bad = rejectUnknownArgs(name, args);
    if (!bad.is_null()) {
      return bad;
    }
    if (name == "view_frame") {
      std::shared_ptr<std::vector<unsigned char>> png =
          std::make_shared<std::vector<unsigned char>>();
      int w = 0;
      int h = 0;
      if (!capturePng(png, w, h)) {
        return errorResult("view_frame: capture failed");
      }
      return imageResult(base64Encode(*png));
    }

    if (name == "screenshot") {
      std::string path =
          args.value("path", std::string("okinawa-screenshot.png"));
      std::shared_ptr<std::vector<unsigned char>> png =
          std::make_shared<std::vector<unsigned char>>();
      int w = 0;
      int h = 0;
      if (!capturePng(png, w, h)) {
        return errorResult("screenshot: capture failed");
      }
      std::ofstream out(path.c_str(), std::ios::binary);
      if (!out) {
        return errorResult("screenshot: cannot open file: " + path);
      }
      out.write(reinterpret_cast<const char *>(png->data()),
                static_cast<std::streamsize>(png->size()));
      out.close();
      return textResult("wrote " + std::to_string(png->size()) + " bytes (" +
                        std::to_string(w) + "x" + std::to_string(h) + ") to " +
                        path);
    }

    if (name == "console") {
      if (!args.contains("line")) {
        json r = runOnLoop([]() -> json {
          std::vector<std::string> names = OkConsole::getCommandNames();
          json                     list  = json::array();
          for (size_t i = 0; i < names.size(); i++) {
            list.push_back(names[i]);
          }
          return json{{"commands", list}};
        });
        return textResult(r.dump(2));
      }
      std::string line = args.value("line", std::string());
      json        r    = runOnLoop([line]() -> json {
        // The count of lines ever printed brackets the command's own
        // answer; the scrollback is trimmed, so line indices would not.
        unsigned long before = OkConsole::getPrintedCount();
        OkConsole::execute(line);
        unsigned long produced = OkConsole::getPrintedCount() - before;
        // The first of those lines is the echoed "> line", not output.
        std::vector<std::string> tail =
            OkConsole::getOutputTail(static_cast<int>(produced));
        json out = json::array();
        for (size_t i = 1; i < tail.size(); i++) {
          out.push_back(tail[i]);
        }
        return json{{"line", line}, {"output", out}};
      });
      return textResult(r.dump(2));
    }

    if (name == "press_key" || name == "press_keys") {
      std::vector<OkKey>       keys;
      std::vector<std::string> names;
      if (name == "press_key") {
        std::string k = args.value("key", std::string());
        names.push_back(k);
        keys.push_back(okKeyFromName(k));
      } else if (args.contains("keys") && args["keys"].is_array()) {
        for (size_t i = 0; i < args["keys"].size(); i++) {
          std::string k = args["keys"][i].get<std::string>();
          names.push_back(k);
          keys.push_back(okKeyFromName(k));
        }
      }
      for (size_t i = 0; i < keys.size(); i++) {
        if (keys[i] == OK_KEY_UNKNOWN) {
          return errorResult("unknown key: " + names[i]);
        }
      }
      double durationMs = args.value("duration_ms", kDefaultHoldMs);
      double seconds    = durationMs / kMsPerSecond;
      runOnLoop([keys, seconds]() -> json {
        OkInput *input = OkCore::getInput();
        for (size_t i = 0; i < keys.size(); i++) {
          input->injectKey(keys[i], seconds);
        }
        return json::object();
      });
      // Let the loop apply the held keys over its frames, then read the pose.
      std::this_thread::sleep_for(std::chrono::milliseconds(
          static_cast<long>(durationMs) + kPoseSettleMs));
      json pose = runOnLoop([]() -> json { return cameraPoseJson(); });
      return textResult("held keys for " +
                        std::to_string(static_cast<long>(durationMs)) + "ms\n" +
                        pose.dump(2));
    }

    if (name == "quit") {
      // Asked for on the loop thread like everything else, so the flag
      // is set between frames rather than in the middle of one. The
      // result is returned first and written by the HTTP thread while
      // the loop is on its way out.
      json asked = runOnLoop([]() {
        OkCore::askForExit();
        json ok;
        ok["closing"] = true;
        return ok;
      });
      if (asked.contains("error")) {
        return errorResult("quit: " + asked["error"].get<std::string>());
      }
      return textResult("closing");
    }

    if (name == "view") {
      // Every optional field is read here, on the request thread, and
      // handed over as plain values. The other commands already do this;
      // this one used to capture the whole request json and look inside
      // it on the render thread, which put the parsing inside the frame
      // and gave the closure a json member -- and a closure holding a
      // json cannot be moved without the move being able to throw.
      // Types are checked before anything is read. nlohmann's accessors
      // throw on a mismatch, and here that throw would leave callTool
      // for the HTTP handler, which answers 500 with an empty body --
      // the caller learns nothing. Inside the lambda it used to be
      // caught by runOnLoop; on this side there is nothing to catch it.
      static const std::array<const char *, 6> NUMBER_FIELDS = {
          "x", "y", "z", "yaw_deg", "pitch_deg", "distance"};
      for (const char *const field : NUMBER_FIELDS) {
        if (args.contains(field) && !args[field].is_number()) {
          return errorResult(std::string(field) + " must be a number");
        }
      }
      if (args.contains("camera") && !args["camera"].is_string()) {
        return errorResult("camera must be a string");
      }

      std::string camName   = args.value("camera", std::string());
      bool        hasCam    = args.contains("camera");
      bool        hasX      = args.contains("x");
      bool        hasY      = args.contains("y");
      bool        hasZ      = args.contains("z");
      double      wantX     = args.value("x", 0.0);
      double      wantY     = args.value("y", 0.0);
      double      wantZ     = args.value("z", 0.0);
      bool        hasYaw    = args.contains("yaw_deg");
      bool        hasPitch  = args.contains("pitch_deg");
      bool        hasDist   = args.contains("distance");
      double      wantYaw   = args.value("yaw_deg", 0.0);
      double      wantPitch = args.value("pitch_deg", 0.0);
      double      wantDist  = args.value("distance", 0.0);

      json out = runOnLoop([camName, hasCam, hasX, hasY, hasZ, wantX, wantY,
                            wantZ, hasYaw, hasPitch, hasDist, wantYaw,
                            wantPitch, wantDist]() -> json {
        // Optional camera selection by name (see get_state.cameras). The
        // tool then drives the active camera -- it no longer force-switches
        // to the orbit camera, so an overhead workflow keeps its framing.
        if (hasCam) {
          int idx = OkCore::findCamera(camName);
          if (idx < 0) {
            json e;
            e["error"] = "unknown camera: " + camName;
            return e;
          }
          OkCore::switchCamera(idx);
        }
        OkAvatar *avatar = OkCore::getActiveAvatar();
        OkObject *obj    = avatar ? avatar->getControlledObject() : nullptr;
        if (obj != nullptr && (hasX || hasY || hasZ)) {
          // Whatever the caller left out keeps the value it has now,
          // which is only knowable here, on the loop thread.
          OkPoint p = obj->getPosition();
          obj->setPosition(hasX ? static_cast<float>(wantX) : p.x(),
                           hasY ? static_cast<float>(wantY) : p.y(),
                           hasZ ? static_cast<float>(wantZ) : p.z());
        }
        OkCamera *cam = OkCore::getCamera();
        if (cam != nullptr) {
          if (cam->isOrbit() && (hasYaw || hasPitch || hasDist)) {
            float yaw =
                hasYaw ? static_cast<float>(wantYaw) : cam->orbitYawDeg();
            float pitch =
                hasPitch ? static_cast<float>(wantPitch) : cam->orbitPitchDeg();
            float dist =
                hasDist ? static_cast<float>(wantDist) : cam->orbitDistance();
            cam->setOrbit(yaw, pitch, dist);
          } else if (!cam->isOrbit() && hasDist) {
            cam->setViewDistance(static_cast<float>(wantDist));
          }
          cam->updateForTarget(obj,
                               0.0f);  // apply now; returned view is current
        }
        return viewJson();
      });
      if (out.contains("error")) {
        return errorResult(out["error"].get<std::string>());
      }
      return textResult(out.dump(2));
    }

    if (name == "set_item_visible") {
      std::string itemName = args.value("name", std::string());
      bool        vis      = args.value("visible", true);
      bool        prefix   = args.value("prefix", false);
      json        r        = runOnLoop([itemName, vis, prefix]() -> json {
        OkSceneHandler *sh = OkCore::getSceneHandler();
        OkScene *scene     = (sh != nullptr) ? sh->getCurrentScene() : nullptr;
        if (scene == nullptr) {
          return json{{"error", "no current scene"}};
        }
        if (prefix) {
          std::vector<OkItem *> items = scene->findItems(itemName);
          for (size_t i = 0; i < items.size(); i++) {
            items[i]->setVisible(vis);
          }
          return json{{"changed", static_cast<int>(items.size())}};
        }
        OkItem *it = scene->findItem(itemName);
        if (it == nullptr) {
          return json{{"error", "item not found: " + itemName}};
        }
        it->setVisible(vis);
        return json{{"changed", 1}};
      });
      return textResult(r.dump(2));
    }

    if (name == "get_performance") {
      bool wantSamples = args.contains("samples") &&
                         args["samples"].is_boolean() &&
                         args["samples"].get<bool>();
      json out         = runOnLoop([wantSamples]() -> json {
        json  r;
        int   count  = 0;
        float lo     = 0.0f;
        float hi     = 0.0f;
        float mean   = 0.0f;
        float median = 0.0f;
        OkGuiStats::getSummary(count, lo, hi, mean, median);
        r["count"] = count;
        if (count == 0) {
          r["note"] = "no frames recorded yet";
          return r;
        }
        r["frame_ms"] = {
            {"min", lo}, {"max", hi}, {"mean", mean}, {"median", median}};
        r["fps"] = {
            {"min", hi > 0.0f ? static_cast<float>(kMsPerSecond) / hi : 0.0f},
            {"max", lo > 0.0f ? static_cast<float>(kMsPerSecond) / lo : 0.0f},
            {"mean",
             mean > 0.0f ? static_cast<float>(kMsPerSecond) / mean : 0.0f},
            {"median",
             median > 0.0f ? static_cast<float>(kMsPerSecond) / median : 0.0f}};
        // The mean sits below the median when long frames are dragging
        // it down, which is what a hitch looks like in a summary.
        r["hitching"] = mean > median * kHitchRatio;
        // CPU time spent issuing the draws, measured before the swap.
        // Where the platform enforces vsync every frame with budget to
        // spare reads as one refresh interval, so frame_ms cannot say
        // whether a change cost anything; this can.
        int   dc      = 0;
        float dlo     = 0.0f;
        float dhi     = 0.0f;
        float dmean   = 0.0f;
        float dmedian = 0.0f;
        OkGuiStats::getDrawSummary(dc, dlo, dhi, dmean, dmedian);
        if (dc > 0) {
          r["draw_ms"] = {
              {"min", dlo}, {"max", dhi}, {"mean", dmean}, {"median", dmedian}};
        }
        if (wantSamples) {
          const std::vector<float> &h   = OkGuiStats::getHistory();
          json                      arr = json::array();
          for (size_t i = 0; i < h.size(); i++) {
            arr.push_back(h[i]);
          }
          r["samples_ms"] = arr;
        }
        return r;
      });
      return textResult(out.dump(2));
    }

    if (name == "config") {
      std::string key    = args.contains("key") && args["key"].is_string()
                               ? args["key"].get<std::string>()
                               : std::string();
      std::string prefix = args.contains("prefix") && args["prefix"].is_string()
                               ? args["prefix"].get<std::string>()
                               : std::string();
      bool        hasVal = args.contains("value") && args["value"].is_string();
      std::string val =
          hasVal ? args["value"].get<std::string>() : std::string();
      json out = runOnLoop([key, prefix, hasVal, val]() -> json {
        json r;
        if (key.empty()) {
          std::vector<std::string> keys = OkConfig::getKeysWithPrefix(prefix);
          json                     m    = json::object();
          for (size_t i = 0; i < keys.size(); i++) {
            m[keys[i]] = OkConfig::getValueAsString(keys[i]);
          }
          r["keys"] = m;
          return r;
        }
        if (!OkConfig::hasKey(key)) {
          r["error"] = "no such key: " + key;
          return r;
        }
        if (hasVal) {
          // Respects the key's existing type, exactly as the console's
          // `set` does: a float key stays a float even when the text
          // has no decimal point.
          OkConfig::setFromString(key, val);
        }
        r["key"]   = key;
        r["value"] = OkConfig::getValueAsString(key);
        return r;
      });
      return textResult(out.dump(2));
    }

    if (name == "input") {
      bool hasEnabled =
          args.contains("enabled") && args["enabled"].is_boolean();
      bool enabled    = hasEnabled && args["enabled"].get<bool>();
      bool hasSeconds = args.contains("seconds") && args["seconds"].is_number();
      double seconds  = hasSeconds ? args["seconds"].get<double>()
                                   : OkInput::BLOCK_DEFAULT_SECONDS;
      json   out      = runOnLoop([hasEnabled, enabled, seconds]() -> json {
        if (hasEnabled) {
          if (enabled) {
            OkCore::setIgnoreUserInput(false);
          } else {
            OkCore::blockUserInput(seconds);
          }
        }
        return inputStateJson();
      });
      return textResult(out.dump(2));
    }

    if (name == "get_state") {
      double measuredFps = fps;
      json   state       = runOnLoop([measuredFps]() -> json {
        json s    = cameraPoseJson();
        s["view"] = viewJson();  // the six numbers to pass back to `view`

        GLFWwindow *window = OkCore::getWindow();
        int         w      = 0;
        int         h      = 0;
        if (window != nullptr) {
          glfwGetFramebufferSize(window, &w, &h);
        }
        s["window"]       = {{"width", w}, {"height", h}};
        s["camera_count"] = OkCore::getCameraCount();
        json camNames     = json::array();
        for (int ci = 0; ci < OkCore::getCameraCount(); ci++) {
          OkCamera *cc = OkCore::getCameraAt(ci);
          camNames.push_back(cc != nullptr ? cc->getName() : "");
        }
        s["cameras"] = camNames;  // registered camera names (view's `camera`)
        s["fps"]     = measuredFps;

        OkSceneHandler *handler = OkCore::getSceneHandler();
        OkScene        *scene = handler ? handler->getCurrentScene() : nullptr;
        s["scene"] = {{"object_count", scene ? scene->getObjectCount() : 0},
                      {"frustum_culled", OkFrustum::getCulledCount()}};
        // Whether the person at the window can drive: an agent about to
        // say it has handed the app back can read it here first.
        s["input"] = inputStateJson();
        return s;
      });
      state["memory"]    = {{"resident_mb", residentMb()}};
      return textResult(state.dump(2));
    }

    return errorResult("unknown tool: " + name);
  }
};

// ---------------------------------------------------------------------------
// JSON-RPC helpers
// ---------------------------------------------------------------------------

namespace {

  json makeError(const json &id, int code, const std::string &message) {
    json response;
    response["jsonrpc"]          = "2.0";
    response["id"]               = id;
    response["error"]["code"]    = code;
    response["error"]["message"] = message;
    return response;
  }

  json makeResult(const json &id, const json &result) {
    json response;
    response["jsonrpc"] = "2.0";
    response["id"]      = id;
    response["result"]  = result;
    return response;
  }

}  // namespace

// ---------------------------------------------------------------------------
// OkMcpServer
// ---------------------------------------------------------------------------

OkMcpServer::OkMcpServer(int port) {
  _impl       = new Impl();
  _impl->port = port;
}

OkMcpServer::~OkMcpServer() {
  stop();
  delete _impl;
}

void OkMcpServer::start() {
  _impl->server.Post("/mcp", [this](const httplib::Request &req,
                                    httplib::Response      &res) {
    json request = json::parse(req.body, nullptr, false);
    if (request.is_discarded() || !request.is_object()) {
      res.set_content(
          makeError(nullptr, kJsonRpcParseError, "parse error").dump(),
          "application/json");
      return;
    }

    json        id     = request.contains("id") ? request["id"] : json(nullptr);
    std::string method = request.value("method", std::string());
    json        params =
        request.contains("params") ? request["params"] : json::object();
    bool isNotification = !request.contains("id");

    if (method == "initialize") {
      json result;
      result["protocolVersion"] =
          params.value("protocolVersion", std::string(kProtocolVersion));
      result["capabilities"]["tools"] = json::object();
      result["serverInfo"]["name"]    = kServerName;
      result["serverInfo"]["version"] = kServerVersion;
      res.set_header("Mcp-Session-Id", "okinawa-mcp");
      res.set_content(makeResult(id, result).dump(), "application/json");
    } else if (method == "ping") {
      res.set_content(makeResult(id, json::object()).dump(),
                      "application/json");
    } else if (method == "tools/list") {
      json result;
      result["tools"] = Impl::toolList();
      res.set_content(makeResult(id, result).dump(), "application/json");
    } else if (method == "tools/call") {
      std::string name = params.value("name", std::string());
      json        args =
          params.contains("arguments") ? params["arguments"] : json::object();
      // A tool reads its arguments with nlohmann's accessors, and those
      // throw when the caller sends the wrong type for a field. Without
      // this the throw reaches httplib, which answers 500 with an empty
      // body: the client is told nothing at all, not even which call
      // failed. Turned into an ordinary tool error instead, so a
      // malformed request reads like any other rejected one.
      json result;
      try {
        result = _impl->callTool(name, args);
      } catch (const std::exception &e) {
        result = errorResult(std::string("bad arguments: ") + e.what());
      }
      res.set_content(makeResult(id, result).dump(), "application/json");
    } else if (isNotification || method.rfind("notifications/", 0) == 0) {
      // A notification is acknowledged and not answered. Both ways of
      // spotting one lead here: no id at all, or a method under the
      // `notifications/` prefix. Testing the prefix later than it used
      // to be tested changes nothing -- no method under it can also be
      // `ping` or `tools/list`.
      res.status = kHttpAccepted;
      res.set_content("", "application/json");
    } else {
      res.set_content(
          makeError(id, kJsonRpcMethodNotFound, "method not found: " + method)
              .dump(),
          "application/json");
    }
  });

  _impl->thread = std::thread([this]() {
    bool ok = _impl->server.listen("127.0.0.1", _impl->port);
    if (!ok) {
      OkLogger::error("MCP", "Failed to bind MCP server to 127.0.0.1:" +
                                 std::to_string(_impl->port));
    }
  });
}

void OkMcpServer::stop() {
  if (_impl == nullptr) {
    return;
  }
  _impl->server.stop();
  if (_impl->thread.joinable()) {
    _impl->thread.join();
  }
}

void OkMcpServer::drainCommands() {
  // Track a simple measured frame rate (this runs once per rendered frame).
  double now = glfwGetTime();
  if (_impl->lastFrameTime > 0.0) {
    double dt = now - _impl->lastFrameTime;
    if (dt > 0.0) {
      _impl->fps =
          _impl->fps * kFpsSmoothing + (1.0 / dt) * (1.0 - kFpsSmoothing);
    }
  }
  _impl->lastFrameTime = now;

  std::deque<std::function<void()>> local;
  {
    std::scoped_lock lock(_impl->queueMutex);
    local.swap(_impl->queue);
  }
  for (size_t i = 0; i < local.size(); i++) {
    local[i]();
  }
}

#endif  // OKINAWA_WITH_MCP
