#ifndef NUVIO_APP_H
#define NUVIO_APP_H

// Portable C++98 code. No console SDK or online service is needed here.
enum NuvioScreen {
    NUVIO_SCREEN_HOME = 0,
    NUVIO_SCREEN_MOVIES,
    NUVIO_SCREEN_SERIES,
    NUVIO_SCREEN_LIBRARY,
    NUVIO_SCREEN_SETTINGS
};

struct NuvioState {
    NuvioScreen screen;
    int selectedTile;
};

void NuvioInitialize(NuvioState *state);
void NuvioMoveSelection(NuvioState *state, int direction);
void NuvioOpenSelection(NuvioState *state);
void NuvioGoHome(NuvioState *state);
const char *NuvioScreenName(NuvioScreen screen);

#endif
