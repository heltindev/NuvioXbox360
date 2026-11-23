
#ifndef NUVIO_APP_H
#define NUVIO_APP_H

// C++98 state and controller navigation, shared by Xbox 360 and PC tests.
// Cloud profiles use the official Nuvio profile index; local profiles remain local.
enum NuvioScreen {
    NUVIO_SCREEN_PROFILE = 0,
    NUVIO_SCREEN_HOME,
    NUVIO_SCREEN_DISCOVER,
    NUVIO_SCREEN_SEARCH,
    NUVIO_SCREEN_LIBRARY,
    NUVIO_SCREEN_SETTINGS,
    NUVIO_SCREEN_DETAILS,
    NUVIO_SCREEN_SEARCH_RESULTS,
    NUVIO_SCREEN_LOGIN,
    NUVIO_SCREEN_CREATE_PROFILE,
    NUVIO_SCREEN_ADDONS,
    NUVIO_SCREEN_ADD_ADDON,
    NUVIO_SCREEN_QR_LOGIN,
    NUVIO_SCREEN_SOURCES,
    NUVIO_SCREEN_PLAYER,
    NUVIO_SCREEN_P2P_SETTINGS
};

enum NuvioCatalogMode {
    NUVIO_CATALOG_TRENDING = 0,
    NUVIO_CATALOG_POPULAR,
    NUVIO_CATALOG_TOP_RATED
};

enum {
    NUVIO_MAX_PROFILES = 6,
    NUVIO_PROFILE_NAME_LEN = 24,
    NUVIO_PROFILE_COLORS = 6,
    NUVIO_TEXT_LEN = 256,
    NUVIO_MAX_ADDONS = 64,
    NUVIO_ADDON_NAME_LEN = 40,
    NUVIO_ADDON_DESC_LEN = 72,
    NUVIO_ADDON_URL_LEN = 2049,
    NUVIO_KEY_ROWS = 5,
    NUVIO_KEY_COLS = 10
};

enum NuvioKeyboardMode { NUVIO_KEYBOARD_NAME = 0, NUVIO_KEYBOARD_URL = 1, NUVIO_KEYBOARD_SEARCH = 2 };
enum NuvioKeyResult { NUVIO_KEY_NONE = 0, NUVIO_KEY_DONE = 1, NUVIO_KEY_CANCEL = 2 };

struct NuvioProfile {
    char name[NUVIO_PROFILE_NAME_LEN];
    int color; // index into the app's avatar palette
    int remoteId; // -1 for local profiles, 1..6 for Nuvio cloud profiles
    int locked; // requires account PIN verification before selecting
};

struct NuvioAddon {
    int id;
    int enabled;
    char name[NUVIO_ADDON_NAME_LEN];
    char desc[NUVIO_ADDON_DESC_LEN];
    char url[NUVIO_ADDON_URL_LEN];
};

struct NuvioAddonList {
    NuvioAddon items[NUVIO_MAX_ADDONS];
    int count;
    int fromCloud; // list synchronized from the active Nuvio cloud profile
};

struct NuvioState {
    NuvioScreen screen;
    NuvioScreen previousScreen;
    int sidebarItem;
    int selectedRow;
    int selectedColumn;
    int selectedCatalogIndex;
    bool sidebarFocused;

    // Search state
    char searchQuery[128];
    int searchCursorPos;
    bool searchActive;

    // Catalog mode
    NuvioCatalogMode catalogMode;
    int catalogPage;

    // Online/offline state
    bool isOnline;

    // Active profile
    char profileName[32];
    int profileIndex;

    // Login / profiles
    int loginCursor;       // always 0: official Nuvio QR sign-in only
    NuvioProfile profiles[NUVIO_MAX_PROFILES];
    int profileCount;
    bool cloudProfiles;
    int profileCursor;     // profileCount means the "Add profile" tile
    int newProfileColor;
    NuvioScreen createReturn;

    // Virtual keyboard (profile name / add-on URL)
    int keyboardMode;
    int keyboardRow;       // -1 = avatar colour row, 0..4 = keys, 5 = actions
    int keyboardCol;
    bool keyboardShift;
    char textBuffer[NUVIO_TEXT_LEN];
    int textLen;
};

void NuvioInitialize(NuvioState *state);
void NuvioMoveHorizontal(NuvioState *state, int direction);
void NuvioMoveVertical(NuvioState *state, int direction, int rowCount);
void NuvioSelect(NuvioState *state, int catalogIndex);
void NuvioBack(NuvioState *state);
void NuvioOpenScreen(NuvioState *state, NuvioScreen screen);
const char *NuvioScreenName(NuvioScreen screen);

// Search input handling
void NuvioSearchAddChar(NuvioState *state, char c);
void NuvioSearchBackspace(NuvioState *state);
void NuvioSearchClear(NuvioState *state);
void NuvioSearchSubmit(NuvioState *state);

// Catalog mode handling
void NuvioSetCatalogMode(NuvioState *state, NuvioCatalogMode mode);
void NuvioNextCatalogPage(NuvioState *state);
void NuvioPrevCatalogPage(NuvioState *state);

// ---- Login and profiles ----
void NuvioLoginMove(NuvioState *state, int direction);
void NuvioLoginSelect(NuvioState *state);          // Sign in -> profile list, or Create profile
void NuvioProfileMove(NuvioState *state, int direction);
// Returns 1 when a profile was entered (screen becomes HOME), 0 when "Add profile" opened the creator.
int NuvioProfileSelect(NuvioState *state);
void NuvioProfilesEnsureAdmin(NuvioState *state);
bool NuvioProfileAdd(NuvioState *state, const char *name, int color);
bool NuvioProfileRemove(NuvioState *state, int index); // index 0 (Admin) is protected
bool NuvioProfileCreateConfirm(NuvioState *state);     // uses textBuffer + newProfileColor
bool NuvioProfilesLoad(NuvioState *state, const char *path);
bool NuvioProfilesSave(const NuvioState *state, const char *path);

// ---- Virtual keyboard ----
void NuvioKeyboardBegin(NuvioState *state, int mode, const char *initialText);
void NuvioKeyboardMove(NuvioState *state, int dx, int dy);
int NuvioKeyboardActionCount(const NuvioState *state);
const char *NuvioKeyboardActionLabel(const NuvioState *state, int index);
char NuvioKeyboardKeyChar(const NuvioState *state, int row, int col);
int NuvioKeyboardPress(NuvioState *state);          // returns NuvioKeyResult
void NuvioKeyboardBackspace(NuvioState *state);
void NuvioKeyboardSpace(NuvioState *state);
void NuvioKeyboardToggleShift(NuvioState *state);

// ---- Plugins (Stremio add-ons) ----
void NuvioAddonsLoadDefaults(NuvioAddonList *list);
// Parses "id<TAB>enabled<TAB>name<TAB>description<TAB>url" lines from the native cloud backend.
bool NuvioAddonsParse(NuvioAddonList *list, const char *text, int length);
int NuvioAddonsEnabledCount(const NuvioAddonList *list);
void NuvioUrlEncode(char *out, int outSize, const char *in);

#endif
