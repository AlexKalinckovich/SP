#include "components/ExcelLikeView.h"
#include <windowsx.h>

#include <iostream>
#include <numeric>
#include <ostream>
#include <cmath>

#define SELECTED_CELL_WIDTH 5
#define SCROLL_POS_MOVE 20
#define CELL_HEIGHT_DX 10
#define CELL_WIDTH_DX 10
#define TEXT_PADDING 10


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

    RECT clientRect;
    GetClientRect(m_hwndParent, &clientRect);
    m_lastClientRect = clientRect;

    if (clientRect.right > 0 && clientRect.bottom > 0)
    {
        m_defaultColumnWidth = clientRect.right / m_colCount;
        m_defaultRowHeight = clientRect.bottom / m_rowCount;
    }

    m_cells.assign(m_rowCount, std::vector<Cell>(m_colCount));
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
        RemoveFontResourceExW(m_fontFilePath.c_str(), FR_PRIVATE, nullptr);
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

void ExcelLikeView::SetFont(const std::wstring &fontName, const int fontSize)
{
    if (m_activeCell.x < 0 || m_activeCell.y < 0)
    {
        return;
    }

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
        Cell* cell = &m_cells[m_activeCell.y][m_activeCell.x];

        if (cell->hFont)
        {
            DeleteObject(cell->hFont);
        }

        cell->hFont = hNewFont;
        cell->fontName = fontName;
        cell->fontSize = fontSize;

        RecalculateColumnWidth(m_activeCell.x);
        RecalculateRowHeight(m_activeCell.y);

        UpdateScrollbars();

        InvalidateRect(m_hwndParent, nullptr, TRUE);
    }
}

void ExcelLikeView::InitializeMessageHandlers()
{
    m_messageHandler.RegisterHandler(WM_PAINT, [this](HWND hwnd, WPARAM, LPARAM) -> LRESULT
    {
        HandlePaint(hwnd);
        return 0;
    });
    m_messageHandler.RegisterHandler(WM_SIZE, [this](HWND, const WPARAM wParam, const LPARAM lParam) -> LRESULT
    {
        if (wParam != SIZE_MINIMIZED)
        {
            HandleSize(lParam);
        }
        return 0;
    });
    m_messageHandler.RegisterHandler(WM_LBUTTONDOWN, [this](HWND, WPARAM, LPARAM lParam) -> LRESULT
    {
        HandleLButtonDown(lParam);
        return 0;
    });
    m_messageHandler.RegisterHandler(WM_CHAR, [this](HWND, const WPARAM wParam, LPARAM) -> LRESULT
    {
        HandleChar(wParam);
        return 0;
    });
    m_messageHandler.RegisterHandler(WM_KEYDOWN, [this](HWND, const WPARAM wParam, LPARAM) -> LRESULT
    {
        HandleKeyDown(wParam);
        return 0;
    });
    m_messageHandler.RegisterHandler(WM_HSCROLL, [this](HWND, const WPARAM wParam, LPARAM) -> LRESULT
    {
        HandleHScroll(wParam);
        return 0;
    });
    m_messageHandler.RegisterHandler(WM_VSCROLL, [this](HWND, const WPARAM wParam, LPARAM) -> LRESULT
    {
        HandleVScroll(wParam);
        return 0;
    });
    m_messageHandler.RegisterHandler(WM_MOUSEWHEEL, [this](HWND, const WPARAM wParam, LPARAM) -> LRESULT
    {
        HandleMouseWheel(wParam);
        return 0;
    });
}

void ExcelLikeView::HandlePaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    FillRect(memDC, &clientRect, (HBRUSH)(COLOR_WINDOW + 1));

    HFONT defaultFont = m_hFont;
    HFONT oldFont = (HFONT)SelectObject(memDC, defaultFont);
    SetBkMode(memDC, TRANSPARENT);

    DrawGrid(memDC, clientRect);
    DrawCells(memDC);

    if (m_activeCell.x >= 0 && m_activeCell.y >= 0)
    {
        const Cell* activeCell = &m_cells[m_activeCell.y][m_activeCell.x];
        if (activeCell->hFont)
        {
            SelectObject(memDC, activeCell->hFont);
        }
        DrawFocusRect(memDC);
    }

    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    SelectObject(memDC, oldFont);
    DeleteObject(memBitmap);
    DeleteDC(memDC);

    EndPaint(hwnd, &ps);
}

