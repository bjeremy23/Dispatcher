# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

An event dispatcher that listens for file descriptor events using Boost.Asio and C++20 coroutines. Applications register interest in FD events, await them via coroutines, and continue processing when events fire. Designed for network I/O (TCP/UDP) with a focus on readability and clean encapsulation.

## Build & Test Commands

```bash
make            # Build the project
make clean      # Clean build artifacts
make test       # Build and run all unit tests
```

## Architecture

### Directory Layout

```
src/
  dispatcher/
    Dispatcher.hpp / Dispatcher.cpp
    CoroutineHandle.hpp / CoroutineHandle.cpp
    ExecutorContext.hpp / ExecutorContext.cpp
  socket/
    Socket.hpp                          # Abstract interface
    TcpSocket.hpp / TcpSocket.cpp
    UdpSocket.hpp / UdpSocket.cpp
  msgbuffer/
    MsgBuffer.hpp / MsgBuffer.cpp
    MsgBufferPool.hpp / MsgBufferPool.cpp
test/
  DispatcherTest.cpp
  TcpSocketTest.cpp
  UdpSocketTest.cpp
  MsgBufferPoolTest.cpp
Makefile
```

### Classes

#### `ExecutorContext`
- Single static class that holds the active `boost::asio::any_io_executor`
- `Dispatcher` registers its executor here on construction via `ExecutorContext::set()`
- Socket types call `ExecutorContext::get()` in their default constructors
- Decouples `Dispatcher` from all socket types — adding a new socket never requires changing `Dispatcher`
- Throws `std::runtime_error` if `get()` is called before any `Dispatcher` has been constructed

#### `Dispatcher`
- Owns `boost::asio::io_context` internally (never exposed)
- Registers its executor with `ExecutorContext` on construction
- Accepts optional thread count at construction (default = 1); runs a thread pool internally when N > 1
- At N > 1 threads, each spawned coroutine runs on a strand for thread safety
- `spawn(awaitable)` launches a coroutine and returns a `CoroutineHandle` for cancellation
- `run()` drives the event loop (single-threaded mode); thread pool starts automatically in multi-thread mode
- `stop()` shuts down the loop
- Has no knowledge of socket types — does not expose any socket factory methods

#### `CoroutineHandle` (returned by `Dispatcher::spawn`)
- Wraps a `boost::asio::cancellation_signal` internally
- `cancel()` emits `cancellation_type::terminal` — cancels at the next `co_await` point
- Coroutine receives `boost::asio::error::operation_aborted` on the awaited op when cancelled

#### `Socket` (abstract interface)
- Defines the async interface all socket types share
- `co_await wait_readable()` → `boost::asio::awaitable<std::error_code>`
- `co_await wait_writable()` → `boost::asio::awaitable<std::error_code>`
- `read(MsgBuffer&)` → `std::error_code` (synchronous after wait_readable signals)
- `write(std::span<char>)` → `std::error_code` (synchronous after wait_writable signals)
- Raw FD is never exposed — internals hidden via pimpl

#### `TcpSocket`
- pimpl hides `boost::asio::ip::tcp::socket`
- Bi-directional: supports both connect (client) and accept (server)
- Error handling via `std::error_code`, no exceptions thrown from I/O ops

#### `UdpSocket`
- pimpl hides `boost::asio::ip::udp::socket`
- Bi-directional: supports both send and receive (with endpoint tracking)
- Error handling via `std::error_code`, no exceptions thrown from I/O ops

#### `MsgBuffer`
- Owns a `unique_ptr<char[]>` buffer, fixed at 4096 bytes by default
- Buffer size is configurable in powers of 2 (512, 1024, 2048, 4096, ...)
- Tracks bytes written; not copyable (move-only)

#### `MsgBufferPool`
- Fixed-size pool of `MsgBuffer` objects
- `acquire()` returns a `shared_ptr<MsgBuffer>` with a custom deleter that returns the buffer to the pool
- Exhaustion policy set at construction via `ExhaustionPolicy` enum:
  - `ExhaustionPolicy::Throw` — throws `PoolExhaustedException` when empty (default)
  - `ExhaustionPolicy::Allocate` — allocates a new buffer outside the pool when empty
- Users only instantiate `MsgBufferPool`; acquiring and returning buffers is managed internally

### Coroutine Pattern

`co_await` signals readiness only — it does not return data. After the await, the caller reads into an acquired `MsgBuffer`:

```cpp
auto handle = dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
    auto ec = co_await socket.wait_readable();
    if (ec) { /* handle error */ co_return; }

    auto buf = pool.acquire();        // shared_ptr<MsgBuffer>, returns to pool on last release
    socket.read(*buf);                // fill buffer now that FD is ready
    // process buf...
});

// Later, if needed:
handle.cancel();
```

### Completion Tokens

- `boost::asio::detached` is used exclusively — all spawned coroutines are fire-and-forget
- Cancellation via `CoroutineHandle` is the mechanism for controlling coroutine lifetime
- Internally, `spawn()` uses `bind_cancellation_slot(signal.slot(), detached)`

### Cancellation

- `CoroutineHandle::cancel()` triggers `cancellation_type::terminal`
- Cancellation takes effect at the next `co_await` point
- Cancelled ops return `boost::asio::error::operation_aborted` via `std::error_code`

## Requirements

- C++20 (coroutines support required)
- Boost.Asio for all async I/O
- Google Test for unit tests
- SOLID principles — one class per file, depend on abstractions (`Socket` interface)
- pimpl idiom to hide Boost.Asio types from public headers
- Prefer `std::error_code` over exceptions for I/O errors; exceptions are acceptable for pool exhaustion
- Prioritize readability over cleverness
- Ask clarifying questions before making architectural changes
