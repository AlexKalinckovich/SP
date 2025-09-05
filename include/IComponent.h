#ifndef ICOMPONENT_H
#define ICOMPONENT_H


#include <windows.h>


namespace ui
{

    class IComponent
    {
        public:
            virtual ~IComponent() noexcept = default;

            virtual void OnCreate(HWND hwndParent) noexcept = 0;
            virtual void OnDestroy() noexcept = 0;
            virtual bool OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept = 0;

            IComponent(const IComponent&) = delete;
            IComponent& operator=(const IComponent&) = delete;
            IComponent(IComponent&&) = delete;
            IComponent& operator=(IComponent&&) = delete;

        protected:
            IComponent() noexcept = default;
    };

} // namespace ui
#endif //ICOMPONENT_H