void ExcelLikeView::HandleSize(const LPARAM lParam)
{
    const int newWidth = LOWORD(lParam);
    const int newHeight = HIWORD(lParam);

    if (newWidth <= 0 || newHeight <= 0 || m_lastClientRect.right <= 0 || m_lastClientRect.bottom <= 0)
    {
        GetClientRect(m_hwndParent, &m_lastClientRect);
        return;
    }

    const double widthRatio = static_cast<double>(newWidth) / m_lastClientRect.right;
    const double heightRatio = static_cast<double>(newHeight) / m_lastClientRect.bottom;

    long long runningTotalWidth = 0;
    for (int &width: m_columnWidths)
    {
        width = static_cast<int>(std::round(width * widthRatio));
        runningTotalWidth += width;
    }

    if (runningTotalWidth != newWidth && !m_columnWidths.empty())
    {
        m_columnWidths.back() += static_cast<int>(newWidth - runningTotalWidth);
    }

    long long runningTotalHeight = 0;
    for (int &height: m_rowHeights)
    {
        height = static_cast<int>(std::round(height * heightRatio));
        runningTotalHeight += height;
    }

    if (runningTotalHeight != newHeight && !m_rowHeights.empty())
    {
        m_rowHeights.back() += static_cast<int>(newHeight - runningTotalHeight);
    }

    m_lastClientRect = {0, 0, newWidth, newHeight};

    UpdateScrollbars();
    InvalidateRect(m_hwndParent, nullptr, TRUE);
}

