/*
 * GERADO por tools/curate_termo_words.py -- nao editar a mao. Reexecute o
 * script se o lexico fonte mudar.
 *
 * Fonte: fserb/pt-br (https://github.com/fserb/pt-br), licenca MIT.
 * Pool de respostas (577 palavras) ranqueado por pontuacao ICF
 * (frequencia inversa de corpus) e adicionalmente aprovado numa revisao
 * manual de adequacao (ver assets/wordlists/README.md) -- nunca confie
 * apenas no filtro de frequencia para conteudo lido todo dia por uma
 * pessoa especifica.
 *
 * Plano 02.1-17: a lista de palpites aceitos (10004 palavras) inclui
 * flexoes comuns (lexico + conjugacoes + formas do ICF + plurais regulares) e
 * a tabela de exibicao (72 formas acentuadas, revisadas a mao)
 * mostra a resposta com acento no fim de jogo ("aviao" vira A-V-I-A-com-til-O).
 */

#ifndef RATIMOS_JOGOS_TERMO_WORDS_H
#define RATIMOS_JOGOS_TERMO_WORDS_H

#include <stdbool.h>
#include <stddef.h>

#define RATIMOS_TERMO_WORD_LEN 5

/* Tamanho minimo do buffer de ratimos_termo_display_form(): 5 code points
 * UTF-8 de ate' 2 bytes + NUL, com folga. */
#define RATIMOS_TERMO_DISPLAY_MAX 16

/* Quantidade de palavras no pool de respostas diarias. */
size_t ratimos_termo_answer_count(void);

/* Palavra de resposta no indice `index` (0-based), minuscula, sem
 * acento, RATIMOS_TERMO_WORD_LEN caracteres. NULL se index estiver
 * fora de [0, ratimos_termo_answer_count()). */
const char * ratimos_termo_answer_at(size_t index);

/* Verdadeiro quando `word` (RATIMOS_TERMO_WORD_LEN caracteres, aceita
 * qualquer caixa) esta na lista mais ampla de palpites validos --
 * comparacao case-insensitive via busca binaria sobre um array
 * ordenado. A entrada e' sem acento por construcao (teclado sem
 * acento): 'aurea' equivale a 'aurea' acentuada na lista. Retorna false
 * para NULL, para comprimento errado, ou para qualquer caractere fora de
 * a-z/A-Z (inclusive bytes UTF-8). */
bool ratimos_termo_is_accepted_guess(const char * word);

/* Forma de EXIBICAO de uma resposta no fim de jogo, em MAIUSCULAS e UTF-8
 * ("aviao" sai com til no A). `word`: 5 letras a-z (qualquer caixa). Sem
 * entrada na tabela, copia o ASCII em maiusculas. A saida sempre tem 5
 * code points. false para NULL, comprimento != 5, caractere fora de a-z
 * ou `out_len` < RATIMOS_TERMO_DISPLAY_MAX. */
bool ratimos_termo_display_form(const char * word, char * out, size_t out_len);

/* Acesso a tabela de formas acentuadas (teste de glifos das fontes). */
size_t ratimos_termo_display_count(void);
const char * ratimos_termo_display_at(size_t i);

#endif /* RATIMOS_JOGOS_TERMO_WORDS_H */
