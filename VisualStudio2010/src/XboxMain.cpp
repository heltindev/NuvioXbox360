// NUVIO.XEX - 720p streaming-style UI for Xbox 360 XDK.
// Original native implementation inspired by Nuvio's TV navigation.
// Posters are decorative procedural placeholders; no video player yet.

#include <xtl.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "NuvioApp.h"
#include "NuvioCatalog.h"

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
    {'?',{14,17,1,2,4,0,4}},    {'+',{0,4,4,31,4,4,0}},
    {'*',{0,21,14,31,14,21,0}}, {'\'',{4,4,4,0,0,0,0}}
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

    void Circle(int centerX, int centerY, int radius, D3DCOLOR color)
    {
        if (radius <= 0) return;
        for (int offset = -radius; offset <= radius; offset += 2) {
            const double squared = static_cast<double>(radius * radius - offset * offset);
            const int reach = static_cast<int>(std::sqrt(squared));
            Rect(centerX - reach, centerY + offset, reach * 2 + 1, 2, color);
        }
    }

    void Outline(int x, int y, int width, int height, int thickness, D3DCOLOR color)
    {
        Rect(x, y, width, thickness, color);
        Rect(x, y + height - thickness, width, thickness, color);
        Rect(x, y, thickness, height, color);
        Rect(x + width - thickness, y, thickness, height, color);
    }

private:
    IDirect3DDevice9 *m_device;
    D3DCOLOR m_color;
    D3DRECT m_rects[MAX_RECTS];
    unsigned m_count;
};

static const D3DCOLOR kBackground = D3DCOLOR_ARGB(255, 12, 15, 19);
static const D3DCOLOR kSide = D3DCOLOR_ARGB(255, 13, 16, 21);
static const D3DCOLOR kSurface = D3DCOLOR_ARGB(255, 31, 37, 44);
static const D3DCOLOR kWhite = D3DCOLOR_ARGB(255, 246, 247, 249);
static const D3DCOLOR kMuted = D3DCOLOR_ARGB(255, 149, 152, 161);
static const D3DCOLOR kBlue = D3DCOLOR_ARGB(255, 66, 157, 246);
static const D3DCOLOR kTeal = D3DCOLOR_ARGB(255, 43, 204, 182);
static const D3DCOLOR kGold = D3DCOLOR_ARGB(255, 252, 174, 40);

static D3DCOLOR Rgb(int red, int green, int blue)
{
    return D3DCOLOR_ARGB(255, red, green, blue);
}

static void DrawBrand(UiPainter &ui, int x, int y, int scale)
{
    const int width = 16 * scale;
    for (int line = 0; line < width; line += scale) {
        const int inset = line < width / 2 ? width / 2 - line : line - width / 2;
        const D3DCOLOR tone = line < width / 2 ? Rgb(33, 174, 239) : Rgb(185, 73, 242);
        ui.Rect(x + line, y + inset, scale, width - 2 * inset, tone);
    }
    ui.Circle(x + 8 * scale, y + 8 * scale, 4 * scale, kSide);
    ui.Text(x + 22 * scale, y + 2 * scale, scale * 2, "NUVIO", kWhite);
}

static void DrawBackgroundGlow(UiPainter &ui)
{
    // Teal-to-near-black gradient evokes the reference sign-in screen.
    for (int y = 0; y < SCREEN_HEIGHT; y += 8) {
        const int red = 4 + y * 10 / SCREEN_HEIGHT;
        const int green = 69 - y * 55 / SCREEN_HEIGHT;
        const int blue = 59 - y * 42 / SCREEN_HEIGHT;
        ui.Rect(0, y, SCREEN_WIDTH, 8, Rgb(red, green, blue));
    }
    ui.Rect(40, 0, 1200, 1, Rgb(15, 87, 80));
}

