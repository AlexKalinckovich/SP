//
// Created by brota on 11.09.2025.
//

#ifndef TEXTEDITORCOMPONENT_H
#define TEXTEDITORCOMPONENT_H
#include <IComponent.h>

#include <windows.h>
#include <string>

#include "utils/ComponentManager.h"
#include "win32/HashMapMessageHandler.h"

class TextEditorComponent final : public ui::IComponent {
public:
    explicit TextEditorComponent(HINSTANCE hInstance);
    ~TextEditorComponent() override;

    void OnCreate(HWND hwndParent) noexcept override;
    void OnDestroy() noexcept override;
    bool OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept override;

    [[nodiscard]] bool LoadFile() const;
    [[nodiscard]] bool SaveFile() const;
    void SetFont(const std::wstring& fontName, int fontSize);
    void Clear() const;
    [[nodiscard]] std::wstring GetText() const;

    void OnCut() const;
    void OnCopy() const;
    void OnPaste() const;
    void OnUndo() const;
    void OnRedo() const;
    void OnSelectAll() const;

    [[nodiscard]] bool HasUnsavedChanges() const;
    void MarkAsSaved();
    void MarkAsModified();
    bool PromptSaveIfNeeded(HWND hwndParent);
private:
    HINSTANCE hInstance_;
    HWND hwndParent_ = nullptr;
    HWND hEditControl_ = nullptr;
    HFONT hEditFont_ = nullptr;

    bool hasUnsavedChanges_ = false;
    std::wstring currentFilePath_;
    std::wstring originalContent_;

    void UpdateModificationState();
    bool SaveChanges();
    void HandleTextChange();

    win32::HashMapMessageHandler messageHandler_;
    ComponentManager componentManager_;
    void InitializeMessageHandlers();
    void CreateEditControl();
    void DestroyEditControl();
    void ResizeEditControl(int width, int height) const;

    static constexpr int DEFAULT_FONT_SIZE = 12;
    static constexpr const wchar_t* DEFAULT_FONT = L"Consolas";
    static constexpr UINT_PTR IDT_TIMER_SAVE_PROMPT = 1001;
    static constexpr int SAVE_PROMPT_DELAY_MS = 500;
};
#endif //TEXTEDITORCOMPONENT_H
