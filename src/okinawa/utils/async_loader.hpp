#ifndef OK_ASYNC_LOADER_HPP
#define OK_ASYNC_LOADER_HPP

#include <functional>
#include <string>

/**
 * @brief Background loading service: the engine's single place where
 *        work happens off the main thread.
 *
 *        A job has two halves. The PREPARE half runs on a worker thread
 *        and may read files and parse them into plain data; it must not
 *        touch the engine, because nothing else in it is written for
 *        concurrent use -- no textures, no scene, no lights, no
 *        logging. The FINISH half runs on the main thread, inside a
 *        time budget, and is where results become engine objects.
 *
 *        That split is not a style preference: a rendering context
 *        belongs to one thread and only that thread may create meshes
 *        or textures. Parsing is the expensive part and it parallelises
 *        cleanly; construction is cheap and stays where it must.
 *
 *        Keeping every thread and every file read behind this one
 *        interface is deliberate. Platforms differ in how they schedule
 *        work and how they reach storage far more than they differ in
 *        C++; when that day comes, this is the file to rewrite, and
 *        nothing else.
 *
 *        Jobs finish in the order they complete, not the order they
 *        were queued. A caller that needs ordering should chain: queue
 *        the next job from the previous one's finish half.
 */
class OkAsyncLoader {
public:
  OkAsyncLoader() = delete;

  // Work done on a worker thread. Anything it produces must reach the
  // finish callback through the caller's own captured state.
  using PrepareFn = std::function<void()>;
  // Work done on the main thread once prepare has returned.
  using FinishFn = std::function<void()>;

  // Start the worker threads. `workers` of 0 picks a sensible number
  // for the machine. Called by OkCore::initialize.
  static void initialize(int workers = 0);

  // Queue a job. `prepare` may be null for work that only needs the
  // main-thread half (useful to spread construction over frames).
  static void submit(const PrepareFn &prepare, const FinishFn &finish);

  // Run finished jobs on the calling (main) thread until the budget is
  // spent. Called once per frame by OkCore. A job is never cut in half:
  // the budget is checked between jobs, so one long job can overrun it.
  static void drain(float budgetMs);

  // Jobs queued or in flight, not yet finished. A loading screen can
  // wait on this reaching zero.
  static int getPendingCount();
  // Jobs prepared and waiting for their main-thread half.
  static int getReadyCount();

  // Stop the workers and drop anything still queued. Called by
  // OkCore::exit.
  static void shutdown();
};

#endif
