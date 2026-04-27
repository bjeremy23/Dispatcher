#pragma once

namespace dispatcher {

// Abstract lifecycle interface for top-level process objects.
// Implement init() to acquire resources and cleanup() to release them.
// ComponentMgr calls init() in registration order and cleanup() in reverse.
class Component {
public:
    virtual ~Component() = default;

    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;
    Component(Component&&) = delete;
    Component& operator=(Component&&) = delete;

    virtual bool init()    = 0;
    virtual void cleanup() = 0;

protected:
    Component() = default;
};

} // namespace dispatcher
