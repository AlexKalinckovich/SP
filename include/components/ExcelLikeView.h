#pragma once
#include "IComponent.h"
#include <string>
#include <vector>

#include "win32/HashMapMessageHandler.h"

class ExcelLikeView final : public ui::IComponent
{
public:
    explicit ExcelLikeView(int initialRows = 50, int initialCols = 20);

    ~ExcelLikeView() noexcept override;

    void OnCreate(HWND hwndParent) noexcept override;
    void OnDestroy() noexcept override;
    bool OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept override;

    void SetFont(const std::wstring &fontName, int fontSize);

private:
    struct Cell
    {
        std::wstring text;
        HFONT hFont = nullptr;
        std::wstring fontName;
        int fontSize = 0;
    };

    void InitializeMessageHandlers();

    void HandlePaint(HWND hwnd);
    void HandleSize(LPARAM lParam);
    void HandleLButtonDown(LPARAM lParam);
    void HandleChar(WPARAM symbolCodeWParam);
    void HandleKeyDown(WPARAM wParam);
    void HandleHScroll(WPARAM currentScrollPosWParam);
    void HandleVScroll(WPARAM currentScrollPosWParam);
    void HandleMouseWheel(WPARAM mouseMoveWParam);

    void DrawGrid(HDC hdc, const RECT& clientRect);
    void DrawCells(HDC hdc) const;
    void DrawFocusRect(HDC hdc) const;

    void UpdateGridSizeForCell(int row, int col);
    void RecalculateColumnWidth(int col);
    void RecalculateRowHeight(int row);
    SIZE GetTextSize(const std::wstring &text, int row, int col) const;
    POINT GetCellFromCoordinates(int x, int y) const;
    RECT GetCellRect(int row, int col) const;
    void UpdateScrollbars();

    HWND m_hwndParent = nullptr;
    win32::HashMapMessageHandler m_messageHandler;

    HFONT m_hFont = nullptr;
    std::wstring m_fontFilePath;

    std::vector<std::vector<Cell>> m_cells;
    std::vector<int> m_columnWidths;
    std::vector<int> m_rowHeights;

    POINT m_activeCell = { -1, -1 };
    int m_rowCount;
    int m_colCount;
    int m_defaultColumnWidth = 100;
    int m_defaultRowHeight = 25;


    RECT m_lastClientRect = { 0 };

    int m_horizontalScrollPos = 0;
    int m_verticalScrollPos = 0;
};

