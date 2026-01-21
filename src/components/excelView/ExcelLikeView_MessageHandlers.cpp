#include "components/excelView/ExcelLikeView.h"
#include <numeric>
#include <cmath>
#include <windowsx.h>

#include "meta_info/message_codes.h"

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

    m_messageHandler.RegisterHandler(WM_READABLE_MEMORY_DATA, [this](HWND, const WPARAM, LPARAM lParam) -> LRESULT
    {
        const std::vector<BYTE> *bytes = reinterpret_cast<const std::vector<BYTE> *>(lParam);

        const Encoding detectedEncoding = EncodingDetector::Detect(bytes->data(), bytes->size());
        const std::string text = EncodingDetector::ConvertToUTF8(*bytes, detectedEncoding);
        const size_t textSize = text.size();
        delete bytes;

        this->SpreadTextToCells(text);

        InvalidateRect(m_hwndParent, nullptr, TRUE);
        UpdateWindow(m_hwndParent);

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
    RecalculateEqualCellSizes();
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
            InflateRect(&oldRect, SELECTED_CELL_WIDTH, SELECTED_CELL_WIDTH);
            InvalidateRect(m_hwndParent, &oldRect, FALSE);
        }
        if (m_activeCell.x >= 0 && m_activeCell.y >= 0)
        {
            RECT newRect = GetCellRect(m_activeCell.y, m_activeCell.x);
            InflateRect(&newRect, SELECTED_CELL_WIDTH, SELECTED_CELL_WIDTH);
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
        const wchar_t newChar = static_cast<wchar_t>(symbolCodeWParam);
        Cell& currentCell = m_cells[m_activeCell.y][m_activeCell.x];

        const std::wstring testText = currentCell.text + newChar;
        const SIZE textSize = GetTextSize(testText, m_activeCell.y, m_activeCell.x);
        const int cellWidth = m_columnWidths[m_activeCell.x];

        if (textSize.cx + TEXT_PADDING <= cellWidth - CELL_WIDTH_DX)
        {
            currentCell.text += newChar;

            UpdateGridSizeForCell(m_activeCell.y, m_activeCell.x);
            const RECT cellRect = GetCellRect(m_activeCell.y, m_activeCell.x);
            InvalidateRect(m_hwndParent, &cellRect, FALSE);
        }
        else
        {
            bool movedToNextCell = false;

            int changeX = 0;
            int changeY = 0;
            if (m_activeCell.x + 1 < m_colCount)
            {
                m_activeCell.x++;
                changeX++;
                m_cells[m_activeCell.y][m_activeCell.x].text += newChar;
                movedToNextCell = true;
            }
            else if (m_activeCell.y + 1 < m_rowCount)
            {
                m_activeCell.x = 0;
                m_activeCell.y++;
                changeY++;
                m_cells[m_activeCell.y][m_activeCell.x].text += newChar;
                movedToNextCell = true;
            }

            if (movedToNextCell)
            {
                UpdateGridSizeForCell(m_activeCell.y, m_activeCell.x);
                const RECT prevCellRect = GetCellRect(m_activeCell.y - changeY,m_activeCell.x - changeX);
                InvalidateRect(m_hwndParent, &prevCellRect, FALSE);

                const RECT newCellRect = GetCellRect(m_activeCell.y, m_activeCell.x);
                InvalidateRect(m_hwndParent, &newCellRect, FALSE);
            }
        }
    }
}

void ExcelLikeView::HandleKeyDown(const WPARAM wParam)
{
    const bool isCellActive = m_activeCell.x >= 0 && m_activeCell.y >= 0;
    if (isCellActive)
    {
        const POINT oldActiveCell = m_activeCell;
        const int r = m_activeCell.y;
        const int c = m_activeCell.x;

        switch (wParam)
        {
            case VK_LEFT:   if (c > 0) m_activeCell.x--; break;
            case VK_RIGHT:  if (c < m_colCount - 1) m_activeCell.x++; break;
            case VK_UP:     if (r > 0) m_activeCell.y--; break;
            case VK_DOWN:   if (r < m_rowCount - 1) m_activeCell.y++; break;
            case VK_BACK:
                if (!m_cells[r][c].text.empty())
                {
                    m_cells[r][c].text.pop_back();
                    UpdateGridSizeForCell(r, c);
                }
                return;
            default: ;
        }

        const bool isNewCellActive = m_activeCell.x != oldActiveCell.x || m_activeCell.y != oldActiveCell.y;
        if (isNewCellActive)
        {
            RECT oldRect = GetCellRect(oldActiveCell.y, oldActiveCell.x);
            oldRect.left -= TEXT_PADDING;
            oldRect.top  -= TEXT_PADDING;

            InvalidateRect(m_hwndParent, &oldRect, FALSE);

            const RECT newRect = GetCellRect(m_activeCell.y, m_activeCell.x);
            InvalidateRect(m_hwndParent, &newRect, FALSE);

            UpdateScrollbars();
        }
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
        case SB_LINELEFT: m_horizontalScrollPos -= SCROLL_POS_MOVE; break;
        case SB_LINERIGHT: m_horizontalScrollPos += SCROLL_POS_MOVE; break;
        case SB_PAGELEFT: m_horizontalScrollPos -= static_cast<int>(si.nPage); break;
        case SB_PAGERIGHT: m_horizontalScrollPos += static_cast<int>(si.nPage); break;
        case SB_THUMBTRACK: m_horizontalScrollPos = currentScrollPos; break;
        default: ;
    }

    if(IsFileLoaded())
    {
        m_verticalScrollPos = std::max(0, std::min(m_verticalScrollPos, static_cast<int>(m_totalFileSize)));

        if (m_verticalScrollPos != oldPos)
        {
            SetScrollPos(m_hwndParent, SB_VERT, m_verticalScrollPos, TRUE);
            m_currentFileOffset = static_cast<size_t>(m_verticalScrollPos);
            LoadFileChunk();
        }
    }
    else
    {
        const long totalWidth = std::accumulate(m_columnWidths.begin(), m_columnWidths.end(), 0L);
        const int maxScrollPos = std::max(0, static_cast<int>(totalWidth - si.nPage));
        m_horizontalScrollPos = std::max(0, std::min(m_horizontalScrollPos, maxScrollPos));

        if (m_horizontalScrollPos != oldPos)
        {
            SetScrollPos(m_hwndParent, SB_HORZ, m_horizontalScrollPos, TRUE);
            InvalidateRect(m_hwndParent, nullptr, TRUE);
        }
    }
}

