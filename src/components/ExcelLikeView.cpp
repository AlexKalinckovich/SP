#include "components/ExcelLikeView.h"

#include <windowsx.h>

ExcelLikeView::ExcelLikeView(const int initialRows, const int initialCols)
    : m_initialRows(initialRows), m_initialCols(initialCols) {}

ExcelLikeView::~ExcelLikeView() noexcept
{
    OnDestroy();
}

void ExcelLikeView::OnCreate(HWND hwndParent) noexcept
{
    m_hwndParent = hwndParent;

    m_cells.assign(m_initialRows, std::vector<Cell>(m_initialCols));
    m_columnWidths.assign(m_initialCols, 100); // Default width
    m_rowHeights.assign(m_initialRows, 25);   // Default height

    LOGFONTW lf = { 0 };
    lf.lfHeight = -MulDiv(10, GetDeviceCaps(GetDC(m_hwndParent), LOGPIXELSY), 72);
    wcscpy_s(lf.lfFaceName, L"Arial");
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
    if (!m_fontFilePath.empty())
    {
        RemoveFontResourceExW(m_fontFilePath.c_str(), FR_PRIVATE, nullptr);
        m_fontFilePath.clear();
    }
    m_hwndParent = nullptr;
}

bool ExcelLikeView::OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept
{
    if (!m_hwndParent) return false;

    switch (msg)
    {
        case WM_PAINT:      HandlePaint(hwnd); *outResult = 0; return true;
        case WM_SIZE:       HandleSize(lParam); return true;
        case WM_LBUTTONDOWN: HandleLButtonDown(lParam); return true;
        case WM_CHAR:       HandleChar(wParam); return true;
        case WM_KEYDOWN:    HandleKeyDown(wParam); return true;
        case WM_HSCROLL:    HandleHScroll(wParam); return true;
        case WM_VSCROLL:    HandleVScroll(wParam); return true;
    }
    return false;
}

void ExcelLikeView::SetFont(const std::wstring& fontFilePath, const std::wstring& fontName, int fontSize)
{
    if (!m_hwndParent) return;

    if (!m_fontFilePath.empty())
    {
        RemoveFontResourceExW(m_fontFilePath.c_str(), FR_PRIVATE, nullptr);
    }
    m_fontFilePath = fontFilePath;

    if (AddFontResourceExW(m_fontFilePath.c_str(), FR_PRIVATE, nullptr) == 0)
    {
        m_fontFilePath.clear();
        return;
    }

    SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

    if (m_hFont) DeleteObject(m_hFont);

    LOGFONTW lf = { 0 };
    HDC hdc = GetDC(m_hwndParent);
    lf.lfHeight = -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(m_hwndParent, hdc);
    lf.lfWeight = FW_NORMAL;
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, fontName.c_str());
    m_hFont = CreateFontIndirectW(&lf);

    for (int r = 0; r < m_initialRows; ++r) {
        for (int c = 0; c < m_initialCols; ++c) {
            AutoSizeColumnAndRowForCell(r, c);
        }
    }
    InvalidateRect(m_hwndParent, nullptr, TRUE);
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

    HFONT oldFont = (HFONT)SelectObject(memDC, m_hFont);
    SetBkMode(memDC, TRANSPARENT);

    DrawGrid(memDC, clientRect);
    DrawCells(memDC);
    DrawFocusRect(memDC);

    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    SelectObject(memDC, oldFont);
    DeleteObject(memBitmap);
    DeleteDC(memDC);

    EndPaint(hwnd, &ps);
}

void ExcelLikeView::HandleSize(LPARAM lParam)
{
    UpdateScrollbars();
    InvalidateRect(m_hwndParent, nullptr, TRUE);
}

