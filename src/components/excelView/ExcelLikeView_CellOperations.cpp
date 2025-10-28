#include "components/excelView/ExcelLikeView.h"
#include <algorithm> // For std::max

#define CELL_WIDTH_DX 10
#define CELL_HEIGHT_DX 10
#define TEXT_PADDING 10
#define MINIMUM_CELL_CAPACITY 1


void ExcelLikeView::UpdateGridSizeForCell(const int row, const int col)
{
    if (row >= 0 && row < m_rowCount && col >= 0 && col < m_colCount)
    {
        UpdateScrollbars();
        InvalidateRect(m_hwndParent, nullptr, false);
    }
}

void ExcelLikeView::RecalculateEqualCellSizes()
{
    if (!m_hwndParent) return;

    RECT clientRect;
    GetClientRect(m_hwndParent, &clientRect);

    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;

    if (clientWidth > 0 && clientHeight > 0 && m_colCount > 0 && m_rowCount > 0)
    {
        const int cellWidth = clientWidth / m_colCount;
        const int cellHeight = clientHeight / m_rowCount;

        m_columnWidths.assign(m_colCount, cellWidth);
        m_rowHeights.assign(m_rowCount, cellHeight);

        const int remainderWidth = clientWidth % m_colCount;
        for (int i = 0; i < remainderWidth; ++i) {
            m_columnWidths[i]++;
        }

        const int remainderHeight = clientHeight % m_rowCount;
        for (int i = 0; i < remainderHeight; ++i) {
            m_rowHeights[i]++;
        }
    }
}

SIZE ExcelLikeView::GetTextSize(const std::wstring& text, const int row, const int col) const
{
    SIZE sz = { 0, 0 };
    if (!m_hwndParent || text.empty()) return sz;

    HDC hdc = GetDC(m_hwndParent);
    if (!hdc) return sz;

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

    // Bounds check
    if(row < 0 || row >= m_rowCount || col < 0 || col >= m_colCount)
    {
        return {left, top, left + m_defaultColumnWidth, top + m_defaultRowHeight};
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

    SCROLLINFO si = {sizeof(si)};
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

    long totalWidth = 0;
    for (const int w: m_columnWidths)
    {
        totalWidth += w;
    }

    si.nMin = 0;
    si.nMax = totalWidth;
    si.nPage = clientRect.right;
    si.nPos = m_horizontalScrollPos;
    SetScrollInfo(m_hwndParent, SB_HORZ, &si, TRUE);

    if (IsFileLoaded())
    {
        si.nMin = 0;
        si.nMax = static_cast<int>(m_totalFileSize);
        si.nPage = static_cast<UINT>(CalculateVisibleCapacity());
        si.nPos = static_cast<int>(m_currentFileOffset);
    }
    else
    {
        long totalHeight = 0;
        for (const int h: m_rowHeights)
        {
            totalHeight += h;
        }
        si.nMin = 0;
        si.nMax = totalHeight;
        si.nPage = clientRect.bottom;
        si.nPos = m_verticalScrollPos;
    }

    SetScrollInfo(m_hwndParent, SB_VERT, &si, TRUE);
}

int ExcelLikeView::CalculateCellCapacity(const int col) const
{
    if (col < 0 || col >= m_colCount)
    {
        return 0;
    }

    HDC hdc = GetDC(m_hwndParent);
    if (!hdc)
    {
        return 0;
    }

    HFONT oldFont = (HFONT)SelectObject(hdc, m_hFont);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    const int avgCharWidth = tm.tmAveCharWidth;

    SelectObject(hdc, oldFont);
    ReleaseDC(m_hwndParent, hdc);

    if (avgCharWidth <= 0)
    {
        return 1;
    }

    const int availableWidth = m_columnWidths[col] - TEXT_PADDING;
    return std::max(MINIMUM_CELL_CAPACITY, availableWidth / avgCharWidth);
}