static void DrawProfile(UiPainter &ui)
{
    DrawBackgroundGlow(ui);
    DrawBrand(ui, 471, 65, 3);
    ui.Text(355, 193, 6, "WHO'S WATCHING?", kWhite);
    ui.Text(439, 265, 3, "SELECT A PROFILE TO CONTINUE", kMuted);

    const int cx = 640;
    const int cy = 448;
    ui.Circle(cx, cy, 104, kWhite);
    ui.Circle(cx, cy, 99, Rgb(2, 55, 59));
    ui.Circle(cx, cy, 91, Rgb(11, 151, 150));
    // Simple native avatar: a portrait placeholder until the art pipeline exists.
    ui.Circle(cx, cy + 87, 79, Rgb(24, 49, 61));
    ui.Circle(cx, cy - 6, 37, Rgb(227, 156, 109));
    ui.Circle(cx, cy - 27, 36, Rgb(42, 36, 38));
    ui.Rect(cx - 37, cy - 18, 74, 30, Rgb(227, 156, 109));
    ui.Circle(cx - 13, cy - 6, 3, Rgb(24, 27, 31));
    ui.Circle(cx + 13, cy - 6, 3, Rgb(24, 27, 31));
    ui.Circle(cx + 77, cy + 76, 23, kGold);
    ui.Text(cx + 71, cy + 65, 3, "*", kWhite);
    ui.Text(585, 575, 5, "ADMIN", kWhite);
    ui.Text(597, 630, 2, "PRIMARY", kGold);
    ui.Text(466, 683, 2, "PRESS A TO CONTINUE", kMuted);
}

static void DrawSidebar(UiPainter &ui, const NuvioState &state)
{
    ui.Rect(0, 0, 96, SCREEN_HEIGHT, kSide);
    ui.Rect(95, 0, 1, SCREEN_HEIGHT, Rgb(32, 37, 44));
    const char *icons[5] = { "H", "D", "?", "L", "G" };
    for (int i = 0; i < 5; ++i) {
        const int y = 119 + i * 116;
        const bool active = state.sidebarItem == i;
        if (active) {
            ui.Circle(48, y + 20, 31, Rgb(24, 35, 47));
            ui.Rect(0, y - 12, 5, 65, kBlue);
        }
        if (state.sidebarFocused && active) ui.Outline(16, y - 8, 64, 56, 3, kBlue);
        ui.Text(32, y + 5, 4, icons[i], active ? kBlue : kMuted);
    }
    ui.Text(38, 684, 2, "N", kTeal);
}

static void DrawTopRight(UiPainter &ui, const NuvioCatalog &catalog)
{
    ui.Circle(1179, 42, 23, Rgb(20, 84, 92));
    ui.Text(1168, 31, 3, "A", kWhite);
    ui.Text(1209, 33, 2, "ADMIN", kWhite);
    ui.Text(1051, 78, 2, catalog.isDemo ? "DEMO CATALOG" : "IMDB DATA", kMuted);
}

static void DrawPosterArtwork(UiPainter &ui, int x, int y, int index)
{
    // Procedural original abstract poster art. Real poster licensing and image
    // decoding are separate tasks; no commercial poster assets are embedded.
    static const int colors[][6] = {
        {42, 39, 92, 128, 62, 178},
        {21, 61, 76, 63, 149, 151},
        {96, 80, 69, 174, 137, 106},
        {141, 83, 60, 247, 181, 107},
        {23, 51, 95, 90, 73, 157},
        {37, 65, 91, 55, 132, 154},
        {76, 39, 107, 173, 78, 161},
        {42, 74, 83, 102, 167, 153}
    };
    const int *c = colors[index % 8];
    for (int strip = 0; strip < 34; ++strip) {
        const int r = c[0] + (c[3] - c[0]) * strip / 34;
        const int g = c[1] + (c[4] - c[1]) * strip / 34;
        const int b = c[2] + (c[5] - c[2]) * strip / 34;
        ui.Rect(x, y + strip * 7, 197, 7, Rgb(r, g, b));
    }
    ui.Circle(x + 143, y + 70, 36, Rgb(c[3], c[4], c[5]));
    // Layered silhouette gives each card a cinematic composition.
    ui.Rect(x + 14, y + 175, 172, 56, Rgb(c[0] / 2, c[1] / 2, c[2] / 2));
    ui.Rect(x + 32, y + 147, 18, 67, Rgb(c[0] / 2, c[1] / 2, c[2] / 2));
    ui.Rect(x + 62, y + 160, 25, 62, Rgb(c[0] / 2, c[1] / 2, c[2] / 2));
    ui.Rect(x + 114, y + 137, 31, 80, Rgb(c[0] / 2, c[1] / 2, c[2] / 2));
    ui.Rect(x + 155, y + 161, 20, 57, Rgb(c[0] / 2, c[1] / 2, c[2] / 2));
    ui.Outline(x, y, 197, 238, 1, Rgb(109, 110, 120));
}

