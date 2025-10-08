//
// Created by brota on 03.09.2025.
//

#ifndef MENUBAR_H
#define MENUBAR_H

#include <string>
#include <unordered_map>
#include <windows.h>

class MenuBar {
public:
    MenuBar();
    ~MenuBar();

    bool Create();
    void Destroy();

    [[nodiscard]] HMENU GetHandle() const { return hMenuBar_; }

    enum {
        ID_FILE_OPEN = 1001,
        ID_FILE_SAVE,
        ID_FILE_SAVE_AS,
        ID_FILE_EXIT,
        ID_EDIT_UNDO,
        ID_EDIT_REDO,
        ID_EDIT_CUT,
        ID_EDIT_COPY,
        ID_EDIT_PASTE,
        ID_EDIT_SELECT_ALL,
        ID_EDIT_FIND,
        ID_EDIT_REPLACE,
        ID_FORMAT_FONT,
        ID_VIEW_WORD_WRAP,
        ID_HELP_ABOUT,
    };

private:
    HMENU hMenuBar_ = nullptr;

    static bool CreateFileMenu(HMENU hMenu);

    static bool CreateEditMenu(HMENU hMenu);

    static bool CreateFormatMenu(HMENU hMenu);

    static bool CreateHelpMenu(HMENU hMenu);
};

#endif //MENUBAR_H
