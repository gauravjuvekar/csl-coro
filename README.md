csl-coro - Lock-free async-interrupt-safe stackless coroutines in C
-------------------------------------------------------------------

csl-coro is a lock-free async-interrupt-safe implementation of stackless coroutines in C for use in interrupt-based (mostly embedded) single-threaded systems. It depends on the [aint-safe](http://github.com/gauravjuvekar/aint-safe) library.


## Features
- Purely [C11 atomics](http://en.cppreference.com/w/c/atomic).
- Lock-free up to the C11 implementation of `stdatomic`
- No dynamic memory use - `malloc`.
