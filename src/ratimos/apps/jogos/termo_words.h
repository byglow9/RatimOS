/*
 * GERADO por tools/curate_termo_words.py -- nao editar a mao. Reexecute o
 * script se o lexico fonte mudar.
 *
 * Fonte: lexico fserb/pt-br (https://github.com/fserb/pt-br), licenca MIT.
 * Pool de respostas (577 palavras) ranqueado por pontuacao ICF
 * (frequencia inversa de corpus) e adicionalmente aprovado numa revisao
 * manual de adequacao (ver assets/wordlists/README.md) -- nunca confie
 * apenas no filtro de frequencia para conteudo lido todo dia por uma
 * pessoa especifica.
 */

#ifndef RATIMOS_JOGOS_TERMO_WORDS_H
#define RATIMOS_JOGOS_TERMO_WORDS_H

#include <stdbool.h>
#include <stddef.h>

#define RATIMOS_TERMO_WORD_LEN 5

/* Quantidade de palavras no pool de respostas diarias. */
size_t ratimos_termo_answer_count(void);

/* Palavra de resposta no indice `index` (0-based), minuscula, sem
 * acento, RATIMOS_TERMO_WORD_LEN caracteres. NULL se index estiver
 * fora de [0, ratimos_termo_answer_count()). */
const char * ratimos_termo_answer_at(size_t index);

/* Verdadeiro quando `word` (RATIMOS_TERMO_WORD_LEN caracteres, aceita
 * qualquer caixa) esta na lista mais ampla de palpites validos --
 * comparacao case-insensitive via busca binaria sobre um array
 * ordenado. Retorna false para NULL, para comprimento errado, ou para
 * qualquer caractere fora de a-z/A-Z. */
bool ratimos_termo_is_accepted_guess(const char * word);

#endif /* RATIMOS_JOGOS_TERMO_WORDS_H */
