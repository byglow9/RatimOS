/*
 * Suite de memoria de navegacao (plano 02.1-15, deferred-items #1).
 *
 * Reproduz a sessao que crashava o simulador por esgotamento do heap do
 * LVGL (512KB, lv_conf.h): home -> jogos -> cada jogo (termo trocando de
 * modo) -> home -> apps de topo -> castelo, 3 voltas, usando as entradas
 * `ratimos_*_show()` REAIS e o botao "voltar" real da bottombar. Mede o
 * heap com ratimos_heap_used() (src/ratimos/heap_probe.h, mesmo numero de
 * lv_mem_monitor()) e exige:
 *   - nenhum crash;
 *   - heap estavel entre voltas (volta 3 <= volta 2 + 2%);
 *   - sair de uma tela devolve o heap (a tela nao fica em cache);
 *   - custo da tela da cruzadinha < 80KB (Task 3).
 *
 * Headless igual a test_game_screens.c: display 320x480 com flush que so'
 * libera o buffer; cada passo renderiza um frame de verdade (lv_refr_now),
 * entao alocacoes de desenho entram na conta.
 *
 * Isolamento: main() faz chdir() pra um diretorio temporario antes de
 * qualquer storage -- os saves nunca tocam o progresso real do simulador.
 *
 * REUSO (plano 02.1-16, temas x boot): o padrao e'
 *   go_home(); size_t base = ratimos_heap_used();
 *   <mostra a tela>; render(); size_t cost = ratimos_heap_used() - base;
 *   go_home(); TEST_ASSERT(ratimos_heap_used() <= base + RETAIN_SLACK);
 * Este arquivo NAO ganha pool extra de heap (lv_mem_add_pool): mede o
 * mesmo LV_MEM_SIZE que o simulador usa.
 */
#include <unity.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lvgl.h"
#include "ratimos/heap_probe.h"
#include "ratimos/theme.h"
#include "ratimos/home_screen.h"
#include "ratimos/apps/album_app.h"
#include "ratimos/apps/cartas_app.h"
#include "ratimos/apps/castelo_app.h"
#include "ratimos/apps/config_app.h"
#include "ratimos/apps/jogos_app.h"
#include "ratimos/apps/musica_app.h"
#include "ratimos/apps/jogos/conexo.h"
#include "ratimos/apps/jogos/cruzadinha.h"
#include "ratimos/apps/jogos/paciencia.h"
#include "ratimos/apps/jogos/sudoku.h"
#include "ratimos/apps/jogos/termo.h"
#include "storage/content_api.h"

static uint8_t s_disp_buf[RATIMOS_SCREEN_W * RATIMOS_SCREEN_H * 2]; /* LV_COLOR_DEPTH 16 */

/* Folga ao voltar pra home: fragmentacao do TLSF + buffers internos do
 * LVGL que nao sao de nenhuma tela. Uma tela de app/jogo em cache custa
 * bem mais que isto (a menor, config, ~10KB). */
#define RETAIN_SLACK (4u * 1024u)

/* Meta do plano 02.1-15 Task 3 (antes: ~194KB). */
#define CRUZADINHA_BUDGET (80u * 1024u)

static void headless_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    (void) area;
    (void) px_map;
    lv_display_flush_ready(disp);
}

void setUp(void) {}
void tearDown(void) {}

typedef void (*show_fn_t)(lv_event_t * e);

typedef struct {
    show_fn_t show;
    const char * name;
    bool is_game; /* "voltar" de jogo leva a ./home/jogos */
} screen_entry_t;

static const screen_entry_t GAMES[] = {
    { ratimos_sudoku_show, "sudoku", true },
    { ratimos_paciencia_show, "paciencia", true },
    { ratimos_termo_show, "termo", true },
    { ratimos_cruzadinha_show, "cruzadinha", true },
    { ratimos_conexo_show, "conexo", true },
};
#define GAME_N (sizeof(GAMES) / sizeof(GAMES[0]))

