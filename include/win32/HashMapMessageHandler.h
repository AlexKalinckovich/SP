//
// Created by brota on 03.09.2025.
//

#ifndef HASHMAPMESSAGEHANDLER_H
#define HASHMAPMESSAGEHANDLER_H
#include "MessageHandler.h"
#include <unordered_map>

namespace win32
{
    class HashMapMessageHandler : public MessageHandler
    {
        public:
            HashMapMessageHandler() = default;
            ~HashMapMessageHandler() override = default;

            void RegisterHandler(UINT message, HandlerFunc handler) override
            {
                messageHandlers_[message] = std::move(handler);
            }

            void RegisterCommandHandler(int commandId, HandlerFunc handler) override
            {
                commandHandlers_[commandId] = std::move(handler);
            }

            LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override
            {
                if (message == WM_COMMAND)
                {
                    const int commandId = LOWORD(wParam);
                    const auto it = commandHandlers_.find(commandId);
                    if (it != commandHandlers_.end())
                    {
                        return it->second(hwnd, wParam, lParam);
                    }
                }

                const auto it = messageHandlers_.find(message);
                if (it != messageHandlers_.end())
                {
                    return it->second(hwnd, wParam, lParam);
                }

                return MSG_NOT_HANDLED;
            }

            static constexpr LRESULT MSG_NOT_HANDLED = -1;

        private:
            std::unordered_map<UINT, HandlerFunc> messageHandlers_;
            std::unordered_map<int, HandlerFunc> commandHandlers_;
    };
}
#endif //HASHMAPMESSAGEHANDLER_H
