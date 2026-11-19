#include "net.h"

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

typedef struct {
    char *buffer;
    size_t capacity;
    size_t length;
    bool overflow;
} XsDownload;

static size_t write_callback(char *data, size_t size, size_t count, void *opaque)
{
    XsDownload *download = (XsDownload *)opaque;
    if (count && size > (size_t)-1 / count) return 0;
    const size_t bytes = size * count;
    if (bytes >= download->capacity - download->length) {
        download->overflow = true;
        return 0;
    }
    memcpy(download->buffer + download->length, data, bytes);
    download->length += bytes;
    download->buffer[download->length] = '\0';
    return bytes;
}

bool xs_http_init(void)
{
    return curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
}

void xs_http_shutdown(void)
{
    curl_global_cleanup();
}

bool xs_http_get(const char *url, char *buffer, size_t capacity,
                 char *error, size_t error_capacity)
{
    if (!url || !buffer || capacity < 2 || !error || !error_capacity) return false;
    buffer[0] = '\0';
    error[0] = '\0';
    if (strncmp(url, "https://", 8) && strncmp(url, "http://", 7)) {
        snprintf(error, error_capacity, "Only HTTP and HTTPS are supported");
        return false;
    }
    CURL *curl = curl_easy_init();
    if (!curl) {
        snprintf(error, error_capacity, "Could not initialize HTTP client");
        return false;
    }
    XsDownload download = { buffer, capacity, 0, false };
    char curl_error[CURL_ERROR_SIZE] = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &download);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "game:/cacert.pem");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "NUVIO.XEX/0.3 Xbox360-RGH");
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
    const CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        snprintf(error, error_capacity, "%s", download.overflow ? "Response too large" :
                 (curl_error[0] ? curl_error : curl_easy_strerror(result)));
    }
    curl_easy_cleanup(curl);
    return result == CURLE_OK;
}
