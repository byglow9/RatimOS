/*
 * Motor do termo/dueto/quarteto (JOGOS-03) -- ver termo_engine.h.
 */
#include "termo_engine.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Tabela de modos: 1/2/4 boards, 6/7/9 tentativas -- direto dos 3 modos
 * reais do term.ooo confirmados pelo RESEARCH. A relacao aparente
 * "board_count + 5" (1+5=6, 2+5=7, 4+5=9) e uma OBSERVACAO a partir de
 * exatamente esses 3 pontos, nunca declarada como regra de design por
 * nenhuma fonte (RESEARCH assumption A7) -- por isso fica como tabela
 * explicita, nao formula, para que um 4o modo futuro nunca herde
 * silenciosamente uma regra nao verificada. */
static const uint8_t TERMO_BOARD_COUNTS[RATIMOS_TERMO_MODE_COUNT] = { 1, 2, 4 };
static const uint8_t TERMO_MAX_TRIES_TABLE[RATIMOS_TERMO_MODE_COUNT] = { 6, 7, 9 };

uint8_t ratimos_termo_board_count(ratimos_termo_mode_t mode)
{
    if (mode >= RATIMOS_TERMO_MODE_COUNT) {
        return 0;
    }
    return TERMO_BOARD_COUNTS[mode];
}

uint8_t ratimos_termo_max_tries(ratimos_termo_mode_t mode)
{
    if (mode >= RATIMOS_TERMO_MODE_COUNT) {
        return 0;
    }
    return TERMO_MAX_TRIES_TABLE[mode];
}

/* Regra duas-passadas (RESEARCH "Wordle-style feedback"): passada 1 marca
 * acertos de posicao exata E consome a contagem daquela letra na resposta;
 * passada 2, so DEPOIS, marca presenca nas posicoes restantes e so
 * enquanto a contagem daquela letra ainda for > 0. Um loop de uma passada
 * so credita de mais uma letra duplicada -- o bug mais comum documentado
 * em clones de Wordle, proibido aqui por construcao. */
void ratimos_termo_score_guess(const char * guess, const char * answer,
                                ratimos_termo_feedback_t out[RATIMOS_TERMO_WORD_LEN])
{
    int counts[26] = { 0 };
    for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
        counts[(unsigned char) answer[i] - 'a']++;
    }

    for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
        out[i] = RATIMOS_TERMO_FEEDBACK_ABSENT;
    }

    /* Passada 1: acertos de posicao exata primeiro, consumindo a contagem. */
    for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
        if (guess[i] == answer[i]) {
            out[i] = RATIMOS_TERMO_FEEDBACK_CORRECT;
            counts[(unsigned char) guess[i] - 'a']--;
        }
    }

    /* Passada 2: letras restantes, so enquanto a contagem for > 0. */
    for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
        if (out[i] == RATIMOS_TERMO_FEEDBACK_CORRECT) {
            continue;
        }
        int idx = (unsigned char) guess[i] - 'a';
        if (idx >= 0 && idx < 26 && counts[idx] > 0) {
            out[i] = RATIMOS_TERMO_FEEDBACK_PRESENT;
            counts[idx]--;
        }
    }
}

/* PRNG local (xorshift32) -- mesmo padrao de sudoku_engine.c: nunca usar
 * srand/rand da libc, para que os testes sejam deterministas e
 * independentes de ordem de chamada. Semente 0 travaria o xorshift em 0
 * para sempre; forcada para 1. */
