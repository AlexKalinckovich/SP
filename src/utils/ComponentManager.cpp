#include "utils/ComponentManager.h"

#include <algorithm>

void ui::ComponentManager::AddComponent(std::shared_ptr<ui::IComponent> component)
{
    if (component)
    {
        components_.push_back(std::move(component));
    }
}

bool ui::ComponentManager::RemoveComponent(const ui::IComponent* component)
{
    const auto it = std::ranges::remove_if(components_,
                                           [component](const std::shared_ptr<ui::IComponent> &up)
                                           {
                                               return up.get() == component;
                                           }).begin();

    if (it == components_.end())
    {
        return false;
    }

    components_.erase(it, components_.end());
    return true;
}

void ui::ComponentManager::OnCreate(HWND hwndParent)
{
    for (const std::shared_ptr<ui::IComponent> &component: components_)
    {
        if (component)
        {
            component->OnCreate(hwndParent);
        }
    }
}

void ui::ComponentManager::OnDestroy()
{
    for (const std::shared_ptr<ui::IComponent> &component: components_)
    {
        if (component)
        {
            component->OnDestroy();
        }
    }
}

bool ui::ComponentManager::OnMessage(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam, LRESULT* outResult)
{
    bool result = false;
    for(const std::shared_ptr<ui::IComponent> &component: components_)
    {
        if(component)
        {
            result |= component->OnMessage(hwnd,msg,wParam,lParam,outResult);
        }
    }
    return result;
}
