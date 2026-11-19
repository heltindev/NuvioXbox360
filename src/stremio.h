#ifndef NUVIO_XEX_STREMIO_H
#define NUVIO_XEX_STREMIO_H

#include <stdbool.h>
#include <stddef.h>

#define XS_TEXT_MAX 160
#define XS_URL_MAX 1024
#define XS_MAX_ITEMS 24
#define XS_MAX_STREAMS 12
#define XS_MAX_ADDONS 4

typedef struct {
    char id[XS_TEXT_MAX];
    char title[XS_TEXT_MAX];
    char type[24];
} XsItem;

typedef struct {
    char title[XS_TEXT_MAX];
    char url[XS_URL_MAX];
    char info_hash[128];
    int file_index;
    bool is_torrent;
} XsStream;

typedef struct {
    char addon_name[XS_TEXT_MAX];
    char base_url[XS_URL_MAX];
    char catalog_id[XS_TEXT_MAX];
    char catalog_type[24];
    bool supports_catalog;
    bool supports_stream;
} XsManifest;

bool xs_manifest_parse(const char *json, const char *manifest_url, XsManifest *out);
size_t xs_catalog_parse(const char *json, XsItem *items, size_t capacity);
size_t xs_stream_parse(const char *json, XsStream *streams, size_t capacity);
bool xs_build_resource_url(const XsManifest *manifest, const char *resource,
                           const char *type, const char *id, char *out, size_t capacity);

#endif
