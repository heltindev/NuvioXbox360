// Xbox 360 XDK smoke test. Requires a separately installed, licensed Xbox 360 XDK.
// This is a color-changing screen and controller test, NOT a media player.

#include <xtl.h>
#include "NuvioApp.h"

static D3DCOLOR NuvioBackground(const NuvioState &state)
{
    switch (state.screen) {
        case NUVIO_SCREEN_MOVIES: return D3DCOLOR_ARGB(255, 11, 58, 75);
        case NUVIO_SCREEN_SERIES: return D3DCOLOR_ARGB(255, 37, 31, 78);
        case NUVIO_SCREEN_LIBRARY: return D3DCOLOR_ARGB(255, 26, 70, 57);
        case NUVIO_SCREEN_SETTINGS: return D3DCOLOR_ARGB(255, 53, 54, 79);
        default: return state.selectedTile % 2 == 0 ?
            D3DCOLOR_ARGB(255, 11, 19, 37) : D3DCOLOR_ARGB(255, 13, 35, 51);
    }
}

int main()
{
    NuvioState app;
    NuvioInitialize(&app);
    OutputDebugStringA("NUVIO.XEX - Xbox 360 XDK starter\n");

    IDirect3D9 *graphics = Direct3DCreate9(D3D_SDK_VERSION);
    if (!graphics) return 1;

    D3DPRESENT_PARAMETERS present;
    ZeroMemory(&present, sizeof(present));
    present.BackBufferWidth = 1280;
    present.BackBufferHeight = 720;
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
        OutputDebugStringA("NUVIO.XEX - Failed to initialize video\n");
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

        device->Clear(0, NULL, D3DCLEAR_TARGET, NuvioBackground(app), 1.0f, 0);
        device->Present(NULL, NULL, NULL, NULL);
        Sleep(16);
    }

    device->Release();
    graphics->Release();
    return 0;
}
