#ifndef COMPONENTMANAGER_H
#define COMPONENTMANAGER_H

#include <IComponent.h>
#include <vector>
#include <memory>

class ComponentManager
{
    public:
        void AddComponent(std::shared_ptr<ui::IComponent> component);
        bool RemoveComponent(const ui::IComponent* component);

        void OnCreate(HWND hwndParent);
        void OnDestroy();
        bool OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult);

    private:
        std::vector<std::shared_ptr<ui::IComponent>> components_;
};

#endif //COMPONENTMANAGER_H
