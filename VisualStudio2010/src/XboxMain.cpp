// NUVIO.XEX - Xbox 360 XDK dashboard preview.
// Draws a visible 720p interface without external fonts, shaders, or assets.
// This is a UI/controller test: video playback is not implemented.

#include <xtl.h>
#include "NuvioApp.h"

enum {
    SCREEN_WIDTH = 1280,
    SCREEN_HEIGHT = 720,
    MAX_RECTS = 256
};

struct FontGlyph {
    char letter;
    unsigned char rows[7];
};

// Compact 5x7 glyphs. Each bit draws a tiny rectangle through Direct3D Clear.
static const FontGlyph kFont[] = {
    {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
    {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
    {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
    {'G',{14,17,16,23,17,17,14}}, {'H',{17,17,17,31,17,17,17}},
    {'I',{31,4,4,4,4,4,31}},     {'J',{7,2,2,2,18,18,12}},
    {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
    {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
    {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
    {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
    {'S',{15,16,16,14,1,1,30}},  {'T',{31,4,4,4,4,4,4}},
    {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
    {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
    {'Y',{17,17,10,4,4,4,4}},    {'Z',{31,1,2,4,8,16,31}},
    {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
    {'2',{14,17,1,2,4,8,31}},   {'3',{30,1,1,14,1,1,30}},
    {'4',{2,6,10,18,31,2,2}},   {'5',{31,16,16,30,1,1,30}},
    {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
    {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
    {'.',{0,0,0,0,0,12,12}},    {':',{0,4,4,0,4,4,0}},
    {'-',{0,0,0,31,0,0,0}},     {'/',{1,1,2,4,8,16,16}},
    {'|',{4,4,4,4,4,4,4}},       {'!',{4,4,4,4,4,0,4}},
    {'>',{16,8,4,2,4,8,16}},    {'<',{1,2,4,8,4,2,1}},
    {'?',{14,17,1,2,4,0,4}},    {'+',{0,4,4,31,4,4,0}}
};

static const FontGlyph *FindGlyph(char letter)
{
    if (letter >= 'a' && letter <= 'z') letter = static_cast<char>(letter - 32);
    for (unsigned i = 0; i < sizeof(kFont) / sizeof(kFont[0]); ++i)
        if (kFont[i].letter == letter) return &kFont[i];
    return NULL;
}

class UiPainter {
public:
    explicit UiPainter(IDirect3DDevice9 *device)
        : m_device(device), m_color(0), m_count(0) {}

    void Flush()
    {
        if (!m_count) return;
        m_device->Clear(m_count, m_rects, D3DCLEAR_TARGET, m_color, 1.0f, 0);
        m_count = 0;
    }

    void Rect(int x, int y, int width, int height, D3DCOLOR color)
    {
        if (width <= 0 || height <= 0) return;
        if (color != m_color) {
            Flush();
            m_color = color;
        }
        if (m_count == MAX_RECTS) Flush();
        D3DRECT &rect = m_rects[m_count++];
        rect.x1 = x;
        rect.y1 = y;
        rect.x2 = x + width;
        rect.y2 = y + height;
    }

    void Text(int x, int y, int scale, const char *message, D3DCOLOR color)
    {
        if (!message || scale < 1) return;
        const int originX = x;
        for (; *message; ++message) {
            if (*message == '\n') {
                x = originX;
                y += scale * 9;
                continue;
            }
            const FontGlyph *glyph = FindGlyph(*message);
            if (glyph) {
                for (int row = 0; row < 7; ++row) {
                    for (int column = 0; column < 5; ++column) {
                        if (glyph->rows[row] & (1 << (4 - column)))
                            Rect(x + column * scale, y + row * scale,
                                 scale, scale, color);
                    }
                }
            }
            x += 6 * scale;
        }
    }

private:
    IDirect3DDevice9 *m_device;
    D3DCOLOR m_color;
    D3DRECT m_rects[MAX_RECTS];
    unsigned m_count;
};

static const D3DCOLOR kBackground = D3DCOLOR_ARGB(255, 8, 14, 28);
static const D3DCOLOR kPanel = D3DCOLOR_ARGB(255, 17, 30, 49);
static const D3DCOLOR kPanelLight = D3DCOLOR_ARGB(255, 25, 43, 66);
static const D3DCOLOR kWhite = D3DCOLOR_ARGB(255, 236, 247, 255);
static const D3DCOLOR kMuted = D3DCOLOR_ARGB(255, 139, 167, 190);
static const D3DCOLOR kMint = D3DCOLOR_ARGB(255, 100, 233, 196);
static const D3DCOLOR kGold = D3DCOLOR_ARGB(255, 249, 211, 135);

static void DrawHeader(UiPainter &ui)
{
    ui.Rect(0, 0, SCREEN_WIDTH, 96, kPanel);
    ui.Rect(0, 94, SCREEN_WIDTH, 2, D3DCOLOR_ARGB(255, 41, 74, 87));
    ui.Rect(50, 22, 58, 55, D3DCOLOR_ARGB(255, 30, 88, 91));
    ui.Text(62, 32, 5, "N", kMint);
    ui.Text(130, 27, 6, "NUVIO", kWhite);
    ui.Text(318, 46, 3, ".XEX", kMint);
    ui.Text(867, 40, 2, "XBOX 360 | RGH PREVIEW", kMuted);
}

static void DrawFooter(UiPainter &ui)
{
    ui.Rect(0, 663, SCREEN_WIDTH, 57, kPanel);
    ui.Rect(0, 663, SCREEN_WIDTH, 2, D3DCOLOR_ARGB(255, 34, 65, 77));
    ui.Text(52, 681, 2, "DPAD NAVIGATE   A SELECT   B BACK   BACK EXIT", kMuted);
}

static void DrawCard(UiPainter &ui, int index, bool selected)
{
    static const char *kTitles[] = { "MOVIES", "SERIES", "MY LIBRARY", "SETTINGS" };
    static const char *kIcons[] = { "M", "S", "L", "G" };
    static const D3DCOLOR kColors[] = {
        D3DCOLOR_ARGB(255, 21, 87, 96),
        D3DCOLOR_ARGB(255, 75, 50, 107),
        D3DCOLOR_ARGB(255, 42, 87, 74),
        D3DCOLOR_ARGB(255, 56, 72, 110)
    };
    const int x = 52 + index * 300;
    const int y = 282;
    if (selected) {
        ui.Rect(x - 4, y - 4, 284, 263, kMint);
        ui.Rect(x - 1, y - 1, 278, 257, kPanelLight);
    } else {
        ui.Rect(x, y, 276, 255, kPanel);
    }

    ui.Rect(x + 20, y + 18, 236, 110, kColors[index]);
    ui.Rect(x + 35, y + 34, 76, 78, D3DCOLOR_ARGB(255, 10, 24, 39));
    ui.Text(x + 49, y + 47, 7, kIcons[index], kWhite);
    ui.Text(x + 20, y + 150, 3, kTitles[index], selected ? kMint : kWhite);
    ui.Text(x + 20, y + 202, 2, selected ? "A TO OPEN" : "COMING SOON", kMuted);
}

static void DrawHome(UiPainter &ui, const NuvioState &state)
{
    ui.Text(54, 125, 2, "HOME  /  DISCOVER  /  LIBRARY", kMint);
    ui.Text(54, 169, 4, "WELCOME TO NUVIO", kWhite);
    ui.Text(54, 222, 2, "YOUR MEDIA HUB ON XBOX 360", kMuted);
    for (int index = 0; index < 4; ++index)
        DrawCard(ui, index, index == state.selectedTile);

    ui.Rect(52, 566, 1176, 75, kPanel);
    ui.Rect(52, 566, 6, 75, kGold);
    ui.Text(78, 580, 2, "DEVELOPMENT PREVIEW", kGold);
    ui.Text(78, 609, 2, "UI AND CONTROLLER TEST - VIDEO PLAYBACK COMING LATER", kMuted);
}

static void DrawSection(UiPainter &ui, const NuvioState &state)
{
    ui.Text(54, 128, 2, "HOME  /  SECTION", kMint);
    ui.Text(54, 175, 5, NuvioScreenName(state.screen), kWhite);
    ui.Rect(52, 267, 1176, 340, kPanel);
    ui.Rect(52, 267, 1176, 8, kMint);
    ui.Rect(90, 309, 150, 150, kPanelLight);
    ui.Text(130, 348, 10, "N", kMint);
    ui.Text(282, 331, 3, "THIS SECTION IS COMING SOON", kWhite);
    ui.Text(282, 400, 2, "THE NUVIO XBOX 360 INTERFACE IS IN DEVELOPMENT", kMuted);
    ui.Text(282, 441, 2, "NO VIDEO PLAYBACK IS AVAILABLE YET", kGold);
    ui.Text(90, 544, 2, "PRESS B TO RETURN TO HOME", kMint);
}

static void DrawDashboard(IDirect3DDevice9 *device, const NuvioState &state)
{
    device->Clear(0, NULL, D3DCLEAR_TARGET, kBackground, 1.0f, 0);
    UiPainter ui(device);
    DrawHeader(ui);
    if (state.screen == NUVIO_SCREEN_HOME) DrawHome(ui, state);
    else DrawSection(ui, state);
    DrawFooter(ui);
    ui.Flush();
}

int main()
{
    NuvioState app;
    NuvioInitialize(&app);
    OutputDebugStringA("NUVIO.XEX - dashboard preview started\n");

    IDirect3D9 *graphics = Direct3DCreate9(D3D_SDK_VERSION);
    if (!graphics) return 1;

    D3DPRESENT_PARAMETERS present;
    ZeroMemory(&present, sizeof(present));
    present.BackBufferWidth = SCREEN_WIDTH;
    present.BackBufferHeight = SCREEN_HEIGHT;
    present.BackBufferFormat = D3DFMT_A8R8G8B8;
    present.BackBufferCount = 1;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.Windowed = FALSE;
    present.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    IDirect3DDevice9 *device = NULL;
    const HRESULT created = graphics->CreateDevice(
        D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &present, &device);
    if (FAILED(created) || !device) {
        graphics->Release();
        OutputDebugStringA("NUVIO.XEX - Failed to initialize graphics\n");
        return 2;
    }

    WORD previousButtons = 0;
    bool running = true;
    while (running) {
        XINPUT_STATE input;
        ZeroMemory(&input, sizeof(input));
        const WORD buttons = XInputGetState(0, &input) == ERROR_SUCCESS ?
            input.Gamepad.wButtons : 0;
        const WORD pressed = buttons & static_cast<WORD>(~previousButtons);
        previousButtons = buttons;

        if (pressed & XINPUT_GAMEPAD_BACK) running = false;
        if (pressed & (XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN))
            NuvioMoveSelection(&app, 1);
        if (pressed & (XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP))
            NuvioMoveSelection(&app, -1);
        if (pressed & XINPUT_GAMEPAD_A) NuvioOpenSelection(&app);
        if (pressed & XINPUT_GAMEPAD_B) NuvioGoHome(&app);

        DrawDashboard(device, app);
        device->Present(NULL, NULL, NULL, NULL);
        Sleep(16);
    }

    device->Release();
    graphics->Release();
    return 0;
}
