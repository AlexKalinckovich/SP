#include <algorithm>
#include "utils/ComponentManager.h"

void ComponentManager::AddComponent(std::shared_ptr<ui::IComponent> component) {
    if (component) {
        components_.push_back(std::move(component));
    }
}

bool ComponentManager::RemoveComponent(const ui::IComponent* component) {
    const auto it = std::ranges::remove_if(components_,
                                           [component](const std::shared_ptr<ui::IComponent>& up) {
                                               return up.get() == component;
                                           }).begin();

    if (it == components_.end()) {
        return false;
    }

    components_.erase(it, components_.end());
    return true;
}

void ComponentManager::OnCreate(HWND hwndParent)
{
    for (const std::shared_ptr<ui::IComponent> &component: components_)
    {
        if (component)
        {
            component->OnCreate(hwndParent);
        }
    }
}

void ComponentManager::OnDestroy()
{
    for (const std::shared_ptr<ui::IComponent> &component: components_)
    {
        if (component)
        {
            component->OnDestroy();
        }
    }
}

bool ComponentManager::OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult)
{
    return std::ranges::any_of(components_, [&](const std::shared_ptr<ui::IComponent>& up) -> bool
    {
        if (up && up->OnMessage(hwnd, msg, wParam, lParam, outResult)) {
            return true;
        }
        return false;
    });
}
