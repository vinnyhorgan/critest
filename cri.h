/* cri v0.1 | public domain - no warranty implied; use at your own risk */

#ifndef CRI_H
#define CRI_H

#ifdef __cplusplus
extern "C" {
#endif

#define CRI_VERSION    "0.1.0"

#define CRI_TRUE     1
#define CRI_FALSE    0

#define CRI_RELEASE    0
#define CRI_PRESS      1
#define CRI_REPEAT     2

#define CRI_MOUSE_BUTTON_1         0
#define CRI_MOUSE_BUTTON_2         1
#define CRI_MOUSE_BUTTON_3         2
#define CRI_MOUSE_BUTTON_4         3
#define CRI_MOUSE_BUTTON_5         4
#define CRI_MOUSE_BUTTON_6         5
#define CRI_MOUSE_BUTTON_7         6
#define CRI_MOUSE_BUTTON_8         7
#define CRI_MOUSE_BUTTON_LAST      CRI_MOUSE_BUTTON_8
#define CRI_MOUSE_BUTTON_LEFT      CRI_MOUSE_BUTTON_1
#define CRI_MOUSE_BUTTON_RIGHT     CRI_MOUSE_BUTTON_2
#define CRI_MOUSE_BUTTON_MIDDLE    CRI_MOUSE_BUTTON_3

#define CRI_MOD_SHIFT      0x0001
#define CRI_MOD_CONTROL    0x0002
#define CRI_MOD_ALT        0x0004
#define CRI_MOD_SUPER      0x0008

#define CRI_NOT_INITIALIZED    0x00010001
#define CRI_INVALID_VALUE      0x00010002
#define CRI_OUT_OF_MEMORY      0x00010003
#define CRI_PLATFORM_ERROR     0x00010004

#define CRI_DONT_CARE    -1

typedef struct cri_Window cri_Window;

typedef void (*cri_ErrorCallback)(int error, const char *description);
typedef void (*cri_WindowSizeCallback)(cri_Window *window, int width, int height);
typedef void (*cri_MouseButtonCallback)(cri_Window *window, int button, int action, int mods);

int cri_init(void);
void cri_terminate(void);
void cri_set_error_callback(cri_ErrorCallback callback);
cri_Window* cri_create_window(int width, int height, const char *title);
void cri_destroy_window(cri_Window *window);
int cri_window_should_close(cri_Window *window);
void cri_set_window_size_callback(cri_Window *window, cri_WindowSizeCallback callback);
void cri_set_mouse_button_callback(cri_Window *window, cri_MouseButtonCallback callback);
void cri_poll_events(void);
void cri_get_cursor_pos(cri_Window *window, double *x, double *y);

int cri_update_buffer(cri_Window *window, void *buffer, int width, int height);
int cri_set_viewport(cri_Window *window, int ox, int oy, int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* CRI_H */

/*
    ██ ███    ███ ██████  ██      ███████ ███    ███ ███████ ███    ██ ████████  █████  ████████ ██  ██████  ███    ██
    ██ ████  ████ ██   ██ ██      ██      ████  ████ ██      ████   ██    ██    ██   ██    ██    ██ ██    ██ ████   ██
    ██ ██ ████ ██ ██████  ██      █████   ██ ████ ██ █████   ██ ██  ██    ██    ███████    ██    ██ ██    ██ ██ ██  ██
    ██ ██  ██  ██ ██      ██      ██      ██  ██  ██ ██      ██  ██ ██    ██    ██   ██    ██    ██ ██    ██ ██  ██ ██
    ██ ██      ██ ██      ███████ ███████ ██      ██ ███████ ██   ████    ██    ██   ██    ██    ██  ██████  ██   ████
*/

#ifdef CRI_IMPL

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define REQUIRE_INIT()                         \
    if (!cri.initialized) {                    \
        cri__error(CRI_NOT_INITIALIZED, NULL); \
        return;                                \
    }

#define REQUIRE_INIT_OR_RETURN(x)              \
    if (!cri.initialized) {                    \
        cri__error(CRI_NOT_INITIALIZED, NULL); \
        return x;                              \
    }

static struct {
    int initialized;
    int resizable_hint;
    int visible_hint;
    int focused_hint;

#ifdef _WIN32
    LARGE_INTEGER timer_frequency;
    LARGE_INTEGER timer_offset;
#endif
} cri;

static cri_ErrorCallback cri__error_callback;

struct cri_Window {
    int should_close;
    int resizable;
    int min_width, min_height, max_width, max_height;

    int width, height;

    void *buffer;
    int buffer_width, buffer_height;
    int viewport_ox, viewport_oy, viewport_width, viewport_height;

    char mouse_buttons[CRI_MOUSE_BUTTON_LAST + 1];

    cri_WindowSizeCallback window_size_callback;
    cri_MouseButtonCallback mouse_button_callback;

#ifdef _WIN32
    HWND hwnd;
    HDC hdc;
    BITMAPINFO *bmi;
#endif
};

static void cri__error(int error, const char *format, ...) {
    if (cri__error_callback) {
        char buffer[8192];
        const char *description;

        if (format) {
            int count;
            va_list vl;

            va_start(vl, format);
            count = vsnprintf(buffer, sizeof(buffer), format, vl);
            va_end(vl);

            if (count < 0)
                buffer[sizeof(buffer) - 1] = '\0';

            description = buffer;
        } else {
            switch (error) {
                case CRI_NOT_INITIALIZED:
                    description = "cri is not initialized";
                    break;
                case CRI_INVALID_VALUE:
                    description = "invalid parameter value";
                    break;
                case CRI_OUT_OF_MEMORY:
                    description = "out of memory";
                    break;
                case CRI_PLATFORM_ERROR:
                    description = "platform error";
                    break;
                default:
                    description = "unknown error";
                    break;
            }
        }

        cri__error_callback(error, description);
    }
}

static void cri__input_mouse_button(cri_Window *window, int button, int action, int mods) {
    if (button < 0 || button > CRI_MOUSE_BUTTON_LAST)
        return;

    window->mouse_buttons[button] = (char) action;

    if (window->mouse_button_callback)
        window->mouse_button_callback(window, button, action, mods);
}

/*
    ██     ██ ██ ███    ██ ██████   ██████  ██     ██ ███████
    ██     ██ ██ ████   ██ ██   ██ ██    ██ ██     ██ ██
    ██  █  ██ ██ ██ ██  ██ ██   ██ ██    ██ ██  █  ██ ███████
    ██ ███ ██ ██ ██  ██ ██ ██   ██ ██    ██ ██ ███ ██      ██
     ███ ███  ██ ██   ████ ██████   ██████   ███ ███  ███████
*/

#ifdef _WIN32

static WCHAR* cri__wide_string_from_utf8(const char *source) {
    WCHAR *target;
    int count;

    count = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
    if (!count) {
        cri__error(CRI_PLATFORM_ERROR, "failed to convert string from UTF-8");
        return NULL;
    }

    target = calloc(count, sizeof(WCHAR));

    if (!MultiByteToWideChar(CP_UTF8, 0, source, -1, target, count)) {
        cri__error(CRI_PLATFORM_ERROR, "failed to convert string from UTF-8");
        free(target);
        return NULL;
    }

    return target;
}

static DWORD cri__get_window_style(cri_Window *window) {
    DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    style |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    if (window->resizable)
        style |= WS_MAXIMIZEBOX | WS_THICKFRAME;

    return style;
}

static int cri__get_key_mods(void) {
    int mods = 0;

    if (GetKeyState(VK_SHIFT) & (1 << 31))
        mods |= CRI_MOD_SHIFT;
    if (GetKeyState(VK_CONTROL) & (1 << 31))
        mods |= CRI_MOD_CONTROL;
    if (GetKeyState(VK_MENU) & (1 << 31))
        mods |= CRI_MOD_ALT;
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & (1 << 31))
        mods |= CRI_MOD_SUPER;

    return mods;
}

static LRESULT CALLBACK cri__window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    cri_Window *window = GetPropW(hwnd, L"CRI");
    if (!window)
        return DefWindowProcW(hwnd, msg, wparam, lparam);

    switch (msg) {
        case WM_PAINT: {
            if (window->buffer) {
                StretchDIBits(window->hdc, window->viewport_ox, window->viewport_oy, window->viewport_width, window->viewport_height,
                            0, 0, window->buffer_width, window->buffer_height,
                            window->buffer, window->bmi, DIB_RGB_COLORS, SRCCOPY);
                ValidateRect(hwnd, NULL);
            }
            break;
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP: {
            int button, action;

            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP)
                button = CRI_MOUSE_BUTTON_LEFT;
            else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP)
                button = CRI_MOUSE_BUTTON_RIGHT;
            else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP)
                button = CRI_MOUSE_BUTTON_MIDDLE;
            else if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
                button = CRI_MOUSE_BUTTON_4;
            else
                button = CRI_MOUSE_BUTTON_5;

            if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_XBUTTONDOWN) {
                action = CRI_PRESS;
                SetCapture(hwnd);
            } else {
                action = CRI_RELEASE;
                ReleaseCapture();
            }

            cri__input_mouse_button(window, button, action, cri__get_key_mods());

            if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONUP)
                return TRUE;

            return 0;
        }

        case WM_SIZE: {
            window->width = LOWORD(lparam);
            window->height = HIWORD(lparam);
            window->viewport_ox = 0;
            window->viewport_oy = 0;
            window->viewport_width = window->width;
            window->viewport_height = window->height;
            BitBlt(window->hdc, 0, 0, window->width, window->height, 0, 0, 0, BLACKNESS);

            if (window->window_size_callback)
                window->window_size_callback(window, LOWORD(lparam), HIWORD(lparam));

            return 0;
        }

        case WM_CLOSE:
            window->should_close = CRI_TRUE;
            break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static int cri__platform_init(void) {
    WNDCLASSEXW wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = cri__window_proc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR) IDC_ARROW);
    wc.lpszClassName = L"CRI";

    wc.hIcon = LoadImageW(GetModuleHandleW(NULL), L"CRI_ICON", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    if (!wc.hIcon)
        wc.hIcon = LoadImageW(NULL, (LPCWSTR) IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

    if (!RegisterClassExW(&wc)) {
        cri__error(CRI_PLATFORM_ERROR, "failed to register window class");
        return CRI_FALSE;
    }

    QueryPerformanceFrequency(&cri.timer_frequency);
    QueryPerformanceCounter(&cri.timer_offset);

    return CRI_TRUE;
}

static void cri__platform_terminate(void) {
    UnregisterClassW(L"CRI", GetModuleHandleW(NULL));
}

static int cri__platform_create_window(cri_Window *window, int width, int height, const char *title) {
    int x, y, full_width, full_height;
    WCHAR *wide_title;
    DWORD style = cri__get_window_style(window);
    RECT rect = { 0 };

    rect.right = width;
    rect.bottom = height;
    AdjustWindowRect(&rect, style, FALSE);
    full_width = rect.right - rect.left;
    full_height = rect.bottom - rect.top;

    x = (GetSystemMetrics(SM_CXSCREEN) - full_width) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - full_height) / 2;

    wide_title = cri__wide_string_from_utf8(title);
    if (!wide_title)
        return CRI_FALSE;

    window->hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"CRI", wide_title, style, x, y, full_width, full_height, NULL, NULL, GetModuleHandleW(NULL), NULL);
    free(wide_title);
    if (!window->hwnd) {
        cri__error(CRI_PLATFORM_ERROR, "failed to create window");
        return CRI_FALSE;
    }

    SetPropW(window->hwnd, L"CRI", window);

    window->hdc = GetDC(window->hwnd);

    window->bmi = calloc(1, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 3);
    if (!window->bmi) {
        cri__error(CRI_PLATFORM_ERROR, "failed to allocate bitmap");
        return CRI_FALSE;
    }

    window->bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    window->bmi->bmiHeader.biPlanes = 1;
    window->bmi->bmiHeader.biBitCount = 32;
    window->bmi->bmiHeader.biCompression = BI_BITFIELDS;
    window->bmi->bmiHeader.biWidth = window->buffer_width;
    window->bmi->bmiHeader.biHeight = -(LONG) window->buffer_height;
    window->bmi->bmiColors[0].rgbRed = 0xff;
    window->bmi->bmiColors[1].rgbGreen = 0xff;
    window->bmi->bmiColors[2].rgbBlue = 0xff;

    return CRI_TRUE;
}

