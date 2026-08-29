#ifndef OK_INPUT_NOTICE_HPP
#define OK_INPUT_NOTICE_HPP

/**
 * @brief The line that appears while physical input is blocked.
 *
 * A block is invisible by nature: the keyboard simply stops answering,
 * and the person at the window has no way of telling that from a hang.
 * So whenever input is being ignored, this says so and says how to take
 * it back -- and it disappears the moment input returns.
 *
 * Engine-side on purpose. Anything that can block input can leave
 * somebody locked out, so the notice belongs with the gate rather than
 * with whichever application remembers to draw one.
 */
class OkInputNotice {
public:
  OkInputNotice() = delete;

  // Show, hide and word the notice for this frame. Called once per frame
  // by the engine loop; does nothing at all while input is free.
  static void update();

  // Drop the UI. Called by OkCore::exit.
  static void shutdown();
};

#endif  // OK_INPUT_NOTICE_HPP