void ExcelLikeView::HandleVScroll(const WPARAM wParam)
{
    if (IsFileLoaded())
    {
        SCROLLINFO si = {sizeof(si), SIF_ALL};
        GetScrollInfo(m_hwndParent, SB_VERT, &si);

        const size_t oldOffset = m_currentFileOffset;
        const int scrollType = LOWORD(wParam);

        switch (scrollType)
        {
            case SB_LINEUP:
                m_currentFileOffset = (m_currentFileOffset > SCROLL_PAGE_SIZE) ? m_currentFileOffset - SCROLL_PAGE_SIZE : 0;
                break;
            case SB_LINEDOWN:
                m_currentFileOffset += SCROLL_PAGE_SIZE;
                break;
            case SB_PAGEUP:
                m_currentFileOffset = (m_currentFileOffset > si.nPage) ? m_currentFileOffset - si.nPage : 0;
                break;
            case SB_PAGEDOWN:
                m_currentFileOffset += si.nPage;
                break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:
                m_currentFileOffset = static_cast<size_t>(HIWORD(wParam));
                break;
            case SB_TOP:
                m_currentFileOffset = 0;
                break;
            case SB_BOTTOM:
                m_currentFileOffset = (m_totalFileSize > si.nPage) ? m_totalFileSize - si.nPage : 0;
                break;
            default:
                break;
        }

        if (m_currentFileOffset > m_totalFileSize)
        {
             m_currentFileOffset = (m_totalFileSize > si.nPage) ? m_totalFileSize - si.nPage : 0;
        }

        if (m_currentFileOffset != oldOffset)
        {
            SetScrollPos(m_hwndParent, SB_VERT, static_cast<int>(m_currentFileOffset), TRUE);
            LoadFileChunk();
        }
    }
    else
    {
        SCROLLINFO si = {sizeof(si), SIF_ALL};
        GetScrollInfo(m_hwndParent, SB_VERT, &si);
        const int oldPos = m_verticalScrollPos;

        const int currentScrollPos  = HIWORD(wParam);
        const int scrollRequestType = LOWORD(wParam);
        switch (scrollRequestType)
        {
            case SB_LINEUP: m_verticalScrollPos -= SCROLL_POS_MOVE; break;
            case SB_LINEDOWN: m_verticalScrollPos += SCROLL_POS_MOVE; break;
            case SB_PAGEUP: m_verticalScrollPos -= static_cast<int>(si.nPage); break;
            case SB_PAGEDOWN: m_verticalScrollPos += static_cast<int>(si.nPage); break;
            case SB_THUMBTRACK: m_verticalScrollPos = currentScrollPos; break;
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
}

void ExcelLikeView::HandleMouseWheel(const WPARAM wParam)
{
    if (IsFileLoaded())
    {
        SCROLLINFO si = {sizeof(si), SIF_ALL};
        GetScrollInfo(m_hwndParent, SB_VERT, &si);

        const int delta        = GET_WHEEL_DELTA_WPARAM(wParam);
        const int scrollAmount = -delta / WHEEL_DELTA * static_cast<int>(si.nPage);

        const size_t oldOffset = m_currentFileOffset;

        if (scrollAmount < 0 && static_cast<size_t>(-scrollAmount) > m_currentFileOffset)
        {
            m_currentFileOffset = 0;
        }
        else
        {
             m_currentFileOffset += scrollAmount;
        }

        if (m_currentFileOffset > m_totalFileSize)
        {
            m_currentFileOffset = (m_totalFileSize > si.nPage) ? m_totalFileSize - si.nPage : 0;
        }

        if (m_currentFileOffset != oldOffset)
        {
            SetScrollPos(m_hwndParent, SB_VERT, static_cast<int>(m_currentFileOffset), TRUE);
            LoadFileChunk();
        }
    }
    else
    {
        const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
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
        m_verticalScrollPos    = std::max(0, std::min(m_verticalScrollPos, maxScrollPos));

        if (m_verticalScrollPos != oldPos)
        {
            SetScrollPos(m_hwndParent, SB_VERT, m_verticalScrollPos, TRUE);
            InvalidateRect(m_hwndParent, nullptr, TRUE);
        }
    }
}


