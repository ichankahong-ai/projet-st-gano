#pragma once
#include <windows.h>
#include <string>

namespace UIDialogs {

    std::string AskMessage(HWND hwnd, const char* title);
    void ShowMessage(HWND hwnd, const char* title, const std::string& message);

}



namespace UIDialogs {

    bool InputBoxA(HWND parent, const char* title, const char* prompt, char* out, int outSize) {
        HWND dlg = CreateWindowExA(
            WS_EX_DLGMODALFRAME,
            "STATIC", title,
            WS_POPUP | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, 320, 160,
            parent, NULL, NULL, NULL
        );
        if (!dlg) return false;

        CreateWindowA("STATIC", prompt, WS_CHILD | WS_VISIBLE,
            10, 10, 300, 20, dlg, 0, 0, 0);

        HWND hEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
            10, 40, 290, 20, dlg, (HMENU)1001, 0, 0);

        HWND hOK = CreateWindowA("BUTTON", "OK", WS_CHILD | WS_VISIBLE,
            70, 80, 80, 25, dlg, (HMENU)IDOK, 0, 0);

        HWND hCancel = CreateWindowA("BUTTON", "Annuler", WS_CHILD | WS_VISIBLE,
            160, 80, 80, 25, dlg, (HMENU)IDCANCEL, 0, 0);

        ShowWindow(dlg, SW_SHOW);

        MSG msg;
        bool accepted = false;
        while (GetMessage(&msg, NULL, 0, 0)) {

            if (msg.message == WM_LBUTTONDOWN && msg.hwnd == hOK) {
                GetWindowTextA(hEdit, out, outSize);
                accepted = true;
                break;
            }

            if (msg.message == WM_LBUTTONDOWN && msg.hwnd == hCancel)
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        DestroyWindow(dlg);
        return accepted;
    }

    std::string AskMessage(HWND hwnd, const char* title) {
        char buf[512] = {};
        return InputBoxA(hwnd, title, "Entrer le message :", buf, sizeof(buf)) ? buf : "";
    }

    void ShowMessage(HWND hwnd, const char* title, const std::string& msg) {
        MessageBoxA(hwnd, msg.c_str(), title, MB_OK);
    }

}