void ExcelLikeView::HandleLButtonDown(LPARAM lParam)
{
    m_activeCell = GetCellFromCoordinates(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    InvalidateRect(m_hwndParent, nullptr, TRUE);
}

void ExcelLikeView::HandleChar(WPARAM wParam)
{
    if (m_activeCell.y == -1 || m_activeCell.x == -1 || wParam < 32) return;

    m_cells[m_activeCell.y][m_activeCell.x].text += static_cast<wchar_t>(wParam);
    AutoSizeColumnAndRowForCell(m_activeCell.y, m_activeCell.x);
    InvalidateRect(m_hwndParent, NULL, TRUE);
}

void ExcelLikeView::HandleKeyDown(WPARAM wParam)
{
    if (m_activeCell.y == -1 || m_activeCell.x == -1) return;

    int r = m_activeCell.y;
    int c = m_activeCell.x;
    auto& text = m_cells[r][c].text;

    switch (wParam) {
        case VK_BACK:   if (!text.empty()) text.pop_back(); break;
        case VK_DELETE: text.clear(); break;
        case VK_LEFT:   if (c > 0) m_activeCell.x--; break;
        case VK_RIGHT:  if (c < m_initialCols - 1) m_activeCell.x++; break;
        case VK_UP:     if (r > 0) m_activeCell.y--; break;
        case VK_DOWN:   if (r < m_initialRows - 1) m_activeCell.y++; break;
        case VK_RETURN: if (r < m_initialRows - 1) m_activeCell.y++; break;
        case VK_ESCAPE: m_activeCell = { -1, -1 }; break;
        default: return;
    }
    InvalidateRect(m_hwndParent, NULL, TRUE);
}

void ExcelLikeView::HandleHScroll(WPARAM wParam)
{
    SCROLLINFO si = { sizeof(si), SIF_ALL };
    GetScrollInfo(m_hwndParent, SB_HORZ, &si);
    int oldPos = m_horizontalScrollPos;

    switch (LOWORD(wParam)) {
        case SB_LINELEFT:   m_horizontalScrollPos -= 20; break;
        case SB_LINERIGHT:  m_horizontalScrollPos += 20; break;
        case SB_PAGELEFT:   m_horizontalScrollPos -= si.nPage; break;
        case SB_PAGERIGHT:  m_horizontalScrollPos += si.nPage; break;
        case SB_THUMBTRACK: m_horizontalScrollPos = HIWORD(wParam); break;
    }

    long totalWidth = 0;
    for (int w : m_columnWidths) totalWidth += w;
    int maxScrollPos = std::max(0, (int)totalWidth - (int)si.nPage);
    m_horizontalScrollPos = std::max(0, std::min(m_horizontalScrollPos, maxScrollPos));

    if (m_horizontalScrollPos != oldPos) {
        SetScrollPos(m_hwndParent, SB_HORZ, m_horizontalScrollPos, TRUE);
        InvalidateRect(m_hwndParent, nullptr, TRUE);
    }
}

void ExcelLikeView::HandleVScroll(WPARAM wParam)
{
    SCROLLINFO si = { sizeof(si), SIF_ALL };
    GetScrollInfo(m_hwndParent, SB_VERT, &si);
    int oldPos = m_verticalScrollPos;

    switch (LOWORD(wParam)) {
        case SB_LINEUP:     m_verticalScrollPos -= 20; break;
        case SB_LINEDOWN:   m_verticalScrollPos += 20; break;
        case SB_PAGEUP:     m_verticalScrollPos -= si.nPage; break;
        case SB_PAGEDOWN:   m_verticalScrollPos += si.nPage; break;
        case SB_THUMBTRACK: m_verticalScrollPos = HIWORD(wParam); break;
    }

    long totalHeight = 0;
    for (int h : m_rowHeights) totalHeight += h;
    int maxScrollPos = std::max(0, (int)totalHeight - (int)si.nPage);
    m_verticalScrollPos = std::max(0, std::min(m_verticalScrollPos, maxScrollPos));

    if (m_verticalScrollPos != oldPos) {
        SetScrollPos(m_hwndParent, SB_VERT, m_verticalScrollPos, TRUE);
        InvalidateRect(m_hwndParent, nullptr, TRUE);
    }
}


// --- Drawing and Layout ---

void ExcelLikeView::DrawGrid(HDC hdc, const RECT& clientRect)
{
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(215, 215, 215));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    int x = -m_horizontalScrollPos;
    for (const int width : m_columnWidths) {
        x += width;
        if (x > clientRect.right) break;
        MoveToEx(hdc, x, 0, nullptr);
        LineTo(hdc, x, clientRect.bottom);
    }

    int y = -m_verticalScrollPos;
    for (int height : m_rowHeights) {
        y += height;
        if (y > clientRect.bottom) break;
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

    for (size_t r = 0; r < m_cells.size(); ++r) {
        for (size_t c = 0; c < m_cells[r].size(); ++c) {
            if (!m_cells[r][c].text.empty()) {
                RECT cellRect = GetCellRect(static_cast<int>(r), static_cast<int>(c));
                if (cellRect.bottom < 0 || cellRect.top > clientRect.bottom) continue;
                if (cellRect.right < 0 || cellRect.left > clientRect.right) continue;

                cellRect.left += 5; // Cell padding
                DrawTextW(hdc, m_cells[r][c].text.c_str(), -1, &cellRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            }
        }
    }
}

void ExcelLikeView::DrawFocusRect(HDC hdc) const
{
    if (m_activeCell.y == -1 || m_activeCell.x == -1) return;

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

void ExcelLikeView::AutoSizeColumnAndRowForCell(const int row, const int col)
{
    if (row < 0 || row >= m_initialRows || col < 0 || col >= m_initialCols) return;

    const auto& text = m_cells[row][col].text;
    if (text.empty()) return;

    HDC hdc = GetDC(m_hwndParent);
    HFONT oldFont = (HFONT)SelectObject(hdc, m_hFont);

    SIZE textSize;
    GetTextExtentPoint32W(hdc, text.c_str(), static_cast<int>(text.length()), &textSize);

    SelectObject(hdc, oldFont);
    ReleaseDC(m_hwndParent, hdc);

    bool changed = false;
    if (textSize.cx + 10 > m_columnWidths[col]) {
        m_columnWidths[col] = textSize.cx + 10;
        changed = true;
    }
    if (textSize.cy + 6 > m_rowHeights[row]) {
        m_rowHeights[row] = textSize.cy + 6;
        changed = true;
    }

    if (changed) {
        UpdateScrollbars();
    }
}

POINT ExcelLikeView::GetCellFromCoordinates(int x, int y) const
{
    POINT cell = { -1, -1 };

    int currentX = -m_horizontalScrollPos;
    for (size_t c = 0; c < m_columnWidths.size(); ++c) {
        if (x < currentX + m_columnWidths[c]) {
            cell.x = static_cast<LONG>(c);
            break;
        }
        currentX += m_columnWidths[c];
    }
    if (cell.x == -1) return { -1, -1 };

    int currentY = -m_verticalScrollPos;
    for (size_t r = 0; r < m_rowHeights.size(); ++r) {
        if (y < currentY + m_rowHeights[r]) {
            cell.y = static_cast<LONG>(r);
            break;
        }
        currentY += m_rowHeights[r];
    }
    if (cell.y == -1) return { -1, -1 };

    return cell;
}

RECT ExcelLikeView::GetCellRect(const int row, const int col) const
{
    int left = -m_horizontalScrollPos;
    for (int i = 0; i < col; ++i) left += m_columnWidths[i];

    int top = -m_verticalScrollPos;
    for (int i = 0; i < row; ++i) top += m_rowHeights[i];

    return { left, top, left + m_columnWidths[col], top + m_rowHeights[row] };
}

void ExcelLikeView::UpdateScrollbars()
{
    if (!m_hwndParent) return;

    RECT clientRect;
    GetClientRect(m_hwndParent, &clientRect);

    long totalWidth = 0;
    for (int w : m_columnWidths) totalWidth += w;

    long totalHeight = 0;
    for (int h : m_rowHeights) totalHeight += h;

    SCROLLINFO si = { sizeof(si) };
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
