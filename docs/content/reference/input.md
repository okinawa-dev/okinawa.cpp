---
title: Input
section: Reference
nav_order: 7
---

# Input

`OkInput` reads the keyboard and mouse each frame and exposes both an edge/level query API and a digested `OkInputState`. You usually reach it through `OkCore::getInput()` inside the step callback. Keys are referenced through the platform-independent `OkKey` enum (see `input/keys.hpp`).

## OkInput methods

| Method | Purpose |
| --- | --- |
| `bool isKeyJustPressed(OkKey key) const` | True only on the frame the key goes down. |
| `bool isKeyHeld(OkKey key) const` | True while the key is held. |
| `bool isKeyJustReleased(OkKey key) const` | True only on the frame the key is released. |
| `OkInputState getState() const` | The digested per-frame input state. |
| `void injectKey(OkKey key, double durationSeconds)` | Synthesize a key press (used to drive the app programmatically). |
| `void injectPointerTo(double x, double y)` | Put the injected pointer at a place, in window pixels from the top left. |
| `void injectPointerBy(double dx, double dy)` | Move it from where it is, in the same pixels. |
| `void injectPointerButton(int button, bool down)` | Hold or release one of its buttons: 0 left, 1 right, 2 middle. |
| `void injectPointerWheel(double notches)` | Turn its wheel, positive away from the hand. |
| `void injectedPointer(double *x, double *y) const` | Where it is. |
| `bool injectedButton(int button) const` | Whether that button is held. |
| `double takeInjectedWheel()` | What the wheel has turned since this was last asked, **and clears it**. |
| `bool injectedPointerUsed() const` | Whether anything has driven it at all. |
| `void setPhysicalInputEnabled(bool enabled)` | Enable/disable physical keyboard/mouse input. |
| `void setCursorCaptured(bool captured)` | Capture (hide + lock for mouse-look) or release the OS cursor. |
| `void setPointerLockOnClick(bool enabled)` | Whether a click may take the pointer at all (default on). |
| `bool isCursorCaptured() const` | Whether the cursor is currently captured. |
| `void setTextCapture(bool captured)` | Text capture (the console): the normal accessors and `getState()` report nothing while set, so typing cannot trigger gameplay keys. |
| `bool isKeyJustPressedRaw/isKeyHeldRaw(OkKey)` | Variants that ignore the text-capture flag (console internals). |
| `void onChar(unsigned int codepoint)` | Queue a typed character (fed by the GLFW char callback; printable injected keys also land here while captured). |
| `std::string drainChars()` | Return and clear the characters typed since the last call. |

## Mouse cursor (pointer lock)

Mouse-look uses a **pointer-lock** model rather than holding the cursor captured for the whole session:

