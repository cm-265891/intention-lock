#include <assert.h>

#include "mgl/mgl.h"
#include "mgl/mgl_mgr.h"

namespace mgl {

std::atomic<uint64_t> MGLock::_idGen(0);
std::list<MGLock*> MGLock::_dummyList{};

MGLock::MGLock()
    :_id(_idGen.fetch_add(1, std::memory_order_relaxed)),
     _target(""),
     _targetHash(0),
     _mode(LockMode::LOCK_NONE),
     _res(LockRes::LOCKRES_UNINITED),
     _resIter(_dummyList.end()) {
}

MGLock::~MGLock() {
    assert(_res == LockRes::LOCKRES_UNINITED);
}

void MGLock::releaseLockResult() {
    std::lock_guard<std::mutex> lk(_mutex);
    _res = LockRes::LOCKRES_UNINITED;
    _resIter = _dummyList.end();
}

void MGLock::setLockResult(LockRes res, std::list<MGLock*>::iterator iter) {
    std::lock_guard<std::mutex> lk(_mutex);
    _res = res;
    _resIter = iter;
}

void MGLock::unlock() {
    LockRes status = getStatus();
    assert(status == LockRes::LOCKRES_OK || status == LockRes::LOCKRES_WAIT);
    MGLockMgr::getInstance().unlock(this);
    status = getStatus();
    assert(status == LockRes::LOCKRES_UNINITED);
}

LockRes MGLock::lock(const std::string& target, LockMode mode, uint64_t timeoutMs) {
    _target = target;
    _mode = mode;
    assert(getStatus() == LockRes::LOCKRES_UNINITED);
    _resIter = _dummyList.end();
    if (_target != "") {
        _targetHash = static_cast<uint64_t>(std::hash<std::string>{}(_target));
    } else {
        _targetHash = 0;
    }
    MGLockMgr::getInstance().lock(this);
    if (getStatus() == LockRes::LOCKRES_OK) {
        return LockRes::LOCKRES_OK;
    }
    if (waitLock(timeoutMs)) {
        return LockRes::LOCKRES_OK;
    } else {
        return LockRes::LOCKRES_TIMEOUT;
    }
}

std::list<MGLock*>::iterator MGLock::getLockIter() const {
    return _resIter;
}

void MGLock::notify() {
    _cv.notify_one();
}

bool MGLock::waitLock(uint64_t timeoutMs) {
    std::unique_lock<std::mutex> lk(_mutex);
    return _cv.wait_for(lk,
                        std::chrono::milliseconds(timeoutMs),
                        [this]() { return _res == LockRes::LOCKRES_OK; });
}

LockRes MGLock::getStatus() const {
    std::lock_guard<std::mutex> lk(_mutex);
    return _res;
}

}  // namespace mgl
