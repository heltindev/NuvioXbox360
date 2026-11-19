// NUVIO.XEX: independent, unofficial Xbox 360 RGH homebrew using xsdk.
// v0.3 discovers stream-only addons such as Torrentio and Brazuca.
// It DOES NOT download torrents or decode video yet.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/text.h"

#include "net.h"
#include "stremio.h"

enum { BACKGROUND = 0xFF080D19, PANEL = 0xFF152139, SELECTED = 0xFF214955,
       WHITE = 0xFFF4F8FF, MUTED = 0xFF92A5C3, ACCENT = 0xFF69EAC8,
       GOLD = 0xFFFFD381, ERROR_COLOR = 0xFFFFAEA9 };
typedef enum { SCREEN_CATALOG, SCREEN_ADDONS, SCREEN_STREAMS, SCREEN_DETAILS } XsScreen;

static XsManifest catalog_manifest;
static XsManifest addon_manifests[XS_MAX_ADDONS];
static XsItem items[XS_MAX_ITEMS];
static XsStream streams[XS_MAX_STREAMS];
static char addon_urls[XS_MAX_ADDONS][XS_URL_MAX];
static size_t item_count, addon_count, stream_count;
static int selection, selected_item, selected_addon, selected_stream;
static XsScreen screen_state = SCREEN_CATALOG;
static char catalog_manifest_url[XS_URL_MAX];
static char status_line[256] = "Aguardando rede...";
static char response[256 * 1024];
static uint32_t display_width, display_height;

// Solid panels through the GPU's public-domain bitmap font: no shader pipeline
// or external image files are required for the initial interface.
static void draw_panel(int x, int y, unsigned cells, unsigned rows, uint32_t colour)
{
    if (cells > 75) cells = 75;
    char glyphs[75 * 3 + 1];
    for (unsigned i = 0; i < cells; ++i)
        memcpy(glyphs + i * 3, "\xE2\x96\x88", 3); // U+2588 FULL BLOCK
    glyphs[cells * 3] = '\0';
    for (unsigned row = 0; row < rows; ++row)
        xsdk_text_draw(x, y + (int)(row * 20), 1, colour, glyphs);
}

static void read_configuration(void)
{
    // Cinemeta supplies titles; stream-only addons are read separately.
    snprintf(catalog_manifest_url, sizeof catalog_manifest_url, "%s", "https://v3-cinemeta.strem.io/manifest.json");
    FILE *file = fopen("game:/addon_url.txt", "rb");
    char line[XS_URL_MAX];
    if (file) {
        if (fgets(line, sizeof line, file)) {
            line[strcspn(line, "\r\n")] = '\0';
            if (!strncmp(line, "https://", 8) || !strncmp(line, "http://", 7))
                snprintf(catalog_manifest_url, sizeof catalog_manifest_url, "%s", line);
        }
        fclose(file);
    }
    file = fopen("game:/stream_addons.txt", "rb");
    if (!file) return;
    while (addon_count < XS_MAX_ADDONS && fgets(line, sizeof line, file)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strstr(line, "/manifest.json") &&
            (!strncmp(line, "https://", 8) || !strncmp(line, "http://", 7))) {
            snprintf(addon_urls[addon_count++], XS_URL_MAX, "%s", line);
        }
    }
    fclose(file);
}

static const char *addon_label(size_t index)
{
    if (addon_manifests[index].addon_name[0]) return addon_manifests[index].addon_name;
    if (strstr(addon_urls[index], "torrentio.strem.fun")) return "Torrentio";
    if (strstr(addon_urls[index], "brazuca-torrents")) return "Brazuca Torrents";
    return "Complemento Stremio";
}

