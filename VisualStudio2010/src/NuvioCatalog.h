#ifndef NUVIO_CATALOG_H
#define NUVIO_CATALOG_H

// Small, bounded offline catalog for the 512 MB Xbox 360.
// Format: IMDb ID<TAB>movie|series<TAB>year<TAB>title (ASCII/UTF-8).
enum { NUVIO_MAX_CATALOG_ITEMS = 240, NUVIO_MAX_TITLE = 72 };

struct NuvioCatalogItem {
    char imdbId[20];
    char type; // 'M' = movie; 'S' = series
    int year;
    char title[NUVIO_MAX_TITLE];
};

struct NuvioCatalog {
    NuvioCatalogItem items[NUVIO_MAX_CATALOG_ITEMS];
    int count;
    bool isDemo;
};

void NuvioCatalogLoadDemo(NuvioCatalog *catalog);
bool NuvioCatalogLoadFile(NuvioCatalog *catalog, const char *path);
int NuvioCatalogIndexForRow(const NuvioCatalog *catalog, int row, int column);
int NuvioCatalogRowCount(const NuvioCatalog *catalog);

#endif
