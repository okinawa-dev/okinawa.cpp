---
title: Core
section: Reference
nav_order: 1
---

# Core

`OkCore` owns the window, the OpenGL context, the cameras and the main loop. It is a static class (the constructor is deleted), so every entry point is called through `OkCore::`. It also exposes the engine's scene handler, input and (optionally) the in-engine MCP server.

`OkCamera` is a transformable view onto the scene. It derives from `OkObject` (see [Items](/reference/items.html) for the shared transform API) and adds projection and view matrices.

## OkCore methods

| Method | Purpose |
| --- | --- |
| `static bool initialize()` | Create the window and OpenGL context. Returns false on failure. |
| `static void loop(step, draw)` | Run the main loop, calling the step and draw callbacks each frame. |
| `static void askForExit()` | Request the loop to end (typically from the step callback). |
| `static void exit()` | Tear down and exit. |
| `static OkSceneHandler *getSceneHandler()` | Access the scene handler. |
| `static OkCamera *getCamera()` | The current camera. |
| `static OkInput *getInput()` | The input subsystem. |
| `static void addCamera(OkCamera *camera)` | Register an additional camera. |
| `static void switchCamera(int index)` | Make the camera at `index` current. |
| `static OkCamera *getCameraAt(int index)` | Camera at `index` (`nullptr` out of range). |
| `static int findCamera(const std::string &name)` | Index of the camera registered with that name, `-1` when not found. |
| `static void enableMcpServer(int port = 8765)` | Start the in-engine MCP server (see [MCP server](/reference/mcp.html)). |
| `static void setIgnoreUserInput(bool ignore)` | Ignore physical input (MCP-driven instances). |
| `static void setOverlayCallback(cb)` | Draw over the finished frame, just before the swap. |
| `static void setExitCallback(cb)` | Run something once when the loop ends, before anything is torn down. |
| `static bool setWindowIcon(const std::vector<std::string> &pngPaths)` | Give the window an icon, from square RGBA PNGs at several sizes. |

The loop callbacks share the signature `void(float deltaTime)`.

### Drawing inside the frame or over it

The draw callback given to `loop()` runs **inside** the frame, before the
post-process composite. Anything it draws belongs to the world and is
treated as such: bloomed, fogged, and blurred by depth of field along
with everything else. That is what you want for a debug overlay meant to
sit in the scene, and wrong for an interface meant to be read.

`setOverlayCallback()` runs **after** the composite and after the
interface pass, with the default framebuffer bound. Use it for anything
painted on the finished image -- an immediate-mode interface library,
for instance, which draws its own geometry and expects the final target.

```cpp
OkCore::setOverlayCallback([](float dt) {
  (void)dt;
  // draw over the finished frame
});
OkCore::loop(stepCallback, drawCallback);
```

Pass an empty function to remove it. The order within a frame is: scene,
draw callback, post-process composite, cameras, interface pass, overlay
callback, swap.

### Icons, and where an application is run from

`setWindowIcon` takes the same picture at several sizes and lets the
window system choose; a 32 and a 64 covers a title bar and a task bar.
Drawing each size separately beats handing over one large one, because
an icon is mostly seen small and a downscaled large one goes soft.

**On macOS it lands on the Dock tile.** Windows there carry no icon at
all, so `glfwSetWindowIcon` has nothing to set — the application's icon
on that platform *is* the tile, and AppKit is the only way to it. The
engine does that for you from the same call, so an application asks once
and needs no `#ifdef`.

A macOS **bundle** says the same thing statically —
`Contents/Resources/<name>.icns`, named by `CFBundleIconFile` in
`Info.plist` — and that is what the Finder and the application switcher
read, which the runtime call cannot reach. The two are complementary:
the bundle for an application that is installed and double-clicked, the
call for a binary run straight out of a build directory, which has no
bundle to be read from.

Elsewhere the packaging differs again: Windows wants the icon compiled
into the executable as a resource for the Explorer to show, and Linux
desktops read a `.desktop` entry and the hicolor theme. One picture,
three packaging stories, one call at runtime.

