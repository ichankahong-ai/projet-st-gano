#pragma once
#include <windows.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <vector>
#include <fstream>
#include <string>

namespace ImageManager {
    void Init();
    void Shutdown();
    std::wstring OpenImage(HWND hwnd);
    std::wstring SaveImageDialog(HWND hwnd);
    HBITMAP LoadBitmap(const std::wstring& path);
    bool SaveBitmap(HBITMAP hBmp, const std::wstring& path);
    void DrawVisibleTextOnBitmap(HBITMAP hBitmap, const std::wstring& text);
    HBITMAP LoadBitmapFromHBITMAP(HBITMAP hBmp);

    struct ComparisonResult {
        int totalPixelsDiff = 0;
        int totalChannelsDiff = 0;
        int totalLSBDiff = 0;
        HBITMAP diffImage = nullptr; // Image pour visualiser les différences
    };

    // Compare deux HBITMAP pixel par pixel et retourne un struct avec résultats + image diff
    ComparisonResult CompareBitmaps(HBITMAP bmpA, HBITMAP bmpB);
}

using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")

namespace ImageManager {

    static ULONG_PTR gdiToken = 0;

    // Init / Shutdown
    void Init() {
        if (gdiToken != 0) return;
        GdiplusStartupInput input;
        GdiplusStartup(&gdiToken, &input, nullptr);
    }

    void Shutdown() {
        if (gdiToken != 0) {
            GdiplusShutdown(gdiToken);
            gdiToken = 0;
        }
    }

    void DrawVisibleTextOnBitmap(HBITMAP hBitmap, const std::wstring& text)
    {
        if (!hBitmap || text.empty()) return;

        HDC hdc = CreateCompatibleDC(NULL);
        HBITMAP oldBmp = (HBITMAP)SelectObject(hdc, hBitmap);

        BITMAP bmp;
        GetObject(hBitmap, sizeof(bmp), &bmp);

        // Couleur gris
        COLORREF gray = RGB(180, 180, 180);

        // Police
        HFONT hFont = CreateFont(
            -32, 0, 0, 0,
            FW_BOLD,
            FALSE, FALSE, FALSE,
            ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            FF_SWISS,
            L"Arial"
        );

        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, gray);

        RECT rc;
        rc.left = 20;
        rc.top = 20;
        rc.right = bmp.bmWidth - 20;
        rc.bottom = bmp.bmHeight - 20;

        DrawText(hdc, text.c_str(), -1, &rc, DT_LEFT | DT_TOP);

        SelectObject(hdc, oldFont);
        DeleteObject(hFont);

