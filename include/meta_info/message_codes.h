// message_codes.h
#pragma once

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
#define ID_PROCESS_INFO                1033

#define ID_HELP_ABOUT                  1051

// ===== Control IDs =====
#define IDC_PROCESS_LIST               2001
#define IDC_PATTERN_EDIT               2002
#define IDC_REPLACE_EDIT               2003
#define IDC_READ_BUTTON                2004
#define IDC_COLLECT_READABLE_BUTTON    2005
#define IDC_SHOW_WRITABLE_BUTTON       2006
#define IDC_REPLACE_BUTTON             2007

// ===== Custom Messages =====
#define WM_APP_READ_COMPLETE           (WM_APP + 101)
#define WM_APP_WRITE_COMPLETE          (WM_APP + 102)
#define WM_APP_DATA_LOADED             (WM_APP + 103)
#define WM_APP_FILE_LOAD_COMPLETE      (WM_APP + 105)
#define WM_APP_FILE_LOAD_ERROR         (WM_APP + 106)
#define WM_PROCESS_DATA_FOUND          (WM_APP + 107)
#define WM_READABLE_MEMORY_DATA        (WM_APP + 108)
#define WM_WRITABLE_MEMORY_DATA        (WM_APP + 109)
#define WM_STRING_REPLACE_RESULT       (WM_APP + 110)

// ===== Memory Access Flags =====
#define MEMORY_ACCESS_READ             (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)
#define MEMORY_ACCESS_WRITE            (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)