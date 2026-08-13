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

      if (job.prepare) {
        job.prepare();
      }

      {
        std::scoped_lock lock(g_mutex);
        g_inFlight--;
        g_ready.push_back(job);
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
    workers = std::min(workers, 4);
  }
  g_running = true;
  for (int i = 0; i < workers; i++) {
    g_workers.push_back(std::thread(workerLoop));
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
  return static_cast<int>(g_queued.size()) + g_inFlight + static_cast<int>(g_ready.size());
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
