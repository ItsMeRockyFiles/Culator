#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <dwmapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define ID_BUTTON_0 0
#define ID_BUTTON_1 1
#define ID_BUTTON_2 2
#define ID_BUTTON_3 3
#define ID_BUTTON_4 4
#define ID_BUTTON_5 5
#define ID_BUTTON_6 6
#define ID_BUTTON_7 7
#define ID_BUTTON_8 8
#define ID_BUTTON_9 9

#define ID_BUTTON_Times     20
#define ID_BUTTON_Subtract  21
#define ID_BUTTON_Add       22
#define ID_BUTTON_Divide    23

#define ID_BUTTON_Switch    24
#define ID_BUTTON_Equal     25
#define ID_BUTTON_Comma     26
#define ID_BUTTON_Clear     27
#define ID_BUTTON_ClearEntry 28
#define ID_BUTTON_Backspace 30
#define ID_BUTTON_GEAR      31

#define SPACING 2
#define DISPLAY_HEIGHT 200
#define HISTORY_HEIGHT 50
#define BUTTON_ROWS 5
#define BUTTON_COLUMNS 4
#define GEAR_SIZE 30

#define STATE_FIRSTNUM 0
#define STATE_SECONDNUM 1

#define BTN_NUM 0
#define BTN_OP 1
#define BTN_EQ 2

#define COLOR_BG        RGB(32, 32, 32)
#define COLOR_BTN_NUM   RGB(59, 59, 59)
#define COLOR_BTN_OP    RGB(50, 50, 50)
#define COLOR_BTN_EQ    RGB(76, 194, 255)
#define COLOR_TEXT      RGB(255, 255, 255)
#define COLOR_TEXT_GRAY RGB(180, 180, 180)

HWND hDisplay = NULL;
HWND hHistoryDisplay = NULL;
HWND hGearButton = NULL;
HFONT hFont = NULL;
HFONT hBtnFont = NULL;
HFONT hHistFont = NULL;
HFONT hGearFont = NULL;
HBRUSH hDarkBrush = NULL;

int buttonTypes[31] = {0};

char FirstNumStr[32] = "0";
char SecondNumStr[32] = "0";
double FirstNum = 0;
double SecondNum = 0;

char operator;
int State = 0;
double total = 0;

char* CurrentStr() {
    return (State == STATE_FIRSTNUM) ? FirstNumStr : SecondNumStr;
}

double* CurrentNumPtr() {
    return (State == STATE_FIRSTNUM) ? &FirstNum : &SecondNum;
}

double Round2(double v) {
    if (v >= 0) {
        return (double)((long long)(v * 100.0 + 0.5)) / 100.0;
    }
    return (double)((long long)(v * 100.0 - 0.5)) / 100.0;
}

void FormatResult(double value, char* buffer) {
    double r = Round2(value);
    if (r == (double)(long long)r) {
        sprintf(buffer, "%.0f", r);
    } else {
        sprintf(buffer, "%.2f", r);
    }
}

void AppendDigit(int d) {
    char* str = CurrentStr();
    if (strcmp(str, "0") == 0) {
        str[0] = '0' + d;
        str[1] = '\0';
    } else {
        int len = (int)strlen(str);
        if (len < 30) {
            str[len] = '0' + d;
            str[len + 1] = '\0';
        }
    }
    *CurrentNumPtr() = atof(str);
    SetWindowTextA(hDisplay, str);
}

void AppendDot() {
    char* str = CurrentStr();
    if (strchr(str, '.') == NULL && strlen(str) < 30) {
        strcat(str, ".");
        SetWindowTextA(hDisplay, str);
    }
}

void ClearCurrent() {
    char* str = CurrentStr();
    strcpy(str, "0");
    *CurrentNumPtr() = 0;
    SetWindowTextA(hDisplay, str);
}

void UpdateHistory(const char* numStr, char op) {
    char buffer[80];
    if (op == 0) {
        sprintf(buffer, "%s", numStr);
    } else if (op == '/') {
        sprintf(buffer, "%s /", numStr);
    } else {
        sprintf(buffer, "%s %c", numStr, op);
    }
    SetWindowTextA(hHistoryDisplay, buffer);
}

double Add(double a, double b) {
    return a + b;
}

double Subtract(double a, double b) {
    return a - b;
}

double Multiply(double a, double b) {
    return a * b;
}

double Divide(double a, double b) {
    if (b == 0) {
        return 0;
    }
    return a / b;
}