Packaging an application as a macOS bundle changes one more thing: a
bundle opened from the desktop starts with a working directory of `/`,
and GLFW moves it again on its own, into `Contents/Resources`. The
engine turns that off (`GLFW_COCOA_CHDIR_RESOURCES`) — where an
application works from is the application's decision — and instead the
asset search settles it: it looks up from the working directory first,
and then up from **the executable's own directory**, moving the working
directory to what it finds. An application started from anywhere
therefore resolves its own relative paths — its data, its assets, the
files it writes — against its project, exactly as it does in
development. `OkFiles::executableDirectory()` is the piece that answers
where the binary itself is, asked of the operating system rather than of
`argv[0]`, which a caller can set to anything.

### The last thing an application does

Most applications have something to write down on the way out — where
the user was, what they had open — and the obvious places to do it are
both wrong. After `loop()` returns is too late: it calls `exit()` before
returning, and that deletes the scene, the input and every camera, so an
application asking where its camera was is asking a deleted object.
Doing it every frame in case this one is the last is paying, sixty times
a second, for something that happens once.

`setExitCallback()` runs in between: once, on the loop thread, with the
loop stopped and the scene still intact. It does not care what ended the
loop — the window's close button, `askForExit()`, or the MCP
[`quit` tool](/reference/mcp.html) all arrive here.

```cpp
OkCore::setExitCallback([]() {
  saveWhereverTheUserWas();   // the camera is still alive here
});
```

Nothing runs when the process is killed from outside. Nothing can.

## OkCamera methods

| Method | Purpose |
| --- | --- |
| `OkCamera(name, width, height)` | Construct a camera for a given framebuffer size. |
| `void setPerspective(fovDegrees, near, far)` | Set the projection. |
| `const glm::mat4 &getView() const` | The view matrix. |
| `const glm::mat4 &getProjection() const` | The projection matrix. |
| `float viewDistance() const` | How far the camera sits from what it observes (orbit distance, overhead height); `0` when the notion does not apply. |
| `void setViewDistance(float d)` | Write counterpart of `viewDistance()`: drive the distance/height directly. Base ignores it; subclasses apply and clamp. |
| `OkRay rayThroughPixel(x, y, width, height) const` | The ray through a point on the surface being drawn to. |
| `bool pixelOfPoint(const OkPoint &world, width, height, double *outX, double *outY) const` | Where a world point lands on it, or `false` when the point is behind the camera. |

`OkCamera` also inherits `setPosition`, `setRotation` and the rest of the `OkObject` transform API.

### Between the screen and the world

`rayThroughPixel` and `pixelOfPoint` are inverses, and they are documented
together because they only mean anything as a pair: a projection and an
unprojection that disagree about which way y counts read as a picking bug
for an afternoon. Both count y **downwards from the top**, the way a
window system reports a cursor, and both take the size of the surface
being drawn to rather than reading the camera's aspect ratio — so a camera
rendering into an offscreen target answers about that target's pixels.

The ray is built by unprojecting at the near plane and again at the far
one and subtracting, rather than from the camera's own position. That is
right for a perspective camera and for an orthographic one alike: an
orthographic ray does not pass through the eye, and the overhead views
that most want a cursor are exactly the ones that tend to be orthographic.

Its direction is a unit vector, so a distance along it is a distance in
world units. See [OkRay](/reference/math.html#okray) for what to do with
it, and [`OkItem::intersectRay`](/reference/items.html) for asking an
object whether the ray crosses it.

Cameras are **identified by the name they are constructed with**: `findCamera` resolves names to indices, and the MCP [`view` tool](/reference/mcp.html) selects cameras by that same name. Indices (and the number-key switching in the stock input handler) are an internal/debug detail.

## Example

```cpp
if (!OkCore::initialize()) {
  return 1;
}

OkCamera *camera = OkCore::getCamera();
camera->setPosition(0.0f, 0.0f, 5.0f);
camera->setPerspective(45.0f, 0.1f, 1000.0f);

OkCore::loop(stepCallback, drawCallback);
```
