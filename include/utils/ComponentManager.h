#pragma once

#include <windows.h>
#include <map>  // Changed from unordered_map
#include <vector>
#include <memory>
#include <functional>
#include <set>
#include "IComponent.h"

namespace ui {

    class ComponentManager
    {
    public:
        using MessageHandler = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;

        ComponentManager() = default;  // This should work now

        void AddComponent(std::shared_ptr<IComponent> component);
        bool RemoveComponent(const IComponent* component);

        bool SubscribeToMessage(HWND hwnd, UINT message, const std::shared_ptr<IComponent> &component);
        bool UnsubscribeFromMessage(HWND hwnd, UINT message, const IComponent* component);
        bool UnsubscribeFromAllMessages(const IComponent* component);

        void OnCreate(HWND hwndParent);
        void OnDestroy();
        bool OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult);

    private:
        std::vector<std::shared_ptr<IComponent>> components_{};

        using MessageKey = std::pair<HWND, UINT>;
        using ComponentSet = std::set<const IComponent*>;

        std::map<MessageKey, ComponentSet> messageSubscriptions_{};

        static MessageKey MakeMessageKey(HWND hwnd, UINT message) noexcept;
    };

} // namespace ui