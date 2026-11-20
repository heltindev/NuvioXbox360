
#ifndef NUVIO_CATALOG_H
#define NUVIO_CATALOG_H

// Catalog loaded from enabled cloud addon endpoints through native HTTPS.
// Format: IMDb ID<TAB>movie|series<TAB>year<TAB>title (ASCII/UTF-8).
enum { NUVIO_MAX_CATALOG_ITEMS = 240, NUVIO_MAX_TITLE = 72, NUVIO_MAX_OVERVIEW = 256,
       NUVIO_MAX_RAILS = 24 };

struct NuvioCatalogItem {
    char imdbId[128]; // addon ID, including tt... or tmdb:...
    char tmdbId[20];
    char type; // 'M' = movie; 'S' = series
    int year;
    char title[NUVIO_MAX_TITLE];
    char overview[NUVIO_MAX_OVERVIEW];
    float rating;
    char posterPath[128];
    char backdropPath[128];
};

struct NuvioCatalogRail {
    char name[NUVIO_MAX_TITLE];
    int start, count;
};

struct NuvioCatalog {
    NuvioCatalogItem items[NUVIO_MAX_CATALOG_ITEMS];
    int count;
    bool isDemo;
    bool isOnline;
    int currentPage;
    int totalPages;
    int totalResults;
    int railCount;
    NuvioCatalogRail rails[NUVIO_MAX_RAILS];
};

// Catalog loading functions
void NuvioCatalogLoadDemo(NuvioCatalog *catalog);
bool NuvioCatalogLoadFile(NuvioCatalog *catalog, const char *path);
// Parses TSV text. Returns false (and leaves the catalog untouched) if it has no valid rows.
bool NuvioCatalogLoadText(NuvioCatalog *catalog, const char *text, int length);

// Catalog navigation
int NuvioCatalogIndexForRow(const NuvioCatalog *catalog, int row, int column);
int NuvioCatalogRowCount(const NuvioCatalog *catalog);

// Clear catalog for new fetch
void NuvioCatalogClear(NuvioCatalog *catalog);

#endif // NUVIO_CATALOG_H