static void draw_screen(void)
{
    uint32_t frame_address;
    if (!NT_SUCCESS(xsdk_present_begin(&frame_address, 5000))) return;
    xsdk_gpu_set_target(display_width, display_height);
    xsdk_text_draw(54, 23, 3, WHITE, "NUVIO");
    xsdk_text_draw(215, 44, 2, ACCENT, ".XEX");
    xsdk_text_draw(765, 48, 1, GOLD, "BETA 0.3  |  XBOX 360 RGH  |  AURORA");
    xsdk_text_draw(57, 116, 1, MUTED, "INICIO    COMPLEMENTOS    MINHA LISTA (EM BREVE)    CONFIGURACOES (EM BREVE)");
    xsdk_text_draw(55, 158, 2, WHITE,
                   screen_state == SCREEN_CATALOG ? "FILMES E SERIES" :
                   screen_state == SCREEN_ADDONS ? "ESCOLHA O COMPLEMENTO" :
                   screen_state == SCREEN_STREAMS ? "FONTES DISPONIVEIS" : "REPRODUCAO");
    draw_panel(872, 196, 35, 18, PANEL);
    xsdk_text_draw(893, 218, 1, ACCENT, "DETALHES");

    char info[100];
    if (item_count) {
        snprintf(info, sizeof info, "%.30s", items[selected_item].title);
        xsdk_text_draw(893, 268, 1, WHITE, info);
        snprintf(info, sizeof info, "TIPO: %.20s", items[selected_item].type);
        xsdk_text_draw(893, 308, 1, MUTED, info);
    }
    if (screen_state == SCREEN_ADDONS || screen_state == SCREEN_STREAMS || screen_state == SCREEN_DETAILS) {
        if (addon_count) {
            snprintf(info, sizeof info, "ADDON: %.24s", addon_label((size_t)selected_addon));
            xsdk_text_draw(893, 364, 1, GOLD, info);
        }
    }
    xsdk_text_draw(893, 444, 1, MUTED, "INTERFACE EXPERIMENTAL");
    xsdk_text_draw(893, 477, 1, MUTED, "Reproducao de video:");
    xsdk_text_draw(893, 501, 1, ERROR_COLOR, "AINDA NAO DISPONIVEL");

    if (screen_state == SCREEN_DETAILS) {
        if (streams[selected_stream].is_torrent) {
            xsdk_text_draw(65, 240, 1, WHITE, "Fonte BitTorrent identificada.");
            xsdk_text_draw(65, 287, 1, MUTED, "Falta um cliente torrent nativo para baixar as partes.");
        } else {
            xsdk_text_draw(65, 240, 1, WHITE, "Fonte HTTP identificada.");
            xsdk_text_draw(65, 287, 1, MUTED, "Falta integrar video H.264 e a saida de audio.");
        }
        xsdk_text_draw(65, 344, 1, GOLD, "Esta versao nao reproduz videos. B para voltar.");
    } else {
        const size_t count = screen_state == SCREEN_CATALOG ? item_count :
                             screen_state == SCREEN_ADDONS ? addon_count : stream_count;
        const size_t offset = selection > 8 ? (size_t)(selection - 8) : 0;
        for (size_t row = 0; row < 9 && offset + row < count; ++row) {
            const size_t i = offset + row;
            const char *name = screen_state == SCREEN_CATALOG ? items[i].title :
                               screen_state == SCREEN_ADDONS ? addon_label(i) : streams[i].title;
            const bool focused = (int)i == selection;
            const int row_y = 210 + (int)row * 40;
            if (focused) draw_panel(55, row_y - 4, 75, 2, SELECTED);
            char line[86];
            snprintf(line, sizeof line, "%s %.67s", focused ? ">" : " ", name);
            xsdk_text_draw(68, row_y + 5, 1, focused ? ACCENT : WHITE, line);
        }
        if (!count)
            xsdk_text_draw(65, 234, 1, MUTED, "Nenhum item encontrado. Confira os arquivos de configuracao.");
    }

    draw_panel(55, 605, 75, 1, PANEL);
    char footer[115];
    snprintf(footer, sizeof footer, "%.70s", status_line);
    xsdk_text_draw(62, 606, 1, GOLD, footer);
    xsdk_text_draw(57, 671, 1, MUTED, "A SELECIONAR  |  B VOLTAR  |  Y ATUALIZAR  |  BACK SAIR");
    xsdk_gpu_resolve(frame_address, display_width, display_height, BACKGROUND);
    xsdk_present_end();
}

static void loading(const char *message)
{
    snprintf(status_line, sizeof status_line, "%s", message);
    // The resolve displays the previous frame's clear, so queue more than one.
    draw_screen();
    draw_screen();
}

static void load_catalog(void)
{
    char url[XS_URL_MAX];
    item_count = 0;
    selection = 0;
    screen_state = SCREEN_CATALOG;
    if (!xs_build_resource_url(&catalog_manifest, "catalog", catalog_manifest.catalog_type,
                               catalog_manifest.catalog_id, url, sizeof url)) {
        snprintf(status_line, sizeof status_line, "URL de catalogo invalida");
        return;
    }
    loading("Buscando catalogo via HTTPS...");
    if (!xs_http_get(url, response, sizeof response, status_line, sizeof status_line)) return;
    item_count = xs_catalog_parse(response, items, XS_MAX_ITEMS);
    snprintf(status_line, sizeof status_line, "%s: %u titulos", catalog_manifest.addon_name,
             (unsigned)item_count);
    xsdk_log_write("NUVIO.XEX: %u titles loaded from %s", (unsigned)item_count, url);
}

static void load_manifest(void)
{
    loading("Conectando ao catalogo...");
    if (!xs_http_get(catalog_manifest_url, response, sizeof response, status_line, sizeof status_line)) return;
    if (!xs_manifest_parse(response, catalog_manifest_url, &catalog_manifest) ||
        !catalog_manifest.catalog_id[0]) {
        snprintf(status_line, sizeof status_line, "Manifesto sem catalogo movie/series valido");
        return;
    }
    load_catalog();
}

