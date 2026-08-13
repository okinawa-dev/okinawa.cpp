// Background loading service.
//
// Threading bugs surface once in a thousand runs and on someone else's
// machine, so these lean on repetition and on hammering the service the
// way real use does: many jobs at once, jobs queued from inside other
// jobs, and shutdown while work is still in flight.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "okinawa/utils/async_loader.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace {

  // Wait until every submitted job has been drained, or give up. Returns
  // false on timeout so a hung service fails the test instead of hanging
  // the suite.
  bool drainUntilIdle(int timeoutMs = 5000) {
    std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();
    for (;;) {
      OkAsyncLoader::drain(1000.0f);
      if (OkAsyncLoader::getPendingCount() == 0) {
        return true;
      }
      if (std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - start)
              .count() > timeoutMs) {
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

}  // namespace

TEST_CASE("OkAsyncLoader runs both halves of a job", "[async]") {
  OkAsyncLoader::initialize(2);

  SECTION("Prepare runs, then finish") {
    std::atomic<int> prepared(0);
    std::atomic<int> finished(0);
    // Order matters: finish must never run before its own prepare.
    std::atomic<bool> orderKept(true);

    OkAsyncLoader::submit([&] { prepared++; },
                          [&] {
                            if (prepared.load() == 0) {
                              orderKept = false;
                            }
                            finished++;
                          });

    REQUIRE(drainUntilIdle());
    REQUIRE(prepared.load() == 1);
    REQUIRE(finished.load() == 1);
    REQUIRE(orderKept.load());
  }

  SECTION("A job may have no prepare half") {
    std::atomic<int> finished(0);
    OkAsyncLoader::submit(nullptr, [&] { finished++; });
    REQUIRE(drainUntilIdle());
    REQUIRE(finished.load() == 1);
  }

  SECTION("A job may have no finish half") {
    std::atomic<int> prepared(0);
    OkAsyncLoader::submit([&] { prepared++; }, nullptr);
    REQUIRE(drainUntilIdle());
    REQUIRE(prepared.load() == 1);
  }

  OkAsyncLoader::shutdown();
}

TEST_CASE("OkAsyncLoader runs every job exactly once", "[async]") {
  OkAsyncLoader::initialize(4);

  const int        COUNT = 200;
  std::atomic<int> prepared(0);
  std::mutex       finishMutex;
  std::vector<int> finishedIds;

  for (int i = 0; i < COUNT; i++) {
    OkAsyncLoader::submit([&prepared] { prepared++; },
                          [&finishMutex, &finishedIds, i] {
                            // The finish half is main-thread only, so it
                            // may touch plain containers without atomics
                            // -- the lock here only guards against the
                            // test itself being wrong about that.
                            std::lock_guard<std::mutex> lock(finishMutex);
                            finishedIds.push_back(i);
                          });
  }

  REQUIRE(drainUntilIdle());
  REQUIRE(prepared.load() == COUNT);
  REQUIRE((int)finishedIds.size() == COUNT);

  // Every id exactly once: no job lost, none run twice.
  std::set<int> unique(finishedIds.begin(), finishedIds.end());
  REQUIRE((int)unique.size() == COUNT);

  OkAsyncLoader::shutdown();
}

TEST_CASE("OkAsyncLoader finishes on the calling thread", "[async]") {
  OkAsyncLoader::initialize(3);

  // This is the service's core promise: whatever creates engine objects
  // must run on the thread that drains, because a rendering context
  // belongs to one thread.
  std::thread::id   drainThread = std::this_thread::get_id();
  std::atomic<bool> finishOnDrainThread(true);
  std::atomic<bool> prepareOffThread(true);
  std::atomic<int>  finished(0);

  for (int i = 0; i < 50; i++) {
    OkAsyncLoader::submit(
        [&] {
          if (std::this_thread::get_id() == drainThread) {
            prepareOffThread = false;
          }
        },
        [&] {
          if (std::this_thread::get_id() != drainThread) {
            finishOnDrainThread = false;
          }
          finished++;
        });
  }

  REQUIRE(drainUntilIdle());
  REQUIRE(finished.load() == 50);
  REQUIRE(finishOnDrainThread.load());
  REQUIRE(prepareOffThread.load());

  OkAsyncLoader::shutdown();
}

TEST_CASE("OkAsyncLoader honours the drain budget", "[async]") {
  OkAsyncLoader::initialize(2);

  const int        COUNT = 40;
  std::atomic<int> finished(0);
  for (int i = 0; i < COUNT; i++) {
    OkAsyncLoader::submit(nullptr, [&] {
      // Each finish costs a measurable slice, so a small budget cannot
      // possibly swallow them all in one call.
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      finished++;
    });
  }

  // Let the workers hand everything over first, so the budget is the
  // only thing limiting the drain.
  for (int spins = 0; spins < 500 && OkAsyncLoader::getReadyCount() < COUNT;
       spins++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  OkAsyncLoader::drain(5.0f);
  int afterBudget = finished.load();
  // A budget is a floor, not a hard cap: a job is never cut in half, so
  // one may overrun. What must not happen is the whole queue running.
  REQUIRE(afterBudget > 0);
  REQUIRE(afterBudget < COUNT);

  REQUIRE(drainUntilIdle());
  REQUIRE(finished.load() == COUNT);

  OkAsyncLoader::shutdown();
}

TEST_CASE("OkAsyncLoader counters track the queue", "[async]") {
  OkAsyncLoader::initialize(1);

  REQUIRE(OkAsyncLoader::getPendingCount() == 0);
  REQUIRE(OkAsyncLoader::getReadyCount() == 0);

  std::atomic<int> finished(0);
  for (int i = 0; i < 10; i++) {
    OkAsyncLoader::submit(nullptr, [&] { finished++; });
  }
  // Pending covers queued, in flight and ready alike: a loading screen
  // waits on it reaching zero.
  REQUIRE(OkAsyncLoader::getPendingCount() > 0);

  REQUIRE(drainUntilIdle());
  REQUIRE(OkAsyncLoader::getPendingCount() == 0);
  REQUIRE(OkAsyncLoader::getReadyCount() == 0);
  REQUIRE(finished.load() == 10);

  OkAsyncLoader::shutdown();
}

TEST_CASE("OkAsyncLoader without initialize runs jobs inline", "[async]") {
  // Callers should not have to care whether the service is up: a tool,
  // a test or an early startup path gets the same result, just blocking.
  std::atomic<int>  prepared(0);
  std::atomic<int>  finished(0);
  std::thread::id   here = std::this_thread::get_id();
  std::atomic<bool> ranHere(true);

  OkAsyncLoader::submit(
      [&] {
        if (std::this_thread::get_id() != here) {
          ranHere = false;
        }
        prepared++;
      },
      [&] { finished++; });

  // No drain call: an inline job is already done when submit returns.
  REQUIRE(prepared.load() == 1);
  REQUIRE(finished.load() == 1);
  REQUIRE(ranHere.load());
  REQUIRE(OkAsyncLoader::getPendingCount() == 0);
}

TEST_CASE("OkAsyncLoader survives shutdown with work in flight", "[async]") {
  // The interesting case is not the tidy one: it is quitting while
  // threads are mid-job, which is what happens when a player closes the
  // window during a load.
  for (int round = 0; round < 5; round++) {
    OkAsyncLoader::initialize(4);
    std::atomic<int> prepared(0);
    for (int i = 0; i < 100; i++) {
      OkAsyncLoader::submit(
          [&] {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
            prepared++;
          },
          [] {});
    }
    // Deliberately no drain: shutdown must join the workers and drop
    // the rest without deadlocking or crashing.
    OkAsyncLoader::shutdown();
    REQUIRE(OkAsyncLoader::getPendingCount() == 0);
  }
}

TEST_CASE("OkAsyncLoader can be restarted", "[async]") {
  OkAsyncLoader::initialize(2);
  std::atomic<int> first(0);
  OkAsyncLoader::submit([&] { first++; }, [] {});
  REQUIRE(drainUntilIdle());
  OkAsyncLoader::shutdown();

  // A second run must behave like the first: no stale threads, no
  // leftovers from the previous queue.
  OkAsyncLoader::initialize(2);
  std::atomic<int> second(0);
  OkAsyncLoader::submit([&] { second++; }, [] {});
  REQUIRE(drainUntilIdle());
  REQUIRE(first.load() == 1);
  REQUIRE(second.load() == 1);
  OkAsyncLoader::shutdown();
}

TEST_CASE("OkAsyncLoader tolerates a double initialize", "[async]") {
  OkAsyncLoader::initialize(2);
  OkAsyncLoader::initialize(8);  // ignored: already running

  std::atomic<int> finished(0);
  OkAsyncLoader::submit(nullptr, [&] { finished++; });
  REQUIRE(drainUntilIdle());
  REQUIRE(finished.load() == 1);

  OkAsyncLoader::shutdown();
  OkAsyncLoader::shutdown();  // and a double shutdown is harmless
}

TEST_CASE("OkAsyncLoader handles jobs queued from a finish half", "[async]") {
  // Chaining is how a caller gets ordering out of a service that has
  // none: each step queues the next from its own main-thread half.
  OkAsyncLoader::initialize(2);

  std::atomic<int> steps(0);
  std::vector<int> order;

  struct Chain {
    static void step(int n, std::atomic<int> *steps, std::vector<int> *order) {
      if (n > 4) {
        return;
      }
      OkAsyncLoader::submit(nullptr, [n, steps, order] {
        order->push_back(n);
        (*steps)++;
        Chain::step(n + 1, steps, order);
      });
    }
  };
  Chain::step(1, &steps, &order);

  REQUIRE(drainUntilIdle());
  REQUIRE(steps.load() == 4);
  REQUIRE(order.size() == 4);
  // Chained jobs DO keep their order, unlike independent ones.
  REQUIRE(order[0] == 1);
  REQUIRE(order[1] == 2);
  REQUIRE(order[2] == 3);
  REQUIRE(order[3] == 4);

  OkAsyncLoader::shutdown();
}

TEST_CASE("OkAsyncLoader survives many small jobs", "[async]") {
  // Repetition is the only real defence against a race: a scheduling
  // window that opens once in a thousand tries needs a thousand tries.
  OkAsyncLoader::initialize(4);

  const int        COUNT = 2000;
  std::atomic<int> prepared(0);
  std::atomic<int> finished(0);

  for (int i = 0; i < COUNT; i++) {
    OkAsyncLoader::submit([&] { prepared++; }, [&] { finished++; });
  }

  REQUIRE(drainUntilIdle(15000));
  REQUIRE(prepared.load() == COUNT);
  REQUIRE(finished.load() == COUNT);

  OkAsyncLoader::shutdown();
}
