#pragma once
#include "IComponent.h"
#include <string>
#include <vector>
#include <memory>

#include "win32/HashMapMessageHandler.h"
#include "utils/FileManager.h"

#define SELECTED_CELL_WIDTH 5
#define SCROLL_POS_MOVE 20
#define CELL_HEIGHT_DX 10
#define CELL_WIDTH_DX 10
#define TEXT_PADDING 10
#define FILE_READ_CHUNK_MULTIPLIER 4
#define MINIMUM_CELL_CAPACITY 1
#define SCROLL_PAGE_SIZE 20


class ExcelLikeView final : public ui::IComponent
{
public:
    explicit ExcelLikeView(int initialRows = 50, int initialCols = 20);
    ~ExcelLikeView() noexcept override;

    void OnCreate(HWND hwndParent) noexcept override;
    void OnDestroy() noexcept override;
    bool OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept override;

    void SetFont(const std::wstring &fontName, int fontSize);

    bool HandleFileOpen();
    void CloseCurrentFile();
    bool IsFileLoaded() const noexcept { return m_fileReader != nullptr; }

private:
    struct Cell
    {
        std::wstring text;
        HFONT hFont = nullptr;
        std::wstring fontName;
        int fontSize = 0;
    };

    void InitializeMessageHandlers();
    void InitializeFileReader();

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

    void RecalculateEqualCellSizes();

    SIZE GetTextSize(const std::wstring &text, int row, int col) const;
    POINT GetCellFromCoordinates(int x, int y) const;
    RECT GetCellRect(int row, int col) const;
    void UpdateScrollbars();

    void LoadFileChunk();
    void SpreadTextToCells(const std::string& text);
    int CalculateCellCapacity(int col) const;
    size_t CalculateVisibleCapacity() const;
    void ClearAllCells();

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

    std::unique_ptr<FileManager::MemoryMappedReader> m_fileReader;
    size_t m_currentFileOffset = 0;
    size_t m_totalFileSize = 0;
    Encoding m_fileEncoding = Encoding::UNKNOWN;
    std::wstring m_currentFilePath;
};