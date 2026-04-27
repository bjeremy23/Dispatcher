#include "ComponentMgr.hpp"
#include "Component.hpp"

#include <vector>

namespace dispatcher {

struct ComponentMgr::Impl {
    std::vector<std::unique_ptr<Component>> components;
    int                                     initCount{0};
};

ComponentMgr::ComponentMgr()
    : impl_(std::make_unique<Impl>())
{}

ComponentMgr::~ComponentMgr() {
    cleanup();
}

void ComponentMgr::add(std::unique_ptr<Component> component) {
    impl_->components.push_back(std::move(component));
}

bool ComponentMgr::init() {
    for (auto& component : impl_->components) {
        if (!component->init()) {
            for (int i = impl_->initCount - 1; i >= 0; --i)
                impl_->components[i]->cleanup();
            impl_->initCount = 0;
            return false;
        }
        ++impl_->initCount;
    }
    return true;
}

void ComponentMgr::cleanup() {
    for (int i = impl_->initCount - 1; i >= 0; --i)
        impl_->components[i]->cleanup();
    impl_->initCount = 0;
}

} // namespace dispatcher