void ExcelLikeView::HandleLButtonDown(const LPARAM lParam)
{
    const POINT oldActiveCell = m_activeCell;

    m_activeCell = GetCellFromCoordinates(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    if (m_activeCell.x != oldActiveCell.x || m_activeCell.y != oldActiveCell.y)
    {
        if (oldActiveCell.x >= 0 && oldActiveCell.y >= 0)
        {
            RECT oldRect = GetCellRect(oldActiveCell.y, oldActiveCell.x);
            oldRect.left -= SELECTED_CELL_WIDTH;
            oldRect.top -= SELECTED_CELL_WIDTH;
            InvalidateRect(m_hwndParent, &oldRect, FALSE);
        }
        if (m_activeCell.x >= 0 && m_activeCell.y >= 0)
        {
            const RECT newRect = GetCellRect(m_activeCell.y, m_activeCell.x);
            InvalidateRect(m_hwndParent, &newRect, FALSE);
        }
    }
}

void ExcelLikeView::HandleChar(const WPARAM symbolCodeWParam)
{
    const bool isCellActive = m_activeCell.x >= 0 && m_activeCell.y >= 0;
    const bool isSymbolDrawable = symbolCodeWParam >= VK_SPACE;
    if (isCellActive && isSymbolDrawable)
    {
        const wchar_t pressedSymbol = static_cast<wchar_t>(symbolCodeWParam);

        const int rowToChange = m_activeCell.y;
        int colToChange = m_activeCell.x;

        const std::wstring newText = m_cells[rowToChange][colToChange].text + pressedSymbol;

        const SIZE textSize = GetTextSize(newText, rowToChange, colToChange);
        const RECT cellRect = GetCellRect(rowToChange, colToChange);
        const long cellWidth = cellRect.right - cellRect.left;


        if (textSize.cx + TEXT_PADDING > cellWidth)
        {
            if (colToChange + 1 < m_colCount)
            {
                const RECT oldCellRect = GetCellRect(rowToChange, colToChange);
                InvalidateRect(m_hwndParent, &oldCellRect, FALSE);

                colToChange++;
                m_activeCell.x = colToChange;

                const RECT newCellRect = GetCellRect(rowToChange, colToChange);
                InvalidateRect(m_hwndParent, &newCellRect, FALSE);

                m_cells[rowToChange][colToChange].text += pressedSymbol;
            }
            else
            {
                m_cells[rowToChange][colToChange].text += pressedSymbol;
            }
        }
        else
        {
            m_cells[rowToChange][colToChange].text += pressedSymbol;
        }

        UpdateGridSizeForCell(rowToChange, colToChange);
    }
}

void ExcelLikeView::HandleKeyDown(const WPARAM wParam)
{
    const bool isCellActive = m_activeCell.x >= 0 && m_activeCell.y >= 0;
    if (!isCellActive)
    {
        return;
    }

    const POINT oldActiveCell = m_activeCell;
    const int r = m_activeCell.y;
    const int c = m_activeCell.x;

    switch (wParam)
    {
        case VK_LEFT:
            if (c > 0)
            {
                m_activeCell.x--;
            }
            break;
        case VK_RIGHT:
            if (c < m_colCount - 1)
            {
                m_activeCell.x++;
            }
            break;
        case VK_UP:
            if (r > 0)
            {
                m_activeCell.y--;
            }
            break;
        case VK_DOWN:
            if (r < m_rowCount - 1)
            {
                m_activeCell.y++;
            }
            break;
        case VK_BACK:
            if (!m_cells[r][c].text.empty())
            {
                m_cells[r][c].text.pop_back();
                UpdateGridSizeForCell(r, c);
            }
            return;
        default: ;
    }

    if (m_activeCell.x != oldActiveCell.x || m_activeCell.y != oldActiveCell.y)
    {
        const RECT oldRect = GetCellRect(oldActiveCell.y, oldActiveCell.x);
        InvalidateRect(m_hwndParent, &oldRect, FALSE);

        const RECT newRect = GetCellRect(m_activeCell.y, m_activeCell.x);
        InvalidateRect(m_hwndParent, &newRect, FALSE);

        UpdateScrollbars();
    }
}

void ExcelLikeView::HandleHScroll(const WPARAM currentScrollPosWParam)
{
    SCROLLINFO si = {sizeof(si), SIF_ALL};
    GetScrollInfo(m_hwndParent, SB_HORZ, &si);
    const int oldPos = m_horizontalScrollPos;

    const int currentScrollPos = HIWORD(currentScrollPosWParam);
    const int scrollRequestType = LOWORD(currentScrollPosWParam);
    switch (scrollRequestType)
    {
        case SB_LINELEFT: m_horizontalScrollPos -= SCROLL_POS_MOVE;
            break;
        case SB_LINERIGHT: m_horizontalScrollPos += SCROLL_POS_MOVE;
            break;
        case SB_PAGELEFT: m_horizontalScrollPos -= static_cast<int>(si.nPage);
            break;
        case SB_PAGERIGHT: m_horizontalScrollPos += static_cast<int>(si.nPage);
            break;
        case SB_THUMBTRACK: m_horizontalScrollPos = currentScrollPos;
            break;
        default: ;
    }

    const long totalWidth = std::accumulate(m_columnWidths.begin(), m_columnWidths.end(), 0L);
    const int maxScrollPos = std::max(0, static_cast<int>(totalWidth - si.nPage));
    m_horizontalScrollPos = std::max(0, std::min(m_horizontalScrollPos, maxScrollPos));

    if (m_horizontalScrollPos != oldPos)
    {
        SetScrollPos(m_hwndParent, SB_HORZ, m_horizontalScrollPos, TRUE);
        InvalidateRect(m_hwndParent, nullptr, TRUE);
    }
}

void ExcelLikeView::HandleVScroll(const WPARAM currentScrollPosWParam)
{
    SCROLLINFO si = {sizeof(si), SIF_ALL};
    GetScrollInfo(m_hwndParent, SB_VERT, &si);
    const int oldPos = m_verticalScrollPos;

    const int currentScrollPos = HIWORD(currentScrollPosWParam);
    const int scrollRequestType = LOWORD(currentScrollPosWParam);
    switch (scrollRequestType)
    {
        case SB_LINEUP: m_verticalScrollPos -= SCROLL_POS_MOVE;
            break;
        case SB_LINEDOWN: m_verticalScrollPos += SCROLL_POS_MOVE;
            break;
        case SB_PAGEUP: m_verticalScrollPos -= static_cast<int>(si.nPage);
            break;
        case SB_PAGEDOWN: m_verticalScrollPos += static_cast<int>(si.nPage);
            break;
        case SB_THUMBTRACK: m_verticalScrollPos = currentScrollPos;
            break;
        default: ;
    }

    const long totalHeight = std::accumulate(m_rowHeights.begin(), m_rowHeights.end(), 0L);
    const int maxScrollPos = std::max(0, static_cast<int>(totalHeight - si.nPage));
    m_verticalScrollPos = std::max(0, std::min(m_verticalScrollPos, maxScrollPos));

    if (m_verticalScrollPos != oldPos)
    {
        SetScrollPos(m_hwndParent, SB_VERT, m_verticalScrollPos, TRUE);
        InvalidateRect(m_hwndParent, nullptr, TRUE);
    }
}

void ExcelLikeView::HandleMouseWheel(const WPARAM mouseMoveWParam)
{
    const int delta = GET_WHEEL_DELTA_WPARAM(mouseMoveWParam);
    const int scrollAmount = -delta / WHEEL_DELTA * 40;

    SCROLLINFO si = {sizeof(si), SIF_ALL};
    GetScrollInfo(m_hwndParent, SB_VERT, &si);
    const int oldPos = m_verticalScrollPos;

    m_verticalScrollPos += scrollAmount;

    long totalHeight = 0;
    for (const int h: m_rowHeights)
    {
        totalHeight += h;
    }
    const int maxScrollPos = std::max(0, static_cast<int>(totalHeight) - static_cast<int>(si.nPage));
    m_verticalScrollPos = std::max(0, std::min(m_verticalScrollPos, maxScrollPos));

    if (m_verticalScrollPos != oldPos)
    {
        SetScrollPos(m_hwndParent, SB_VERT, m_verticalScrollPos, TRUE);
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
        {
            break;
        }
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
    const size_t colSize = m_cells[0].size();
    for (size_t r = 0; r < rowSize; ++r)
    {
        for (size_t c = 0; c < colSize; ++c)
        {
            if (!m_cells[r][c].text.empty())
            {
                RECT cellRect = GetCellRect(static_cast<int>(r), static_cast<int>(c));
                if (cellRect.bottom < 0 || cellRect.top > clientRect.bottom)
                {
                    continue;
                }
                if (cellRect.right < 0 || cellRect.left > clientRect.right)
                {
                    continue;
                }

                HFONT fontToUse = m_cells[r][c].hFont ? m_cells[r][c].hFont : defaultFont;
                HFONT oldFont = (HFONT)SelectObject(hdc, fontToUse);

                cellRect.left += SELECTED_CELL_WIDTH;
                DrawTextW(hdc, m_cells[r][c].text.c_str(), -1, &cellRect,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

                SelectObject(hdc, oldFont);
            }
        }
    }
}

void ExcelLikeView::DrawFocusRect(HDC hdc) const
{
    if (m_activeCell.y == -1 || m_activeCell.x == -1)
    {
        return;
    }

    RECT focusRect = GetCellRect(m_activeCell.y, m_activeCell.x);

    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 120, 215));
    HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);

    HPEN oldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    Rectangle(hdc, focusRect.left, focusRect.top, focusRect.right, focusRect.bottom);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(hPen);
}