static const screen_entry_t APPS[] = {
    { ratimos_jogos_show, "jogos", false },
    { ratimos_musica_show, "musica", false },
    { ratimos_album_show, "album", false },
    { ratimos_cartas_show, "cartas", false },
    { ratimos_config_show, "config", false },
    { ratimos_castelo_show, "castelo", false },
};
#define APP_N (sizeof(APPS) / sizeof(APPS[0]))

/* Layout + um frame real da tela ativa. */
static void render(void)
{
    lv_obj_update_layout(lv_screen_active());
    lv_refr_now(NULL);
}

static void go_home(void)
{
    ratimos_home_screen_show(NULL);
    render();
}

/* Texto da sectionbar (filho 2 da tela -> label 0) da tela ativa. */
static const char * active_path(void)
{
    lv_obj_t * sec = lv_obj_get_child(lv_screen_active(), 2);
    return lv_label_get_text(lv_obj_get_child(sec, 0));
}

/* Clica no "voltar" da bottombar (filho 4 da tela -> botao 0). */
static void press_back(void)
{
    lv_obj_t * bottombar = lv_obj_get_child(lv_screen_active(), 4);
    lv_obj_t * back = lv_obj_get_child(bottombar, 0);
    TEST_ASSERT_NOT_NULL(back);
    lv_obj_send_event(back, LV_EVENT_CLICKED, NULL);
    render();
}

static bool subtree_has_text(lv_obj_t * obj, const char * needle)
{
    if (lv_obj_get_class(obj) == &lv_label_class && strstr(lv_label_get_text(obj), needle)) {
        return true;
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(obj); i++) {
        if (subtree_has_text(lv_obj_get_child(obj, (int32_t) i), needle)) {
            return true;
        }
    }
    return false;
}

/* Painel do dialogo "comecar de novo?" visivel na tela ativa, ou NULL. */
static lv_obj_t * visible_confirm_panel(void)
{
    lv_obj_t * scr = lv_screen_active();
    for (uint32_t i = 0; i < lv_obj_get_child_count(scr); i++) {
        lv_obj_t * c = lv_obj_get_child(scr, (int32_t) i);
        if (lv_obj_has_flag(c, LV_OBJ_FLAG_FLOATING) && lv_obj_get_child_count(c) == 1) {
            lv_obj_t * panel = lv_obj_get_child(c, 0);
            if (subtree_has_text(panel, "comecar de novo?") && ratimos_modal_is_visible(panel)) {
                return panel;
            }
        }
    }
    return NULL;
}

/* Termo: content = [0 erro, 1 pilulas, ...]. Clica a pilula do modo e
 * confirma o dialogo se ele aparecer. */
static void termo_switch_mode(int mode)
{
    lv_obj_t * content = lv_obj_get_child(lv_screen_active(), 3);
    lv_obj_t * pill = lv_obj_get_child(lv_obj_get_child(content, 1), mode);
    lv_obj_send_event(pill, LV_EVENT_CLICKED, NULL);
    lv_obj_t * panel = visible_confirm_panel();
    if (panel) {
        lv_obj_t * actions = lv_obj_get_child(panel, 1);
        lv_obj_send_event(lv_obj_get_child(actions, 1), LV_EVENT_CLICKED, NULL);
    }
    render();
}

static void log_heap(int lap, const char * tag)
{
    fprintf(stderr, "lap %d %-12s used %7u / %u (%u%%)\n", lap, tag, (unsigned) ratimos_heap_used(),
            (unsigned) ratimos_heap_total(), (unsigned) ratimos_heap_used_pct());
}

