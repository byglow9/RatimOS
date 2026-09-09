/*
 * Banco de quebra-cabecas do conexo — conteudo autoral, PT-BR (D-11).
 *
 * Cada quebra-cabeca tem 4 grupos de 4 palavras, ordenados do mais facil
 * (amarelo) ao mais dificil (roxo). Toda palavra e minuscula, sem acento e
 * com no maximo 14 caracteres para caber no buffer fixo e na tile de 72px.
 */
#include <string.h>

#include "conexo_puzzles.h"

static const ratimos_conexo_puzzle_t s_puzzles[] = {
    {
        "conexo-001",
        {
            { "frutas", RATIMOS_CONEXO_DIFF_YELLOW, { "banana", "manga", "uva", "caju" } },
            { "cores", RATIMOS_CONEXO_DIFF_GREEN, { "azul", "verde", "roxo", "cinza" } },
            { "instrumentos", RATIMOS_CONEXO_DIFF_BLUE, { "violao", "flauta", "tambor", "piano" } },
            { "coisas que voam", RATIMOS_CONEXO_DIFF_PURPLE, { "pipa", "balao", "abelha", "aviao" } },
        },
    },
};

size_t ratimos_conexo_puzzle_count(void)
{
    return sizeof(s_puzzles) / sizeof(s_puzzles[0]);
}

bool ratimos_conexo_get_puzzle(size_t index, ratimos_conexo_puzzle_t * out)
{
    if (!out || index >= ratimos_conexo_puzzle_count()) {
        return false;
    }

    memcpy(out, &s_puzzles[index], sizeof(*out));
    return true;
}
