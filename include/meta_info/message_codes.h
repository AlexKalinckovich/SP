//
// Created by brota on 27.10.2025.
//

#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H

// ===== Menu Command IDs =====
#define ID_FILE_OPEN                   1001
#define ID_FILE_SAVE                   1002
#define ID_FILE_SAVE_AS                1003
#define ID_FILE_EXIT                   1004

#define ID_EDIT_UNDO                   1011
#define ID_EDIT_REDO                   1012
#define ID_EDIT_CUT                    1013
#define ID_EDIT_COPY                   1014
#define ID_EDIT_PASTE                  1015
#define ID_EDIT_SELECT_ALL             1016
#define ID_EDIT_FIND                   1017
#define ID_EDIT_REPLACE                1018

#define ID_FORMAT_FONT                 1031
#define ID_VIEW_WORD_WRAP              1032

#define ID_HELP_ABOUT                  1051

// ===== Control IDs =====
#define IDC_PROCESS_LIST               2001
#define IDC_EDIT_FIND                  2002
#define IDC_EDIT_REPLACE               2003
#define IDC_BUTTON_WRITE               2004
#define IDC_BUTTON_READ                2005

// ===== Custom Messages =====
// These are the messages that our dialog will send to its parent window.
// WM_APP is a starting point for user-defined messages.
#define WM_APP_READ_COMPLETE           (WM_APP + 101)
#define WM_APP_WRITE_COMPLETE          (WM_APP + 102)
#define WM_APP_DATA_LOADED             (WM_APP + 103)
#define WM_APP_DATA_LOADED             (WM_APP + 104)
#define WM_APP_FILE_LOAD_COMPLETE      (WM_APP + 105)
#define WM_APP_FILE_LOAD_ERROR         (WM_APP + 106)

#endif // MESSAGE_CODES_H