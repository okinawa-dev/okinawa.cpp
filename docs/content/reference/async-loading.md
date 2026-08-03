---
title: Background loading
section: Reference
nav_order: 16
---

# Background loading

`OkAsyncLoader` is the engine's single place where work happens off the
main thread. It exists so that reading and parsing a large piece of
world — a terrain cell, a level chunk, a model set — does not freeze the
frame while it happens.

## The two halves of a job

Every job has a **prepare** half and a **finish** half:

```cpp
OkAsyncLoader::submit(
    [data] { data->parseFromDisk(); },   // worker thread
    [data] { data->buildIntoScene(); }); // main thread
```

- **Prepare** runs on a worker thread. It may read files and turn them
  into plain data. It **must not touch the engine**: no textures, no
  scene, no lights, no logging. Nothing else in the engine is written
  for concurrent use, and a data race there produces corruption that
  shows up once in a thousand runs on somebody else's machine.
- **Finish** runs on the main thread, inside a time budget, and is where
  results become engine objects.

That split is not a matter of taste. **A rendering context belongs to
one thread**, and only that thread may create meshes or textures. What
makes the arrangement worthwhile is that parsing is the expensive part
and it parallelises cleanly, while construction is cheap and stays where
it must.

Either half may be null. A job with no prepare is a way to spread pure
main-thread construction across several frames.

## Draining

The core calls `drain(budgetMs)` once per frame, so a project normally
does nothing. The budget bounds how long the main thread spends turning
finished work into objects; it is set by `render.loadbudget`.

A budget is a floor, not a hard cap: **a job is never cut in half**, so
one long job can overrun it. Keep finish halves small — that is the
knob that actually controls the hitch.

## Ordering

Jobs finish in the order they *complete*, not the order they were
submitted: two jobs of different sizes on different workers will
overtake each other. When order matters, chain them — queue the next
step from the previous one's finish half:

```cpp
void loadStep(int n) {
  if (n > lastStep) {
    return;
  }
  OkAsyncLoader::submit([n] { parseStep(n); },
                        [n] { buildStep(n); loadStep(n + 1); });
}
```

## Waiting

`getPendingCount()` counts everything queued, in flight or waiting for
its main-thread half. A loading screen waits on it reaching zero.
`getReadyCount()` is the narrower question of how much is waiting to be
built.

## When the service is not running

If `submit` is called before `initialize` (or after `shutdown`), the job
runs **inline**: prepare and finish both execute on the calling thread
before submit returns. Callers do not have to care whether the service
is up — a tool, a test or an early startup path gets the same result,
only blocking. Nothing is silently dropped.

`shutdown` joins the workers and discards whatever is still queued,
which is what closing a window mid-load must do without deadlocking.

| Method | Purpose |
| --- | --- |
| `static void initialize(int workers = 0)` | Start the workers; `0` picks a number to suit the machine. |
| `static void submit(prepare, finish)` | Queue a job; either half may be null. |
| `static void drain(float budgetMs)` | Run finished jobs on the calling thread within a budget. |
| `static int getPendingCount()` | Jobs queued, in flight or waiting to be built. |
| `static int getReadyCount()` | Jobs waiting for their main-thread half. |
| `static void shutdown()` | Join the workers and drop what is queued. |

| Key | Default | Meaning |
| --- | --- | --- |
| `render.loadbudget` | `3.0` | Milliseconds per frame spent finishing background work. |

## Portability

Everything here is standard C++ threading, which behaves the same on
the desktop platforms. Keeping every thread and every file read behind
this one interface is deliberate: platforms differ in how they schedule
work and how they reach storage far more than they differ in C++, so
when a platform needs its own job system or its own storage API, this is
the file to rewrite — and nothing else.