HWND buttons[20] = {0};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            RECT rc = {0, 0, 320, 480};
            AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, 0);
            mmi->ptMinTrackSize.x = rc.right - rc.left;
            mmi->ptMinTrackSize.y = rc.bottom - rc.top;
            return 0;
        }

        case WM_ERASEBKGND:
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, hDarkBrush);
            return 1;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
            HDC hdc = lpdis->hDC;
            RECT rect = lpdis->rcItem;
            int id = lpdis->CtlID;

            if (id == ID_BUTTON_GEAR)
            {
                HBRUSH hBrush = CreateSolidBrush(COLOR_BG);
                FillRect(hdc, &rect, hBrush);
                DeleteObject(hBrush);

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, COLOR_TEXT);

                HFONT hOldFont = (HFONT)SelectObject(hdc, hGearFont);
                DrawTextW(hdc, L"\uE713", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                SelectObject(hdc, hOldFont);

                if (lpdis->itemState & ODS_SELECTED) {
                    HBRUSH hSelBrush = CreateSolidBrush(RGB(255, 255, 255));
                    FrameRect(hdc, &rect, hSelBrush);
                    DeleteObject(hSelBrush);
                }

                return TRUE;
            }

            COLORREF bgColor = COLOR_BTN_NUM;
            if (buttonTypes[id] == BTN_OP) {
                bgColor = COLOR_BTN_OP;
            } else if (buttonTypes[id] == BTN_EQ) {
                bgColor = COLOR_BTN_EQ;
            }

            HBRUSH hBrush = CreateSolidBrush(bgColor);
            FillRect(hdc, &rect, hBrush);
            DeleteObject(hBrush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, COLOR_TEXT);

            char text[10];
            GetWindowTextA(lpdis->hwndItem, text, 10);

            HFONT hOldFont = (HFONT)SelectObject(hdc, hBtnFont);
            DrawTextA(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, hOldFont);

            if (lpdis->itemState & ODS_SELECTED) {
                HBRUSH hSelBrush = CreateSolidBrush(RGB(255, 255, 255));
                FrameRect(hdc, &rect, hSelBrush);
                DeleteObject(hSelBrush);
            }

            return TRUE;
        }

        case WM_CTLCOLORSTATIC:
        {
            if ((HWND)lParam == hDisplay)
            {
                HDC hdcStatic = (HDC)wParam;
                SetTextColor(hdcStatic, COLOR_TEXT);
                SetBkColor(hdcStatic, COLOR_BG);
                return (LRESULT)hDarkBrush;
            }
            if ((HWND)lParam == hHistoryDisplay)
            {
                HDC hdcStatic = (HDC)wParam;
                SetTextColor(hdcStatic, COLOR_TEXT_GRAY);
                SetBkColor(hdcStatic, COLOR_BG);
                return (LRESULT)hDarkBrush;
            }
            break;
        }

        case WM_SIZE:
        {
            RECT rect;
            GetClientRect(hwnd, &rect);

            int clientWidth = rect.right - rect.left;
            int clientHeight = rect.bottom - rect.top;

            HDWP hdwp = BeginDeferWindowPos(23);

            if (hGearButton != NULL)
            {
                hdwp = DeferWindowPos(
                    hdwp, hGearButton, NULL,
                    SPACING,
                    SPACING,
                    GEAR_SIZE,
                    GEAR_SIZE,
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
            }

            if (hHistoryDisplay != NULL)
            {
                hdwp = DeferWindowPos(
                    hdwp, hHistoryDisplay, NULL,
                    SPACING + GEAR_SIZE + SPACING,
                    SPACING,
                    clientWidth - (SPACING * 2) - 20 - GEAR_SIZE - SPACING,
                    HISTORY_HEIGHT,
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
            }

            if (hDisplay != NULL)
            {
                hdwp = DeferWindowPos(
                    hdwp, hDisplay, NULL,
                    SPACING,
                    SPACING + HISTORY_HEIGHT,
                    clientWidth - (SPACING * 2) - 20,
                    DISPLAY_HEIGHT - HISTORY_HEIGHT - (SPACING * 2),
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
            }

            int buttonWidth =
                (clientWidth - (SPACING * (BUTTON_COLUMNS + 1)))
                / BUTTON_COLUMNS;

            int availableHeight =
                clientHeight - DISPLAY_HEIGHT;

            int buttonHeight =
                (availableHeight - (SPACING * (BUTTON_ROWS + 1)))
                / BUTTON_ROWS;

            for (int row = 0; row < BUTTON_ROWS; row++)
            {
                for (int column = 0; column < BUTTON_COLUMNS; column++)
                {
                    int index = row * BUTTON_COLUMNS + column;

                    if (buttons[index] != NULL)
                    {
                        int x = SPACING +
                                column * (buttonWidth + SPACING);

                        int y = DISPLAY_HEIGHT +
                                SPACING +
                                row * (buttonHeight + SPACING);

                        hdwp = DeferWindowPos(
                            hdwp, buttons[index], NULL,
                            x, y,
                            buttonWidth,
                            buttonHeight,
                            SWP_NOZORDER | SWP_NOACTIVATE
                        );
                    }
                }
            }

            EndDeferWindowPos(hdwp);
            return 0;
        }

        case WM_COMMAND:
        {
            int buttonID = LOWORD(wParam);
            switch (buttonID) {
                case ID_BUTTON_0:
                    AppendDigit(0);
                    break;
                case ID_BUTTON_1:
                    AppendDigit(1);
                    break;
                case ID_BUTTON_2:
                    AppendDigit(2); 
                    break;
                case ID_BUTTON_3:
                    AppendDigit(3);
                    break;
                case ID_BUTTON_4:
                    AppendDigit(4);
                    break;
                case ID_BUTTON_5:
                    AppendDigit(5);
                    break;
                case ID_BUTTON_6: 
                    AppendDigit(6); 
                    break;
                case ID_BUTTON_7: 
                    AppendDigit(7); 
                    break;
                case ID_BUTTON_8: 
                    AppendDigit(8); 
                    break;
                case ID_BUTTON_9: 
                    AppendDigit(9); 
                    break;

                case ID_BUTTON_Comma:
                    AppendDot();
                    break;

                case ID_BUTTON_Switch:
                {
                    char* str = CurrentStr();
                    double* num = CurrentNumPtr();
                    *num = -*num;
                    if (str[0] == '-') {
                        memmove(str, str + 1, strlen(str));
                    } else {
                        memmove(str + 1, str, strlen(str) + 1);
                        str[0] = '-';
                    }
                    SetWindowTextA(hDisplay, str);
                    break;
                }

                case ID_BUTTON_ClearEntry:
                    ClearCurrent();
                    break;

                case ID_BUTTON_Clear:
                    strcpy(FirstNumStr, "0");
                    strcpy(SecondNumStr, "0");
                    FirstNum = 0;
                    SecondNum = 0;
                    State = STATE_FIRSTNUM;
                    operator = 0;
                    total = 0;
                    SetWindowTextA(hDisplay, "0");
                    SetWindowTextA(hHistoryDisplay, "");
                    break;

                case ID_BUTTON_Backspace:
                {
                    char* str = CurrentStr();
                    int len = (int)strlen(str);
                    if (len > 1) {
                        str[len - 1] = '\0';
                    } else {
                        str[0] = '0';
                        str[1] = '\0';
                    }
                    if (strcmp(str, "-") == 0) {
                        str[0] = '0';
                        str[1] = '\0';
                    }
                    *CurrentNumPtr() = atof(str);
                    SetWindowTextA(hDisplay, str);
                    break;
                }

                case ID_BUTTON_Equal:
                {
                    if (operator == '+') {
                        total = Add(FirstNum, SecondNum);
                    } else if (operator == '-') {
                        total = Subtract(FirstNum, SecondNum);
                    } else if (operator == 'x') {
                        total = Multiply(FirstNum, SecondNum);
                    } else if (operator == '/') {
                        total = Divide(FirstNum, SecondNum);
                    } else {
                        total = FirstNum;
                    }

                    char buf[64];
                    FormatResult(total, buf);
                    SetWindowTextA(hDisplay, buf);
                    SetWindowTextA(hHistoryDisplay, "");

                    strcpy(FirstNumStr, "0");
                    strcpy(SecondNumStr, "0");
                    FirstNum = 0;
                    SecondNum = 0;
                    State = STATE_FIRSTNUM;
                    operator = 0;
                    break;
                }

                case ID_BUTTON_Add:
                    operator = '+';
                    State = STATE_SECONDNUM;
                    UpdateHistory(FirstNumStr, operator);
                    break;

                case ID_BUTTON_Subtract:
                    operator = '-';
                    State = STATE_SECONDNUM;
                    UpdateHistory(FirstNumStr, operator);
                    break;

                case ID_BUTTON_Times:
                    operator = 'x';
                    State = STATE_SECONDNUM;
                    UpdateHistory(FirstNumStr, operator);
                    break;

                case ID_BUTTON_Divide:
                    operator = '/';
                    State = STATE_SECONDNUM;
                    UpdateHistory(FirstNumStr, operator);
                    break;

                case ID_BUTTON_GEAR:
                    MessageBoxA(
                        hwnd,
                        "Culator - A Windows Calculator Clone in C\n\n"
                        "Credits:\n"
                        "  made by Toto\n"
                        "  Github:https://github.com/ItsMeRockyFiles\n"
                        "  Website:https://melonfarming.win/\n"
                        "Thanks to everyone who is using this!\n",
                        "Credits",
                        MB_OK | MB_ICONINFORMATION
                    );
                    break;
            }
            return 0;
        }

        case WM_DESTROY:
            if (hFont) {
                DeleteObject(hFont);
            }
            if (hBtnFont) {
                DeleteObject(hBtnFont);
            }
            if (hHistFont) {
                DeleteObject(hHistFont);
            }
            if (hGearFont) {
                DeleteObject(hGearFont);
            }
            if (hDarkBrush) {
                DeleteObject(hDarkBrush);
            }
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

HWND CreateCalcButton(
    HWND hwnd,
    HINSTANCE hInstance,
    const char* text,
    int id,
    int type
)
{
    buttonTypes[id] = type;
    return CreateWindowExA(
        0,
        "BUTTON",
        text,
        WS_TABSTOP |
        WS_VISIBLE |
        WS_CHILD |
        BS_OWNERDRAW,
        0,
        0,
        100,
        100,
        hwnd,
        (HMENU)(INT_PTR)id,
        hInstance,
        NULL
    );
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd
)
{
    FreeConsole();

    WNDCLASSA wc = {0};

    hDarkBrush = CreateSolidBrush(COLOR_BG);

    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = "CalculatorWindow";
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = hDarkBrush;
    wc.style         = CS_HREDRAW | CS_VREDRAW;

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        "CalculatorWindow",
        "Culator",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

    hFont = CreateFontA(
        60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"
    );

    hBtnFont = CreateFontA(
        22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"
    );

    hHistFont = CreateFontA(
        20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"
    );

    hGearFont = CreateFontW(
        18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe MDL2 Assets"
    );

    hGearButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"\uE713",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        0, 0, GEAR_SIZE, GEAR_SIZE,
        hwnd,
        (HMENU)(INT_PTR)ID_BUTTON_GEAR,
        hInstance,
        NULL
    );

    SendMessageW(hGearButton, WM_SETFONT, (WPARAM)hGearFont, TRUE);

    hHistoryDisplay = CreateWindowExA(
        0,
        "STATIC",
        "",
        WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE,
        0, 0, 100, 100,
        hwnd,
        (HMENU)1001,
        hInstance,
        NULL
    );

    SendMessageA(hHistoryDisplay, WM_SETFONT, (WPARAM)hHistFont, TRUE);

    hDisplay = CreateWindowExA(
        0,
        "STATIC",
        "0",
        WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE,
        0, 0, 100, 100,
        hwnd,
        (HMENU)1000,
        hInstance,
        NULL
    );

    SendMessageA(hDisplay, WM_SETFONT, (WPARAM)hFont, TRUE);

    buttons[0] = CreateCalcButton(hwnd, hInstance, "CE", ID_BUTTON_ClearEntry, BTN_OP);
    buttons[1] = CreateCalcButton(hwnd, hInstance, "C", ID_BUTTON_Clear, BTN_OP);
    buttons[2] = CreateCalcButton(hwnd, hInstance, "÷", ID_BUTTON_Divide, BTN_OP);
    buttons[3] = CreateCalcButton(hwnd, hInstance, "<x]", ID_BUTTON_Backspace, BTN_OP);

    buttons[4] = CreateCalcButton(hwnd, hInstance, "7", ID_BUTTON_7, BTN_NUM);
    buttons[5] = CreateCalcButton(hwnd, hInstance, "8", ID_BUTTON_8, BTN_NUM);
    buttons[6] = CreateCalcButton(hwnd, hInstance, "9", ID_BUTTON_9, BTN_NUM);
    buttons[7] = CreateCalcButton(hwnd, hInstance, "x", ID_BUTTON_Times, BTN_OP);

    buttons[8] = CreateCalcButton(hwnd, hInstance, "4", ID_BUTTON_4, BTN_NUM);
    buttons[9] = CreateCalcButton(hwnd, hInstance, "5", ID_BUTTON_5, BTN_NUM);
    buttons[10] = CreateCalcButton(hwnd, hInstance, "6", ID_BUTTON_6, BTN_NUM);
    buttons[11] = CreateCalcButton(hwnd, hInstance, "-", ID_BUTTON_Subtract, BTN_OP);

    buttons[12] = CreateCalcButton(hwnd, hInstance, "1", ID_BUTTON_1, BTN_NUM);
    buttons[13] = CreateCalcButton(hwnd, hInstance, "2", ID_BUTTON_2, BTN_NUM);
    buttons[14] = CreateCalcButton(hwnd, hInstance, "3", ID_BUTTON_3, BTN_NUM);
    buttons[15] = CreateCalcButton(hwnd, hInstance, "+", ID_BUTTON_Add, BTN_OP);

    buttons[16] = CreateCalcButton(hwnd, hInstance, "±", ID_BUTTON_Switch, BTN_OP);
    buttons[17] = CreateCalcButton(hwnd, hInstance, "0", ID_BUTTON_0, BTN_NUM);
    buttons[18] = CreateCalcButton(hwnd, hInstance, ",", ID_BUTTON_Comma, BTN_NUM);
    buttons[19] = CreateCalcButton(hwnd, hInstance, "=", ID_BUTTON_Equal, BTN_EQ);

    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    MSG msg;

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}