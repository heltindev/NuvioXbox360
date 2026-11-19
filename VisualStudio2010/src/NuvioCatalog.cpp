#include "NuvioCatalog.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static void CopyText(char *target, int length, const char *source)
{
    if (!target || length <= 0) return;
    if (!source) source = "";
    std::strncpy(target, source, length - 1);
    target[length - 1] = '\0';
}

static void AddDemo(NuvioCatalog *catalog, const char *title, char type, int year)
{
    if (catalog->count >= NUVIO_MAX_CATALOG_ITEMS) return;
    NuvioCatalogItem &item = catalog->items[catalog->count++];
    CopyText(item.imdbId, sizeof(item.imdbId), "");
    CopyText(item.title, sizeof(item.title), title);
    item.type = type;
    item.year = year;
}

void NuvioCatalogLoadDemo(NuvioCatalog *catalog)
{
    if (!catalog) return;
    catalog->count = 0;
    catalog->isDemo = true;
    AddDemo(catalog, "Starlight Signal", 'M', 2026);
    AddDemo(catalog, "Beyond the Harbor", 'M', 2024);
    AddDemo(catalog, "A Quiet Horizon", 'M', 2025);
    AddDemo(catalog, "Golden September", 'M', 2023);
    AddDemo(catalog, "The Last Lantern", 'M', 2026);
    AddDemo(catalog, "Northern Echoes", 'S', 2025);
    AddDemo(catalog, "Fragments of Time", 'S', 2024);
    AddDemo(catalog, "Dreamline", 'S', 2026);
    AddDemo(catalog, "Deep Blue", 'S', 2022);
    AddDemo(catalog, "Future Days", 'S', 2026);
    AddDemo(catalog, "Paper Clouds", 'M', 2024);
    AddDemo(catalog, "Ocean of Memories", 'M', 2025);
    AddDemo(catalog, "Somewhere Here", 'M', 2023);
    AddDemo(catalog, "River of Dreams", 'S', 2026);
    AddDemo(catalog, "A Different Sky", 'S', 2025);
}

bool NuvioCatalogLoadFile(NuvioCatalog *catalog, const char *path)
{
    if (!catalog || !path) return false;
    std::FILE *file = std::fopen(path, "rb");
    if (!file) return false; // Keep the demo catalog if the file is missing.

    catalog->count = 0;
    char line[400];
    while (catalog->count < NUVIO_MAX_CATALOG_ITEMS && std::fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\r' || line[0] == '\n') continue;
        char *id = std::strtok(line, "\t");
        char *type = std::strtok(NULL, "\t");
        char *year = std::strtok(NULL, "\t");
        char *title = std::strtok(NULL, "\r\n");
        if (!id || !type || !year || !title) continue;
        if (std::strcmp(type, "movie") && std::strcmp(type, "series")) continue;
        if (std::strncmp(id, "tt", 2)) continue;
        const int parsedYear = std::atoi(year);
        if (parsedYear < 1888 || parsedYear > 2100) continue;
        NuvioCatalogItem &item = catalog->items[catalog->count++];
        CopyText(item.imdbId, sizeof(item.imdbId), id);
        CopyText(item.title, sizeof(item.title), title);
        item.type = type[0] == 'm' ? 'M' : 'S';
        item.year = parsedYear;
    }
    std::fclose(file);
    catalog->isDemo = false;
    return catalog->count > 0;
}

int NuvioCatalogIndexForRow(const NuvioCatalog *catalog, int row, int column)
{
    if (!catalog || row < 0 || column < 0 || column > 4) return -1;
    if (row == 0 || row == 1) {
        const char type = row == 0 ? 'M' : 'S';
        int matched = 0;
        for (int index = 0; index < catalog->count; ++index) {
            if (catalog->items[index].type != type) continue;
            if (matched++ == column) return index;
        }
        return -1;
    }
    const int index = (row - 2) * 5 + column + 10;
    return index < catalog->count ? index : -1;
}

int NuvioCatalogRowCount(const NuvioCatalog *catalog)
{
    if (!catalog || catalog->count <= 0) return 0;
    const int moreRows = catalog->count > 10 ? (catalog->count - 10 + 4) / 5 : 0;
    return 2 + moreRows;
}
