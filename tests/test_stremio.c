#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "stremio.h"

int main(void)
{
    XsManifest manifest;
    const char *manifest_json =
        "{\"name\":\"Teste Cinema\",\"catalogs\":[{\"type\":\"other\",\"id\":\"ignored\"},"
        "{\"type\":\"movie\",\"id\":\"top\"}]}";
    assert(xs_manifest_parse(manifest_json, "https://example.org/api/manifest.json", &manifest));
    assert(!strcmp(manifest.addon_name, "Teste Cinema"));
    assert(!strcmp(manifest.base_url, "https://example.org/api"));
    assert(!strcmp(manifest.catalog_id, "top"));
    assert(!xs_manifest_parse("{}", "https://example.org/manifest.json", &manifest));
    assert(!xs_manifest_parse(manifest_json, "ftp://example.org/manifest.json", &manifest));
    assert(xs_manifest_parse(manifest_json, "https://example.org/api/manifest.json", &manifest));

    const char *torrentio_manifest = "{\"name\":\"Torrentio\",\"catalogs\":[],"
        "\"resources\":[{\"name\":\"stream\",\"types\":[\"movie\",\"series\"]}]}";
    assert(xs_manifest_parse(torrentio_manifest,
        "https://torrentio.strem.fun/providers=test/manifest.json", &manifest));
    assert(manifest.supports_stream);
    assert(!manifest.catalog_id[0]);
    assert(!strcmp(manifest.base_url, "https://torrentio.strem.fun/providers=test"));
    assert(xs_manifest_parse(manifest_json, "https://example.org/api/manifest.json", &manifest));

    char url[XS_URL_MAX];
    assert(xs_build_resource_url(&manifest, "catalog", "movie", "top", url, sizeof url));
    assert(!strcmp(url, "https://example.org/api/catalog/movie/top.json"));
    assert(!xs_build_resource_url(&manifest, "stream", "movie", "foo/bar", url, sizeof url));

    XsItem items[XS_MAX_ITEMS];
    const char *catalog_json = "{\"metas\":["
        "{\"id\":\"tt123\",\"name\":\"Filme um\",\"type\":\"movie\"},"
        "{\"id\":\"tt456\",\"name\":\"Filme dois\",\"type\":\"movie\"},"
        "{\"id\":\"x/y\",\"name\":\"Invalido\",\"type\":\"movie\"}]}";
    assert(xs_catalog_parse(catalog_json, items, XS_MAX_ITEMS) == 2);
    assert(!strcmp(items[0].title, "Filme um"));
    const char *cinemeta_json = "{\"metas\":[{\"imdb_id\":\"tt789\",\"name\":\"Cinemeta\",\"type\":\"movie\"}]}";
    assert(xs_catalog_parse(cinemeta_json, items, XS_MAX_ITEMS) == 1);
    assert(!strcmp(items[0].id, "tt789"));
    assert(xs_catalog_parse(catalog_json, items, 1) == 1);
    assert(xs_catalog_parse("{invalid", items, XS_MAX_ITEMS) == 0);

    XsStream streams[XS_MAX_STREAMS];
    const char *stream_json = "{\"streams\":["
        "{\"title\":\"Video direto\",\"url\":\"https://media.example.org/video.mp4\"},"
        "{\"title\":\"Torrent\",\"infoHash\":\"abcdef0123456789\",\"fileIdx\":2},"
        "{\"name\":\"Outro stream\",\"url\":\"http://media.example.org/clip.mp4\"},"
        "{\"url\":\"file:///etc/passwd\"}]}";
    assert(xs_stream_parse(stream_json, streams, XS_MAX_STREAMS) == 3);
    assert(!strcmp(streams[0].title, "Video direto"));
    assert(streams[1].is_torrent);
    assert(!strcmp(streams[1].info_hash, "abcdef0123456789"));
    assert(streams[1].file_index == 2);
    assert(!strcmp(streams[2].title, "Outro stream"));
    assert(xs_stream_parse(stream_json, streams, 1) == 1);

    puts("OK: stream-only addons, catalog, HTTP and torrent metadata");
    return 0;
}
