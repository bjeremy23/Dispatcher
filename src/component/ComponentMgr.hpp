#pragma once

#include <memory>

namespace dispatcher {

class Component;

// Manages the lifecycle of a ordered list of Component objects.
// Components are init'd in the order they were added and cleaned up
// in reverse. If init() fails partway through, already-init'd components
// are cleaned up in reverse before returning false.
class ComponentMgr {
public:
    ComponentMgr();
    ~ComponentMgr();

    ComponentMgr(const ComponentMgr&) = delete;
    ComponentMgr& operator=(const ComponentMgr&) = delete;
    ComponentMgr(ComponentMgr&&) = delete;
    ComponentMgr& operator=(ComponentMgr&&) = delete;

    void add(std::unique_ptr<Component> component);

    // Calls init() on each component in order. Returns false and rolls back
    // on the first failure.
    bool init();

    // Calls cleanup() on each component in reverse init order.
    void cleanup();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
