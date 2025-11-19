#include "components/excelView/ExcelLikeView.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include "utils/ErrorFormatter.h"


ExcelLikeView::ExcelLikeView(const int initialRows, const int initialCols)
    : m_rowCount(initialRows > 0 ? initialRows : 1),
      m_colCount(initialCols > 0 ? initialCols : 1) {}

ExcelLikeView::~ExcelLikeView() noexcept
{
    ExcelLikeView::OnDestroy();
}

void ExcelLikeView::OnCreate(HWND hwndParent) noexcept
{
    m_hwndParent = hwndParent;
    InitializeMessageHandlers();

    m_cells.assign(m_rowCount, std::vector<Cell>(m_colCount));
    m_columnWidths.assign(m_colCount, m_defaultColumnWidth);
    m_rowHeights.assign(m_rowCount, m_defaultRowHeight);

    RECT clientRect;
    GetClientRect(m_hwndParent, &clientRect);
    m_lastClientRect = clientRect;
    if (clientRect.right > 0 && clientRect.bottom > 0)
    {
        m_defaultColumnWidth = clientRect.right / m_colCount;
        m_defaultRowHeight = clientRect.bottom / m_rowCount;
    }
    m_columnWidths.assign(m_colCount, m_defaultColumnWidth);
    m_rowHeights.assign(m_rowCount, m_defaultRowHeight);


    LOGFONTW lf = {0};
    lf.lfHeight = -MulDiv(10, GetDeviceCaps(GetDC(m_hwndParent), LOGPIXELSY), 72);
    wcscpy_s(lf.lfFaceName, L"Consolas");
    m_hFont = CreateFontIndirectW(&lf);

    UpdateScrollbars();
}

void ExcelLikeView::OnDestroy() noexcept
{
    CloseCurrentFile(); 

    if (m_hFont)
    {
        DeleteObject(m_hFont);
        m_hFont = nullptr;
    }
    for (std::vector<Cell> &row: m_cells)
    {
        for (Cell &cell: row)
        {
            if (cell.hFont)
            {
                DeleteObject(cell.hFont);
                cell.hFont = nullptr;
            }
        }
    }

    if (!m_fontFilePath.empty())
    {
        m_fontFilePath.clear();
    }
    m_hwndParent = nullptr;
}


bool ExcelLikeView::OnMessage(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam, LRESULT* outResult) noexcept
{
    if (!m_hwndParent)
    {
        return false;
    }
    return m_messageHandler.HandleMessage(hwnd, msg, wParam, lParam);
}

bool ExcelLikeView::HandleFileOpen()
{
    CloseCurrentFile();

    FileManager::FileLoadResult fileLoadResult;
    const bool isSuccess = FileManager::LoadFileForMapping(m_hwndParent, fileLoadResult);
    if (!isSuccess) 
    {
        const DWORD err = GetLastError();
        const std::wstring errorDescription = ErrorFormatter::GetErrorStringW(err);
        MessageBoxW(m_hwndParent,(L"Failed to open file: " + errorDescription).c_str(),
                   L"Error",
                   MB_ICONERROR);
        return false;
    }

    if (fileLoadResult.fileSize == 0)
    {
        ClearAllCells();
        UpdateScrollbars();
        InvalidateRect(m_hwndParent, nullptr, TRUE);
        return true;
    }

    m_currentFilePath = fileLoadResult.filePath;
    m_totalFileSize = fileLoadResult.fileSize;
    m_fileEncoding = fileLoadResult.encoding;
    m_currentFileOffset = 0;

    InitializeFileReader();

    if (m_fileReader && m_fileReader->IsFileOpen())
    {
        LoadFileChunk();
        return true;
    }

    return false;
}

void ExcelLikeView::CloseCurrentFile()
{
    if (m_fileReader)
    {
        m_fileReader->CloseFile();
        m_fileReader.reset();
    }
    m_currentFileOffset = 0;
    m_totalFileSize = 0;
    m_fileEncoding = Encoding::UNKNOWN;
    m_currentFilePath.clear();
    ClearAllCells();
    UpdateScrollbars();
    InvalidateRect(m_hwndParent, nullptr, TRUE);
}

void ExcelLikeView::InitializeFileReader()
{
    m_fileReader = std::make_unique<FileManager::MemoryMappedReader>();

    if (!m_fileReader->OpenFile(m_currentFilePath))
    {
        const DWORD err = GetLastError();
        const std::wstring errorDescription = ErrorFormatter::GetErrorStringW(err);
        MessageBoxW(m_hwndParent,(L"Failed to map file: " + errorDescription).c_str(),
                   L"Error",
                   MB_ICONERROR);
        m_fileReader.reset();
    }
}

void ExcelLikeView::LoadFileChunk()
{
    if (m_fileReader && m_fileReader->IsFileOpen())
    {
        const size_t visibleCapacity = CalculateVisibleCapacity();
        if (visibleCapacity > 0)
        {
            if (m_currentFileOffset >= m_totalFileSize)
            {
                m_currentFileOffset = (m_totalFileSize > visibleCapacity) ?
                    m_totalFileSize - visibleCapacity : 0;
            }

            const size_t readSize = std::min(visibleCapacity * FILE_READ_CHUNK_MULTIPLIER,
                                             m_totalFileSize - m_currentFileOffset);

            const std::string chunk = m_fileReader->GetTextChunk(m_currentFileOffset, readSize, m_fileEncoding);

            if (!chunk.empty())
            {
                SpreadTextToCells(chunk);
                InvalidateRect(m_hwndParent, nullptr, TRUE);
                SetScrollPos(m_hwndParent, SB_VERT, static_cast<int>(m_currentFileOffset), TRUE);
                UpdateScrollbars();
            }
        }
    }
}

