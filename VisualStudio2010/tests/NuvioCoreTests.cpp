#include "../src/NuvioApp.h"
#include "../src/NuvioCatalog.h"

#include <cassert>
#include <cstring>
#include <cstdio>

int main(int argc, char **argv)
{
    NuvioState state;
    NuvioInitialize(&state);
    assert(state.screen == NUVIO_SCREEN_PROFILE);
    assert(state.selectedCatalogIndex == -1);
    NuvioSelect(&state, -1); // Select the sole Admin profile.
    assert(state.screen == NUVIO_SCREEN_HOME);
    NuvioMoveHorizontal(&state, 1);
    assert(state.selectedColumn == 1);
    NuvioMoveHorizontal(&state, -1);
    NuvioMoveHorizontal(&state, -1);
    assert(state.sidebarFocused);
    NuvioMoveVertical(&state, 1, 10);
    assert(state.sidebarItem == 1);
    NuvioSelect(&state, -1);
    assert(state.screen == NUVIO_SCREEN_DISCOVER);
    assert(!state.sidebarFocused);
    NuvioMoveVertical(&state, 1, 4);
    assert(state.selectedRow == 1);
    NuvioSelect(&state, 9);
    assert(state.screen == NUVIO_SCREEN_DETAILS);
    assert(state.selectedCatalogIndex == 9);
    NuvioBack(&state);
    assert(state.screen == NUVIO_SCREEN_DISCOVER);
    NuvioBack(&state);
    assert(state.screen == NUVIO_SCREEN_PROFILE);
    assert(std::strcmp(NuvioScreenName(NUVIO_SCREEN_SETTINGS), "Settings") == 0);

    NuvioCatalog catalog;
    NuvioCatalogLoadDemo(&catalog);
    assert(catalog.count == 15 && catalog.isDemo);
    assert(NuvioCatalogRowCount(&catalog) == 3);
    assert(NuvioCatalogIndexForRow(&catalog, 0, 0) == 0);
    assert(NuvioCatalogIndexForRow(&catalog, 1, 0) == 5);
    assert(NuvioCatalogIndexForRow(&catalog, 2, 0) == 10);
    assert(NuvioCatalogIndexForRow(&catalog, 3, 0) == -1);
    assert(!NuvioCatalogLoadFile(&catalog, "not_found.tsv"));
    assert(catalog.count == 15);
    if (argc > 1) {
        assert(NuvioCatalogLoadFile(&catalog, argv[1]));
        assert(!catalog.isDemo);
        assert(catalog.count == 4);
        assert(catalog.items[0].year == 2024);
        assert(std::strcmp(catalog.items[0].title, "A Real Movie") == 0);
    }

    NuvioInitialize(NULL);
    NuvioMoveHorizontal(NULL, 1);
    NuvioMoveVertical(NULL, 1, 5);
    NuvioSelect(NULL, 0);
    NuvioBack(NULL);
    assert(std::strcmp(NuvioScreenName(static_cast<NuvioScreen>(999)),
                       "Unknown screen") == 0);
    std::puts("OK: NUVIO profile navigation, browse and offline catalog");
    return 0;
}
