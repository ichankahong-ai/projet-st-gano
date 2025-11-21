#pragma once


#include <windows.h>

namespace RendererGDI {
    void RenderBitmap(HDC hdc, HBITMAP hBitmap);
}

namespace RendererGDI {

    void RenderBitmap(HDC hdc, HBITMAP hBitmap) {
        if (!hBitmap) return;

        HDC hMem = CreateCompatibleDC(hdc);
        HBITMAP old = (HBITMAP)SelectObject(hMem, hBitmap);

        BITMAP bm;
        GetObject(hBitmap, sizeof(bm), &bm);

        BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hMem, 0, 0, SRCCOPY);

        SelectObject(hMem, old);
        DeleteDC(hMem);
    }

}
