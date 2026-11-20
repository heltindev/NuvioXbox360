#include "NuvioApp.h"

void NuvioNavigationRepeaterReset(NuvioNavigationRepeater *repeater)
{
    if (!repeater) return;
    repeater->heldDirection = 0;
    repeater->lastActionMs = 0;
    repeater->waitingForFirstRepeat = true;
}

int NuvioNavigationRepeat(NuvioNavigationRepeater *repeater,
                          int direction, unsigned long nowMs)
{
    if (!repeater) return 0;
    if (direction > 0) direction = 1;
    if (direction < 0) direction = -1;
    if (direction == 0) {
        NuvioNavigationRepeaterReset(repeater);
        return 0;
    }
    if (direction != repeater->heldDirection) {
        repeater->heldDirection = direction;
        repeater->lastActionMs = nowMs;
        repeater->waitingForFirstRepeat = true;
        return direction;
    }

    const unsigned long delayMs = repeater->waitingForFirstRepeat ? 320UL : 110UL;
    if (nowMs - repeater->lastActionMs >= delayMs) {
        repeater->lastActionMs = nowMs;
        repeater->waitingForFirstRepeat = false;
        return direction;
    }
    return 0;
}

int NuvioThumbDirection(short axis, int deadzone)
{
    if (deadzone < 0) deadzone = 0;
    if (axis > deadzone) return 1;
    if (axis < -deadzone) return -1;
    return 0;
}

static NuvioScreen SidebarScreen(int index)
{
    static const NuvioScreen kScreens[5] = {
        NUVIO_SCREEN_HOME, NUVIO_SCREEN_DISCOVER, NUVIO_SCREEN_SEARCH,
        NUVIO_SCREEN_LIBRARY, NUVIO_SCREEN_SETTINGS
    };
    return kScreens[index];
}

void NuvioInitialize(NuvioState *state)
{
    if (!state) return;
    state->screen = NUVIO_SCREEN_PROFILE;
    state->previousScreen = NUVIO_SCREEN_HOME;
    state->sidebarItem = 0;
    state->selectedRow = 0;
    state->selectedColumn = 0;
    state->selectedCatalogIndex = -1;
    state->sidebarFocused = false;
}

void NuvioOpenScreen(NuvioState *state, NuvioScreen screen)
{
    if (!state) return;
    if (screen == NUVIO_SCREEN_DETAILS || screen == NUVIO_SCREEN_PROFILE) return;
    state->screen = screen;
    state->selectedRow = 0;
    state->selectedColumn = 0;
    state->sidebarFocused = false;
    if (screen == NUVIO_SCREEN_HOME) state->sidebarItem = 0;
    if (screen == NUVIO_SCREEN_DISCOVER) state->sidebarItem = 1;
    if (screen == NUVIO_SCREEN_SEARCH) state->sidebarItem = 2;
    if (screen == NUVIO_SCREEN_LIBRARY) state->sidebarItem = 3;
    if (screen == NUVIO_SCREEN_SETTINGS) state->sidebarItem = 4;
}

void NuvioMoveHorizontal(NuvioState *state, int direction)
{
    if (!state || direction == 0 || state->screen == NUVIO_SCREEN_PROFILE ||
        state->screen == NUVIO_SCREEN_DETAILS) return;

    if (state->sidebarFocused) {
        if (direction > 0) state->sidebarFocused = false;
        return;
    }
    if (direction < 0) {
        if (state->selectedColumn == 0) state->sidebarFocused = true;
        else --state->selectedColumn;
    } else if (state->selectedColumn < 4) {
        ++state->selectedColumn;
    }
}

void NuvioMoveVertical(NuvioState *state, int direction, int rowCount)
{
    if (!state || direction == 0 || state->screen == NUVIO_SCREEN_PROFILE ||
        state->screen == NUVIO_SCREEN_DETAILS) return;

    if (state->sidebarFocused) {
        state->sidebarItem += direction > 0 ? 1 : -1;
        if (state->sidebarItem < 0) state->sidebarItem = 4;
        if (state->sidebarItem > 4) state->sidebarItem = 0;
    } else {
        const int lastRow = rowCount > 0 ? rowCount - 1 : 0;
        state->selectedRow += direction > 0 ? 1 : -1;
        if (state->selectedRow < 0) state->selectedRow = 0;
        if (state->selectedRow > lastRow) state->selectedRow = lastRow;
    }
}

void NuvioSelect(NuvioState *state, int catalogIndex)
{
    if (!state) return;
    if (state->screen == NUVIO_SCREEN_PROFILE) {
        NuvioOpenScreen(state, NUVIO_SCREEN_HOME);
        return;
    }
    if (state->screen == NUVIO_SCREEN_DETAILS) return;
    if (state->sidebarFocused) {
        NuvioOpenScreen(state, SidebarScreen(state->sidebarItem));
        return;
    }
    if (catalogIndex >= 0 && (state->screen == NUVIO_SCREEN_HOME ||
        state->screen == NUVIO_SCREEN_DISCOVER || state->screen == NUVIO_SCREEN_SEARCH)) {
        state->selectedCatalogIndex = catalogIndex;
        state->previousScreen = state->screen;
        state->screen = NUVIO_SCREEN_DETAILS;
    }
}

void NuvioBack(NuvioState *state)
{
    if (!state) return;
    if (state->screen == NUVIO_SCREEN_DETAILS) {
        state->screen = state->previousScreen;
    } else if (state->screen != NUVIO_SCREEN_PROFILE) {
        state->screen = NUVIO_SCREEN_PROFILE;
        state->sidebarFocused = false;
    }
}

const char *NuvioScreenName(NuvioScreen screen)
{
    switch (screen) {
        case NUVIO_SCREEN_PROFILE: return "Choose a profile";
        case NUVIO_SCREEN_HOME: return "Home";
        case NUVIO_SCREEN_DISCOVER: return "Discover";
        case NUVIO_SCREEN_SEARCH: return "Search";
        case NUVIO_SCREEN_LIBRARY: return "My Library";
        case NUVIO_SCREEN_SETTINGS: return "Settings";
        case NUVIO_SCREEN_DETAILS: return "Details";
        default: return "Unknown screen";
    }
}
