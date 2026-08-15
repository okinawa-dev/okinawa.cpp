#include "async_loader.hpp"

#include "logger.hpp"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace {

  struct Job {
    OkAsyncLoader::PrepareFn prepare;
    OkAsyncLoader::FinishFn  finish;
  };

  std::vector<std::thread> g_workers;
  std::deque<Job>          g_queued;  // waiting for a worker
  std::deque<Job>          g_ready;   // prepared, waiting for the main thread
  std::mutex               g_mutex;
  std::condition_variable  g_wake;
  bool                     g_running  = false;
  int                      g_inFlight = 0;  // taken by a worker, not yet ready

  /**
   * @brief Joins the workers on the way out, whatever the way out was.
   *
   *        `shutdown()` is called by OkCore::exit(), which an
   *        application only reaches if it got as far as running its
   *        loop. One that fails earlier -- no data to open, a file it
   *        cannot read, anything that leaves main by the error door --
   *        never calls it, and then these threads are destroyed while
   *        still joinable. Destroying a joinable std::thread calls
   *        std::terminate, so a clean "could not start, here is why"
   *        turned into an abort: on a desktop, a crash dialog reporting
   *        nothing, in place of the message that says what went wrong.
   *
   *        Declared after the workers on purpose. Statics are destroyed
   *        in reverse order of construction, so this one goes first and
   *        empties the vector the next one would have destroyed.
   */
  struct WorkerReaper {
    ~WorkerReaper() {
      // A destructor that throws during static teardown is the very
      // abort this exists to prevent, so whatever shutdown() runs into
      // stops here.
      try {
        OkAsyncLoader::shutdown();
      } catch (...) {  // NOLINT(bugprone-empty-catch)
      }
    }
  };
  WorkerReaper g_reaper;

  void workerLoop() {
    for (;;) {
      Job job;
      {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_wake.wait(lock, [] { return !g_running || !g_queued.empty(); });
        if (!g_running && g_queued.empty()) {
          return;
        }
        job = g_queued.front();
        g_queued.pop_front();
        g_inFlight++;
      }

      // Nothing may leave a worker by throwing. An exception that
      // escapes a thread function is std::terminate, and terminate is
      // an abort: the application dies with no message worth reading,
      // and on a desktop with a "quit unexpectedly" dialog that names
      // nothing. Files are exactly where this bites -- one being
      // rewritten while it is read, one that vanished between the
      // listing and the open -- which is ordinary and must not be
      // fatal.
      bool prepared = true;
      if (job.prepare) {
        try {
          job.prepare();
        } catch (const std::exception &err) {
          prepared = false;
          OkLogger::error("AsyncLoader",
                          std::string("Job failed while preparing: ") +
                              err.what());
        } catch (...) {
          prepared = false;
          OkLogger::error("AsyncLoader", "Job failed while preparing");
        }
      }

      {
        std::scoped_lock lock(g_mutex);
        g_inFlight--;
        // A job whose preparation failed is dropped rather than passed
        // on: its finish half would be building something out of
        // whatever the failure left behind.
        if (prepared) {
          g_ready.push_back(job);
        }
      }
    }
  }

}  // namespace

void OkAsyncLoader::initialize(int workers) {
  if (g_running) {
    return;
  }
  if (workers <= 0) {
    // Leave the main thread its core, and do not flood a small machine:
    // this service exists to hide latency, not to saturate the CPU.
    unsigned int hw = std::thread::hardware_concurrency();
    workers         = static_cast<int>(hw > 2 ? hw - 1 : 1);
    workers         = std::min(workers, 4);
  }
  g_running = true;
  for (int i = 0; i < workers; i++) {
    g_workers.emplace_back(workerLoop);
  }
  OkLogger::info("AsyncLoader",
                 "Started with " + std::to_string(workers) + " worker(s)");
}

void OkAsyncLoader::submit(const PrepareFn &prepare, const FinishFn &finish) {
  Job job;
  job.prepare = prepare;
  job.finish  = finish;
  if (!g_running) {
    // No service: run it here so callers work the same either way.
    if (job.prepare) {
      job.prepare();
    }
    if (job.finish) {
      job.finish();
    }
    return;
  }
  {
    std::scoped_lock lock(g_mutex);
    g_queued.push_back(job);
  }
  g_wake.notify_one();
}

void OkAsyncLoader::drain(float budgetMs) {
  std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();
  for (;;) {
    Job job;
    {
      std::scoped_lock lock(g_mutex);
      if (g_ready.empty()) {
        return;
      }
      job = g_ready.front();
      g_ready.pop_front();
    }
    if (job.finish) {
      job.finish();
    }
    double spent = std::chrono::duration<double, std::milli>(
                       std::chrono::steady_clock::now() - start)
                       .count();
    if (spent >= static_cast<double>(budgetMs)) {
      return;
    }
  }
}

int OkAsyncLoader::getPendingCount() {
  std::scoped_lock lock(g_mutex);
  return static_cast<int>(g_queued.size()) + g_inFlight +
         static_cast<int>(g_ready.size());
}

int OkAsyncLoader::getReadyCount() {
  std::scoped_lock lock(g_mutex);
  return static_cast<int>(g_ready.size());
}

void OkAsyncLoader::shutdown() {
  if (!g_running) {
    return;
  }
  {
    std::scoped_lock lock(g_mutex);
    g_running = false;
    g_queued.clear();
  }
  g_wake.notify_all();
  for (size_t i = 0; i < g_workers.size(); i++) {
    if (g_workers[i].joinable()) {
      g_workers[i].join();
    }
  }
  g_workers.clear();
  {
    std::scoped_lock lock(g_mutex);
    g_ready.clear();
    g_inFlight = 0;
  }
  OkLogger::info("AsyncLoader", "Stopped");
}
