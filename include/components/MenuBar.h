//
// Created by brota on 03.09.2025.
//

#ifndef MENUBAR_H
#define MENUBAR_H

#include <windows.h>

class MenuBar {
public:
    MenuBar();
    ~MenuBar();

    bool Create();
    void Destroy();

    [[nodiscard]] HMENU GetHandle() const { return hMenuBar_; }

private:
    HMENU hMenuBar_ = nullptr;

    static bool CreateFileMenu(HMENU hMenu);

    static bool CreateEditMenu(HMENU hMenu);

    static bool CreateFormatMenu(HMENU hMenu);

    static bool CreateHelpMenu(HMENU hMenu);
};

#endif //MENUBAR_H
