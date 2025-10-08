#pragma once
#include "IComponent.h"
#include <string>
#include <vector>

/**
 * @class ExcelLikeView
 * @brief An owner-drawn component that provides a grid of cells similar to a spreadsheet.
 *
 * This class implements the ui::IComponent interface to draw and manage a grid control
 * within a parent window's client area. It supports text input, font customization from
 * a file, and automatic resizing of columns and rows to fit content.
 */
class ExcelLikeView final : public ui::IComponent
{
public:
    /**
     * @brief Constructs the Excel-like view component.
     * @param initialRows The number of rows to create initially.
     * @param initialCols The number of columns to create initially.
     */
    explicit ExcelLikeView(int initialRows = 50, int initialCols = 20);

    ~ExcelLikeView() noexcept override;

    void OnCreate(HWND hwndParent) noexcept override;
    void OnDestroy() noexcept override;
    bool OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept override;

    /**
     * @brief Sets a new font for the entire grid, loading it from a specified file.
     * @param fontFilePath The absolute path to the font file (e.g., .ttf).
     * @param fontName The name of the font (e.g., "Roboto").
     * @param fontSize The point size of the font.
     */
    void SetFont(const std::wstring& fontFilePath, const std::wstring& fontName, int fontSize);

private:
    struct Cell {
        std::wstring text;
    };

    void HandlePaint(HWND hwnd);
    void HandleSize(LPARAM lParam);
    void HandleLButtonDown(LPARAM lParam);
    void HandleChar(WPARAM wParam);
    void HandleKeyDown(WPARAM wParam);
    void HandleHScroll(WPARAM wParam);
    void HandleVScroll(WPARAM wParam);

    void DrawGrid(HDC hdc, const RECT& clientRect);
    void DrawCells(HDC hdc) const;
    void DrawFocusRect(HDC hdc) const;

    void AutoSizeColumnAndRowForCell(int row, int col);
    [[nodiscard]] POINT GetCellFromCoordinates(int x, int y) const;
    [[nodiscard]] RECT GetCellRect(int row, int col) const;
    void UpdateScrollbars();

    HWND m_hwndParent = nullptr;

    HFONT m_hFont = nullptr;
    std::wstring m_fontFilePath;
    std::vector<std::vector<Cell>> m_cells;
    std::vector<int> m_columnWidths;
    std::vector<int> m_rowHeights;

    POINT m_activeCell = { -1, -1 };
    int m_initialRows;
    int m_initialCols;

    int m_horizontalScrollPos = 0;
    int m_verticalScrollPos = 0;
};
