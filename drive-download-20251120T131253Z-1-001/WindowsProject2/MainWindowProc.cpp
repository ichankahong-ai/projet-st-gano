
#include "ImageManager.h"
#include "Renderer.h"
#include "StegEngine.h"
#include "UIDialogs.h"
#include <iostream>
using namespace std;
using namespace ImageManager;
// Commandes menu
#define ID_FICHIER_OUVRIR_IMAGE 401
#define ID_FICHIER_ENREGISTRER  403
#define ID_FICHIER_QUITTER 402
#define ID_LIRE_MESSAGE 502
#define ID_ENTRER_MESSAGE 503
#define ID_VERIFIER_CHECKSUM 504
#define ID_EFFACER_TEXTE 601
#define ID_AFFICHER_TEXTE 602
#define ID_COMPARER_IMAGES 701



std::wstring g_currentImagePath = L"";
HBITMAP hBitmap = NULL;
HBITMAP hBitmapOriginal = NULL;

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    ImageManager::Init();

    WNDCLASS wc = {};
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ImageWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, L"ImageWindow", L"Editeur BMP + Steganographie LSB",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImageManager::Shutdown();
    return (int)msg.wParam;
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   
    switch (uMsg) {

    case WM_CREATE: {
        HMENU hMenuBar = CreateMenu();
        HMENU hMenuFichier = CreateMenu();
        HMENU hMenuTexte = CreateMenu();

        AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_OUVRIR_IMAGE, L"Ouvrir...");
        AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_ENREGISTRER, L"Enregistrer sous...");
        AppendMenu(hMenuFichier, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenuFichier, MF_STRING, ID_ENTRER_MESSAGE, L"Intégrer un message...");
        AppendMenu(hMenuFichier, MF_STRING, ID_LIRE_MESSAGE, L"Lire message caché");
        AppendMenu(hMenuFichier, MF_STRING, ID_VERIFIER_CHECKSUM, L"Vérifier checksum");
        AppendMenu(hMenuFichier, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_QUITTER, L"Quitter");
        AppendMenu(hMenuTexte, MF_STRING, ID_AFFICHER_TEXTE, L"Afficher texte");
        AppendMenu(hMenuTexte, MF_STRING, ID_EFFACER_TEXTE, L"Effacer texte");
        AppendMenu(hMenuFichier, MF_STRING, ID_COMPARER_IMAGES, L"Comparer deux images...");

        AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hMenuTexte, L"Texte");
        AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hMenuFichier, L"Fichier");
        SetMenu(hwnd, hMenuBar);
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        if (id == ID_FICHIER_QUITTER) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        else if (id == ID_FICHIER_OUVRIR_IMAGE) {
            std::wstring fichier = ImageManager::OpenImage(hwnd);
            if (!fichier.empty()) {
                g_currentImagePath = fichier; // mémoriser le chemin
                if (hBitmap) DeleteObject(hBitmap);
                if (hBitmapOriginal) DeleteObject(hBitmapOriginal);

                hBitmapOriginal = ImageManager::LoadBitmap(fichier);
                hBitmap = ImageManager::LoadBitmap(fichier);
                
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        else if (id == ID_FICHIER_ENREGISTRER) {
            if (hBitmap) {
                std::wstring path = ImageManager::SaveImageDialog(hwnd);
                if (!path.empty()) {

                    // Aucun message demandé ici
                    ImageManager::SaveBitmap(hBitmapOriginal, path);

                    UIDialogs::ShowMessage(hwnd,
                        "Image enregistrée",
                        "L'image a été enregistrée sans modification.");
                }
            }
        }

        else if (id == ID_VERIFIER_CHECKSUM) {
            if (hBitmap) {
                std::string msg = StegEngine::ExtractLSB(hBitmap);
                if (msg.empty()) {
                    UIDialogs::ShowMessage(hwnd, "Checksum", "Aucun message ou message corrompu !");
                }
                else {
                    UIDialogs::ShowMessage(hwnd, "Checksum", "Message valide ");
                }
            }
            else {
                UIDialogs::ShowMessage(hwnd, "Checksum", "Aucune image chargée !");
            }
        }

        else if (id == ID_ENTRER_MESSAGE) {
            if (hBitmap && hBitmapOriginal) {
                // 1) Demander le nouveau message à l'utilisateur
                std::string msg = UIDialogs::AskMessage(hwnd, "Intégrer message");
                if (msg.empty()) return 0;

                std::wstring wmsg(msg.begin(), msg.end());

                // 2) Repartir d'une copie propre de hBitmapOriginal (ancien message effacé)
                HBITMAP hClean = ImageManager::LoadBitmapFromHBITMAP(hBitmapOriginal);
                if (hBitmap) DeleteObject(hBitmap);
                hBitmap = hClean;

                // 3) Intégrer le nouveau message LSB
                StegEngine::EmbedLSB(hBitmap, msg);

                // 4) Mettre à jour hBitmapOriginal pour contenir le nouveau LSB mais sans texte visible
                if (hBitmapOriginal) DeleteObject(hBitmapOriginal);
                hBitmapOriginal = ImageManager::LoadBitmapFromHBITMAP(hBitmap);

                // 5) Créer une copie temporaire pour afficher le texte visible
                HBITMAP hTmp = ImageManager::LoadBitmapFromHBITMAP(hBitmapOriginal);
                ImageManager::DrawVisibleTextOnBitmap(hTmp, wmsg);

                // 6) Mettre à jour hBitmap pour l'affichage
                if (hBitmap) DeleteObject(hBitmap);
                hBitmap = hTmp;

                InvalidateRect(hwnd, NULL, TRUE);

                // 7) Sauvegarder automatiquement le LSB (sans texte visible)
                if (!g_currentImagePath.empty()) {
                    ImageManager::SaveBitmap(hBitmapOriginal, g_currentImagePath);

                    UIDialogs::ShowMessage(hwnd,
                        "Message intégré",
                        "Le message a été intégré et l'image a été\n"
                        "automatiquement sauvegardée dans le fichier original.");
                }
                else {
                    UIDialogs::ShowMessage(hwnd,
                        "Avertissement",
                        "Image ouverte depuis une source inconnue :\n"
                        "impossible de sauvegarder automatiquement.");
                }
            }
        }



        else if (id == ID_LIRE_MESSAGE) {
            if (hBitmap) {
                std::string msg = StegEngine::ExtractLSB(hBitmap);
                if (msg.empty())
                    UIDialogs::ShowMessage(hwnd, "Erreur", "Aucun message caché trouvé.");
                else
                    UIDialogs::ShowMessage(hwnd, "Message décodé", msg);
            }
        }
        else if (id == ID_EFFACER_TEXTE) {
            if (hBitmap && hBitmapOriginal) {
                // Remettre l'image originale
                if (hBitmap) DeleteObject(hBitmap);
                hBitmap = ImageManager::LoadBitmapFromHBITMAP(hBitmapOriginal); // copie
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }


        else if (id == ID_AFFICHER_TEXTE) {
            if (hBitmap && hBitmapOriginal) {
                // Restaurer original avant d’écrire texte
                if (hBitmap) DeleteObject(hBitmap);
                hBitmap = ImageManager::LoadBitmapFromHBITMAP(hBitmapOriginal); // copie

                std::string msg = StegEngine::ExtractLSB(hBitmap);
                if (!msg.empty()) {
                    std::wstring wmsg(msg.begin(), msg.end());
                    ImageManager::DrawVisibleTextOnBitmap(hBitmap, wmsg);
                }
                else {
                    UIDialogs::ShowMessage(hwnd, "Texte visible", "Aucun message LSB trouvé !");
                }

                InvalidateRect(hwnd, NULL, TRUE);
            }
        }

        else if (id == ID_COMPARER_IMAGES) {
            std::wstring fileA = ImageManager::OpenImage(hwnd);
            if (fileA.empty()) return 0;

            std::wstring fileB = ImageManager::OpenImage(hwnd);
            if (fileB.empty()) return 0;

            HBITMAP bmpA = ImageManager::LoadBitmap(fileA);
            HBITMAP bmpB = ImageManager::LoadBitmap(fileB);

            auto result = ImageManager::CompareBitmaps(bmpA, bmpB);

            // Construire le message en std::string (ASCII)
            std::string message = "Pixels modifiés : " + std::to_string(result.totalPixelsDiff) +
                "\nCanaux modifiés : " + std::to_string(result.totalChannelsDiff) +
                "\nBits LSB modifiés : " + std::to_string(result.totalLSBDiff) +
                "\nVerdict : " + (result.totalPixelsDiff == 0 ?
                    "Aucune différence détectée." :
                    "Différences détectées !");

            UIDialogs::ShowMessage(hwnd, "Résultat comparaison", message);

            if (result.diffImage) {
                if (hBitmap) DeleteObject(hBitmap);
                hBitmap = result.diffImage;
                InvalidateRect(hwnd, NULL, TRUE);
            }

            // Libérer les bitmaps temporaires
            if (bmpA) DeleteObject(bmpA);
            if (bmpB) DeleteObject(bmpB);
            }



        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RendererGDI::RenderBitmap(hdc, hBitmap);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        if (hBitmap) DeleteObject(hBitmap);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}