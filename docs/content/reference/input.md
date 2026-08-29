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

So a block **expires by itself**, `releaseHeldFor()` reads
**escape** straight from the device before the gate that would have
swallowed it, and a hold of `RELEASE_HOLD_SECONDS` lifts the block, and the engine draws a line saying input is held
and how to take it back. Nothing running in the background can talk its
way past those. `physicalInputBlockedFor()` reports what is left: `0`
when input is free, and a negative number for a block with no deadline
(what the launch flag asks for).

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