/* Uma volta da sessao do deferred-items #1. Devolve o uso ao fim (na home). */
static size_t run_lap(int lap)
{
    go_home();

    for (size_t g = 0; g < GAME_N; g++) {
        ratimos_jogos_show(NULL);
        render();
        GAMES[g].show(NULL);
        render();
        TEST_ASSERT_TRUE_MESSAGE(strncmp(active_path(), "./home/jogos/", 13) == 0, GAMES[g].name);
        log_heap(lap, GAMES[g].name);

        if (GAMES[g].show == ratimos_termo_show) {
            termo_switch_mode(1); /* dueto */
            termo_switch_mode(2); /* quarteto */
            termo_switch_mode(0); /* volta pro termo */
            log_heap(lap, "termo modes");
        }

        press_back(); /* -> ./home/jogos */
        TEST_ASSERT_EQUAL_STRING_MESSAGE("./home/jogos", active_path(), GAMES[g].name);
        press_back(); /* -> ./home */
        TEST_ASSERT_EQUAL_STRING_MESSAGE("./home", active_path(), GAMES[g].name);
    }

    for (size_t a = 1; a < APP_N; a++) { /* jogos ja visitado acima */
        APPS[a].show(NULL);
        render();
        log_heap(lap, APPS[a].name);
        press_back();
        TEST_ASSERT_EQUAL_STRING_MESSAGE("./home", active_path(), APPS[a].name);
    }

    size_t used = ratimos_heap_used();
    log_heap(lap, "end (home)");
    return used;
}

/*
 * Must-have 1: a sessao inteira, 3 voltas, sem crash e sem crescimento
 * entre a 2a e a 3a volta (volta 1 aquece caches do LVGL).
 */
void test_full_session_three_laps_is_stable(void)
{
    size_t lap_used[3];
    for (int lap = 0; lap < 3; lap++) {
        lap_used[lap] = run_lap(lap + 1);
    }
    fprintf(stderr, "session peak: %u / %u\n", (unsigned) ratimos_heap_peak(), (unsigned) ratimos_heap_total());
    TEST_ASSERT_TRUE_MESSAGE(lap_used[2] <= lap_used[1] + lap_used[1] / 50, "heap grew between lap 2 and lap 3");
}

/*
 * Must-have 2: ao sair de uma tela de app/jogo ela e' deletada -- o heap
 * volta pro nivel da home (so' a home fica em cache). Tambem imprime o
 * custo de cada tela (usado no SUMMARY do 02.1-15).
 */
void test_leaving_any_screen_releases_its_heap(void)
{
    go_home();
    size_t base = ratimos_heap_used();

    for (size_t i = 0; i < GAME_N + APP_N; i++) {
        const screen_entry_t * s = (i < GAME_N) ? &GAMES[i] : &APPS[i - GAME_N];

        s->show(NULL);
        render();
        size_t cost = ratimos_heap_used() - base;
        fprintf(stderr, "screen cost %-12s %7u bytes\n", s->name, (unsigned) cost);

        go_home();
        size_t after = ratimos_heap_used();
        char msg[64];
        snprintf(msg, sizeof(msg), "%s stayed cached (+%d bytes)", s->name, (int) (after - base));
        TEST_ASSERT_TRUE_MESSAGE(after <= base + RETAIN_SLACK, msg);
    }
}

/*
 * Must-have 3 (Task 3): a tela da cruzadinha custa < 80KB (antes ~170KB:
 * grade de 121 lv_obj + 242 labels = ~125KB, lista de dicas montada
 * escondida = ~30KB). Abrir e fechar "dicas" nao deixa nada pra tras.
 */