        SelectObject(hdc, oldBmp);
        DeleteDC(hdc);
    }

    HBITMAP LoadBitmapFromHBITMAP(HBITMAP hBmp) {
        if (!hBmp) return nullptr;

        BITMAP bm;
        GetObject(hBmp, sizeof(bm), &bm);

        HDC hdc = CreateCompatibleDC(NULL);
        HBITMAP copy = CreateCompatibleBitmap(GetDC(NULL), bm.bmWidth, bm.bmHeight);
        HBITMAP old = (HBITMAP)SelectObject(hdc, copy);

        HDC hSrc = CreateCompatibleDC(NULL);
        HBITMAP oldSrc = (HBITMAP)SelectObject(hSrc, hBmp);

        BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hSrc, 0, 0, SRCCOPY);

        SelectObject(hSrc, oldSrc);
        DeleteDC(hSrc);

        SelectObject(hdc, old);
        DeleteDC(hdc);

        return copy;
    }

    // Dialogues Open / Save
    std::wstring OpenImage(HWND hwnd) {
        OPENFILENAME ofn = {};
        wchar_t file[260] = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = file;
        ofn.nMaxFile = 260;
        ofn.lpstrFilter =
            L"Images (*.bmp;*.png;*.jpg)\0*.bmp;*.png;*.jpg\0"
            L"Tous fichiers\0*.*\0";
        ofn.Flags = OFN_FILEMUSTEXIST;
        if (GetOpenFileName(&ofn)) return file;
        return L"";
    }

    std::wstring SaveImageDialog(HWND hwnd) {
        OPENFILENAME ofn = {};
        wchar_t file[260] = L"image_modifiee.bmp";
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = file;
        ofn.nMaxFile = 260;
        ofn.lpstrFilter =
            L"BMP (*.bmp)\0*.bmp\0"
            L"Tous fichiers\0*.*\0";
        ofn.Flags = OFN_OVERWRITEPROMPT;
        if (GetSaveFileName(&ofn)) return file;
        return L"";
    }




    // Charger image via GDI+ et convertir en 24bpp
    HBITMAP LoadBitmap(const std::wstring& path) {
        Bitmap bmp(path.c_str());
        if (bmp.GetLastStatus() != Ok) return nullptr;

        Bitmap bmp24(bmp.GetWidth(), bmp.GetHeight(), PixelFormat24bppRGB);
        Graphics g(&bmp24);
        g.DrawImage(&bmp, 0, 0, bmp.GetWidth(), bmp.GetHeight());

        HBITMAP hBmp = nullptr;
        bmp24.GetHBITMAP(Color(0, 0, 0), &hBmp);
        return hBmp;
    }

    // Sauvegarde BMP via GDI+
    bool SaveBitmap(HBITMAP hBmp, const std::wstring& path) {
        Bitmap bmp(hBmp, nullptr);
        CLSID clsid;
        CLSIDFromString(L"{557CF400-1A04-11D3-9A73-0000F81EF32E}", &clsid); // BMP CLSID
        return bmp.Save(path.c_str(), &clsid, nullptr) == Ok;
    }

    ImageManager::ComparisonResult ImageManager::CompareBitmaps(HBITMAP bmpA, HBITMAP bmpB)
    {
        ComparisonResult result;

        if (!bmpA || !bmpB) return result;

        BITMAP bmA, bmB;
        GetObject(bmpA, sizeof(bmA), &bmA);
        GetObject(bmpB, sizeof(bmB), &bmB);

        // Vérifier largeur, hauteur et profondeur
        if (bmA.bmWidth != bmB.bmWidth ||
            bmA.bmHeight != bmB.bmHeight ||
            bmA.bmBitsPixel != bmB.bmBitsPixel) {
            MessageBoxW(NULL, L"Les images n'ont pas la même taille ou profondeur.", L"Erreur", MB_OK | MB_ICONERROR);
            return result;
        }

        int width = bmA.bmWidth;
        int height = bmA.bmHeight;
        int rowSize = width * (bmA.bmBitsPixel / 8);

        std::vector<unsigned char> pxA(rowSize * height);
        std::vector<unsigned char> pxB(rowSize * height);

        GetBitmapBits(bmpA, pxA.size(), pxA.data());
        GetBitmapBits(bmpB, pxB.size(), pxB.data());

        // Créer image de diff
        HBITMAP diffBmp = CreateCompatibleBitmap(GetDC(NULL), width, height);
        HDC hdcMem = CreateCompatibleDC(NULL);
        HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, diffBmp);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // inversé pour GDI+
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;   // RGBA pour diff
        bmi.bmiHeader.biCompression = BI_RGB;

        std::vector<unsigned char> diffPixels(width * height * 4);

        for (int i = 0; i < pxA.size(); i += 3) { // parcourir RGB
            bool different = false;
            for (int c = 0; c < 3; ++c) {
                if (pxA[i + c] != pxB[i + c]) {
                    different = true;
                    result.totalChannelsDiff++;
                    if ((pxA[i + c] & 1) != (pxB[i + c] & 1))
                        result.totalLSBDiff++;
                }
            }

            int idx = (i / 3) * 4; // RGBA dans diffPixels
            if (different) {
                result.totalPixelsDiff++;
                diffPixels[idx + 0] = 0;   // B
                diffPixels[idx + 1] = 0;   // G
                diffPixels[idx + 2] = 255; // R
            }
            else {
                diffPixels[idx + 0] = 0;
                diffPixels[idx + 1] = 0;
                diffPixels[idx + 2] = 0;
            }
            diffPixels[idx + 3] = 255; // Alpha
        }

        SetDIBits(hdcMem, diffBmp, 0, height, diffPixels.data(), &bmi, DIB_RGB_COLORS);
        result.diffImage = diffBmp;

        SelectObject(hdcMem, oldBmp);
        DeleteDC(hdcMem);

        return result;
    }
}