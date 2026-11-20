#ifndef NUVIO_APP_H
#define NUVIO_APP_H

// C++98 state and controller navigation, shared by Xbox 360 and PC tests.
// Profiles are intentionally local-only. No password or account is created.
enum NuvioScreen {
    NUVIO_SCREEN_PROFILE = 0,
    NUVIO_SCREEN_HOME,
    NUVIO_SCREEN_DISCOVER,
    NUVIO_SCREEN_SEARCH,
    NUVIO_SCREEN_LIBRARY,
    NUVIO_SCREEN_SETTINGS,
    NUVIO_SCREEN_DETAILS
};

struct NuvioState {
    NuvioScreen screen;
    NuvioScreen previousScreen;
    int sidebarItem;
    int selectedRow;
    int selectedColumn;
    int selectedCatalogIndex;
    bool sidebarFocused;
};

// Small, allocation-free controller helpers shared with the host tests.
// The caller supplies its monotonic millisecond counter (GetTickCount on Xbox).
struct NuvioNavigationRepeater {
    int heldDirection;
    unsigned long lastActionMs;
    bool waitingForFirstRepeat;
};

void NuvioNavigationRepeaterReset(NuvioNavigationRepeater *repeater);
int NuvioNavigationRepeat(NuvioNavigationRepeater *repeater,
                          int direction, unsigned long nowMs);
int NuvioThumbDirection(short axis, int deadzone);

void NuvioInitialize(NuvioState *state);
void NuvioMoveHorizontal(NuvioState *state, int direction);
void NuvioMoveVertical(NuvioState *state, int direction, int rowCount);
void NuvioSelect(NuvioState *state, int catalogIndex);
void NuvioBack(NuvioState *state);
void NuvioOpenScreen(NuvioState *state, NuvioScreen screen);
const char *NuvioScreenName(NuvioScreen screen);

#endif