void test_cruzadinha_screen_costs_under_80kb(void)
{
    go_home();
    size_t base = ratimos_heap_used();
    ratimos_cruzadinha_show(NULL);
    render();
    size_t cost = ratimos_heap_used() - base;
    fprintf(stderr, "cruzadinha screen cost: %u bytes (budget %u)\n", (unsigned) cost, (unsigned) CRUZADINHA_BUDGET);
    TEST_ASSERT_TRUE_MESSAGE(cost < CRUZADINHA_BUDGET, "cruzadinha screen over 80KB");

    /* content = [0 erro, 1 dica, 2 acoes (proxima/dicas/novo jogo), ...] */
    lv_obj_t * actions = lv_obj_get_child(lv_obj_get_child(lv_screen_active(), 3), 2);
    lv_obj_send_event(lv_obj_get_child(actions, 1), LV_EVENT_CLICKED, NULL); /* dicas */
    render();
    size_t open_cost = ratimos_heap_used() - base;
    fprintf(stderr, "cruzadinha with clue list open: %u bytes\n", (unsigned) open_cost);
    lv_obj_t * scrim = lv_obj_get_child(lv_screen_active(), -1);
    lv_obj_send_event(lv_obj_get_child(lv_obj_get_child(scrim, 0), 0), LV_EVENT_CLICKED, NULL); /* fechar */
    render();
    TEST_ASSERT_TRUE_MESSAGE(ratimos_heap_used() <= base + cost + RETAIN_SLACK, "clue list leaked after close");

    go_home();
    TEST_ASSERT_TRUE(ratimos_heap_used() <= base + RETAIN_SLACK);
}

/* ------------------------------------------------------------------------
 * Clique de verdade (indev pointer) no "voltar": a tela antiga e' deletada
 * DENTRO do callback de clique de um filho dela -- o LVGL tem que sair
 * limpo do processamento do indev.
 * ------------------------------------------------------------------------ */

static lv_point_t s_ptr_pos;
static bool s_ptr_pressed;

static void ptr_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    (void) indev;
    data->point = s_ptr_pos;
    data->state = s_ptr_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void pump_ms(int ms)
{
    for (int t = 0; t < ms; t += 10) {
        lv_tick_inc(10);
        lv_timer_handler();
    }
}

static void tap_obj(lv_obj_t * obj)
{
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    s_ptr_pos.x = (a.x1 + a.x2) / 2;
    s_ptr_pos.y = (a.y1 + a.y2) / 2;
    s_ptr_pressed = true;
    pump_ms(60);
    s_ptr_pressed = false;
    pump_ms(60);
}

void test_real_pointer_tap_on_voltar_deletes_old_screen_cleanly(void)
{
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, ptr_read_cb);

    go_home();
    for (size_t g = 0; g < GAME_N; g++) {
        ratimos_jogos_show(NULL);
        GAMES[g].show(NULL);
        render();
        lv_obj_t * back = lv_obj_get_child(lv_obj_get_child(lv_screen_active(), 4), 0);
        tap_obj(back);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("./home/jogos", active_path(), GAMES[g].name);
        back = lv_obj_get_child(lv_obj_get_child(lv_screen_active(), 4), 0);
        tap_obj(back);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("./home", active_path(), GAMES[g].name);
    }

    lv_indev_delete(indev);
}

int main(void)
{
    char tmpl[] = "/tmp/ratimos_nav_memory_XXXXXX";
    if (mkdtemp(tmpl) == NULL || chdir(tmpl) != 0) {
        return 1;
    }

    lv_init();
    lv_display_t * disp = lv_display_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_display_set_buffers(disp, s_disp_buf, NULL, sizeof(s_disp_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, headless_flush_cb);

    /* Mesmos passos de boot da splash (splash.c s_steps). */
    ratimos_storage_mount();
    ratimos_storage_index_letters();
    ratimos_storage_index_photos();
    ratimos_storage_index_tracks();
    ratimos_storage_index_games();
    ratimos_storage_index_settings();
    ratimos_storage_index_game_state();

    UNITY_BEGIN();
    RUN_TEST(test_full_session_three_laps_is_stable);
    RUN_TEST(test_leaving_any_screen_releases_its_heap);
    RUN_TEST(test_cruzadinha_screen_costs_under_80kb);
    RUN_TEST(test_real_pointer_tap_on_voltar_deletes_old_screen_cleanly);
    return UNITY_END();
}