void ExcelLikeView::SpreadTextToCells(const std::string& text)
{
    ClearAllCells();

    if (!text.empty())
    {

        const int wideLength = MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
            static_cast<int>(text.length()), nullptr,0);
        if (wideLength > 0)
        {
            std::wstring wideText;
            wideText.resize(wideLength);
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
                static_cast<int>(text.length()), &wideText[0], wideLength);

            size_t textIndex = 0;
            const size_t totalTextLength = wideText.length();


            for (int row = 0; row < m_rowCount && textIndex < totalTextLength; ++row)
            {
                for (int col = 0; col < m_colCount && textIndex < totalTextLength; ++col)
                {
                    const int cellCapacity = CalculateCellCapacity(col);
                    if (cellCapacity > MINIMUM_CELL_CAPACITY)
                    {
                        const size_t chunkSize = std::min(static_cast<size_t>(cellCapacity),
                                                          totalTextLength - textIndex);
                        m_cells[row][col].text = wideText.substr(textIndex, chunkSize);
                        textIndex += chunkSize;
                    }
                }
            }
        }
    }
}

size_t ExcelLikeView::CalculateVisibleCapacity() const
{
    size_t totalCapacity = 0;

    for (int col = 0; col < m_colCount; ++col)
    {
        const int cellCapacity = CalculateCellCapacity(col);
        totalCapacity += static_cast<size_t>(std::max(0, cellCapacity)) * m_rowCount;
    }

    return totalCapacity;
}

void ExcelLikeView::ClearAllCells()
{
    for (std::vector<Cell> &row: m_cells)
    {
        for (Cell &cell: row)
        {
            cell.text.clear();
        }
    }
}

void ExcelLikeView::SetFont(const std::wstring &fontName, const int fontSize)
{
    if (m_activeCell.x < 0 || m_activeCell.y < 0) return;

    LOGFONTW lf = {0};
    lf.lfHeight = -MulDiv(fontSize, GetDeviceCaps(GetDC(m_hwndParent), LOGPIXELSY), 72);
    wcscpy_s(lf.lfFaceName, fontName.c_str());
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    HFONT hNewFont = CreateFontIndirectW(&lf);
    if (hNewFont)
    {
        Cell *cell = &m_cells[m_activeCell.y][m_activeCell.x];

        if (cell->hFont)
        {
            DeleteObject(cell->hFont);
        }

        cell->hFont = hNewFont;
        cell->fontName = fontName;
        cell->fontSize = fontSize;

        RecalculateEqualCellSizes();

        UpdateScrollbars();
        InvalidateRect(m_hwndParent, nullptr, TRUE);
    }
}

void ExcelLikeView::DrawGrid(HDC hdc, const RECT& clientRect)
{
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(215, 215, 215));
    HPEN hOldPen = (HPEN) SelectObject(hdc, hPen);

    int x = -m_horizontalScrollPos;
    for (const int width: m_columnWidths)
    {
        x += width;
        if (x > clientRect.right)
        {
            break;
        }
        MoveToEx(hdc, x, 0, nullptr);
        LineTo(hdc, x, clientRect.bottom);
    }

    int y = -m_verticalScrollPos;
    for (const int height: m_rowHeights)
    {
        y += height;
        if (y > clientRect.bottom)
            break;
        MoveToEx(hdc, 0, y, nullptr);
        LineTo(hdc, clientRect.right, y);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void ExcelLikeView::DrawCells(HDC hdc) const
{
    RECT clientRect;
    GetClientRect(m_hwndParent, &clientRect);

    HFONT defaultFont = m_hFont;

    const size_t rowSize = m_cells.size();
    if (rowSize == 0) return;
    const size_t colSize = m_cells[0].size();

    for (size_t r = 0; r < rowSize; ++r)
    {
        for (size_t c = 0; c < colSize; ++c)
        {
            if (!m_cells[r][c].text.empty())
            {
                RECT cellRect = GetCellRect(static_cast<int>(r), static_cast<int>(c));
                if (cellRect.bottom < 0 || cellRect.top  > clientRect.bottom ||
                    cellRect.right  < 0 || cellRect.left > clientRect.right)
                {
                    continue;
                }

                HFONT fontToUse = m_cells[r][c].hFont ? m_cells[r][c].hFont : defaultFont;
                HFONT oldFont = (HFONT)SelectObject(hdc, fontToUse);

                cellRect.left += SELECTED_CELL_WIDTH;
                DrawTextW(hdc, m_cells[r][c].text.c_str(), -1, &cellRect,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_WORD_ELLIPSIS);

                SelectObject(hdc, oldFont);
            }
        }
    }
}

void ExcelLikeView::DrawFocusRect(HDC hdc) const
{
    if (m_activeCell.y != -1 && m_activeCell.x != -1)
    {
        RECT focusRect = GetCellRect(m_activeCell.y, m_activeCell.x);

        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 120, 215));
        HBRUSH hBrush = (HBRUSH) GetStockObject(NULL_BRUSH);

        HPEN oldPen = (HPEN) SelectObject(hdc, hPen);
        HBRUSH oldBrush = (HBRUSH) SelectObject(hdc, hBrush);

        Rectangle(hdc, focusRect.left,focusRect.top, focusRect.right,focusRect.bottom);

        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(hPen);
    }
}