void ExcelLikeView::UpdateGridSizeForCell(const int row, const int col)
{
    if (row >= 0 && row < m_rowCount && col >= 0 && col < m_colCount)
    {
        RecalculateRowHeight(row);
        UpdateScrollbars();
        InvalidateRect(m_hwndParent, nullptr, false);
    }
}


void ExcelLikeView::RecalculateColumnWidth(const int col)
{
    int maxWidth = 0;
    for (int r = 0; r < m_rowCount; ++r)
    {
        const std::wstring &text = m_cells[r][col].text;
        if (!text.empty())
        {
            const SIZE textSize = GetTextSize(text, r, col);
            if (textSize.cx > maxWidth)
            {
                maxWidth = textSize.cx;
            }
        }
    }
    m_columnWidths[col] = std::max(m_defaultColumnWidth, maxWidth + CELL_WIDTH_DX);
}

void ExcelLikeView::RecalculateRowHeight(const int row)
{
    int maxHeight = 0;
    for (int c = 0; c < m_colCount; ++c)
    {
        const std::wstring &text = m_cells[row][c].text;
        if (!text.empty())
        {
            const SIZE textSize = GetTextSize(text, row, c);
            if (textSize.cy > maxHeight)
            {
                maxHeight = textSize.cy;
            }
        }
    }
    m_rowHeights[row] = std::max(m_defaultRowHeight, maxHeight + CELL_HEIGHT_DX);
}

