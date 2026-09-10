#include "jogos_app.h"
#include "../app_shell.h"
#include "../row_list.h"
#include "../../storage/content_api.h"
#include "jogos/conexo.h"
#include "jogos/cruzadinha.h"
#include "jogos/paciencia.h"
#include "jogos/sudoku.h"
#include "jogos/termo.h"

/*
 * Cache-once, like ratimos_cartas_show() (01-01) / ratimos_home_screen_show():
 * games are only ever indexed once, synchronously, during the splash's staged
 * init (D-10), so building this screen's row list once and reusing it forever
 * is both correct and required -- without caching, every visit builds a
 * brand-new, never-freed lv_obj_t screen, which exhausts LVGL's builtin heap
 * after only a handful of visits (same leak already fixed in cartas_app.c
 * during 01-01; deferred here for 01-03 per deferred-items.md).
 */
static lv_obj_t * s_jogos_screen = NULL;

/*
 * Icone pixel-art de cada jogo (VISUAL-01/D-07), indexado por
 * ratimos_game_kind_t -- substitui o selo compartilhado de letra unica que
 * toda linha usava antes. Fica ao lado da tabela de callbacks logo abaixo
 * para que as duas nunca fiquem fora de ordem uma da outra.
 *
 * CONSISTENCIA ENTRE 4 TABELAS: se um sexto jogo for adicionado no futuro,
 * estas DUAS tabelas (icones + callbacks, aqui) precisam mudar junto com
 * mais DUAS outras que tambem codificam a mesma ordem: o enum
 * ratimos_game_kind_t + a tabela de caminhos de save em
 * src/storage/game_state.c, a lista de titulos compilada em
 * src/storage/games.c, e a tabela de ids de desbloqueio exclusivo em
 * src/ratimos/progression.c. Nenhuma delas pode mudar sozinha.
 */
static const char * const s_game_icon_ids[RATIMOS_GAME_COUNT] = {
    "game_sudoku",
    "game_paciencia",
    "game_termo",
    "game_cruzadinha",
    "game_conexo",
};

static const lv_event_cb_t s_game_callbacks[RATIMOS_GAME_COUNT] = {
    ratimos_sudoku_show,
    ratimos_paciencia_show,
    ratimos_termo_show,
    ratimos_cruzadinha_show,
    ratimos_conexo_show,
};

/*
 * Handles dos cinco labels de subtitulo, na mesma ordem de
 * ratimos_game_kind_t -- pegos pela posicao estrutural conhecida da linha
 * (ver ratimos_row_create() em row_list.c: filho 1 de `row` = text_col,
 * filho 1 de text_col = subtitulo, quando letter/icon_id e subtitle sao
 * ambos nao-NULL) em vez de mudar a API compartilhada por um unico
 * chamador -- mesmo padrao ja usado por termo.c pro titulo da sectionbar.
 * Ficam NULL quando a tela cai no estado vazio defensivo (n == 0).
 */
static lv_obj_t * s_game_subtitle_labels[RATIMOS_GAME_COUNT] = { NULL };

static lv_obj_t * build_jogos_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("jogos", "toque para abrir");

    ratimos_game_t games[RATIMOS_GAME_COUNT];
    size_t n = ratimos_storage_list_games(games, RATIMOS_GAME_COUNT);

    if (n == 0) {
        ratimos_row_create(shell.content, "!", "nenhum jogo disponivel", "verifique a instalacao do RatimOS", NULL);
    } else {
        for (size_t i = 0; i < n; i++) {
            /* Todos os 5 jogos (02.1-01/02.1-04/02.1-05/02.1-06/02.1-07) ja
             * estao prontos -- cada linha do launcher abre sua tela real.
             * O texto inicial do subtitulo e' sempre "jogar";
             * refresh_jogos_rows() corrige para "continuar" logo em
             * seguida, em toda visita, com base em progresso salvo real. */
            const char * icon_id = (i < RATIMOS_GAME_COUNT) ? s_game_icon_ids[i] : NULL;
            lv_event_cb_t click_cb = (i < RATIMOS_GAME_COUNT) ? s_game_callbacks[i] : NULL;

            lv_obj_t * row = ratimos_row_create(shell.content, icon_id, games[i].title, "jogar", click_cb);

            if (i < RATIMOS_GAME_COUNT) {
                lv_obj_t * text_col = lv_obj_get_child(row, 1);
                s_game_subtitle_labels[i] = lv_obj_get_child(text_col, 1);
            }
        }
    }

    return shell.screen;
}

/*
 * Reavalia a probe barata ratimos_storage_has_game_state() (plano 08 Task 1)
 * pra cada jogo e atualiza os cinco labels de subtitulo -- chamada em TODA
 * visita ao launcher. A tela e' cacheada uma unica vez (build_jogos_screen
 * so roda no primeiro ratimos_jogos_show()), entao sem isto o rotulo
 * "jogar" ficaria congelado mesmo depois da jogadora voltar de um jogo em
 * que acabou de salvar progresso real.
 */
static void refresh_jogos_rows(void)
{
    for (size_t i = 0; i < RATIMOS_GAME_COUNT; i++) {
        if (!s_game_subtitle_labels[i]) {
            continue;
        }
        bool has_progress = ratimos_storage_has_game_state((ratimos_game_kind_t) i);
        lv_label_set_text(s_game_subtitle_labels[i], has_progress ? "continuar" : "jogar");
    }
}

void ratimos_jogos_show(lv_event_t * e)
{
    (void) e;
    if (!s_jogos_screen) {
        s_jogos_screen = build_jogos_screen();
    }
    refresh_jogos_rows();
    lv_screen_load(s_jogos_screen);
}
