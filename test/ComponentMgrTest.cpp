#include "component/Component.hpp"
#include "component/ComponentMgr.hpp"

#include <gtest/gtest.h>

#include <vector>
#include <string>

using dispatcher::Component;
using dispatcher::ComponentMgr;

// Tracks init/cleanup calls by name for order verification.
struct TrackingComponent : public Component {
    std::string                name;
    std::vector<std::string>&  log;
    bool                       initResult{true};

    TrackingComponent(std::string name, std::vector<std::string>& log, bool initResult = true)
        : name(std::move(name)), log(log), initResult(initResult) {}

    bool init()    override { log.push_back(name + ".init");    return initResult; }
    void cleanup() override { log.push_back(name + ".cleanup"); }
};

TEST(ComponentMgrTest, InitCalledInOrder) {
    std::vector<std::string> log;
    ComponentMgr mgr;
    mgr.add(std::make_unique<TrackingComponent>("A", log));
    mgr.add(std::make_unique<TrackingComponent>("B", log));
    mgr.add(std::make_unique<TrackingComponent>("C", log));

    EXPECT_TRUE(mgr.init());
    mgr.cleanup();

    EXPECT_EQ(log[0], "A.init");
    EXPECT_EQ(log[1], "B.init");
    EXPECT_EQ(log[2], "C.init");
}

TEST(ComponentMgrTest, CleanupCalledInReverseOrder) {
    std::vector<std::string> log;
    ComponentMgr mgr;
    mgr.add(std::make_unique<TrackingComponent>("A", log));
    mgr.add(std::make_unique<TrackingComponent>("B", log));
    mgr.add(std::make_unique<TrackingComponent>("C", log));

    mgr.init();
    log.clear();
    mgr.cleanup();

    EXPECT_EQ(log[0], "C.cleanup");
    EXPECT_EQ(log[1], "B.cleanup");
    EXPECT_EQ(log[2], "A.cleanup");
}

TEST(ComponentMgrTest, InitFailureRollsBackInReverse) {
    std::vector<std::string> log;
    ComponentMgr mgr;
    mgr.add(std::make_unique<TrackingComponent>("A", log));
    mgr.add(std::make_unique<TrackingComponent>("B", log));
    mgr.add(std::make_unique<TrackingComponent>("C", log, /*initResult=*/false));
    mgr.add(std::make_unique<TrackingComponent>("D", log));

    EXPECT_FALSE(mgr.init());

    // A and B init'd successfully; C failed; D never started.
    // Rollback should cleanup B then A.
    EXPECT_EQ(log[0], "A.init");
    EXPECT_EQ(log[1], "B.init");
    EXPECT_EQ(log[2], "C.init");
    EXPECT_EQ(log[3], "B.cleanup");
    EXPECT_EQ(log[4], "A.cleanup");
    EXPECT_EQ(log.size(), 5u);
}

TEST(ComponentMgrTest, DestructorCallsCleanup) {
    std::vector<std::string> log;
    {
        ComponentMgr mgr;
        mgr.add(std::make_unique<TrackingComponent>("A", log));
        mgr.add(std::make_unique<TrackingComponent>("B", log));
        mgr.init();
        log.clear();
    } // mgr destroyed here

    EXPECT_EQ(log[0], "B.cleanup");
    EXPECT_EQ(log[1], "A.cleanup");
}

TEST(ComponentMgrTest, CleanupIsIdempotent) {
    std::vector<std::string> log;
    ComponentMgr mgr;
    mgr.add(std::make_unique<TrackingComponent>("A", log));
    mgr.init();
    mgr.cleanup();
    log.clear();
    mgr.cleanup(); // second call should be a no-op

    EXPECT_TRUE(log.empty());
}