static void cri__platform_destroy_window(cri_Window *window) {
    if (window->bmi) {
        free(window->bmi);
        window->bmi = NULL;
    }

    if (window->hdc) {
        ReleaseDC(window->hwnd, window->hdc);
        window->hdc = NULL;
    }

    if (window->hwnd) {
        RemovePropW(window->hwnd, L"CRI");
        DestroyWindow(window->hwnd);
        window->hwnd = NULL;
    }
}

static void cri__platform_show_window(cri_Window *window) {
    ShowWindow(window->hwnd, SW_SHOW);
}

static void cri__platform_focus_window(cri_Window *window) {
    BringWindowToTop(window->hwnd);
    SetForegroundWindow(window->hwnd);
    SetFocus(window->hwnd);
}

static void cri__platform_poll_events(void) {
    MSG msg;

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static void cri__platform_get_cursor_pos(cri_Window *window, double *x, double *y) {
    POINT pos;

    if (GetCursorPos(&pos)) {
        ScreenToClient(window->hwnd, &pos);

        if (x) *x = pos.x;
        if (y) *y = pos.y;
    }
}

static void cri__platform_update_buffer(cri_Window *window) {
    window->bmi->bmiHeader.biWidth = window->buffer_width;
    window->bmi->bmiHeader.biHeight = -(LONG) window->buffer_height;
    InvalidateRect(window->hwnd, NULL, TRUE);
    SendMessage(window->hwnd, WM_PAINT, 0, 0);
}

#endif

/*
    ██      ██ ███    ██ ██    ██ ██   ██
    ██      ██ ████   ██ ██    ██  ██ ██
    ██      ██ ██ ██  ██ ██    ██   ███
    ██      ██ ██  ██ ██ ██    ██  ██ ██
    ███████ ██ ██   ████  ██████  ██   ██
*/

#ifdef __linux__

// LINUX HERE

#endif

/*
    ██████  ██    ██ ██████  ██      ██  ██████
    ██   ██ ██    ██ ██   ██ ██      ██ ██
    ██████  ██    ██ ██████  ██      ██ ██
    ██      ██    ██ ██   ██ ██      ██ ██
    ██       ██████  ██████  ███████ ██  ██████
*/

int cri_init(void) {
    if (cri.initialized)
        return CRI_TRUE;

    memset(&cri, 0, sizeof(cri));

    if (!cri__platform_init()) {
        cri__platform_terminate();
        return CRI_FALSE;
    }

    cri.resizable_hint = CRI_TRUE;
    cri.visible_hint = CRI_TRUE;
    cri.focused_hint = CRI_TRUE;

    cri.initialized = CRI_TRUE;

    return CRI_TRUE;
}

void cri_terminate(void) {
    if (!cri.initialized)
        return;

    cri__platform_terminate();
    memset(&cri, 0, sizeof(cri));
}

void cri_set_error_callback(cri_ErrorCallback callback) {
    cri__error_callback = callback;
}

cri_Window* cri_create_window(int width, int height, const char *title) {
    cri_Window *window;

    assert(title != NULL);
    assert(width >= 0);
    assert(height >= 0);

    REQUIRE_INIT_OR_RETURN(NULL);

    if (width <= 0 || height <= 0) {
        cri__error(CRI_INVALID_VALUE, "invalid window size");
        return NULL;
    }

    window = calloc(1, sizeof(cri_Window));

    window->resizable = cri.resizable_hint;
    window->min_width = CRI_DONT_CARE;
    window->min_height = CRI_DONT_CARE;
    window->max_width = CRI_DONT_CARE;
    window->max_height = CRI_DONT_CARE;

    if (!cri__platform_create_window(window, width, height, title)) {
        cri_destroy_window(window);
        return NULL;
    }

    if (cri.visible_hint) {
        cri__platform_show_window(window);
        if (cri.focused_hint)
            cri__platform_focus_window(window);
    }

    return window;
}

void cri_destroy_window(cri_Window *window) {
    REQUIRE_INIT();

    if (window == NULL)
        return;

    cri__platform_destroy_window(window);
    free(window);
}

int cri_window_should_close(cri_Window *window) {
    assert(window != NULL);
    REQUIRE_INIT_OR_RETURN(0);
    return window->should_close;
}

void cri_set_window_size_callback(cri_Window *window, cri_WindowSizeCallback callback) {
    assert(window != NULL);
    REQUIRE_INIT();
    window->window_size_callback = callback;
}

void cri_set_mouse_button_callback(cri_Window *window, cri_MouseButtonCallback callback) {
    assert(window != NULL);
    REQUIRE_INIT();
    window->mouse_button_callback = callback;
}

void cri_poll_events(void) {
    REQUIRE_INIT();
    cri__platform_poll_events();
}

void cri_get_cursor_pos(cri_Window *window, double *x, double *y) {
    assert(window != NULL);

    if (x) *x = 0;
    if (y) *y = 0;

    REQUIRE_INIT();

    cri__platform_get_cursor_pos(window, x, y);
}

int cri_update_buffer(cri_Window *window, void *buffer, int width, int height) {
    assert(window != NULL);
    assert(buffer != NULL);
    assert(width >= 0);
    assert(height >= 0);

    REQUIRE_INIT_OR_RETURN(0);

    if (width <= 0 || height <= 0) {
        cri__error(CRI_INVALID_VALUE, "invalid buffer size");
        return CRI_FALSE;
    }

    window->buffer = buffer;
    window->buffer_width = width;
    window->buffer_height = height;

    cri__platform_update_buffer(window);

    return CRI_TRUE;
}

int cri_set_viewport(cri_Window *window, int ox, int oy, int width, int height) {
    assert(window != NULL);
    assert(ox >= 0);
    assert(oy >= 0);
    assert(width >= 0);
    assert(height >= 0);

    REQUIRE_INIT_OR_RETURN(0);

    if (ox + width > window->width || oy + height > window->height) {
        cri__error(CRI_INVALID_VALUE, "invalid viewport size");
        return CRI_FALSE;
    }

    window->viewport_ox = ox;
    window->viewport_oy = oy;
    window->viewport_width = width;
    window->viewport_height = height;

    return CRI_TRUE;
}

#endif /* CRI_IMPL */
