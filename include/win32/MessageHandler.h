//
// Created by brota on 03.09.2025.
//

#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H
#include <windows.h>
#include <functional>
#include <unordered_map>

namespace win32
{
    class MessageHandler {
        public:
            using HandlerFunc = std::function<LRESULT(HWND, WPARAM, LPARAM)>;

            virtual ~MessageHandler() = default;

            virtual void RegisterHandler(UINT message, HandlerFunc handler) = 0;
            virtual void RegisterCommandHandler(int commandId, HandlerFunc handler) = 0;
            virtual LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;

            MessageHandler(const MessageHandler&) = delete;
            MessageHandler& operator=(const MessageHandler&) = delete;

        protected:
            MessageHandler() = default;
    };
}
#endif //MESSAGEHANDLER_H
