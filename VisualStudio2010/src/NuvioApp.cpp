#include "NuvioApp.h"

void NuvioInitialize(NuvioState *state)
{
    if (!state) return;
    state->screen = NUVIO_SCREEN_HOME;
    state->selectedTile = 0;
}

void NuvioMoveSelection(NuvioState *state, int direction)
{
    if (!state || state->screen != NUVIO_SCREEN_HOME || direction == 0) return;
    const int tileCount = 4;
    const int step = direction > 0 ? 1 : -1;
    state->selectedTile = (state->selectedTile + tileCount + step) % tileCount;
}

void NuvioOpenSelection(NuvioState *state)
{
    if (!state || state->screen != NUVIO_SCREEN_HOME) return;
    state->screen = static_cast<NuvioScreen>(NUVIO_SCREEN_MOVIES + state->selectedTile);
}

void NuvioGoHome(NuvioState *state)
{
    if (!state) return;
    state->screen = NUVIO_SCREEN_HOME;
}

const char *NuvioScreenName(NuvioScreen screen)
{
    switch (screen) {
        case NUVIO_SCREEN_HOME: return "NUVIO - Home";
        case NUVIO_SCREEN_MOVIES: return "Movies";
        case NUVIO_SCREEN_SERIES: return "Series";
        case NUVIO_SCREEN_LIBRARY: return "My Library";
        case NUVIO_SCREEN_SETTINGS: return "Settings";
        default: return "Unknown screen";
    }
}