static void load_streams(void)
{
    if ((size_t)selected_item >= item_count || (size_t)selected_addon >= addon_count) return;
    const XsItem item = items[selected_item];
    char url[XS_URL_MAX];
    if (!xs_build_resource_url(&addon_manifests[selected_addon], "stream", item.type,
                               item.id, url, sizeof url)) {
        snprintf(status_line, sizeof status_line, "URL de stream invalida");
        return;
    }
    loading("Consultando fontes de streaming...");
    if (!xs_http_get(url, response, sizeof response, status_line, sizeof status_line)) return;
    stream_count = xs_stream_parse(response, streams, XS_MAX_STREAMS);
    selection = 0;
    screen_state = SCREEN_STREAMS;
    snprintf(status_line, sizeof status_line, "%u fontes encontradas; player ainda pendente",
             (unsigned)stream_count);
}

static void open_addon(void)
{
    if ((size_t)selection >= addon_count) return;
    selected_addon = selection;
    XsManifest *addon = &addon_manifests[selected_addon];
    if (!addon->supports_stream) {
        loading("Verificando o manifesto do complemento...");
        if (!xs_http_get(addon_urls[selected_addon], response, sizeof response,
                         status_line, sizeof status_line)) return;
        if (!xs_manifest_parse(response, addon_urls[selected_addon], addon) ||
            !addon->supports_stream) {
            snprintf(status_line, sizeof status_line, "Complemento nao possui recurso stream");
            return;
        }
    }
    load_streams();
}

static void process_button(uint16_t pressed)
{
    const int count = (int)(screen_state == SCREEN_CATALOG ? item_count :
                             screen_state == SCREEN_ADDONS ? addon_count : stream_count);
    if (pressed & INPUT_GAMEPAD_B) {
        if (screen_state == SCREEN_DETAILS) {
            screen_state = SCREEN_STREAMS;
            selection = selected_stream;
        } else if (screen_state == SCREEN_STREAMS) {
            screen_state = SCREEN_ADDONS;
            selection = selected_addon;
        } else if (screen_state == SCREEN_ADDONS) {
            screen_state = SCREEN_CATALOG;
            selection = selected_item;
        }
    } else if (pressed & INPUT_GAMEPAD_Y) {
        if (screen_state == SCREEN_CATALOG) load_manifest();
    } else if (pressed & INPUT_GAMEPAD_DPAD_UP) {
        if (screen_state != SCREEN_DETAILS && count) selection = (selection + count - 1) % count;
    } else if (pressed & INPUT_GAMEPAD_DPAD_DOWN) {
        if (screen_state != SCREEN_DETAILS && count) selection = (selection + 1) % count;
    } else if (pressed & INPUT_GAMEPAD_A) {
        if (screen_state == SCREEN_CATALOG && item_count) {
            selected_item = selection;
            selection = 0;
            screen_state = SCREEN_ADDONS;
            snprintf(status_line, sizeof status_line, "Selecione Torrentio ou Brazuca");
        } else if (screen_state == SCREEN_ADDONS) open_addon();
        else if (screen_state == SCREEN_STREAMS && stream_count) {
            selected_stream = selection;
            xsdk_log_write("NUVIO.XEX: source type=%s", streams[selection].is_torrent ? "torrent" : "http");
            screen_state = SCREEN_DETAILS;
        }
    }
}

int main(void)
{
    read_configuration();
    if (!NT_SUCCESS(xsdk_gpu_open())) return 1;
    if (!NT_SUCCESS(xsdk_present_open(XSDK_DISPLAY_720P, 2, 1,
                                       &display_width, &display_height))) {
        xsdk_gpu_close();
        return 1;
    }
    if (!NT_SUCCESS(xsdk_text_open())) {
        xsdk_present_close();
        xsdk_gpu_close();
        return 1;
    }
    if (xs_http_init()) load_manifest();
    else snprintf(status_line, sizeof status_line, "Nao foi possivel iniciar a rede");

    uint16_t previous = 0;
    for (;;) {
        INPUT_STATE input = { 0 };
        const uint16_t buttons = XamInputGetState(0, 0, &input) == ERROR_SUCCESS ?
                                 input.gamepad.buttons : 0;
        const uint16_t pressed = buttons & (uint16_t)~previous;
        previous = buttons;
        if (pressed & INPUT_GAMEPAD_BACK) break;
        if (pressed) process_button(pressed);
        draw_screen();
    }
    xs_http_shutdown();
    xsdk_text_close();
    xsdk_present_close();
    xsdk_gpu_close();
    return 0;
}