static void DrawCatalogCard(UiPainter &ui, const NuvioCatalog &catalog,
                            int catalogIndex, int x, int y, bool selected)
{
    if (catalogIndex < 0 || catalogIndex >= catalog.count) return;
    const NuvioCatalogItem &item = catalog.items[catalogIndex];
    if (selected) {
        ui.Rect(x - 4, y - 4, 205, 246, kBlue);
        ui.Rect(x - 1, y - 1, 199, 240, kBackground);
    }
    DrawPosterArtwork(ui, x, y, catalogIndex);
    // 16 glyphs at 2x fit within the 197 pixel poster width.
    const int longest = 16;
    char title[longest + 1];
    std::strncpy(title, item.title, longest);
    title[longest] = '\0';
    ui.Text(x, y + 250, 2, title, kWhite);
    char year[12];
    std::sprintf(year, "%d", item.year);
    ui.Text(x, y + 272, 2, year, kMuted);
}

static void DrawCatalogScreen(UiPainter &ui, const NuvioState &state,
                              const NuvioCatalog &catalog)
{
    const int firstRow = state.selectedRow > 0 ? state.selectedRow - 1 : 0;
    for (int visibleRow = 0; visibleRow < 2; ++visibleRow) {
        const int row = firstRow + visibleRow;
        if (row >= NuvioCatalogRowCount(&catalog)) break;
        const int y = 54 + visibleRow * 408;
        const char *heading = row == 0 ? "FEATURED MOVIES" :
                              row == 1 ? "FEATURED SERIES" : "MORE TO EXPLORE";
        ui.Text(138, y, 4, heading, kWhite);
        ui.Text(140, y + 42, 2, catalog.isDemo ? "FROM DEMO CATALOG" : "FROM IMDB DATASET", kMuted);
        for (int column = 0; column < 5; ++column) {
            const int index = NuvioCatalogIndexForRow(&catalog, row, column);
            if (index < 0) continue;
            const int x = 139 + column * 220;
            DrawCatalogCard(ui, catalog, index, x, y + 78,
                !state.sidebarFocused && state.selectedRow == row &&
                state.selectedColumn == column);
        }
    }
    DrawTopRight(ui, catalog);
    ui.Text(1042, 690, 2, "A DETAILS   B PROFILES", kMuted);
}

static void DrawInfoScreen(UiPainter &ui, const NuvioState &state,
                           const NuvioCatalog &catalog)
{
    ui.Text(145, 81, 5, NuvioScreenName(state.screen), kWhite);
    ui.Rect(142, 157, 1070, 400, kSurface);
    if (state.screen == NUVIO_SCREEN_LIBRARY) {
        ui.Circle(672, 305, 65, Rgb(47, 56, 69));
        ui.Text(650, 274, 7, "L", kBlue);
        ui.Text(454, 416, 3, "YOUR LIBRARY IS EMPTY", kWhite);
        ui.Text(429, 469, 2, "SAVED TITLES WILL APPEAR HERE", kMuted);
    } else if (state.screen == NUVIO_SCREEN_SETTINGS) {
        ui.Text(196, 220, 3, "ACTIVE PROFILE", kMuted);
        ui.Text(196, 270, 4, "ADMIN", kWhite);
        ui.Text(196, 355, 3, "CATALOG SOURCE", kMuted);
        ui.Text(196, 410, 3, catalog.isDemo ? "OFFLINE DEMO" : "LOCAL IMDB IMPORT", kWhite);
        ui.Text(196, 479, 2, "PROFILE EDITING IS PLANNED", kMuted);
    }
    ui.Text(151, 629, 2, "PRESS B TO SWITCH PROFILE", kMuted);
}

