#ifndef SRC_MGL_MGL_MGR_H__
#define SRC_MGL_MGL_MGR_H__

#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>

#include "lock_defines.h"

namespace mgl {

class MGLock;

// TODO(deyukong): this class should only be in mgl_mgr.cpp
// not thread safe, protected by LockShard's mutex
class LockSchedCtx {
 public:
    LockSchedCtx();
    LockSchedCtx(LockSchedCtx&&) = default;
    void lock(MGLock* core);
    void unlock(MGLock* core);
 private:
    void schedPendingLocks();
    void incrPendingRef(LockMode mode);
    void incrRunningRef(LockMode mode);
    void decPendingRef(LockMode mode);
    void decRunningRef(LockMode mode);
    uint16_t _runningModes;
    uint16_t _pendingModes;
    std::vector<uint16_t> _runningRefCnt;
    std::vector<uint16_t> _pendingRefCnt;
    std::list<MGLock*> _runningList;
    std::list<MGLock*> _pendingList;
};

// class alignas(std::hardware_destructive_interference_size) LockShard {
// hardware_destructive_interference_size requires quite high version
// gcc. 128 should work for most cases

struct alignas(128) LockShard {
    std::mutex mutex;
    std::unordered_map<std::string, LockSchedCtx> map;
};

class MGLockMgr {
 public:
    void lock(MGLock* core);
    void unlock(MGLock* core);
    static MGLockMgr& getInstance();
 private:
    MGLockMgr() = default;
    static constexpr size_t SHARD_NUM = 32;
    LockShard _shards[SHARD_NUM];
};

}  // namespace mgl

#endif  // SRC_MGL_MGL_MGR_H__
