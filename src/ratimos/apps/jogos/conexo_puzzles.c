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
    {
        "conexo-002",
        {
            { "animais de estimacao", RATIMOS_CONEXO_DIFF_YELLOW, { "cachorro", "gato", "hamster", "coelho" } },
            { "sentimentos", RATIMOS_CONEXO_DIFF_GREEN, { "alegria", "saudade", "carinho", "ternura" } },
            { "partes da casa", RATIMOS_CONEXO_DIFF_BLUE, { "cozinha", "quintal", "varanda", "quarto" } },
            { "doces", RATIMOS_CONEXO_DIFF_PURPLE, { "brigadeiro", "beijinho", "pudim", "sorvete" } },
        },
    },
    {
        "conexo-003",
        {
            { "clima", RATIMOS_CONEXO_DIFF_YELLOW, { "chuva", "sol", "vento", "neve" } },
            { "estacoes do ano", RATIMOS_CONEXO_DIFF_GREEN, { "verao", "outono", "inverno", "primavera" } },
            { "corpo celeste", RATIMOS_CONEXO_DIFF_BLUE, { "lua", "estrela", "planeta", "cometa" } },
            { "musica", RATIMOS_CONEXO_DIFF_PURPLE, { "melodia", "ritmo", "acorde", "refrao" } },
        },
    },
    {
        "conexo-004",
        {
            { "esportes", RATIMOS_CONEXO_DIFF_YELLOW, { "futebol", "natacao", "corrida", "ciclismo" } },
            { "profissoes", RATIMOS_CONEXO_DIFF_GREEN, { "medico", "professor", "engenheiro", "artista" } },
            { "transportes", RATIMOS_CONEXO_DIFF_BLUE, { "carro", "onibus", "bicicleta", "trem" } },
            { "planetas", RATIMOS_CONEXO_DIFF_PURPLE, { "mercurio", "venus", "marte", "jupiter" } },
        },
    },
    {
        "conexo-005",
        {
            { "sobremesas", RATIMOS_CONEXO_DIFF_YELLOW, { "pudim", "sorvete", "torta", "bolo" } },
            { "partes do dia", RATIMOS_CONEXO_DIFF_GREEN, { "manha", "tarde", "noite", "madrugada" } },
            { "moveis", RATIMOS_CONEXO_DIFF_BLUE, { "sofa", "cama", "mesa", "cadeira" } },
            { "sentimentos bons", RATIMOS_CONEXO_DIFF_PURPLE, { "gratidao", "esperanca", "coragem", "alegria" } },
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
