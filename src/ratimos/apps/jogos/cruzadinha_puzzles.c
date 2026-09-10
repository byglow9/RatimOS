/*
 * GERADO por tools/generate_crossword.py a partir de
 * assets/crosswords/wordlists.json -- nao editar a mao. Reexecute o script
 * se a lista de palavras/dicas fonte mudar (ver assets/crosswords/README.md).
 */

#include "cruzadinha_puzzles.h"

#include <string.h>

static const ratimos_cruzadinha_puzzle_t s_puzzles[] = {
    {
        "cruz-001",
        9, 9, 8,
        {
            { 0, 7, 5, false, 1, "abre e fecha para entrar ou sair de um comodo", "PORTA" },
            { 1, 2, 6, true, 2, "comodo da casa onde dormimos", "QUARTO" },
            { 3, 4, 4, false, 3, "movel onde comemos e colocamos objetos", "MESA" },
            { 4, 1, 7, true, 4, "movel com encosto onde nos sentamos", "CADEIRA" },
            { 4, 1, 4, false, 4, "lugar onde moramos, com quartos, sala e cozinha", "CASA" },
            { 5, 8, 4, false, 5, "comodo da casa onde recebemos visitas", "SALA" },
            { 6, 3, 6, true, 6, "abertura na parede com vidro, por onde entra a luz", "JANELA" },
            { 8, 2, 7, true, 7, "comodo da casa onde preparamos a comida", "COZINHA" },
        },
    },
    {
        "cruz-002",
        9, 9, 10,
        {
            { 0, 0, 4, true, 1, "parte colorida e perfumada de uma planta", "FLOR" },
            { 0, 3, 3, false, 2, "curso de agua doce que corre ate o mar", "RIO" },
            { 0, 7, 5, false, 3, "agua que cai do ceu em forma de gotas", "CHUVA" },
            { 2, 0, 6, true, 4, "planta grande com tronco, galhos e folhas", "ARVORE" },
            { 2, 2, 5, false, 5, "ar em movimento que sentimos no rosto", "VENTO" },
            { 4, 0, 8, true, 6, "elevacao alta de terra, maior que uma colina", "MONTANHA" },
            { 6, 0, 8, true, 7, "area com muitas arvores proximas umas das outras", "FLORESTA" },
            { 6, 1, 3, false, 8, "satelite natural da terra, visivel a noite", "LUA" },
            { 6, 5, 3, false, 9, "estrela que ilumina e aquece a terra de dia", "SOL" },
            { 8, 3, 5, true, 10, "parte verde da planta presa ao galho", "FOLHA" },
        },
    },
    {
        "cruz-003",
        9, 9, 8,
        {
            { 0, 0, 4, false, 1, "doce assado no forno, comum em aniversarios", "BOLO" },
            { 0, 2, 6, false, 2, "grao que cozinha com o arroz, prato tipico brasileiro", "FEIJAO" },
            { 1, 7, 6, false, 3, "mistura de verduras e legumes, geralmente crua", "SALADA" },
            { 2, 0, 5, true, 4, "liquido branco que vem da vaca, usado no cafe", "LEITE" },
            { 4, 1, 7, true, 5, "fruta citrica, redonda e suculenta, rica em vitamina c", "LARANJA" },
            { 4, 4, 5, false, 6, "grao branco cozido, acompanha o feijao todo dia", "ARROZ" },
            { 6, 3, 5, true, 7, "alimento doce que cresce em arvores ou plantas", "FRUTA" },
            { 8, 1, 5, true, 8, "massa redonda coberta com molho e queijo", "PIZZA" },
        },
    },
    {
        "cruz-004",
        9, 9, 8,
        {
            { 0, 1, 7, false, 1, "animal com penas e bico que geralmente voa", "PASSARO" },
            { 0, 3, 6, true, 2, "animal grande usado para montaria e corrida", "CAVALO" },
            { 0, 3, 6, false, 2, "animal de orelhas compridas que adora cenoura", "COELHO" },
            { 1, 0, 4, true, 3, "animal domestico que mia e adora dormir no sol", "GATO" },
            { 2, 6, 7, false, 4, "inseto pequeno que trabalha em grupo, vive em formigueiro", "FORMIGA" },
            { 4, 0, 8, true, 5, "melhor amigo do homem, late e abana o rabo", "CACHORRO" },
            { 6, 4, 5, true, 6, "animal que vive na agua e respira por guelras", "PEIXE" },
            { 8, 1, 6, true, 7, "animal de oito patas que tece teias", "ARANHA" },
        },
    },
    {
        "cruz-005",
        11, 11, 10,
        {
            { 0, 1, 7, true, 1, "pessoas unidas por laco de sangue ou de afeto", "FAMILIA" },
            { 0, 3, 6, false, 2, "arte feita de sons e ritmos, boa para dancar", "MUSICA" },
            { 1, 8, 5, false, 3, "objeto com paginas cheio de historias e conhecimento", "LIVRO" },
            { 2, 1, 5, false, 4, "mensagem escrita enviada para alguem querido", "CARTA" },
            { 2, 6, 5, true, 5, "pessoa querida com quem compartilhamos momentos", "AMIGO" },
            { 3, 5, 6, false, 6, "deslocamento para conhecer um novo lugar", "VIAGEM" },
            { 5, 1, 8, true, 7, "atividade que fazemos para ganhar a vida", "TRABALHO" },
            { 6, 10, 5, false, 8, "reuniao alegre para comemorar algo especial", "FESTA" },
            { 7, 3, 8, true, 9, "objeto dado com carinho em uma data especial", "PRESENTE" },
            { 10, 5, 6, true, 10, "lugar onde estudamos e aprendemos coisas novas", "ESCOLA" },
        },
    },
};

size_t ratimos_cruzadinha_puzzle_count(void)
{
    return sizeof(s_puzzles) / sizeof(s_puzzles[0]);
}

bool ratimos_cruzadinha_get_puzzle(size_t index, ratimos_cruzadinha_puzzle_t * out)
{
    if (!out || index >= ratimos_cruzadinha_puzzle_count()) {
        return false;
    }

    memcpy(out, &s_puzzles[index], sizeof(*out));
    return true;
}
