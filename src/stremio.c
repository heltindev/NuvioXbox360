#include "stremio.h"

#include <stdio.h>
#include <string.h>
#include "cJSON.h"

static void copy_text(char *destination, size_t capacity, const char *source)
{
    if (capacity == 0) return;
    snprintf(destination, capacity, "%s", source ? source : "");
}

static const char *string_field(const cJSON *object, const char *key)
{
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, key);
    return cJSON_IsString(value) ? value->valuestring : NULL;
}

static bool safe_segment(const char *text)
{
    if (!text || !*text) return false;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        const unsigned char c = *p;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' ||
              c == '.' || c == ':' || c == '=')) return false;
    }
    return true;
}

bool xs_manifest_parse(const char *json, const char *url, XsManifest *out)
{
    if (!json || !url || !out) return false;
    memset(out, 0, sizeof(*out));
    const char *suffix = "/manifest.json";
    const size_t length = strlen(url), suffix_length = strlen(suffix);
    if (length <= suffix_length || length >= XS_URL_MAX ||
        strcmp(url + length - suffix_length, suffix) != 0 ||
        (strncmp(url, "https://", 8) && strncmp(url, "http://", 7))) return false;

    cJSON *root = cJSON_Parse(json);
    if (!root) return false;
    const char *name = string_field(root, "name");
    const cJSON *catalogs = cJSON_GetObjectItemCaseSensitive(root, "catalogs");
    const cJSON *resources = cJSON_GetObjectItemCaseSensitive(root, "resources");
    if (name && cJSON_IsArray(resources)) {
        const cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, resources) {
            const char *resource = cJSON_IsString(entry) ? entry->valuestring : string_field(entry, "name");
            if (resource && strcmp(resource, "stream") == 0) out->supports_stream = true;
            if (resource && strcmp(resource, "catalog") == 0) out->supports_catalog = true;
        }
    }
    if (name && cJSON_IsArray(catalogs)) {
        const cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, catalogs) {
            const char *id = string_field(entry, "id");
            const char *type = string_field(entry, "type");
            if (!safe_segment(id) || !safe_segment(type)) continue;
            if (strcmp(type, "movie") && strcmp(type, "series")) continue;
            copy_text(out->addon_name, sizeof(out->addon_name), name);
            copy_text(out->catalog_id, sizeof(out->catalog_id), id);
            copy_text(out->catalog_type, sizeof(out->catalog_type), type);
            out->supports_catalog = true;
            break;
        }
    }
    if (name) {
        copy_text(out->addon_name, sizeof(out->addon_name), name);
        memcpy(out->base_url, url, length - suffix_length);
        out->base_url[length - suffix_length] = '\0';
    }
    const bool found = name && (out->supports_stream || out->catalog_id[0]);
    cJSON_Delete(root);
    return found;
}

size_t xs_catalog_parse(const char *json, XsItem *items, size_t capacity)
{
    if (!json || !items || !capacity) return 0;
    cJSON *root = cJSON_Parse(json);
    if (!root) return 0;
    const cJSON *metas = cJSON_GetObjectItemCaseSensitive(root, "metas");
    size_t count = 0;
    if (cJSON_IsArray(metas)) {
        const cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, metas) {
            const char *id = string_field(entry, "id");
            if (!id) id = string_field(entry, "imdb_id");
            const char *name = string_field(entry, "name");
            const char *type = string_field(entry, "type");
            if (!safe_segment(id) || !name || !type ||
                (strcmp(type, "movie") && strcmp(type, "series"))) continue;
            memset(&items[count], 0, sizeof(items[count]));
            copy_text(items[count].id, sizeof(items[count].id), id);
            copy_text(items[count].title, sizeof(items[count].title), name);
            copy_text(items[count].type, sizeof(items[count].type), type);
            if (++count == capacity) break;
        }
    }
    cJSON_Delete(root);
    return count;
}

size_t xs_stream_parse(const char *json, XsStream *streams, size_t capacity)
{
    if (!json || !streams || !capacity) return 0;
    cJSON *root = cJSON_Parse(json);
    if (!root) return 0;
    const cJSON *list = cJSON_GetObjectItemCaseSensitive(root, "streams");
    size_t count = 0;
    if (cJSON_IsArray(list)) {
        const cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, list) {
            const char *url = string_field(entry, "url");
            const char *hash = string_field(entry, "infoHash");
            const bool direct = url && (!strncmp(url, "https://", 8) || !strncmp(url, "http://", 7));
            const bool torrent = (hash && *hash) || (url && !strncmp(url, "magnet:?", 8));
            if (!direct && !torrent) continue;
            if (direct && strlen(url) >= XS_URL_MAX) continue;
            const char *title = string_field(entry, "title");
            if (!title) title = string_field(entry, "name");
            memset(&streams[count], 0, sizeof(streams[count]));
            copy_text(streams[count].title, sizeof(streams[count].title), title ? title : "Fonte de streaming");
            if (direct) copy_text(streams[count].url, sizeof(streams[count].url), url);
            if (hash) copy_text(streams[count].info_hash, sizeof(streams[count].info_hash), hash);
            const cJSON *idx = cJSON_GetObjectItemCaseSensitive(entry, "fileIdx");
            streams[count].file_index = cJSON_IsNumber(idx) ? idx->valueint : -1;
            streams[count].is_torrent = !direct && torrent;
            if (++count == capacity) break;
        }
    }
    cJSON_Delete(root);
    return count;
}

bool xs_build_resource_url(const XsManifest *manifest, const char *resource,
                           const char *type, const char *id, char *out, size_t capacity)
{
    if (!manifest || !out || !capacity || !safe_segment(resource) ||
        !safe_segment(type) || !safe_segment(id)) return false;
    const int length = snprintf(out, capacity, "%s/%s/%s/%s.json",
                                manifest->base_url, resource, type, id);
    return length > 0 && (size_t)length < capacity;
}
