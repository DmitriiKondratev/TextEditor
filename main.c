#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>

#include "Error.h"
#include "Menu.h"
#include "String.h"
#include "Document.h"
#include "ScrollBar.h"

#include "DisplayedModel.h"

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);

const TCHAR szClassName[]   = _T("TextEdit");
const TCHAR szTitle[]       = _T("FileName - TextEdit");
const char* example = "example.txt";

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszArgument, int nCmdShow) {
    HWND        hwnd;       /* This is the handle for our window */
    MSG         messages;   /* Here messages to the application are saved */
    WNDCLASSEX  wincl;      /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance     = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc   = WindowProcedure; /* This function is called by windows */
    wincl.style         = CS_HREDRAW | CS_VREDRAW;
    wincl.cbSize        = sizeof(WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wincl.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    wincl.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wincl.lpszMenuName  = _T("Menu");
    wincl.cbClsExtra    = 0; /* No extra bytes after the window class */
    wincl.cbWndExtra    = 0; /* structure or the window instance */

    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx(&wincl)) {
        PrintError(NULL, ERR_UNKNOWN, __FILE__, __LINE__);
        return ERR_UNKNOWN;
    }

    /* The class is registered, let's create the program */
    hwnd = CreateWindowEx(
        0,              /* Extended possibilites for variation */
        szClassName,    /* Classname */
        szTitle,        /* Title Text */
        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT,  /* Windows decides the position */
        CW_USEDEFAULT,  /* where the window ends up on the screen */
        CW_USEDEFAULT,  /* The programs width */
        CW_USEDEFAULT,  /* and height in pixels */
        HWND_DESKTOP,   /* The window is a child-window to desktop */
        NULL,           /* No menu */
        hThisInstance,  /* Program Instance handler */
        NULL            /* No Window Creation data */
    );

    /* Make the window visible on the screen */
    ShowWindow(hwnd, nCmdShow);
    // UpdateWindow(hwnd);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage(&messages, NULL, 0, 0)) {
        TranslateMessage(&messages); /* Translate virtual-key messages into character messages */
        DispatchMessage(&messages);  /* Send message to WindowProcedure */
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}

void InitOpenFilename(HWND hwnd, OPENFILENAME* ofn) {
    static char szFilter[] = "Text Files(*.TXT)\0*.txt\0";

    ofn->lStructSize        = sizeof(OPENFILENAME);
    ofn->hwndOwner          = hwnd;
    ofn->hInstance          = NULL;
    ofn->lpstrFilter        = szFilter;
    ofn->lpstrCustomFilter  = NULL;
    ofn->nMaxCustFilter     = 0;
    ofn->nFilterIndex       = 0;
    ofn->lpstrFile          = NULL; // Set in Open and Close functions
    ofn->nMaxFile           = _MAX_PATH;
    ofn->lpstrFileTitle     = NULL; // Set in Open and Close functions
    ofn->nMaxFileTitle      = _MAX_FNAME + _MAX_EXT;
    ofn->lpstrInitialDir    = NULL;
    ofn->lpstrTitle         = NULL;
    ofn->Flags              = 0;    // Set in Open and Close functions
    ofn->nFileOffset        = 0;
    ofn->nFileExtension     = 0;
    ofn->lpstrDefExt        = _T("txt");
    ofn->lCustData          = 0L;
    ofn->lpfnHook           = NULL;
    ofn->lpTemplateName     = NULL;
}

// BOOL FileOpenDlg(HWND hwnd, PSTR pstrFileName, PSTR pstrTitleName) {
BOOL FileOpenDlg(HWND hwnd, OPENFILENAME* ofn, PSTR pstrFileName) {
    ofn->hwndOwner  = hwnd;
    ofn->lpstrFile  = pstrFileName;
    // ofn->lpstrFileTitle = pstrTitleName;
    ofn->Flags      = OFN_HIDEREADONLY | OFN_CREATEPROMPT;

    return GetOpenFileName(ofn);
}