- The app starts with a **normal OS cursor**, so the window can be moved (title bar), sent to another desktop, and the OS chrome used normally.
- **Clicking inside the render area captures** the cursor (hidden + locked) and enables mouse-look. Clicks on the title bar / OS chrome are not delivered to the engine, so they keep working.
- **ESC releases** the cursor (browser style). When the cursor is already released, ESC sets `exit` instead (the app's quit request).
- **Losing window focus** releases the cursor automatically (frees the OS pointer and, on macOS, restores system-wide mouse acceleration); the user clicks back into the view to resume mouse-look.
- With physical input disabled (`setPhysicalInputEnabled(false)`, e.g. `--no-input`) the cursor is never captured; drive the view through the MCP `view` tool instead.

### Blocking input for a while

`blockPhysicalInput(seconds)` is the same gate with a deadline on it, for
an agent that needs the view to hold still while it measures something.
It exists in that shape because of what goes wrong: a block is invisible
-- the keyboard just stops answering -- so one that is never lifted looks
from a chair exactly like a hang.

So a block **expires by itself**, the combo in `RELEASE_MODS` +
`RELEASE_KEY` lifts it, and the engine draws a line saying input is held
and how to take it back. Nothing running in the background can talk its
way past those. `physicalInputBlockedFor()` reports what is left: `0`
when input is free, and a negative number for a block with no deadline
(what the launch flag asks for).

### The injected pointer

Keys are half of driving an application; the other half is the pointer,
and an application with an interface of its own is nearly all pointer.
The engine keeps an injected pointer -- a position in **window pixels
from the top left**, three buttons and a wheel -- and says whether
anything has used it. What to do with it is the application's business,
exactly as with injected keys: nothing here pretends to be the window
system, and a UI library reading the window system directly will not
see it unless the application hands it over. The MCP `mouse` tool drives
these, and the [MCP reference](mcp.html#driving-the-pointer) shows the
handover for Dear ImGui.

`takeInjectedWheel()` clears what it returns. Read it once a frame: a
notch delivered twice is one turn of the wheel doing two steps.

### An application gate is not somebody being held out

`setPhysicalInputEnabled(false)` and `blockPhysicalInput(seconds)` reach
the same gate, and they are not the same thing. The first is an
application routing input somewhere else for a moment -- an editor does
it every time the cursor crosses one of its panels, so that a drag
inside a window does not also fly the camera. The second is somebody
outside holding the keyboard.

Only the second is reported by `physicalInputBlockedFor()` and only the
second draws the notice on screen. They were one thing once, and the
result was an editor announcing that an agent held the keyboard every
time the mouse touched a window.

### Combos: at once, and one after another

A combo comes in the two kinds `okinawa.js` named: **simultaneous**,
several keys held together, and **consecutive**, one key after another
inside a time window. Only the simultaneous one is implemented; the
press history below is what the other would be matched on.

`isComboJustPressed(mods, key)` is the simultaneous one: the
modifiers already held, the key going down this frame, and **exactly**
those modifiers -- a combo that fired on "at least" would swallow every
gesture that contains it.

It reads the DEVICE and not what the application is allowed to see, so a
combo still works while input is blocked. That is the case the engine
itself needs: the gesture that gives the keyboard back is wanted
precisely when the keyboard is being ignored.

`RELEASE_MODS`, `RELEASE_KEY` and `releaseComboName()` hold that gesture
in one place, so the line the engine draws and the combo that actually
works cannot drift apart. It is **not** escape, which is what it used
first: **macOS does not deliver Escape to an application while Control
is held** -- measured on the device, with the modifiers arriving, the
Escape never arriving, and `ctrl+shift+E` arriving in the same breath.

**`consumeKeyUntilReleased(key)` is what makes a combo usable rather
than merely detectable.** The keys of a combo mean something on their
own, so whoever acts on one has to take them out of the frame and keep
them out until the gesture is over. Without it the release worked
exactly once: it lifted the block, and the escape still held closed the
application a frame later. A letter is no safer by itself --
`ctrl+shift+E` walks the avatar upwards, because `E` is a movement key
and the modifiers mean nothing to it.

### The press history

`recentPresses()` keeps the last `PRESS_HISTORY` keys the DEVICE
reported going down, each with the modifiers held at the time and when
it happened. It answers the question that is normally asked afterwards
-- did that key ever arrive? was the combo performed the way it was
described? -- instead of needing somebody to catch the moment. The macOS
fault above was found with it in one reading, after three rounds of
guessing at it.

It is also what a CONSECUTIVE combo would be matched on: one key after
another inside a time window. That matcher does not exist yet; the
record it needs does.

### Applications that are pointed at rather than steered

The model above is what a game wants. An application whose cursor is the
instrument -- one where clicking is how things are chosen -- wants the
opposite, and `setPointerLockOnClick(false)` gives it: no click ever
takes the pointer, and the cursor stays on screen for good.

It has to be a deliberate choice, because it is a trade. Mouse-look
reads the locked pointer's motion, so switching pointer lock off
switches mouse-look off with it; a tool that does this drives its camera
from keys, or from a drag it interprets itself.

Without it the first click is the last one the user can aim: the cursor
disappears on the way in, and every click after that is made blind at a
window that no longer shows where the pointer is.

```cpp
OkCore::getInput()->setPointerLockOnClick(false);
```

The frame's mouse pan delta (`panX` / `panY`) follows the same rule from
the other side. It is fed while the cursor is captured — or, in an
application that has switched capture off altogether, always. Otherwise
a delta that waited for a capture which never comes would never arrive,
and every pan-style controller would sit still in exactly the kind of
application that most wants one. Applications that leave capture on are
unaffected: for them the condition is still "captured", so a mouse
crossing the window before any click moves nothing.

Mouse-**look** keeps needing the capture either way. A free cursor aimed
at a menu would be swinging the view along behind it.

## OkInputState fields

`getState()` returns a struct with ready-to-use flags. Movement: `forward`, `backward`, `strafeLeft`, `strafeRight`, plus the vertical nudge `moveUp` / `moveDown` (E / Q held). Rotation: `turnLeft`, `turnRight`, `turnUp`, `turnDown`. Mouse pan: `panX` / `panY`, the frame's raw mouse pixel delta (accumulated by the cursor callback while the cursor is captured, or always when the application has switched capture off; consumed by pan-style controllers). Edge-triggered actions (true only on the frame first pressed): `action1`, `action2`, `action3`, `action4`. Camera selection: `changeCamera` (-1 if none). And `exit`, set when the user asks to quit (ESC while the cursor is already released; a captured cursor consumes ESC to release first).

## Example

```cpp
void stepCallback(float deltaTime) {
  OkInput     *input  = OkCore::getInput();
  OkInputState state  = input->getState();

  if (state.exit) {
    OkCore::askForExit();
    return;
  }

  OkCamera *camera = OkCore::getCamera();
  OkPoint   forward = camera->getRotation().getForwardVector();
  if (state.forward) {
    camera->move(forward.x() * deltaTime,
                 forward.y() * deltaTime,
                 forward.z() * deltaTime);
  }
}
```