static uint32_t termo_xorshift32(uint32_t * state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/* Semente diaria por modo: espelha a forma de daily_seed.c (dia * folga +
 * fluxo) mas recebe `day_index` diretamente em vez de chamar o relogio real
 * -- mantem o motor livre de LVGL/relogio e determinista para os testes. O
 * offset 8 fica acima dos 5 indices de jogo reservados por
 * ratimos_game_kind_t (0-4) para que nenhum dos 3 fluxos do termo colida
 * com a semente diaria de outro jogo nem entre si (RESEARCH round 2: "se
 * todos os 3 modos forem conteudo diario no MESMO dia, derive uma
 * sub-semente distinta por modo"). */
static uint32_t termo_daily_seed(uint32_t day_index, ratimos_termo_mode_t mode)
{
    return day_index * 32u + (uint32_t) (8u + (uint32_t) mode);
}

void ratimos_termo_start_daily(ratimos_termo_state_t * out, ratimos_termo_mode_t mode, uint32_t day_index)
{
    memset(out, 0, sizeof(*out));
    out->mode = mode;
    out->board_count = ratimos_termo_board_count(mode);
    out->max_tries = ratimos_termo_max_tries(mode);

    uint32_t seed = termo_daily_seed(day_index, mode);
    uint32_t rng = seed != 0 ? seed : 1u;

    size_t pool = ratimos_termo_answer_count();

    for (uint8_t b = 0; b < out->board_count; b++) {
        const char * candidate = NULL;

        /* Reject-on-duplicate: o pool tem centenas de entradas contra no
         * maximo 4 sorteios por sessao, entao colisoes sao raras e um
         * embaralhamento completo seria overhead desnecessario (RESEARCH
         * round 2). */
        for (int attempt = 0; attempt < 64; attempt++) {
            size_t idx = (pool > 0) ? (size_t) (termo_xorshift32(&rng) % pool) : 0;
            const char * try_word = ratimos_termo_answer_at(idx);
            if (!try_word) {
                continue;
            }

            bool duplicate = false;
            for (uint8_t prev = 0; prev < b; prev++) {
                if (strcmp(out->boards[prev].answer, try_word) == 0) {
                    duplicate = true;
                    break;
                }
            }

            candidate = try_word;
            if (!duplicate) {
                break;
            }
        }

        if (candidate) {
            snprintf(out->boards[b].answer, sizeof(out->boards[b].answer), "%s", candidate);
        }
    }
}

bool ratimos_termo_submit(ratimos_termo_state_t * st, const char * guess)
{
    if (!st || st->finished != 0) {
        return false;
    }
    if (!guess || strlen(guess) != RATIMOS_TERMO_WORD_LEN) {
        return false;
    }
    if (st->tries_used >= st->max_tries || st->tries_used >= RATIMOS_TERMO_MAX_TRIES) {
        return false;
    }

    char lower[RATIMOS_TERMO_WORD_LEN + 1];
    for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
        unsigned char c = (unsigned char) guess[i];
        if (!isalpha(c)) {
            return false;
        }
        lower[i] = (char) tolower(c);
    }
    lower[RATIMOS_TERMO_WORD_LEN] = '\0';

    /* Rejeita SEM consumir tentativa quando a palavra nao esta na lista de
     * palpites aceitos -- nunca penaliza o jogador por um erro de
     * digitacao ou uma palavra que o motor nao reconhece. */
    if (!ratimos_termo_is_accepted_guess(lower)) {
        return false;
    }

    snprintf(st->history[st->tries_used], sizeof(st->history[st->tries_used]), "%s", lower);

    /* O mesmo palpite compartilhado e aplicado a TODO board ainda nao
     * resolvido, e a NENHUM board ja resolvido -- um board resolvido
     * congela (nao ganha nova linha de feedback) enquanto os outros
     * continuam jogando (RESEARCH round 2, precedente Dordle/Quordle). */
    for (uint8_t b = 0; b < st->board_count; b++) {
        ratimos_termo_board_t * board = &st->boards[b];
        if (board->solved) {
            continue;
        }

        ratimos_termo_feedback_t fb[RATIMOS_TERMO_WORD_LEN];
        ratimos_termo_score_guess(lower, board->answer, fb);
        for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
            board->feedback[st->tries_used][i] = (uint8_t) fb[i];
        }
        board->guesses_made++;

        bool all_correct = true;
        for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
            if (fb[i] != RATIMOS_TERMO_FEEDBACK_CORRECT) {
                all_correct = false;
                break;
            }
        }
        if (all_correct) {
            board->solved = 1;
        }
    }

    /* Uma unica tentativa e consumida do orcamento da SESSAO, nao um
     * incremento por board -- o guess compartilhado custa uma tentativa
     * total, nao uma por tabuleiro. */
    st->tries_used++;

    bool all_solved = true;
    for (uint8_t b = 0; b < st->board_count; b++) {
        if (!st->boards[b].solved) {
            all_solved = false;
            break;
        }
    }

    if (all_solved) {
        st->finished = 1;
    } else if (st->tries_used >= st->max_tries) {
        st->finished = 2;
    }

    return true;
}
