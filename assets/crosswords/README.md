# Banco de quebra-cabecas do cruzadinha

`wordlists.json` e a fonte hand-authored (D-11/D-12) do banco compilado de
cruzadinhas do RatimOS. Este arquivo e a unica coisa que um humano precisa
editar para adicionar um quebra-cabeca novo -- o layout do grid, o math de
intersecao entre as palavras e a numeracao das dicas sao inteiramente
automatizados por `tools/generate_crossword.py`.

## Como adicionar um sexto quebra-cabeca

1. Acrescente um objeto ao array `"puzzles"` em `wordlists.json`:
   ```json
   {
     "id": "cruz-006",
     "grid_size": 9,
     "words": [
       { "answer": "PALAVRA", "clue": "dica curta em portugues, no maximo 90 caracteres" }
       // pelo menos 8 palavras no total
     ]
   }
   ```
   - `answer`: maiusculo, ASCII sem acento (a-z sem á/ç/ã/...), 2 a 15 letras.
   - `clue`: portugues, no maximo 90 caracteres, tom carinhoso -- cada dica e
     lida por uma pessoa especifica (nunca profanidade, calunia ou conteudo
     morbido).
   - `grid_size`: 9 e o alvo padrao (celulas de ~30-32px, UI-SPEC); use 11
     apenas se a lista de palavras genuinamente nao couber num grid 9x9 --
     o gerador falha alto com uma mensagem clara se a busca de
     posicionamento esgotar as tentativas, entao um erro nesse passo e o
     sinal de que a lista precisa de menos palavras, palavras com mais
     letras em comum, ou um grid maior.
2. Rode o gerador:
   ```bash
   python3 tools/generate_crossword.py \
       --in assets/crosswords/wordlists.json \
       --out-c src/ratimos/apps/jogos/cruzadinha_puzzles.c \
       --out-h src/ratimos/apps/jogos/cruzadinha_puzzles.h
   ```
3. Commit `wordlists.json` junto com os dois arquivos C regenerados. Nenhuma
   outra mudanca de codigo e necessaria -- `ratimos_cruzadinha_puzzle_count()`
   automaticamente passa a incluir o quebra-cabeca novo.

Rodar o comando acima duas vezes seguidas contra a mesma entrada produz saida
C byte-identica (determinismo: nenhuma etapa do gerador usa aleatoriedade).

## Sobre o algoritmo de posicionamento

`tools/generate_crossword.py` roda uma busca com backtracking (CSP) que
tenta cruzar cada palavra com as ja posicionadas, respeitando as regras
padrao de cruzadinha: uma celula so pode ser compartilhada por uma entrada
horizontal e uma vertical (nunca duas horizontais ou duas verticais na mesma
celula), e uma palavra nunca pode colar em outra sem de fato cruzar com ela.
Isto e trabalho de layout tedioso e totalmente automatizavel -- a autoria
real (as proprias palavras e dicas) continua sendo trabalho manual, ver
`docs/visual-identity/README.md` para a licenca do conteudo.
