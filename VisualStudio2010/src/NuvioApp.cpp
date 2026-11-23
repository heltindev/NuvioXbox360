
#include "NuvioApp.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

static NuvioScreen SidebarScreen(int index)
{
    static const NuvioScreen kScreens[4] = {
        NUVIO_SCREEN_HOME, NUVIO_SCREEN_SEARCH, NUVIO_SCREEN_LIBRARY,
        NUVIO_SCREEN_SETTINGS
    };
    return index>=0&&index<4?kScreens[index]:NUVIO_SCREEN_HOME;
}
int NuvioSettingsItemCount(int category)
{
    static const int count[5]={3,2,2,2,2};
    return category>=0&&category<5?count[category]:0;
}
bool NuvioSettingsItemEnabled(int category,int row)
{
    // Only implemented settings can be opened. Other entries are displayed dimmed.
    return (category==0&&(row==0||row==1))||(category==2&&(row==0||row==1));
}

static void CopyText(char *target, int length, const char *source)
{
    if (!target || length <= 0) return;
    if (!source) source = "";
    std::strncpy(target, source, length - 1);
    target[length - 1] = '\0';
}

static char LowerChar(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

static bool EqualNoCase(const char *a, const char *b)
{
    while (*a && *b) {
        if (LowerChar(*a) != LowerChar(*b)) return false;
        ++a; ++b;
    }
    return *a == *b;
}

void NuvioInitialize(NuvioState *state)
{
    if (!state) return;
    std::memset(state, 0, sizeof(*state));
    state->screen = NUVIO_SCREEN_LOGIN;
    state->previousScreen = NUVIO_SCREEN_HOME;
    state->selectedCatalogIndex = -1;
    state->catalogMode = NUVIO_CATALOG_TRENDING;
    state->catalogPage = 1;
    state->createReturn = NUVIO_SCREEN_LOGIN;
    state->keyboardRow = 1;
    NuvioProfilesEnsureAdmin(state);
}

static bool IsStandaloneScreen(NuvioScreen screen)
{
    return screen == NUVIO_SCREEN_PROFILE || screen == NUVIO_SCREEN_LOGIN ||
           screen == NUVIO_SCREEN_CREATE_PROFILE || screen == NUVIO_SCREEN_ADD_ADDON ||
           screen == NUVIO_SCREEN_DETAILS || screen == NUVIO_SCREEN_QR_LOGIN ||
           screen == NUVIO_SCREEN_ACCOUNT;
}

void NuvioOpenScreen(NuvioState *state, NuvioScreen screen)
{
    if (!state) return;
    if (IsStandaloneScreen(screen) || screen == NUVIO_SCREEN_ADDONS) return;
    state->screen = screen;
    state->selectedRow = 0;
    state->selectedColumn = 0;
    state->sidebarFocused = false;
    if (screen == NUVIO_SCREEN_HOME || screen == NUVIO_SCREEN_DISCOVER) state->sidebarItem = 0;
    if (screen == NUVIO_SCREEN_SEARCH) {
        state->sidebarItem = 1;
        state->searchActive = true;
        state->searchCursorPos = (int)std::strlen(state->searchQuery);
    }
    if (screen == NUVIO_SCREEN_LIBRARY) state->sidebarItem = 2;
    if (screen == NUVIO_SCREEN_SETTINGS) {
        state->sidebarItem = 3;
        state->settingsCategory=0;
        state->settingsCategoriesFocused=true;
    }
}

void NuvioMoveHorizontal(NuvioState *state, int direction)
{
    if (!state || direction == 0 || IsStandaloneScreen(state->screen) ||
        state->screen == NUVIO_SCREEN_ADDONS) return;

    if (state->sidebarFocused) {
        if (direction > 0) state->sidebarFocused = false;
        return;
    }
    if (state->screen == NUVIO_SCREEN_SETTINGS) {
        if (direction < 0) {
            if (state->settingsCategoriesFocused) state->sidebarFocused=true;
            else state->settingsCategoriesFocused=true;
        } else state->settingsCategoriesFocused=false;
        return;
    }
    if (direction < 0) {
        if (state->selectedColumn == 0) state->sidebarFocused = true;
        else --state->selectedColumn;
    } else if (state->screen != NUVIO_SCREEN_SETTINGS && state->selectedColumn < 19) {
        ++state->selectedColumn;
    }
}

void NuvioMoveVertical(NuvioState *state, int direction, int rowCount)
{
    if (!state || direction == 0 || IsStandaloneScreen(state->screen)) return;

    if (state->sidebarFocused) {
        state->sidebarItem += direction > 0 ? 1 : -1;
        if (state->sidebarItem < 0) state->sidebarItem = 3;
        if (state->sidebarItem > 3) state->sidebarItem = 0;
    } else {
        if (state->screen == NUVIO_SCREEN_SETTINGS && state->settingsCategoriesFocused) {
            state->settingsCategory+=direction>0?1:-1;
            if(state->settingsCategory<0)state->settingsCategory=0;
            if(state->settingsCategory>4)state->settingsCategory=4;
            state->selectedRow=0;
            return;
        }
        if (state->screen == NUVIO_SCREEN_SETTINGS)
            rowCount=NuvioSettingsItemCount(state->settingsCategory);
        const int lastRow = rowCount > 0 ? rowCount - 1 : 0;
        state->selectedRow += direction > 0 ? 1 : -1;
        if (state->selectedRow < 0) state->selectedRow = 0;
        if (state->selectedRow > lastRow) state->selectedRow = lastRow;
    }
}

static void StartCreateProfile(NuvioState *state, NuvioScreen returnTo)
{
    state->createReturn = returnTo;
    state->screen = NUVIO_SCREEN_CREATE_PROFILE;
    state->newProfileColor = state->profileCount % NUVIO_PROFILE_COLORS;
    NuvioKeyboardBegin(state, NUVIO_KEYBOARD_NAME, "");
}

void NuvioSelect(NuvioState *state, int catalogIndex)
{
    if (!state) return;
    if (state->screen == NUVIO_SCREEN_LOGIN) { NuvioLoginSelect(state); return; }
    if (state->screen == NUVIO_SCREEN_PROFILE) { NuvioProfileSelect(state); return; }
    if (state->screen == NUVIO_SCREEN_DETAILS || state->screen == NUVIO_SCREEN_CREATE_PROFILE ||
        state->screen == NUVIO_SCREEN_ADDONS || state->screen == NUVIO_SCREEN_ADD_ADDON) return;
    if (state->sidebarFocused) {
        NuvioOpenScreen(state, SidebarScreen(state->sidebarItem));
        return;
    }
    if (state->screen == NUVIO_SCREEN_SETTINGS) {
        if(state->settingsCategoriesFocused) {
            state->settingsCategoriesFocused=false;
            state->selectedRow=0;
            return;
        }
        if(!NuvioSettingsItemEnabled(state->settingsCategory,state->selectedRow))return;
        if(state->settingsCategory==0&&state->selectedRow==0) {
            state->screen=NUVIO_SCREEN_ACCOUNT;
        } else if(state->settingsCategory==0&&state->selectedRow==1) {
            state->previousScreen=NUVIO_SCREEN_SETTINGS;
            state->screen=NUVIO_SCREEN_PROFILE;
            state->profileCursor=state->profileIndex;
        } else if(state->settingsCategory==2&&state->selectedRow==0) {
            state->screen=NUVIO_SCREEN_ADDONS;
            state->selectedRow=0;state->selectedColumn=0;
        } else if(state->settingsCategory==2&&state->selectedRow==1) {
            state->screen=NUVIO_SCREEN_P2P_SETTINGS;
            state->selectedRow=0;
        }
        return;
    }
    if (catalogIndex >= 0 && (state->screen == NUVIO_SCREEN_HOME ||
        state->screen == NUVIO_SCREEN_DISCOVER || state->screen == NUVIO_SCREEN_SEARCH ||
        state->screen == NUVIO_SCREEN_SEARCH_RESULTS || state->screen == NUVIO_SCREEN_LIBRARY)) {
        state->selectedCatalogIndex = catalogIndex;
        state->previousScreen = state->screen;
        state->screen = NUVIO_SCREEN_DETAILS;
    }
}

void NuvioBack(NuvioState *state)
{
    if (!state) return;
    switch (state->screen) {
        case NUVIO_SCREEN_LOGIN:
            break;
        case NUVIO_SCREEN_DETAILS:
            state->screen = state->previousScreen;
            break;
        case NUVIO_SCREEN_SEARCH_RESULTS:
            state->screen = NUVIO_SCREEN_SEARCH;
            state->searchActive = true;
            break;
        case NUVIO_SCREEN_SEARCH:
            NuvioOpenScreen(state,NUVIO_SCREEN_HOME);
            break;
        case NUVIO_SCREEN_ACCOUNT:
            state->screen=NUVIO_SCREEN_SETTINGS;
            state->settingsCategory=0;
            state->settingsCategoriesFocused=false;
            state->selectedRow=0;
            state->sidebarItem=3;
            state->sidebarFocused=false;
            break;
        case NUVIO_SCREEN_P2P_SETTINGS:
            state->screen = NUVIO_SCREEN_SETTINGS;
            state->settingsCategory=2;
            state->settingsCategoriesFocused=false;
            state->selectedRow = 1;
            state->sidebarItem = 3;
            state->sidebarFocused = false;
            break;
        case NUVIO_SCREEN_SETTINGS:
            state->screen = NUVIO_SCREEN_HOME;
            state->selectedRow = 0;
            state->sidebarItem = 0;
            state->sidebarFocused = false;
            break;
        case NUVIO_SCREEN_ADDONS:
            state->screen = NUVIO_SCREEN_SETTINGS;
            state->settingsCategory=2;
            state->settingsCategoriesFocused=false;
            state->sidebarItem = 3;
            state->selectedRow = 0;
            state->sidebarFocused = false;
            break;
        case NUVIO_SCREEN_ADD_ADDON:
            state->screen = NUVIO_SCREEN_ADDONS;
            break;
        case NUVIO_SCREEN_QR_LOGIN:
            state->screen = NUVIO_SCREEN_LOGIN;
            break;
        case NUVIO_SCREEN_CREATE_PROFILE:
            state->screen = state->createReturn;
            break;
        case NUVIO_SCREEN_PROFILE:
            if(state->previousScreen==NUVIO_SCREEN_SETTINGS) {
                state->screen=NUVIO_SCREEN_SETTINGS;
                state->settingsCategory=0;
                state->settingsCategoriesFocused=false;
                state->selectedRow=1;state->sidebarItem=3;
            } else {
                state->screen = NUVIO_SCREEN_LOGIN;
                state->loginCursor = 0;
            }
            break;
        default:
            state->screen = NUVIO_SCREEN_PROFILE;
            state->profileCursor = state->profileIndex;
            state->sidebarFocused = false;
            break;
    }
}

const char *NuvioScreenName(NuvioScreen screen)
{
    switch (screen) {
        case NUVIO_SCREEN_PROFILE: return "Choose a profile";
        case NUVIO_SCREEN_LOGIN: return "Sign in";
        case NUVIO_SCREEN_CREATE_PROFILE: return "Create profile";
        case NUVIO_SCREEN_ADDONS: return "Plugins";
        case NUVIO_SCREEN_ADD_ADDON: return "Install plugin";
        case NUVIO_SCREEN_QR_LOGIN: return "Sign in with QR code";
        case NUVIO_SCREEN_HOME: return "Home";
        case NUVIO_SCREEN_DISCOVER: return "Discover";
        case NUVIO_SCREEN_SEARCH: return "Search";
        case NUVIO_SCREEN_SEARCH_RESULTS: return "Search Results";
        case NUVIO_SCREEN_LIBRARY: return "My Library";
        case NUVIO_SCREEN_SETTINGS: return "Settings";
        case NUVIO_SCREEN_ACCOUNT: return "Conta";
        case NUVIO_SCREEN_P2P_SETTINGS: return "Streaming P2P";
        case NUVIO_SCREEN_DETAILS: return "Details";
        case NUVIO_SCREEN_SOURCES: return "Addon sources";
        default: return "Unknown screen";
    }
}

// Search input handling
void NuvioSearchAddChar(NuvioState *state, char c)
{
    if (!state || !state->searchActive) return;
    int len = (int)std::strlen(state->searchQuery);
    if (len < 127 && c >= 32 && c <= 126) {
        state->searchQuery[len] = c;
        state->searchQuery[len + 1] = '\0';
        state->searchCursorPos = len + 1;
    }
}

void NuvioSearchBackspace(NuvioState *state)
{
    if (!state || !state->searchActive) return;
    int len = (int)std::strlen(state->searchQuery);
    if (len > 0 && state->searchCursorPos > 0) {
        state->searchQuery[len - 1] = '\0';
        state->searchCursorPos = len - 1;
    }
}

void NuvioSearchClear(NuvioState *state)
{
    if (!state) return;
    state->searchQuery[0] = '\0';
    state->searchCursorPos = 0;
}

void NuvioSearchSubmit(NuvioState *state)
{
    if (!state) return;
    if (state->searchQuery[0] != '\0') {
        state->searchActive = false;
        state->screen = NUVIO_SCREEN_SEARCH_RESULTS;
        state->selectedRow = 0;
        state->selectedColumn = 0;
        state->sidebarFocused = false;
    }
}

// Catalog mode handling
void NuvioSetCatalogMode(NuvioState *state, NuvioCatalogMode mode)
{
    if (!state) return;
    state->catalogMode = mode;
    state->catalogPage = 1;
    state->selectedRow = 0;
    state->selectedColumn = 0;
}

void NuvioNextCatalogPage(NuvioState *state)
{
    if (!state) return;
    state->catalogPage++;
    state->selectedRow = 0;
    state->selectedColumn = 0;
}

void NuvioPrevCatalogPage(NuvioState *state)
{
    if (!state) return;
    if (state->catalogPage > 1) {
        state->catalogPage--;
        state->selectedRow = 0;
        state->selectedColumn = 0;
    }
}

// ================= Login and profiles =================

void NuvioLoginMove(NuvioState *state, int direction)
{
    // Account login is exclusively through the official Nuvio device QR flow.
    (void)direction;
    if (!state || state->screen != NUVIO_SCREEN_LOGIN) return;
    state->loginCursor = 0;
}

void NuvioLoginSelect(NuvioState *state)
{
    if (!state || state->screen != NUVIO_SCREEN_LOGIN) return;
    state->loginCursor = 0;
    state->screen = NUVIO_SCREEN_QR_LOGIN; // XboxMain starts official device login.
}

void NuvioProfileMove(NuvioState *state, int direction)
{
    if (!state || state->screen != NUVIO_SCREEN_PROFILE) return;
    const int tiles = state->profileCount + (!state->cloudProfiles && state->profileCount < NUVIO_MAX_PROFILES ? 1 : 0);
    state->profileCursor += direction;
    if (state->profileCursor < 0) state->profileCursor = 0;
    if (state->profileCursor > tiles - 1) state->profileCursor = tiles - 1;
}

int NuvioProfileSelect(NuvioState *state)
{
    if (!state || state->screen != NUVIO_SCREEN_PROFILE) return 0;
    if (state->profileCursor >= 0 && state->profileCursor < state->profileCount) {
        if (state->profiles[state->profileCursor].locked) return 0; // PIN screen is not implemented yet.
        state->profileIndex = state->profileCursor;
        CopyText(state->profileName, sizeof(state->profileName),
                 state->profiles[state->profileIndex].name);
        state->screen = NUVIO_SCREEN_LOGIN; // so OpenScreen accepts the target below
        NuvioOpenScreen(state, NUVIO_SCREEN_HOME);
        // A finished profile switch must not leave Settings as a stale back target.
        state->previousScreen=NUVIO_SCREEN_HOME;
        return 1;
    }
    if (state->cloudProfiles) return 0;
    StartCreateProfile(state, NUVIO_SCREEN_PROFILE);
    return 0;
}

void NuvioProfilesEnsureAdmin(NuvioState *state)
{
    if (!state) return;
    if (state->profileCount <= 0) {
        state->profileCount = 1;
        CopyText(state->profiles[0].name, NUVIO_PROFILE_NAME_LEN, "Admin");
        state->profiles[0].color = 0;
        state->profiles[0].remoteId = -1;
        state->profiles[0].locked = 0;
    }
    if (state->profileIndex < 0 || state->profileIndex >= state->profileCount)
        state->profileIndex = 0;
    CopyText(state->profileName, sizeof(state->profileName),
             state->profiles[state->profileIndex].name);
}

bool NuvioProfileAdd(NuvioState *state, const char *name, int color)
{
    if (!state || !name || state->profileCount >= NUVIO_MAX_PROFILES) return false;
    char clean[NUVIO_PROFILE_NAME_LEN];
    int n = 0;
    while (*name == ' ') ++name;
    for (; *name && n < NUVIO_PROFILE_NAME_LEN - 1; ++name) {
        if (*name == '\t' || *name == '\r' || *name == '\n') continue;
        clean[n++] = *name;
    }
    while (n > 0 && clean[n - 1] == ' ') --n;
    clean[n] = '\0';
    if (n == 0) return false;
    for (int i = 0; i < state->profileCount; ++i)
        if (EqualNoCase(state->profiles[i].name, clean)) return false;
    NuvioProfile &p = state->profiles[state->profileCount++];
    CopyText(p.name, sizeof(p.name), clean);
    p.color = (color >= 0 && color < NUVIO_PROFILE_COLORS) ? color : 0;
    p.remoteId = -1;
    p.locked = 0;
    return true;
}

bool NuvioProfileRemove(NuvioState *state, int index)
{
    if (!state || index <= 0 || index >= state->profileCount) return false;
    for (int i = index; i < state->profileCount - 1; ++i)
        state->profiles[i] = state->profiles[i + 1];
    --state->profileCount;
    if (state->profileIndex == index) state->profileIndex = 0;
    else if (state->profileIndex > index) --state->profileIndex;
    if (state->profileCursor > state->profileCount) state->profileCursor = state->profileCount;
    NuvioProfilesEnsureAdmin(state);
    return true;
}

bool NuvioProfileCreateConfirm(NuvioState *state)
{
    if (!state || state->screen != NUVIO_SCREEN_CREATE_PROFILE) return false;
    if (!NuvioProfileAdd(state, state->textBuffer, state->newProfileColor)) return false;
    state->screen = NUVIO_SCREEN_PROFILE;
    state->profileCursor = state->profileCount - 1;
    return true;
}

bool NuvioProfilesLoad(NuvioState *state, const char *path)
{
    if (!state || !path) return false;
    std::FILE *file = std::fopen(path, "rb");
    if (!file) return false;
    NuvioProfile loaded[NUVIO_MAX_PROFILES];
    int count = 0;
    char line[128];
    while (count < NUVIO_MAX_PROFILES && std::fgets(line, sizeof(line), file)) {
        char *tab = std::strchr(line, '\t');
        if (!tab) continue;
        *tab = '\0';
        int color = std::atoi(tab + 1);
        if (line[0] == '\0') continue;
        CopyText(loaded[count].name, NUVIO_PROFILE_NAME_LEN, line);
        loaded[count].color = (color >= 0 && color < NUVIO_PROFILE_COLORS) ? color : 0;
        loaded[count].remoteId = -1;
        loaded[count].locked = 0;
        ++count;
    }
    std::fclose(file);
    if (count == 0) return false;
    for (int i = 0; i < count; ++i) state->profiles[i] = loaded[i];
    state->profileCount = count;
    state->profileIndex = 0;
    NuvioProfilesEnsureAdmin(state);
    return true;
}

bool NuvioProfilesSave(const NuvioState *state, const char *path)
{
    if (!state || !path) return false;
    std::FILE *file = std::fopen(path, "wb");
    if (!file) return false;
    for (int i = 0; i < state->profileCount; ++i)
        std::fprintf(file, "%s\t%d\n", state->profiles[i].name, state->profiles[i].color);
    std::fclose(file);
    return true;
}

// ================= Virtual keyboard =================

enum { KA_SHIFT, KA_SPACE, KA_DELETE, KA_CLEAR, KA_HTTPS, KA_MANIFEST, KA_CANCEL, KA_OK };

static const char *kKeyRows[NUVIO_KEY_ROWS] = {
    "1234567890", "qwertyuiop", "asdfghjkl-", "zxcvbnm._/", ":?=&%@#+,~"
};

static int ActionId(const NuvioState *state, int index)
{
    static const int nameActions[6] = { KA_SHIFT, KA_SPACE, KA_DELETE, KA_CLEAR, KA_CANCEL, KA_OK };
    static const int urlActions[8] = { KA_SHIFT, KA_SPACE, KA_DELETE, KA_CLEAR, KA_HTTPS,
                                       KA_MANIFEST, KA_CANCEL, KA_OK };
    if (index < 0 || index >= NuvioKeyboardActionCount(state)) return -1;
    return state->keyboardMode == NUVIO_KEYBOARD_URL ? urlActions[index] : nameActions[index];
}

static int TextLimit(const NuvioState *state)
{
    return state->keyboardMode == NUVIO_KEYBOARD_NAME ? 20 :
        state->keyboardMode == NUVIO_KEYBOARD_SEARCH ? 127 : NUVIO_TEXT_LEN - 1;
}

static void AppendText(NuvioState *state, const char *text)
{
    const int limit = TextLimit(state);
    while (*text && state->textLen < limit) {
        state->textBuffer[state->textLen++] = *text++;
    }
    state->textBuffer[state->textLen] = '\0';
}

void NuvioKeyboardBegin(NuvioState *state, int mode, const char *initialText)
{
    if (!state) return;
    state->keyboardMode = mode;
    state->keyboardRow = 1;
    state->keyboardCol = 0;
    state->keyboardShift = false;
    state->textBuffer[0] = '\0';
    state->textLen = 0;
    if (initialText) AppendText(state, initialText);
}

int NuvioKeyboardActionCount(const NuvioState *state)
{
    return (state && state->keyboardMode == NUVIO_KEYBOARD_URL) ? 8 : 6;
}

const char *NuvioKeyboardActionLabel(const NuvioState *state, int index)
{
    static const char *labels[8] = { "Shift", "Space", "Delete", "Clear", "https://",
                                     "/manifest.json", "Cancel", "OK" };
    const int id = ActionId(state, index);
    return id < 0 ? "" : labels[id];
}

char NuvioKeyboardKeyChar(const NuvioState *state, int row, int col)
{
    if (!state || row < 0 || row >= NUVIO_KEY_ROWS || col < 0 || col >= NUVIO_KEY_COLS) return 0;
    char c = kKeyRows[row][col];
    if (state->keyboardShift && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    return c;
}

void NuvioKeyboardMove(NuvioState *state, int dx, int dy)
{
    if (!state) return;
    const bool canColor = state->screen == NUVIO_SCREEN_CREATE_PROFILE;
    const int actions = NuvioKeyboardActionCount(state);
    if (dy != 0) {
        const int oldRow = state->keyboardRow;
        int row = oldRow + (dy > 0 ? 1 : -1);
        const int minRow = canColor ? -1 : 0;
        if (row < minRow) row = minRow;
        if (row > NUVIO_KEY_ROWS) row = NUVIO_KEY_ROWS;
        if (oldRow >= 0 && oldRow < NUVIO_KEY_ROWS && row == NUVIO_KEY_ROWS)
            state->keyboardCol = state->keyboardCol * actions / NUVIO_KEY_COLS;
        else if (oldRow == NUVIO_KEY_ROWS && row >= 0 && row < NUVIO_KEY_ROWS)
            state->keyboardCol = (state->keyboardCol * NUVIO_KEY_COLS + NUVIO_KEY_COLS / 2) / actions;
        if (state->keyboardCol > NUVIO_KEY_COLS - 1) state->keyboardCol = NUVIO_KEY_COLS - 1;
        state->keyboardRow = row;
    }
    if (dx != 0) {
        const int step = dx > 0 ? 1 : -1;
        if (state->keyboardRow < 0) {
            state->newProfileColor =
                (state->newProfileColor + step + NUVIO_PROFILE_COLORS) % NUVIO_PROFILE_COLORS;
        } else {
            const int limit = state->keyboardRow == NUVIO_KEY_ROWS ? actions : NUVIO_KEY_COLS;
            state->keyboardCol = (state->keyboardCol + step + limit) % limit;
        }
    }
}

void NuvioKeyboardBackspace(NuvioState *state)
{
    if (!state || state->textLen <= 0) return;
    state->textBuffer[--state->textLen] = '\0';
}

void NuvioKeyboardSpace(NuvioState *state)
{
    if (state) AppendText(state, " ");
}

void NuvioKeyboardToggleShift(NuvioState *state)
{
    if (state) state->keyboardShift = !state->keyboardShift;
}

int NuvioKeyboardPress(NuvioState *state)
{
    if (!state) return NUVIO_KEY_NONE;
    if (state->keyboardRow < 0) return NUVIO_KEY_NONE;
    if (state->keyboardRow < NUVIO_KEY_ROWS) {
        const char c = NuvioKeyboardKeyChar(state, state->keyboardRow, state->keyboardCol);
        if (c) {
            char one[2] = { c, 0 };
            AppendText(state, one);
            state->keyboardShift = false;
        }
        return NUVIO_KEY_NONE;
    }
    switch (ActionId(state, state->keyboardCol)) {
        case KA_SHIFT:    NuvioKeyboardToggleShift(state); break;
        case KA_SPACE:    NuvioKeyboardSpace(state); break;
        case KA_DELETE:   NuvioKeyboardBackspace(state); break;
        case KA_CLEAR:    state->textBuffer[0] = '\0'; state->textLen = 0; break;
        case KA_HTTPS:    AppendText(state, "https://"); break;
        case KA_MANIFEST: AppendText(state, "/manifest.json"); break;
        case KA_CANCEL:   return NUVIO_KEY_CANCEL;
        case KA_OK:       return NUVIO_KEY_DONE;
        default: break;
    }
    return NUVIO_KEY_NONE;
}

// ================= Plugins (Stremio add-ons) =================

static void SetAddon(NuvioAddonList *list, int enabled, const char *name,
                     const char *desc, const char *url)
{
    if (list->count >= NUVIO_MAX_ADDONS) return;
    NuvioAddon &a = list->items[list->count];
    a.id = list->count + 1;
    a.enabled = enabled;
    CopyText(a.name, sizeof(a.name), name);
    CopyText(a.desc, sizeof(a.desc), desc);
    CopyText(a.url, sizeof(a.url), url);
    ++list->count;
}

void NuvioAddonsLoadDefaults(NuvioAddonList *list)
{
    if (!list) return;
    std::memset(list, 0, sizeof(*list));
    // Only the official catalog is offered locally; installed addons come from cloud.
    SetAddon(list, 1, "Nuvio Catalog Addon", "Automatic catalog fallback",
             "https://catalog.nuvio.tv/manifest.json");
}

bool NuvioAddonsParse(NuvioAddonList *list, const char *text, int length)
{
    if (!list || !text || length <= 0) return false;
    // Heap allocation avoids a >130 KB stack frame on the Xbox.
    NuvioAddonList *storage = new NuvioAddonList;
    NuvioAddonList &parsed = *storage;
    std::memset(&parsed, 0, sizeof(parsed));
    int pos = 0;
    char line[2400];
    while (pos < length && parsed.count < NUVIO_MAX_ADDONS) {
        int end = pos;
        while (end < length && text[end] != '\n') ++end;
        int n = end - pos;
        if (n > (int)sizeof(line) - 1) n = (int)sizeof(line) - 1;
        std::memcpy(line, text + pos, n);
        line[n] = '\0';
        pos = end + 1;
        if (n > 0 && line[n - 1] == '\r') line[n - 1] = '\0';
        char *field[5];
        int found = 0;
        field[found++] = line;
        for (char *p = line; *p && found < 5; ++p) {
            if (*p == '\t') { *p = '\0'; field[found++] = p + 1; }
        }
        if (found < 5) continue;
        const int id = std::atoi(field[0]);
        if (id <= 0 || field[4][0] == '\0') continue;
        NuvioAddon &a = parsed.items[parsed.count++];
        a.id = id;
        a.enabled = std::atoi(field[1]) != 0;
        CopyText(a.name, sizeof(a.name), field[2]);
        CopyText(a.desc, sizeof(a.desc), field[3]);
        CopyText(a.url, sizeof(a.url), field[4]);
    }
    if (parsed.count == 0 && std::strncmp(text, "#cloud\n", 7)) { delete storage; return false; }
    parsed.fromCloud = 1;
    *list = parsed;
    delete storage;
    return true;
}

int NuvioAddonsEnabledCount(const NuvioAddonList *list)
{
    int n = 0;
    if (!list) return 0;
    for (int i = 0; i < list->count; ++i) if (list->items[i].enabled) ++n;
    return n;
}

void NuvioUrlEncode(char *out, int outSize, const char *in)
{
    static const char hex[] = "0123456789ABCDEF";
    if (!out || outSize <= 0) return;
    int o = 0;
    for (; in && *in && o + 4 < outSize; ++in) {
        const unsigned char c = (unsigned char)*in;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            out[o++] = (char)c;
        } else {
            out[o++] = '%';
            out[o++] = hex[c >> 4];
            out[o++] = hex[c & 15];
        }
    }
    out[o] = '\0';
}
