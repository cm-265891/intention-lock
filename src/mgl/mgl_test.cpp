#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include "gtest/gtest.h"
#include "mgl/mgl.h"
#include "mgl/mgl_mgr.h"

namespace mgl {
TEST(ConflictTable, Common) {
    uint16_t none = 0;
    EXPECT_FALSE(isConflict(none, LockMode::LOCK_NONE));
    EXPECT_FALSE(isConflict(none, LockMode::LOCK_IS));
    EXPECT_FALSE(isConflict(none, LockMode::LOCK_IX));
    EXPECT_FALSE(isConflict(none, LockMode::LOCK_S));
    EXPECT_FALSE(isConflict(none, LockMode::LOCK_X));

    uint16_t isix = (1<<1)|(1<<2);
    EXPECT_FALSE(isConflict(isix, LockMode::LOCK_NONE));
    EXPECT_FALSE(isConflict(isix, LockMode::LOCK_IS));
    EXPECT_FALSE(isConflict(isix, LockMode::LOCK_IX));
    EXPECT_TRUE(isConflict(isix, LockMode::LOCK_S));
    EXPECT_TRUE(isConflict(isix, LockMode::LOCK_X));

    uint16_t x = (1<<4);
    EXPECT_FALSE(isConflict(x, LockMode::LOCK_NONE));
    EXPECT_TRUE(isConflict(x, LockMode::LOCK_IS));
    EXPECT_TRUE(isConflict(x, LockMode::LOCK_IX));
    EXPECT_TRUE(isConflict(x, LockMode::LOCK_S));
    EXPECT_TRUE(isConflict(x, LockMode::LOCK_X));
}

TEST(LockShard, Align) {
    EXPECT_TRUE(sizeof(LockShard) >= 128);
}

TEST(MGL, OneTarget) {
    MGLock l1, l2, l3, l4, l5;
    EXPECT_EQ(l1.lock("something", LockMode::LOCK_IS, 1000), LockRes::LOCKRES_OK);
    EXPECT_EQ(l2.lock("something", LockMode::LOCK_IS, 1000), LockRes::LOCKRES_OK);
    EXPECT_EQ(l3.lock("something", LockMode::LOCK_IX, 1000), LockRes::LOCKRES_OK);
    EXPECT_EQ(l4.lock("something", LockMode::LOCK_IX, 1000), LockRes::LOCKRES_OK);
    EXPECT_EQ(l5.lock("something", LockMode::LOCK_S, 1000), LockRes::LOCKRES_TIMEOUT);
    l1.unlock();
    l2.unlock();
    l3.unlock();
    l4.unlock();
    l5.unlock();
}

TEST(MGL, MultiTarget) {
    MGLock l1, l2;
    EXPECT_EQ(l1.lock("something", LockMode::LOCK_IS, 1000), LockRes::LOCKRES_OK);
    EXPECT_EQ(l2.lock("something1", LockMode::LOCK_S, 1000), LockRes::LOCKRES_OK);
    l1.unlock();
    l2.unlock();
}

TEST(MGL, MultiThread) {
    MGLock l1, l2, l3, l4, l5;
    EXPECT_EQ(l1.lock("something", LockMode::LOCK_IS, 1000), LockRes::LOCKRES_OK);
    EXPECT_EQ(l2.lock("something", LockMode::LOCK_IS, 1000), LockRes::LOCKRES_OK);
    EXPECT_EQ(l3.lock("something", LockMode::LOCK_IX, 1000), LockRes::LOCKRES_OK);
    EXPECT_EQ(l4.lock("something", LockMode::LOCK_IX, 1000), LockRes::LOCKRES_OK);
    std::thread tmp([&l5]() {
        EXPECT_EQ(l5.lock("something", LockMode::LOCK_S, 10000), LockRes::LOCKRES_OK);
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    l1.unlock();
    l2.unlock();
    l3.unlock();
    l4.unlock();
    tmp.join();
    l5.unlock();
}

}
