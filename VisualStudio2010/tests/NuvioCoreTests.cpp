#include "../src/NuvioApp.h"
#include "../src/NuvioCatalog.h"
#include "../src/NuvioInput.h"

#include <cassert>
#include <cstring>
#include <cstdio>

static NuvioCatalog g_catalog;

static void TypeText(NuvioState *s, const char *text)
{
    for (; *text; ++text) {
        bool found = false;
        for (int r = 0; r < NUVIO_KEY_ROWS && !found; ++r)
            for (int c = 0; c < NUVIO_KEY_COLS && !found; ++c)
                if (NuvioKeyboardKeyChar(s, r, c) == *text) {
                    s->keyboardRow = r; s->keyboardCol = c;
                    NuvioKeyboardPress(s);
                    found = true;
                }
        assert(found);
    }
}

int main(int argc, char **argv)
{
    assert(NuvioStickDirections(0,0)==0);
    assert(NuvioStickDirections(8999,0)==0);
    assert(NuvioStickDirections(-16000,2000)==4);
    assert(NuvioStickDirections(12000,0)==8);
    assert(NuvioStickDirections(1000,15000)==1);
    assert(NuvioStickDirections(1000,-15000)==2);
    NuvioInputRepeat repeat;
    assert(NuvioRepeatDirections(repeat,4,100)==4);
    assert(NuvioRepeatDirections(repeat,4,429)==0);
    assert(NuvioRepeatDirections(repeat,4,430)==4);
    assert(NuvioRepeatDirections(repeat,4,539)==0);
    assert(NuvioRepeatDirections(repeat,4,540)==4);
    assert(NuvioRepeatDirections(repeat,0,550)==0);
    assert(NuvioRepeatDirections(repeat,8,551)==8);
    NuvioState state;
    NuvioInitialize(&state);

    // ---- Login -> profiles -> home ----
    assert(state.screen == NUVIO_SCREEN_LOGIN);
    assert(state.profileCount == 1 && std::strcmp(state.profiles[0].name, "Admin") == 0);
    NuvioLoginSelect(&state);                       // Sign in with QR code
    assert(state.screen == NUVIO_SCREEN_QR_LOGIN);
    NuvioBack(&state);
    assert(state.screen == NUVIO_SCREEN_LOGIN);

    // ---- Only QR code can start account login ----
    NuvioLoginMove(&state, 1);
    assert(state.loginCursor == 0);
    NuvioLoginSelect(&state);
    assert(state.screen == NUVIO_SCREEN_QR_LOGIN);
    // Simulate the Xbox completing remote QR approval and showing profiles.
    state.screen = NUVIO_SCREEN_PROFILE;

    // ---- Add a local profile from the profile picker, not the login menu ----
    state.profileCursor = state.profileCount;
    assert(NuvioProfileSelect(&state) == 0);
    assert(state.screen == NUVIO_SCREEN_CREATE_PROFILE);
    state.keyboardShift = true;
    TypeText(&state, "M");                          // shift applies once
    assert(!state.keyboardShift);
    TypeText(&state, "aria");
    assert(std::strcmp(state.textBuffer, "Maria") == 0);
    NuvioKeyboardBackspace(&state);
    assert(std::strcmp(state.textBuffer, "Mari") == 0);
    state.keyboardRow = NUVIO_KEY_ROWS;             // actions row: ... Cancel, OK
    state.keyboardCol = NuvioKeyboardActionCount(&state) - 1;
    assert(NuvioKeyboardPress(&state) == NUVIO_KEY_DONE);
    assert(NuvioProfileCreateConfirm(&state));
    assert(state.screen == NUVIO_SCREEN_PROFILE && state.profileCount == 2);
    assert(state.profileCursor == 1);
    assert(!NuvioProfileAdd(&state, "admin", 0));   // duplicate (case-insensitive)
    assert(!NuvioProfileAdd(&state, "   ", 0));     // empty

    NuvioProfileMove(&state, 1);                    // "Add profile" tile
    assert(NuvioProfileSelect(&state) == 0);
    assert(state.screen == NUVIO_SCREEN_CREATE_PROFILE);
    NuvioBack(&state);
    assert(state.screen == NUVIO_SCREEN_PROFILE);

    NuvioProfileMove(&state, -1);
    assert(NuvioProfileSelect(&state) == 1);        // enter Maria
    assert(state.screen == NUVIO_SCREEN_HOME);
    assert(std::strcmp(state.profileName, "Mari") == 0);

    assert(!NuvioProfileRemove(&state, 0));         // Admin is protected
    assert(NuvioProfileRemove(&state, 1));
    assert(state.profileCount == 1 && state.profileIndex == 0);

    // ---- Home / discover navigation ----
    NuvioMoveHorizontal(&state, 1);
    assert(state.selectedColumn == 1);
    NuvioMoveHorizontal(&state, -1);
    NuvioMoveHorizontal(&state, -1);
    assert(state.sidebarFocused);
    NuvioMoveVertical(&state, 1, 10);
    assert(state.sidebarItem == 1);
    NuvioSelect(&state, -1);
    assert(state.screen == NUVIO_SCREEN_SEARCH);
    assert(!state.sidebarFocused);
    NuvioOpenScreen(&state,NUVIO_SCREEN_DISCOVER); // Still reachable from catalog flow.
    NuvioMoveVertical(&state, 1, 4);
    assert(state.selectedRow == 1);
    NuvioSelect(&state, 9);
    assert(state.screen == NUVIO_SCREEN_DETAILS);
    assert(state.selectedCatalogIndex == 9);
    NuvioBack(&state);
    assert(state.screen == NUVIO_SCREEN_DISCOVER);
    NuvioBack(&state);
    assert(state.screen == NUVIO_SCREEN_PROFILE);

    // ---- PIN-protected profiles remain locked until PIN verification exists ----
    state.profiles[0].locked = 1;
    state.profileCursor = 0;
    assert(NuvioProfileSelect(&state) == 0);
    assert(state.screen == NUVIO_SCREEN_PROFILE);
    state.profiles[0].locked = 0;

    // ---- Reference settings: five categories, only implemented actions enabled ----
    NuvioProfileSelect(&state);
    NuvioOpenScreen(&state, NUVIO_SCREEN_SETTINGS);
    assert(state.screen == NUVIO_SCREEN_SETTINGS);
    assert(state.sidebarItem==3 && state.settingsCategoriesFocused);
    assert(NuvioSettingsItemCount(0)==3 && NuvioSettingsItemCount(2)==2);
    assert(!NuvioSettingsItemEnabled(0,0));
    assert(NuvioSettingsItemEnabled(0,1));
    assert(!NuvioSettingsItemEnabled(1,0));
    assert(NuvioSettingsItemEnabled(2,0) && NuvioSettingsItemEnabled(2,1));
    assert(!NuvioSettingsItemEnabled(4,1));
    NuvioSelect(&state,-1); // Enter "Voce" category; "Conta" is disabled.
    assert(!state.settingsCategoriesFocused);
    NuvioSelect(&state,-1);
    assert(state.screen==NUVIO_SCREEN_SETTINGS);
    NuvioMoveVertical(&state,1,3);
    NuvioSelect(&state,-1); // Alternar perfil is implemented.
    assert(state.screen==NUVIO_SCREEN_PROFILE);
    NuvioBack(&state);
    assert(state.screen==NUVIO_SCREEN_SETTINGS && state.settingsCategory==0 && state.selectedRow==1);
    NuvioMoveHorizontal(&state,-1); // Back to category list.
    NuvioMoveVertical(&state,1,3);
    NuvioMoveVertical(&state,1,3);
    assert(state.settingsCategory==2);
    NuvioSelect(&state,-1);
    assert(!state.settingsCategoriesFocused && state.selectedRow==0);
    NuvioSelect(&state,-1);
    assert(state.screen==NUVIO_SCREEN_ADDONS);
    NuvioMoveVertical(&state,1,7);
    assert(state.selectedRow==1);
    NuvioBack(&state);
    assert(state.screen==NUVIO_SCREEN_SETTINGS && state.settingsCategory==2 && state.selectedRow==0);
    assert(std::strcmp(NuvioScreenName(NUVIO_SCREEN_ADDONS),"Plugins")==0);
    NuvioMoveVertical(&state,1,2);
    NuvioSelect(&state,-1);
    assert(state.screen==NUVIO_SCREEN_P2P_SETTINGS && state.selectedRow==0);
    NuvioBack(&state);
    assert(state.screen==NUVIO_SCREEN_SETTINGS && state.settingsCategory==2 && state.selectedRow==1);
    NuvioBack(&state);
    assert(state.screen==NUVIO_SCREEN_HOME);
    NuvioOpenScreen(&state,NUVIO_SCREEN_SETTINGS);

    // ---- Plugin list ----
    NuvioAddonList addons;
    NuvioAddonsLoadDefaults(&addons);
    assert(addons.count == 1);
    assert(addons.items[0].enabled && std::strstr(addons.items[0].url, "catalog.nuvio.tv"));
    assert(NuvioAddonsEnabledCount(&addons) == 1);

    const char *tsv = "1\t1\tCinemeta\tCatalog\thttps://v3-cinemeta.strem.io/manifest.json\n"
                      "2\t0\tTorrentio\t\thttps://example.com/manifest.json\n";
    assert(NuvioAddonsParse(&addons, tsv, (int)std::strlen(tsv)));
    assert(addons.count == 2 && addons.fromCloud == 1);
    assert(addons.items[0].enabled == 1 && addons.items[1].enabled == 0);
    assert(addons.items[1].desc[0] == '\0');
    assert(!NuvioAddonsParse(&addons, "garbage", 7) && addons.count == 2);

    char enc[64];
    NuvioUrlEncode(enc, sizeof(enc), "a b:c/%7C");
    assert(std::strcmp(enc, "a%20b%3Ac%2F%257C") == 0);

    // URL keyboard shortcuts
    NuvioKeyboardBegin(&state, NUVIO_KEYBOARD_URL, "https://");
    state.keyboardRow = NUVIO_KEY_ROWS;
    state.keyboardCol = 5;                          // "/manifest.json"
    NuvioKeyboardPress(&state);
    assert(std::strcmp(state.textBuffer, "https:///manifest.json") == 0);

    // ---- Profile persistence ----
    NuvioState saved;
    NuvioInitialize(&saved);
    NuvioProfileAdd(&saved, "Kids", 3);
    assert(NuvioProfilesSave(&saved, "nuvio_profiles_test.cfg"));
    NuvioState loaded;
    NuvioInitialize(&loaded);
    assert(NuvioProfilesLoad(&loaded, "nuvio_profiles_test.cfg"));
    assert(loaded.profileCount == 2 && std::strcmp(loaded.profiles[1].name, "Kids") == 0);
    assert(loaded.profiles[1].color == 3);
    std::remove("nuvio_profiles_test.cfg");

    // ---- Catalog ----
    NuvioCatalogLoadDemo(&g_catalog);
    assert(g_catalog.count == 15 && g_catalog.isDemo);
    assert(NuvioCatalogRowCount(&g_catalog) == 3);
    assert(NuvioCatalogIndexForRow(&g_catalog, 0, 0) == 0);
    assert(NuvioCatalogIndexForRow(&g_catalog, 1, 0) == 5);
    assert(NuvioCatalogIndexForRow(&g_catalog, 2, 0) == 10);
    assert(NuvioCatalogIndexForRow(&g_catalog, 3, 0) == -1);
    assert(!NuvioCatalogLoadFile(&g_catalog, "not_found.tsv"));
    assert(g_catalog.count == 15);
    if (argc > 1) {
        assert(NuvioCatalogLoadFile(&g_catalog, argv[1]));
        assert(!g_catalog.isDemo);
        assert(g_catalog.count == 4);
        assert(g_catalog.items[0].year == 2024);
        assert(std::strcmp(g_catalog.items[0].title, "A Real Movie") == 0);
    }
    const char *online = "tt1234567\tmovie\t2024\tFirst Online Movie\n"
                         "tt7654321\tseries\t2025\tFirst Online Series\n";
    assert(NuvioCatalogLoadText(&g_catalog, online, (int)std::strlen(online)));
    assert(g_catalog.count == 2 && !g_catalog.isDemo);
    assert(std::strcmp(g_catalog.items[1].imdbId, "tt7654321") == 0);
    assert(!NuvioCatalogLoadText(&g_catalog, "bad", 3));
    assert(g_catalog.count == 2);

    const char *rails = "#catalog\n#row\tTrending\ntmdb:42\tmovie\t0\tUpcoming movie\n#row\tSeries\ntt12\tseries\t2026\tSeries title\n";
    assert(NuvioCatalogLoadText(&g_catalog, rails, (int)std::strlen(rails)));
    assert(g_catalog.railCount == 2 && g_catalog.count == 2);
    assert(g_catalog.items[0].year == 0 && !std::strcmp(g_catalog.items[0].imdbId,"tmdb:42"));
    assert(NuvioCatalogIndexForRow(&g_catalog, 1, 0) == 1);
    assert(NuvioCatalogIndexForRow(&g_catalog, 0, 1) == -1);
    assert(NuvioCatalogLoadText(&g_catalog,"#catalog\n",9) && g_catalog.count == 0);
    assert(NuvioAddonsParse(&addons,"#cloud\n",7) && addons.count == 0 && addons.fromCloud);
    state.screen = NUVIO_SCREEN_LIBRARY;
    state.sidebarFocused = false;
    NuvioSelect(&state, 0);
    assert(state.screen == NUVIO_SCREEN_DETAILS && state.previousScreen == NUVIO_SCREEN_LIBRARY);

    NuvioInitialize(NULL);
    NuvioMoveHorizontal(NULL, 1);
    NuvioMoveVertical(NULL, 1, 5);
    NuvioSelect(NULL, 0);
    NuvioBack(NULL);
    NuvioKeyboardMove(NULL, 1, 1);
    assert(NuvioKeyboardPress(NULL) == NUVIO_KEY_NONE);
    assert(std::strcmp(NuvioScreenName(static_cast<NuvioScreen>(999)),
                       "Unknown screen") == 0);
    std::puts("OK: NUVIO login, profiles, keyboard, plugins, browse and catalog");
    return 0;
}