static void DrawDetails(UiPainter &ui, const NuvioState &state,
                        const NuvioCatalog &catalog)
{
    if (state.selectedCatalogIndex < 0 || state.selectedCatalogIndex >= catalog.count) return;
    const NuvioCatalogItem &item = catalog.items[state.selectedCatalogIndex];
    DrawPosterArtwork(ui, 164, 129, state.selectedCatalogIndex);
    char titleFirst[23];
    char titleSecond[23];
    std::strncpy(titleFirst, item.title, 22);
    titleFirst[22] = '\0';
    std::strncpy(titleSecond, item.title + std::strlen(titleFirst), 22);
    titleSecond[22] = '\0';
    ui.Text(428, 170, 4, titleFirst, kWhite);
    if (titleSecond[0]) ui.Text(428, 214, 3, titleSecond, kWhite);
    ui.Text(428, 238, 3, item.type == 'M' ? "MOVIE" : "SERIES", kBlue);
    char year[12];
    std::sprintf(year, "%d", item.year);
    ui.Text(428, 280, 3, year, kMuted);
    ui.Text(428, 363, 2, catalog.isDemo ? "DEMONSTRATION ITEM" : "IMDB TITLE METADATA", kMuted);
    if (!catalog.isDemo) ui.Text(428, 409, 2, item.imdbId, kMuted);
    ui.Rect(428, 492, 422, 74, kSurface);
    ui.Text(448, 514, 3, "PLAYBACK COMING LATER", kMuted);
    ui.Text(174, 617, 2, "B BACK", kBlue);
}

static void DrawDashboard(IDirect3DDevice9 *device, const NuvioState &state,
                          const NuvioCatalog &catalog)
{
    device->Clear(0, NULL, D3DCLEAR_TARGET, kBackground, 1.0f, 0);
    UiPainter ui(device);
    if (state.screen == NUVIO_SCREEN_PROFILE) {
        DrawProfile(ui);
    } else {
        DrawSidebar(ui, state);
        if (state.screen == NUVIO_SCREEN_HOME || state.screen == NUVIO_SCREEN_DISCOVER ||
            state.screen == NUVIO_SCREEN_SEARCH) {
            DrawCatalogScreen(ui, state, catalog);
        } else if (state.screen == NUVIO_SCREEN_DETAILS) {
            DrawDetails(ui, state, catalog);
        } else {
            DrawInfoScreen(ui, state, catalog);
        }
    }
    ui.Flush();
}

int main()
{
    NuvioState app;
    NuvioInitialize(&app);
    // The catalog is optional: a demo is used until the user imports IMDb TSV.
    NuvioCatalog catalog;
    NuvioCatalogLoadDemo(&catalog);
    const char *paths[] = { "game:\\catalog.tsv", "D:\\catalog.tsv", "catalog.tsv" };
    for (int pathIndex = 0; pathIndex < 3; ++pathIndex) {
        if (NuvioCatalogLoadFile(&catalog, paths[pathIndex])) break;
    }
    if (!catalog.count) NuvioCatalogLoadDemo(&catalog);
    OutputDebugStringA("NUVIO.XEX - streaming UI started\n");

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
        if (pressed & XINPUT_GAMEPAD_DPAD_RIGHT) NuvioMoveHorizontal(&app, 1);
        if (pressed & XINPUT_GAMEPAD_DPAD_LEFT) NuvioMoveHorizontal(&app, -1);
        if (pressed & XINPUT_GAMEPAD_DPAD_DOWN)
            NuvioMoveVertical(&app, 1, NuvioCatalogRowCount(&catalog));
        if (pressed & XINPUT_GAMEPAD_DPAD_UP)
            NuvioMoveVertical(&app, -1, NuvioCatalogRowCount(&catalog));
        if (pressed & XINPUT_GAMEPAD_A)
            NuvioSelect(&app, NuvioCatalogIndexForRow(&catalog,
                app.selectedRow, app.selectedColumn));
        if (pressed & XINPUT_GAMEPAD_B) NuvioBack(&app);

        DrawDashboard(device, app, catalog);
        device->Present(NULL, NULL, NULL, NULL);
        Sleep(16);
    }

    device->Release();
    graphics->Release();
    return 0;
}
