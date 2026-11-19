#include "../src/NuvioApp.h"

#include <cassert>
#include <cstring>
#include <cstdio>

int main()
{
    NuvioState state;
    NuvioInitialize(&state);
    assert(state.screen == NUVIO_SCREEN_HOME);
    assert(state.selectedTile == 0);

    NuvioMoveSelection(&state, -1);
    assert(state.selectedTile == 3);
    NuvioOpenSelection(&state);
    assert(state.screen == NUVIO_SCREEN_SETTINGS);
    assert(std::strcmp(NuvioScreenName(state.screen), "Settings") == 0);

    NuvioMoveSelection(&state, 1);
    assert(state.selectedTile == 3); // Only the home screen has a tile list.
    NuvioGoHome(&state);
    NuvioMoveSelection(&state, 1);
    assert(state.selectedTile == 0);
    NuvioOpenSelection(&state);
    assert(state.screen == NUVIO_SCREEN_MOVIES);

    NuvioInitialize(NULL);
    NuvioMoveSelection(NULL, 1);
    NuvioGoHome(NULL);
    assert(std::strcmp(NuvioScreenName(static_cast<NuvioScreen>(99)), "Unknown screen") == 0);

    std::puts("OK: NUVIO core navigation and screen state");
    return 0;
}