/*  This function is called by the Windows function DispatchMessage()  */
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static OPENFILENAME ofn;
    static PSTR pstrFilename;

    static Document*        doc;
    static DisplayedModel   dm;

    HDC         hdc;
    PAINTSTRUCT ps;
    HMENU       hMenu;
    RECT        rectangle;

    #ifdef CARET_ON
        size_t scrollValue;
    #endif

    /* handle the messages */
    switch (message) {
    case WM_CREATE:
        // device context initialization
        hdc = GetDC(hwnd);

        SetMapMode(hdc, MM_TEXT);
        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

        {
            TEXTMETRIC  tm;
            GetTextMetrics(hdc, &tm);
            InitDisplayedModel(&dm, &tm);
        }

        ReleaseDC(hwnd, hdc);

        doc = CreateDocument(example);
        if (!doc) {
            printf("All is so bad");
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        }

        #ifndef DEBUG // ==============================================/
            PrintDocumentParameters(NULL, doc);
            // PrintDocument(NULL, doc);
        #endif // =====================================================/

        CoverDocument(hwnd, &dm, doc);

        hMenu = GetMenu(hwnd);
        CheckMenuItem(hMenu, IDM_FORMAT_WRAP, MF_UNCHECKED);
        break;
    // WM_CREATE

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_FILE_OPEN:
            #ifndef DEBUG // ==================/
                printf("Open is activated\n");
            #endif // =========================/

            InitOpenFilename(hwnd, &ofn);
            pstrFilename = (PSTR)calloc(_MAX_PATH, sizeof(char));

            if (FileOpenDlg(hwnd, &ofn, pstrFilename)) {
                Document* newDoc = CreateDocument(ofn.lpstrFile);
                if (newDoc) {
                    DestroyDocument(&doc);
                    doc = newDoc;
                    
                    #ifndef DEBUG // ==============================================/
                        PrintDocumentParameters(NULL, doc);
                        // PrintDocument(NULL, doc);
                    #endif // =====================================================/
                    CoverDocument(hwnd, &dm, doc);
                }

                #ifndef DEBUG // ==============================================/
                    printf("%s %s\n", pstrFilename, ofn.lpstrFile);
                #endif // =====================================================/
            }
            free(pstrFilename);
            break;

        case IDM_FILE_EXIT:
            #ifndef DEBUG // ==================/
                printf("Exit is activated\n");
            #endif // =========================/

            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case IDM_FORMAT_WRAP:
            hMenu = GetMenu(hwnd);

            switch (dm.mode) {
            case FORMAT_MODE_DEFAULT:
                SwitchMode(hwnd, &dm, FORMAT_MODE_WRAP);
                CheckMenuItem(hMenu, IDM_FORMAT_WRAP, MF_CHECKED);
                break;

            case FORMAT_MODE_WRAP:
                SwitchMode(hwnd, &dm, FORMAT_MODE_DEFAULT);
                CheckMenuItem(hMenu, IDM_FORMAT_WRAP, MF_UNCHECKED);
                break;

            default:
                PrintError(NULL, ERR_UNKNOWN, __FILE__, __LINE__);
                return ERR_UNKNOWN;
            }
            break;

        default:
            PrintError(NULL, ERR_UNKNOWN, __FILE__, __LINE__);
            return ERR_UNKNOWN;
        }

        // common actions for listed commands
        if (LOWORD(wParam) == IDM_FILE_OPEN || LOWORD(wParam) == IDM_FORMAT_WRAP) {
            // force repaint
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
        }
        break;
    // WM_COMMAND

    case WM_SIZE:
        UpdateDisplayedModel(hwnd, &dm, lParam);
        
        #ifdef CARET_ON
            if(hwnd == GetFocus()) {
                CaretSetPos(&dm);
            }
        #endif
        break;
    // WM_SIZE

#ifdef CARET_ON
    case WM_SETFOCUS:
        // create and show the caret
        CaretCreate(hwnd, &dm);
        break;
    // WM_SETFOCUS
    
    case WM_KILLFOCUS:
        // hide and destroy the caret
        CaretDestroy(hwnd);
        break;
    // WM_KILLFOCUS
