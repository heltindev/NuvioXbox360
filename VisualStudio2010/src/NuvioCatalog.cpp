
// NuvioCatalog.cpp - catalog parsing for the NUVIO.XEX Xbox 360 client.
// Data arrives as internal TSV from the native HTTPS backend.
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

static void ResetItem(NuvioCatalogItem &item)
{
    item.imdbId[0] = '\0';
    item.tmdbId[0] = '\0';
    item.title[0] = '\0';
    item.overview[0] = '\0';
    item.posterPath[0] = '\0';
    item.backdropPath[0] = '\0';
    item.type = 'M';
    item.year = 0;
    item.rating = 0.0f;
}

static void AddDemo(NuvioCatalog *catalog, const char *title, char type, int year)
{
    if (catalog->count >= NUVIO_MAX_CATALOG_ITEMS) return;
    NuvioCatalogItem &item = catalog->items[catalog->count++];
    ResetItem(item);
    CopyText(item.title, sizeof(item.title), title);
    item.type = type;
    item.year = year;
}

void NuvioCatalogLoadDemo(NuvioCatalog *catalog)
{
    if (!catalog) return;
    catalog->count = 0;
    catalog->isDemo = true;
    catalog->isOnline = false;
    catalog->currentPage = 1;
    catalog->totalPages = 1;
    catalog->totalResults = 0;
    catalog->railCount = 0;
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

// Parses one TSV line (already copied into a writable buffer). Returns true if valid.
static bool ParseLine(char *line, NuvioCatalogItem *out)
{
    char *id = std::strtok(line, "\t");
    char *type = std::strtok(NULL, "\t");
    char *year = std::strtok(NULL, "\t");
    char *title = std::strtok(NULL, "\r\n");
    if (!id || !type || !year || !title) return false;
    if (std::strcmp(type, "movie") && std::strcmp(type, "series")) return false;
    if (!*id || std::strlen(id) >= 128) return false;
    const int parsedYear = std::atoi(year);
    if (parsedYear != 0 && (parsedYear < 1888 || parsedYear > 2100)) return false;
    if (out) {
        ResetItem(*out);
        CopyText(out->imdbId, sizeof(out->imdbId), id);
        CopyText(out->title, sizeof(out->title), title);
        out->type = type[0] == 'm' ? 'M' : 'S';
        out->year = parsedYear;
    }
    return true;
}

// Walks the text line by line. With store == false it only counts valid rows.
static int WalkText(NuvioCatalog *catalog, const char *text, int length, bool store)
{
    int valid = 0;
    int pos = 0;
    char line[400];
    while (pos < length && valid < NUVIO_MAX_CATALOG_ITEMS) {
        int end = pos;
        while (end < length && text[end] != '\n') ++end;
        int n = end - pos;
        if (n > (int)sizeof(line) - 1) n = (int)sizeof(line) - 1;
        std::memcpy(line, text + pos, n);
        line[n] = '\0';
        pos = end + 1;
        if (line[0] == '#' || line[0] == '\r' || line[0] == '\0') continue;
        NuvioCatalogItem *slot = store ? &catalog->items[valid] : NULL;
        if (ParseLine(line, slot)) ++valid;
    }
    return valid;
}

bool NuvioCatalogLoadText(NuvioCatalog *catalog, const char *text, int length)
{
    if (!catalog || !text || length <= 0) return false;
    const bool tagged = length >= 8 && !std::memcmp(text, "#catalog", 8);
    if (!tagged && WalkText(catalog, text, length, false) == 0) return false;
    catalog->railCount = 0;
    catalog->count = 0;
    int pos = 0, active = -1;
    while (pos < length && catalog->count < NUVIO_MAX_CATALOG_ITEMS) {
        int end = pos;
        while (end < length && text[end] != '\n') ++end;
        char line[512];
        int n = end - pos;
        if (n >= (int)sizeof(line)) n = sizeof(line) - 1;
        std::memcpy(line, text + pos, n); line[n] = 0;
        pos = end + 1;
        if (!std::strncmp(line, "#row\t", 5) && catalog->railCount < NUVIO_MAX_RAILS) {
            active = catalog->railCount++;
            CopyText(catalog->rails[active].name, NUVIO_MAX_TITLE, line + 5);
            catalog->rails[active].start = catalog->count;
            catalog->rails[active].count = 0;
        } else if (line[0] != '#' && ParseLine(line, &catalog->items[catalog->count])) {
            ++catalog->count;
            if (active >= 0) ++catalog->rails[active].count;
        }
    }
    catalog->isDemo = false;
    catalog->isOnline = true;
    catalog->currentPage = 1;
    catalog->totalPages = 1;
    catalog->totalResults = catalog->count;
    return tagged || catalog->count > 0;
}

bool NuvioCatalogLoadFile(NuvioCatalog *catalog, const char *path)
{
    if (!catalog || !path) return false;
    std::FILE *file = std::fopen(path, "rb");
    if (!file) return false; // Keep the current catalog if the file is missing.
    const int maxBytes = 200000;
    char *buffer = new char[maxBytes + 1];
    const int got = (int)std::fread(buffer, 1, maxBytes, file);
    std::fclose(file);
    buffer[got > 0 ? got : 0] = '\0';
    const bool ok = NuvioCatalogLoadText(catalog, buffer, got);
    delete[] buffer;
    if (ok) catalog->isOnline = false;
    return ok;
}

void NuvioCatalogClear(NuvioCatalog *catalog)
{
    if (!catalog) return;
    catalog->count = 0;
    catalog->isDemo = false;
    catalog->isOnline = true;
    catalog->currentPage = 1;
    catalog->totalPages = 1;
    catalog->totalResults = 0;
    catalog->railCount = 0;
}

int NuvioCatalogIndexForRow(const NuvioCatalog *catalog, int row, int column)
{
    if (!catalog || row < 0 || column < 0) return -1;
    if (catalog->railCount) {
        if (row >= catalog->railCount || column >= catalog->rails[row].count) return -1;
        return catalog->rails[row].start + column;
    }
    if (column > 4) return -1;
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
    if (catalog->railCount) return catalog->railCount;
    const int moreRows = catalog->count > 10 ? (catalog->count - 10 + 4) / 5 : 0;
    return 2 + moreRows;
}
