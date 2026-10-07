#include "jogos_app.h"
#include "../app_shell.h"
#include "../row_list.h"
#include "../../storage/content_api.h"
#include <stdio.h>
#include "jogos/conexo.h"
#include "jogos/cruzadinha.h"
#include "jogos/paciencia.h"
#include "jogos/sudoku.h"
#include "jogos/termo.h"

/*
 * Construida a cada visita e deletada ao sair (ratimos_screen_load, plano
 * 02.1-15) -- antes ficava em cache pra sempre, e todas as telas em cache
 * juntas esgotavam o heap do LVGL (deferred-items #1). O LV_EVENT_DELETE
 * (jogos_screen_deleted_cb) zera o ponteiro da tela e os handles de label.
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

/*
 * Descricao de cada jogo (subtitulo da linha, sketch 003-C), mesma ordem
 * de ratimos_game_kind_t -- entra na regra de CONSISTENCIA acima junto
 * com as tabelas de icones e callbacks.
 */
static const char * const s_game_descriptions[RATIMOS_GAME_COUNT] = {
    "grade 9x9 · fácil, médio, difícil",
    "clássico de baralho, 7 colunas",
    "adivinhe a palavra em 6 tentativas",
    "palavras cruzadas temáticas",
    "agrupe 16 palavras em 4 categorias",
};

/* Titulo + " #recolor > continuar#", com folga. */
#define RATIMOS_JOGOS_TITLE_LEN 64

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
static lv_obj_t * s_game_title_labels[RATIMOS_GAME_COUNT] = { NULL };
static char s_game_titles[RATIMOS_GAME_COUNT][RATIMOS_JOGOS_TITLE_LEN];

static void jogos_screen_deleted_cb(lv_event_t * e)
{
    (void) e;
    s_jogos_screen = NULL;
    for (size_t i = 0; i < RATIMOS_GAME_COUNT; i++) {
        s_game_title_labels[i] = NULL;
    }
}

static lv_obj_t * build_jogos_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("./home/jogos", NULL);
    lv_obj_set_style_pad_row(shell.content, RATIMOS_ROW_LIST_GAP, 0);

    ratimos_game_t games[RATIMOS_GAME_COUNT];
    size_t n = ratimos_storage_list_games(games, RATIMOS_GAME_COUNT);

    if (n == 0) {
        ratimos_row_create(shell.content, "row_empty", "nenhum jogo disponivel", "verifique a instalacao do RatimOS", NULL);
    } else {
        for (size_t i = 0; i < n; i++) {
            /* Todos os 5 jogos (02.1-01/02.1-04/02.1-05/02.1-06/02.1-07) ja
             * estao prontos -- cada linha do launcher abre sua tela real.
             * O subtitulo inicial e' a descricao do jogo;
             * refresh_jogos_rows() prefixa "continuar · " logo em seguida,
             * em toda visita, com base em progresso salvo real. */
            const char * icon_id = (i < RATIMOS_GAME_COUNT) ? s_game_icon_ids[i] : NULL;
            lv_event_cb_t click_cb = (i < RATIMOS_GAME_COUNT) ? s_game_callbacks[i] : NULL;

            const char * desc = (i < RATIMOS_GAME_COUNT) ? s_game_descriptions[i] : "";
            lv_obj_t * row = ratimos_row_create(shell.content, icon_id, games[i].title, desc, click_cb);

            if (i < RATIMOS_GAME_COUNT) {
                lv_obj_t * text_col = lv_obj_get_child(row, 1);
                s_game_title_labels[i] = lv_obj_get_child(text_col, 0);
                lv_label_set_recolor(s_game_title_labels[i], true);
                snprintf(s_game_titles[i], sizeof(s_game_titles[i]), "%s", games[i].title);
            }
        }
    }

    return shell.screen;
}

/*
 * Reavalia a probe barata ratimos_storage_has_game_state() (plano 08 Task 1)
 * pra cada jogo e atualiza os cinco labels de subtitulo ("<descricao>" ou
 * "continuar · <descricao>") -- chamada em TODA
 * visita ao launcher, logo depois de (re)construir a tela.
 */
static void refresh_jogos_rows(void)
{
    /* "continuar" vai na linha do titulo (fonte pixel, esmaecido), nao na
     * descricao: com o prefixo "continuar · " a descricao passava das ~40
     * colunas da linha e era cortada com "..." (checkpoint 02.1-14). Assim
     * a descricao fica sozinha na 2a linha e cabe inteira. */
    for (size_t i = 0; i < RATIMOS_GAME_COUNT; i++) {
        if (!s_game_title_labels[i]) {
            continue;
        }
        if (ratimos_storage_has_game_state((ratimos_game_kind_t) i)) {
            char buf[RATIMOS_JOGOS_TITLE_LEN + 32];
            snprintf(buf, sizeof(buf), "%s #a997ba > continuar#", s_game_titles[i]);
            lv_label_set_text(s_game_title_labels[i], buf);
        } else {
            lv_label_set_text(s_game_title_labels[i], s_game_titles[i]);
        }
    }
}

void ratimos_jogos_show(lv_event_t * e)
{
    (void) e;
    if (!s_jogos_screen) {
        s_jogos_screen = build_jogos_screen();
        lv_obj_add_event_cb(s_jogos_screen, jogos_screen_deleted_cb, LV_EVENT_DELETE, NULL);
    }
    refresh_jogos_rows();
    ratimos_screen_load(s_jogos_screen);
}