SIZE ExcelLikeView::GetTextSize(const std::wstring& text, const int row, const int col) const
{
    SIZE sz = { 0, 0 };
    if (!m_hwndParent || text.empty()) return sz;

    HDC hdc = GetDC(m_hwndParent);

    HFONT fontToUse = m_hFont;
    if (row >= 0 && col >= 0 && row < m_rowCount && col < m_colCount && m_cells[row][col].hFont)
    {
        fontToUse = m_cells[row][col].hFont;
    }

    HFONT oldFont = (HFONT)SelectObject(hdc, fontToUse);
    GetTextExtentPoint32W(hdc, text.c_str(), static_cast<int>(text.length()), &sz);
    SelectObject(hdc, oldFont);
    ReleaseDC(m_hwndParent, hdc);
    return sz;
}

POINT ExcelLikeView::GetCellFromCoordinates(const int x, const int y) const
{
    POINT cell = {-1, -1};

    int currentX = -m_horizontalScrollPos;
    for (int c = 0; c < m_colCount; ++c)
    {
        if (x < currentX + m_columnWidths[c])
        {
            cell.x = c;
            break;
        }
        currentX += m_columnWidths[c];
    }
    if (cell.x == -1)
    {
        return {-1, -1};
    }

    int currentY = -m_verticalScrollPos;
    for (int r = 0; r < m_rowCount; ++r)
    {
        if (y < currentY + m_rowHeights[r])
        {
            cell.y = r;
            break;
        }
        currentY += m_rowHeights[r];
    }
    if (cell.y == -1)
    {
        return {-1, -1};
    }

    return cell;
}

RECT ExcelLikeView::GetCellRect(const int row, const int col) const
{
    int left = -m_horizontalScrollPos;
    for (int i = 0; i < col; ++i)
    {
        left += m_columnWidths[i];
    }

    int top = -m_verticalScrollPos;
    for (int i = 0; i < row; ++i)
    {
        top += m_rowHeights[i];
    }

    return {left, top, left + m_columnWidths[col], top + m_rowHeights[row]};
}

void ExcelLikeView::UpdateScrollbars()
{
    if (!m_hwndParent)
    {
        return;
    }

    RECT clientRect;
    GetClientRect(m_hwndParent, &clientRect);

    long totalWidth = 0;
    for (const int w: m_columnWidths)
    {
        totalWidth += w;
    }

    long totalHeight = 0;
    for (const int h: m_rowHeights)
    {
        totalHeight += h;
    }

    SCROLLINFO si = {sizeof(si)};
    si.fMask = SIF_RANGE | SIF_PAGE;

    si.nMin = 0;
    si.nMax = totalWidth;
    si.nPage = clientRect.right;
    SetScrollInfo(m_hwndParent, SB_HORZ, &si, TRUE);

    si.nMin = 0;
    si.nMax = totalHeight;
    si.nPage = clientRect.bottom;
    SetScrollInfo(m_hwndParent, SB_VERT, &si, TRUE);
}

