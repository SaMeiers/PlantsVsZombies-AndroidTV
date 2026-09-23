#ifndef PVZ2NATIVE_RUNTIME_GUEST_SYNC_H
#define PVZ2NATIVE_RUNTIME_GUEST_SYNC_H

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace pvz_tv {

/* Minimal mutex primitive for guest pthread_mutex_t emulation. Deliberately
 * not backed directly by std::mutex: std::mutex::unlock() on an unlocked
 * mutex is UB, and guest code racing ahead of our still-incomplete
 * emulation is expected to occasionally do exactly that -- this must
 * degrade gracefully rather than crash the host. */
struct GuestMutex {
    std::mutex m;
    std::condition_variable cv;
    bool locked = false;

    /* Recursion, which pthread_mutex_t genuinely has and this genuinely needs.
     * A mutex created with PTHREAD_MUTEX_RECURSIVE may be locked again by the
     * thread that already holds it; without that, the second lock waits for a
     * release that only the waiting thread itself could perform -- a hang with
     * no bad state anywhere to point at. `owner` is the guest thread id. */
    bool recursive = false;
    bool type_known = false;
    std::uint32_t owner = 0;
    std::uint32_t depth = 0;

    void set_recursive(bool value) {
        std::lock_guard<std::mutex> lk(m);
        recursive = value;
        type_known = true;
    }

    void lock(std::uint32_t self = 0) {
        std::unique_lock<std::mutex> lk(m);
        if (locked && owner == self && self != 0 && recursive) {
            ++depth;
            return;
        }
        cv.wait(lk, [&] { return !locked; });
        locked = true;
        owner = self;
        depth = 1;
    }

    /* Bounded variant used by the import layer so a guest deadlock can be
     * reported instead of silently freezing the process. A blocked guest
     * thread never returns from jit.Run(), so the host pump stops running and
     * the window goes "Not responding" with no clue as to why. Returns false
     * on timeout, leaving the mutex untaken. */
    bool try_lock_for(std::chrono::milliseconds timeout, std::uint32_t self = 0) {
        std::unique_lock<std::mutex> lk(m);
        if (locked && owner == self && self != 0 && recursive) {
            ++depth;
            return true;
        }
        if (!cv.wait_for(lk, timeout, [&] { return !locked; })) return false;
        locked = true;
        owner = self;
        depth = 1;
        return true;
    }

    bool try_lock(std::uint32_t self = 0) {
        std::lock_guard<std::mutex> lk(m);
        if (locked && owner == self && self != 0 && recursive) {
            ++depth;
            return true;
        }
        if (locked) return false;
        locked = true;
        owner = self;
        depth = 1;
        return true;
    }

    void unlock(std::uint32_t self = 0) {
        std::lock_guard<std::mutex> lk(m);
        if (recursive && locked && owner == self && depth > 1) {
            --depth;
            return;
        }
        locked = false;
        owner = 0;
        depth = 0;
        cv.notify_one();
    }

    /* True when `self` is about to block on a lock it already holds and the
     * mutex is not recursive -- i.e. a guaranteed self-deadlock, which is worth
     * saying out loud rather than waiting out. */
    bool would_self_deadlock(std::uint32_t self) {
        std::lock_guard<std::mutex> lk(m);
        return locked && !recursive && self != 0 && owner == self;
    }
};

/* pthread_rwlock_t. New in 9.6.1, whose libc++_shared.so and engine both use
 * them.
 *
 * Deliberately NOT aliased onto GuestMutex, tempting as that is. Taking a read
 * lock twice on one thread is legal and ordinary -- a reader calls into another
 * reader -- and against a plain mutex the second one waits for a release only
 * the waiting thread could perform. That is a hang with nothing anywhere to
 * point at, which is the failure mode this project keeps paying for. So readers
 * are counted, and a recursive read is just another reader.
 *
 * Writers are exclusive and given no priority over readers: a writer-preferring
 * lock can starve readers, and nothing here needs the guarantee. */
struct GuestRwLock {
    std::mutex m;
    std::condition_variable cv;
    uint32_t readers = 0;
    bool writer = false;
    uint32_t writer_owner = 0; /* guest thread id, for the self-deadlock check */

    void rdlock() {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return !writer; });
        ++readers;
    }

    bool try_rdlock() {
        std::lock_guard<std::mutex> lk(m);
        if (writer) return false;
        ++readers;
        return true;
    }

    void wrlock(uint32_t self = 0) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return !writer && readers == 0; });
        writer = true;
        writer_owner = self;
    }

    bool try_wrlock(uint32_t self = 0) {
        std::lock_guard<std::mutex> lk(m);
        if (writer || readers != 0) return false;
        writer = true;
        writer_owner = self;
        return true;
    }

    /* One entry point for both, because pthread_rwlock_unlock is one function
     * and the caller does not say which kind it holds. */
    void unlock() {
        std::lock_guard<std::mutex> lk(m);
        if (writer) {
            writer = false;
            writer_owner = 0;
        } else if (readers > 0) {
            --readers;
        }
        /* notify_all, not notify_one: releasing a writer may free several
         * readers at once, and waking only one would leave the rest parked. */
        cv.notify_all();
    }
};

struct GuestCond {
    std::mutex m;
    std::condition_variable cv;
    uint64_t generation = 0;
};

struct GuestSem {
    std::mutex m;
    std::condition_variable cv;
    int count = 0;

    /* Bookkeeping for the deadlock report in libc_pthread.cpp, not for the
     * semantics. A hung guest thread only tells us WHERE it is blocked; these
     * say whether the wakeup it is waiting for was ever sent, and by whom --
     * which is the difference between "the post went to another address" and
     * "the post landed and we lost it". */
    uint32_t posts = 0;    /* sem_post calls seen at this address */
    uint32_t waiters = 0;  /* threads currently blocked in sem_wait here */
    uint32_t last_post_tid = 0;
    uint32_t last_post_lr = 0;
};

}  // namespace pvz_tv

#endif