#endif

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

        DisplayModel(hdc, &dm);
        
        EndPaint(hwnd, &ps);
        break;
    // WM_PAINT

    case WM_HSCROLL:
        if (dm.mode != FORMAT_MODE_DEFAULT) { break; }

        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            #ifdef CARET_ON
                scrollValue = Scroll(hwnd, &dm, 1, LEFT, &rectangle);
                CaretTopLeftBorder(hwnd, &(dm.caret.isHidden.x), scrollValue,
                                        dm.caret.modelPos.pos.x, dm.scrollBars.horizontal.pos,
                                        &(dm.caret.clientPos.x), DECREMENT_OF(dm.clientArea.chars));
            #else
                Scroll(hwnd, &dm, 1, LEFT, &rectangle);
            #endif
            break;
        
        case SB_LINEDOWN:
            #ifdef CARET_ON
                scrollValue = Scroll(hwnd, &dm, 1, RIGHT, &rectangle);
                CaretBottomRightBorder(hwnd, &(dm.caret.isHidden.x), scrollValue,
                                        dm.caret.modelPos.pos.x, dm.scrollBars.horizontal.pos,
                                        &(dm.caret.clientPos.x), DECREMENT_OF(dm.clientArea.chars));
            #else
                Scroll(hwnd, &dm, 1, RIGHT, &rectangle);
            #endif
            break;
        
        case SB_PAGEUP:
            #ifndef CARET_ON
                Scroll(hwnd, &dm, dm.clientArea.chars, LEFT, &rectangle);
            #endif
            break;

        case SB_PAGEDOWN:
            #ifndef CARET_ON
                Scroll(hwnd, &dm, dm.clientArea.chars, RIGHT, &rectangle);
            #endif
            break;
        
        case SB_THUMBTRACK: {
            size_t absolutePos = GetAbsolutePos(HIWORD(wParam), dm.scrollBars.horizontal.maxPos);
            
            if (dm.scrollBars.horizontal.pos > absolutePos) {
                #ifdef CARET_ON
                    scrollValue = Scroll(hwnd, &dm, dm.scrollBars.horizontal.pos - absolutePos, LEFT, &rectangle);
                    CaretTopLeftBorder(hwnd, &(dm.caret.isHidden.x), scrollValue,
                                        dm.caret.modelPos.pos.x, dm.scrollBars.horizontal.pos,
                                        &(dm.caret.clientPos.x), DECREMENT_OF(dm.clientArea.chars));
                #else
                    Scroll(hwnd, &dm, dm.scrollBars.horizontal.pos - absolutePos, LEFT, &rectangle);
                #endif
            } else if (dm.scrollBars.horizontal.pos < absolutePos) {
                #ifdef CARET_ON
                    scrollValue = Scroll(hwnd, &dm, absolutePos - dm.scrollBars.horizontal.pos, RIGHT, &rectangle);
                    CaretBottomRightBorder(hwnd, &(dm.caret.isHidden.x), scrollValue,
                                        dm.caret.modelPos.pos.x, dm.scrollBars.horizontal.pos,
                                        &(dm.caret.clientPos.x), DECREMENT_OF(dm.clientArea.chars));
                #else
                    Scroll(hwnd, &dm, absolutePos - dm.scrollBars.horizontal.pos, RIGHT, &rectangle);
                #endif
            }
            break;
        }
        default:
            break;
        }

        #ifdef CARET_ON
            CaretSetPos(&dm);
        #endif
        break;
    // WM_HSCROLL

    case WM_VSCROLL:
        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            #ifdef CARET_ON
                scrollValue = Scroll(hwnd, &dm, 1, UP, &rectangle);
                CaretTopLeftBorder(hwnd, &(dm.caret.isHidden.y), scrollValue,
                                        dm.caret.modelPos.pos.y, dm.scrollBars.vertical.pos,
                                        &(dm.caret.clientPos.y), DECREMENT_OF(dm.clientArea.lines));
            #else
                Scroll(hwnd, &dm, 1, UP, &rectangle);
            #endif
            break;

        case SB_LINEDOWN:
            #ifdef CARET_ON
                scrollValue = Scroll(hwnd, &dm, 1, DOWN, &rectangle);
                CaretBottomRightBorder(hwnd, &(dm.caret.isHidden.y), scrollValue,
                                        dm.caret.modelPos.pos.y, dm.scrollBars.vertical.pos,
                                        &(dm.caret.clientPos.y), DECREMENT_OF(dm.clientArea.lines));
            #else
                Scroll(hwnd, &dm, 1, DOWN, &rectangle);
            #endif
            break;

        case SB_PAGEUP:
            #ifndef CARET_ON
                Scroll(hwnd, &dm, dm.clientArea.lines, UP, &rectangle);
            #endif
            break;

        case SB_PAGEDOWN:
            #ifndef CARET_ON
                Scroll(hwnd, &dm, dm.clientArea.lines, DOWN, &rectangle);
            #endif
            break;

        case SB_THUMBTRACK: {
            size_t absolutePos = GetAbsolutePos(HIWORD(wParam), dm.scrollBars.vertical.maxPos);
            
            if (dm.scrollBars.vertical.pos > absolutePos) {
                #ifdef CARET_ON
                    scrollValue = Scroll(hwnd, &dm, dm.scrollBars.vertical.pos - absolutePos, UP, &rectangle);
                    CaretTopLeftBorder(hwnd, &(dm.caret.isHidden.y), scrollValue,
                                        dm.caret.modelPos.pos.y, dm.scrollBars.vertical.pos,
                                        &(dm.caret.clientPos.y), DECREMENT_OF(dm.clientArea.lines));
                #else
                    Scroll(hwnd, &dm, dm.scrollBars.vertical.pos - absolutePos, UP, &rectangle);
                #endif
            } else if (dm.scrollBars.vertical.pos < absolutePos) {
                #ifdef CARET_ON
                    scrollValue = Scroll(hwnd, &dm, absolutePos - dm.scrollBars.vertical.pos, DOWN, &rectangle);
                    CaretBottomRightBorder(hwnd, &(dm.caret.isHidden.y), scrollValue,
                                        dm.caret.modelPos.pos.y, dm.scrollBars.vertical.pos,
                                        &(dm.caret.clientPos.y), DECREMENT_OF(dm.clientArea.lines));;
                #else
                    Scroll(hwnd, &dm, absolutePos - dm.scrollBars.vertical.pos, DOWN, &rectangle);
                #endif
            } 
            break;
        }
        default:
            break;
        }

        #ifdef CARET_ON
            CaretSetPos(&dm);
        #endif
        break;
    // WM_VSCROLL

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_UP:
            #ifdef CARET_ON
                if (dm.mode == FORMAT_MODE_WRAP) { break; }

                FindCaret(hwnd, &dm, &rectangle);

                if (dm.caret.modelPos.pos.y > 0) {
                    CaretMoveToTop(hwnd, &dm, &rectangle);
                    FindEnd_Left(hwnd, &dm, &rectangle);
                }
            #else
                PostMessage(hwnd, WM_VSCROLL, SB_LINEUP, (LPARAM)0);
            #endif
            break;

        case VK_DOWN:
            #ifdef CARET_ON
                if (dm.mode == FORMAT_MODE_WRAP) { break; }

                FindCaret(hwnd, &dm, &rectangle);

                if (dm.caret.modelPos.pos.y < dm.documentArea.lines - 1) {
                    CaretMoveToBottom(hwnd, &dm, &rectangle);
                    FindEnd_Left(hwnd, &dm, &rectangle);
                }
            #else
                PostMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, (LPARAM)0);
            #endif
            break;

        case VK_LEFT:
            #ifdef CARET_ON
                if (dm.mode == FORMAT_MODE_WRAP) { break; }

                FindCaret(hwnd, &dm, &rectangle);

                if (dm.caret.modelPos.pos.x > 0) {
                    CaretMoveToLeft(hwnd, &dm, &rectangle);
                } else if (dm.caret.modelPos.pos.y > 0) {    
                    CaretMoveToTop(hwnd, &dm, &rectangle);
                    PostMessage(hwnd, WM_KEYDOWN, VK_END, (LPARAM)0);
                }
            #else
                PostMessage(hwnd, WM_HSCROLL, SB_LINEUP, (LPARAM)0);
            #endif
            break;

        case VK_RIGHT:
            #ifdef CARET_ON
                if (dm.mode == FORMAT_MODE_WRAP) { break; }

                FindCaret(hwnd, &dm, &rectangle);

                if (dm.caret.modelPos.pos.x < dm.caret.modelPos.block->data.len) {
                    CaretMoveToRight(hwnd, &dm, &rectangle);
                } else if (dm.caret.modelPos.pos.y < (dm.documentArea.lines - 1)) {                    
                    CaretMoveToBottom(hwnd, &dm, &rectangle);
                    PostMessage(hwnd, WM_KEYDOWN, VK_HOME, (LPARAM)0);
                }
            #else
                PostMessage(hwnd, WM_HSCROLL, SB_LINEDOWN, (LPARAM)0);
            #endif

            break;

        case VK_PRIOR:
            #ifdef CARET_ON
                if (dm.mode == FORMAT_MODE_WRAP) { break; }

                FindCaret(hwnd, &dm, &rectangle);
                CaretPrintParams(&dm);

                CaretPageUp(hwnd, &dm, &rectangle);
                
                CaretPrintParams(&dm);
                FindEnd_Left(hwnd, &dm, &rectangle);
            #else
                PostMessage(hwnd, WM_VSCROLL, SB_PAGEUP, (LPARAM)0);
            #endif
            break;

        case VK_NEXT:
            #ifdef CARET_ON
                if (dm.mode == FORMAT_MODE_WRAP) { break; }

                FindCaret(hwnd, &dm, &rectangle);
                CaretPrintParams(&dm);

                CaretPageDown(hwnd, &dm, &rectangle);

                CaretPrintParams(&dm);
                FindEnd_Left(hwnd, &dm, &rectangle);
            #else
                PostMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, (LPARAM)0);
            #endif
            break;

        case VK_HOME:
            #ifdef CARET_ON
                if (dm.mode == FORMAT_MODE_WRAP) { break; }
                
                FindCaret(hwnd, &dm, &rectangle);

                FindHome(hwnd, &dm, &rectangle);
            #else
                PostMessage(hwnd, WM_HSCROLL, SB_PAGEUP, (LPARAM)0);
            #endif
            break;

        case VK_END:
            #ifdef CARET_ON
                if (dm.mode == FORMAT_MODE_WRAP) { break; }
                
                FindCaret(hwnd, &dm, &rectangle);
                FindEnd_Right(hwnd, &dm, &rectangle);
            #else
                PostMessage(hwnd, WM_HSCROLL, SB_PAGEDOWN, (LPARAM)0);
            #endif
            break;

        default:
            break;
        }

        #ifdef CARET_ON
            CaretSetPos(&dm);
        #endif
        break;
    // WM_KEYDOWN

    case WM_CHAR :
        for(int i = 0; i < (int) LOWORD(lParam); i++) {
            switch(wParam) {
            case '\b' : // backspace
                printf("backspace\n");
                // if(xCaret > 0) {
                //     xCaret--;
                //     SendMessage(hwnd, WM_KEYDOWN, VK_DELETE, 1L);
                // }
                break;

            case '\t' : // tab
                printf("tab\n");
                // do {
                //     SendMessage(hwnd, WM_CHAR, ' ', 1L);
                // } while(xCaret % 8 != 0);
                break;

            case '\n' : // line feed
                printf("line feed\n");
                // if(++yCaret == cyBuffer) { yCaret = 0; }
                break;

            case '\r' : // carriage return
                printf("carriage return\n");
                // xCaret = 0;
                // if(++yCaret == cyBuffer) { yCaret = 0; }
                break;

            case '\x1B' : // escape
                printf("escape\n");
                // for(y = 0; y < cyBuffer; y++) {
                //     for(x = 0; x < cxBuffer; x++) {
                //         BUFFER(x, y) = ' ';
                //     }
                // }

                // xCaret = 0;
                // yCaret = 0;

                // InvalidateRect(hwnd, NULL, FALSE);
                break;

            default : // character codes
                printf("%c\n", (char) wParam);
                // BUFFER(xCaret, yCaret) = (char) wParam;

                // HideCaret(hwnd);
                // hdc = GetDC(hwnd);

                // SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
                
                // TextOut(hdc, xCaret * cxChar, yCaret * cyChar, & BUFFER(xCaret, yCaret), 1);
                
                // ShowCaret(hwnd);
                // ReleaseDC(hwnd, hdc);
                
                // if(++xCaret == cxBuffer) {
                //     xCaret = 0;
                //     if(++yCaret == cyBuffer) { yCaret = 0; }
                // }
                break;
            }
        }

        // SetCaretPos(dm.caret.clientPos.x * dm.charMetric.x, dm.caret.clientPos.y * dm.charMetric.y);
        break;
    // WM_CHAR

    case WM_DESTROY:
        if (doc) { DestroyDocument(&doc); }
        PostQuitMessage(0); /* send a WM_QUIT to the message queue */
        break;
    // WM_DESTROY

    default: /* for messages that we don't deal with */
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
