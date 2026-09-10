#!/usr/bin/env python3
"""
Gera o banco de quebra-cabecas do cruzadinha (JOGOS-04) a partir de listas de
palavras+dicas hand-authored em assets/crosswords/wordlists.json.

Isto e trabalho de autoria com um esqueleto scriptado, igual a
tools/curate_termo_words.py: a parte tediosa (posicionamento no grid, math de
intersecao, numeracao das dicas) e automatizada por uma busca com
backtracking; a parte criativa (as proprias palavras e dicas) e sempre
hand-authored -- ver secao 2 de "Follow-up Research (Round 2)" em
02.1-RESEARCH.md.

Algoritmo (backtracking/CSP, generico o bastante para qualquer lista de
palavras em qualquer lingua -- opera sobre strings e coordenadas, nao sobre
linguistica):
  1. Ordena as palavras do quebra-cabeca da mais longa para a mais curta
     (empate pela ordem original, para reprodutibilidade).
  2. Posiciona a primeira (mais longa) centralizada no grid, na horizontal.
  3. Para cada palavra seguinte, gera todo posicionamento valido que
     compartilha pelo menos uma letra com o grid ja preenchido -- valido
     significa: dentro dos limites, toda celula de intersecao bate a mesma
     letra, e nenhuma celula nova toca (na direcao perpendicular) uma celula
     ja ocupada por outra palavra, o que evitaria criar uma palavra acidental
     nao declarada.
  4. A cada passo, o backtracking tenta QUALQUER palavra ainda nao colocada
     (nao so a proxima da lista ordenada por tamanho) contra o grid parcial,
     em ordem deterministica (palavra mais longa primeiro no empate de
     candidatos, depois mais intersecoes, depois posicao) -- uma ordem fixa
     de colocacao sozinha trava cedo demais em listas de palavras reais,
     onde uma palavra do meio pode so encontrar cruzamento valido se
     colocada antes ou depois de outra. Se nenhuma palavra restante tem
     posicionamento valido, faz backtrack (desfaz a ultima palavra colocada
     e tenta o proximo candidato dela).
  5. Um orcamento de nos (--node-budget) limita a busca -- estourar o
     orcamento falha ALTO com o id do quebra-cabeca responsavel, nunca emite
     um grid parcial.

Ao final, roda a passada de numeracao padrao (varredura esquerda->direita,
cima->baixo; uma celula recebe o proximo numero se inicia uma entrada
horizontal e/ou vertical, com uma celula que inicia as duas compartilhando um
unico numero) e emite o banco compilado em C.

Uso:
  python3 tools/generate_crossword.py \
      --in assets/crosswords/wordlists.json \
      --out-c src/ratimos/apps/jogos/cruzadinha_puzzles.c \
      --out-h src/ratimos/apps/jogos/cruzadinha_puzzles.h \
      [--node-budget 500000]

Determinismo: nenhuma aleatoriedade e usada em nenhum passo -- rodar o script
duas vezes seguidas contra a mesma entrada produz saida C byte-identica.
"""
import argparse
import json
import sys
from pathlib import Path

MAX_WORDS = 16
MAX_DIM = 11
MAX_ANSWER_LEN = 15  # cabe em char answer[16] (+ terminador nulo)
DEFAULT_NODE_BUDGET = 2000000


class CrosswordError(Exception):
    pass


# ---------------------------------------------------------------------------
# Validacao de entrada
# ---------------------------------------------------------------------------

def validate_answer(answer: str, puzzle_id: str) -> str:
    if not answer or not answer.isascii() or not answer.isalpha() or answer != answer.upper():
        raise CrosswordError(
            f"puzzle {puzzle_id}: resposta '{answer}' precisa ser ASCII maiusculo, "
            f"sem acento, apenas letras A-Z"
        )
    if len(answer) > MAX_ANSWER_LEN:
        raise CrosswordError(
            f"puzzle {puzzle_id}: resposta '{answer}' tem {len(answer)} letras, "
            f"maximo permitido e {MAX_ANSWER_LEN}"
        )
    if len(answer) < 2:
        raise CrosswordError(f"puzzle {puzzle_id}: resposta '{answer}' e curta demais")
    return answer


def validate_clue(clue: str, answer: str, puzzle_id: str) -> str:
    if not clue or len(clue) > 90:
        raise CrosswordError(
            f"puzzle {puzzle_id}: dica de '{answer}' precisa ter entre 1 e 90 caracteres "
            f"(tem {len(clue)})"
        )
    if not clue.isascii():
        raise CrosswordError(
            f"puzzle {puzzle_id}: dica de '{answer}' precisa ser ASCII (sem acento) -- "
            f"as fontes bitmap LVGL do dispositivo so cobrem ASCII (ver "
            f"conexo_puzzles.c e a convencao 'sem acento' do crossword README)"
        )
    # WR-01: len() acima conta CARACTERES Python, nao bytes -- uma dica com
    # exatamente 90 caracteres ASCII sempre cabe (90 <= 95), mas a checagem
    # de bytes UTF-8 fica aqui como cinto-e-suspensorio caso o limite de
    # caracteres suba no futuro sem alguem revisitar o tamanho do buffer C
    # fixo (`char clue_text[96]`, 95 bytes uteis + terminador nulo).
    encoded_len = len(clue.encode("utf-8"))
    if encoded_len > 95:
        raise CrosswordError(
            f"puzzle {puzzle_id}: dica de '{answer}' ocupa {encoded_len} bytes UTF-8, "
            f"maximo permitido e 95 (buffer C fixo char clue_text[96])"
        )
    return clue


# ---------------------------------------------------------------------------
# Grid + busca com backtracking
# ---------------------------------------------------------------------------

def make_grid(w: int, h: int):
    return [[None for _ in range(w)] for _ in range(h)]


def can_place(grid, word: str, row: int, col: int, is_across: bool, w: int, h: int):
    """Retorna a contagem de intersecoes se o posicionamento e valido, ou
    None se invalido (fora dos limites, letra conflitante, ou tocaria uma
    palavra ja colocada de um jeito que criaria uma palavra acidental)."""
    length = len(word)

    if is_across:
        if col < 0 or col + length > w or row < 0 or row >= h:
            return None
        before = (col - 1, row)
        after = (col + length, row)
    else:
        if row < 0 or row + length > h or col < 0 or col >= w:
            return None
        before = (col, row - 1)
        after = (col, row + length)

    # Celula imediatamente antes/depois da palavra precisa estar vazia --
    # senao a palavra colaria em outra e formaria uma sequencia nao
    # declarada.
    bx, by = before
    if 0 <= bx < w and 0 <= by < h and grid[by][bx] is not None:
        return None
    ax, ay = after
    if 0 <= ax < w and 0 <= ay < h and grid[ay][ax] is not None:
        return None

    intersections = 0
    for i, ch in enumerate(word):
        r = row if is_across else row + i
        c = col + i if is_across else col
        cell = grid[r][c]
        if cell is not None:
            if cell["letter"] != ch:
                return None
            # Uma celula so pode ser compartilhada por UMA entrada
            # horizontal e UMA vertical -- duas palavras horizontais (ou
            # duas verticais) nunca podem comecar/ocupar a mesma celula,
            # senao viram a mesma sequencia de letras disfarcada de duas
            # entradas diferentes (bug real encontrado durante o
            # desenvolvimento deste script: "FLOR" e "FLORESTA" começando
            # na mesma celula da mesma linha).
            if is_across and cell["across"]:
                return None
            if not is_across and cell["down"]:
                return None
            intersections += 1
        else:
            # Celula nova: as vizinhas perpendiculares precisam estar vazias
            # (senao a nova palavra tocaria uma palavra paralela existente
            # sem de fato cruzar com ela).
            if is_across:
                if (r - 1 >= 0 and grid[r - 1][c] is not None) or \
                   (r + 1 < h and grid[r + 1][c] is not None):
                    return None
            else:
                if (c - 1 >= 0 and grid[r][c - 1] is not None) or \
                   (c + 1 < w and grid[r][c + 1] is not None):
                    return None

    return intersections


def place(grid, word: str, row: int, col: int, is_across: bool):
    """Escreve `word` no grid. Cada celula guarda a letra mais quais
    orientacoes (across/down) ja a reivindicam. Retorna, para cada celula
    tocada, se ela e nova (para apagar por completo no backtrack) ou se ja
    existia (para so reverter a flag de orientacao que esta chamada acabou
    de marcar, preservando a intersecao legitima da outra palavra)."""
    touched = []
    for i, ch in enumerate(word):
        r = row if is_across else row + i
        c = col + i if is_across else col
        cell = grid[r][c]
        is_new = cell is None
        if is_new:
            cell = {"letter": ch, "across": False, "down": False}
            grid[r][c] = cell
        if is_across:
            cell["across"] = True
        else:
            cell["down"] = True
        touched.append((r, c, is_new))
    return touched


def unplace(grid, touched, is_across: bool):
    for (r, c, is_new) in touched:
        if is_new:
            grid[r][c] = None
        else:
            grid[r][c]["across" if is_across else "down"] = False


def get_candidates(grid, word: str, w: int, h: int):
    """Todo posicionamento valido de `word` que cruza pelo menos uma letra
    ja no grid, deduplicado por (row, col, is_across), ordenado
    deterministicamente (mais intersecoes primeiro)."""
    found = {}
    for r in range(h):
        for c in range(w):
            cell = grid[r][c]
            if cell is None:
                continue
            for i, ch in enumerate(word):
                if ch != cell["letter"]:
                    continue
                # Horizontal: a letra `i` da palavra cai na celula (r, c).
                across_col = c - i
                isec = can_place(grid, word, r, across_col, True, w, h)
                if isec is not None and isec >= 1:
                    found[(r, across_col, True)] = isec
                # Vertical: idem.
                down_row = r - i
                isec = can_place(grid, word, down_row, c, False, w, h)
                if isec is not None and isec >= 1:
                    found[(down_row, c, False)] = isec

    candidates = [(isec, row, col, is_across) for (row, col, is_across), isec in found.items()]
    candidates.sort(key=lambda t: (-t[0], t[1], t[2], 0 if t[3] else 1))
    return candidates


def solve(grid, remaining, placements, budget, puzzle_id, w, h, first=True):
    """Backtracking sobre QUAL palavra colocar em seguida, nao so ONDE: a
    cada passo tenta toda palavra ainda nao colocada (nao so a proxima da
    lista ordenada por tamanho), porque a ordem fixa por tamanho sozinha
    trava cedo demais em listas de palavras reais (uma palavra do meio da
    lista pode nao ter nenhum cruzamento valido com o grid parcial de um
    caminho especifico, mas ter cruzamento valido se colocada antes ou
    depois). `remaining` preserva a ordem original (mais longa primeiro)
    como ordem de TENTATIVA em cada passo -- reprodutibilidade sem perder a
    flexibilidade de backtrack sobre a escolha da proxima palavra."""
    if not remaining:
        return True

    if first:
        word_idx, word = remaining[0]
        row = h // 2
        col = (w - len(word)) // 2
        if col < 0:
            col = 0
        if col + len(word) > w:
            col = max(0, w - len(word))
        isec = can_place(grid, word, row, col, True, w, h)
        candidates = [(isec, row, col, True)] if isec is not None else []
        rest = remaining[1:]

        for (_isec, row, col, is_across) in candidates:
            budget[0] += 1
            if budget[0] > budget[1]:
                raise CrosswordError(
                    f"puzzle {puzzle_id}: orcamento de {budget[1]} nos de busca estourado -- "
                    f"lista de palavras precisa de revisao (menos palavras, palavras com mais "
                    f"letras em comum, ou um grid maior) em vez de um grid parcial ser emitido"
                )
            newly = place(grid, word, row, col, is_across)
            placements[word_idx] = (row, col, is_across)
            if solve(grid, rest, placements, budget, puzzle_id, w, h, first=False):
                return True
            unplace(grid, newly, is_across)
            del placements[word_idx]
        return False

    for pos, (word_idx, word) in enumerate(remaining):
        candidates = get_candidates(grid, word, w, h)
        for (_isec, row, col, is_across) in candidates:
            budget[0] += 1
            if budget[0] > budget[1]:
                raise CrosswordError(
                    f"puzzle {puzzle_id}: orcamento de {budget[1]} nos de busca estourado -- "
                    f"lista de palavras precisa de revisao (menos palavras, palavras com mais "
                    f"letras em comum, ou um grid maior) em vez de um grid parcial ser emitido"
                )
            newly = place(grid, word, row, col, is_across)
            placements[word_idx] = (row, col, is_across)
            rest = remaining[:pos] + remaining[pos + 1:]
            if solve(grid, rest, placements, budget, puzzle_id, w, h, first=False):
                return True
            unplace(grid, newly, is_across)
            del placements[word_idx]

    return False


def number_grid(grid, w, h):
    """Varredura esquerda->direita, cima->baixo (algoritmo padrao de
    numeracao de cruzadinha): uma celula recebe o proximo numero se inicia
    uma entrada horizontal e/ou vertical; uma celula que inicia as duas
    compartilha um unico numero entre elas."""
    starts = {}
    next_num = 1
    for r in range(h):
        for c in range(w):
            if grid[r][c] is None:
                continue
            starts_across = (c == 0 or grid[r][c - 1] is None) and \
                            (c + 1 < w and grid[r][c + 1] is not None)
            starts_down = (r == 0 or grid[r - 1][c] is None) and \
                          (r + 1 < h and grid[r + 1][c] is not None)
            if starts_across or starts_down:
                starts[(r, c)] = next_num
                next_num += 1
    return starts


def build_puzzle(raw_puzzle: dict, node_budget: int):
    puzzle_id = raw_puzzle["id"]
    size = int(raw_puzzle["grid_size"])
    if size > MAX_DIM:
        raise CrosswordError(
            f"puzzle {puzzle_id}: grid_size {size} excede RATIMOS_CRUZADINHA_MAX_DIM ({MAX_DIM})"
        )

    raw_words = raw_puzzle["words"]
    if len(raw_words) > MAX_WORDS:
        raise CrosswordError(
            f"puzzle {puzzle_id}: {len(raw_words)} palavras excede RATIMOS_CRUZADINHA_MAX_WORDS "
            f"({MAX_WORDS})"
        )
    if len(raw_words) < 8:
        raise CrosswordError(
            f"puzzle {puzzle_id}: precisa de pelo menos 8 palavras (tem {len(raw_words)})"
        )

    entries = []
    seen_answers = set()
    for w in raw_words:
        answer = validate_answer(w["answer"], puzzle_id)
        if answer in seen_answers:
            raise CrosswordError(f"puzzle {puzzle_id}: resposta '{answer}' repetida")
        seen_answers.add(answer)
        clue = validate_clue(w["clue"], answer, puzzle_id)
        entries.append({"answer": answer, "clue": clue})

    # Ordena da mais longa para a mais curta (empate: ordem original) --
    # determina tanto a ordem de tentativa quanto a palavra-ancora (a
    # primeira, mais longa, e sempre a primeira colocada).
    indexed = list(enumerate(entries))
    words_sorted = sorted(indexed, key=lambda kv: (-len(kv[1]["answer"]), kv[0]))
    words_sorted = [(idx, e["answer"]) for idx, e in words_sorted]

    grid = make_grid(size, size)
    placements = {}
    budget = [0, node_budget]

    ok = solve(grid, words_sorted, placements, budget, puzzle_id, size, size)
    if not ok:
        raise CrosswordError(
            f"puzzle {puzzle_id}: nenhum posicionamento valido encontrado para todas as "
            f"palavras -- revise a lista (palavras precisam compartilhar letras entre si)"
        )

    starts = number_grid(grid, size, size)

    words_out = []
    for idx, entry in enumerate(entries):
        row, col, is_across = placements[idx]
        clue_number = starts.get((row, col))
        if clue_number is None:
            raise CrosswordError(
                f"puzzle {puzzle_id}: palavra '{entry['answer']}' nao tem celula inicial "
                f"numerada -- bug interno do gerador"
            )
        words_out.append({
            "row": row,
            "col": col,
            "length": len(entry["answer"]),
            "is_across": is_across,
            "clue_number": clue_number,
            "clue_text": entry["clue"],
            "answer": entry["answer"],
        })

    # Ordem de saida estavel para leitura humana do C gerado: numero da dica
    # crescente, horizontal antes de vertical no empate (mesmo numero
    # compartilhado por uma celula que inicia as duas).
    words_out.sort(key=lambda w: (w["clue_number"], 0 if w["is_across"] else 1))

    # Toda celula de letra pertence a pelo menos uma entrada por construcao
    # (todo grid[r][c] preenchido veio de place(), que so e chamado a partir
    # de uma palavra da lista) -- checagem explicita mesmo assim, como
    # cinto-e-suspensorio contra um bug futuro no algoritmo acima.
    letter_cells = sum(1 for r in range(size) for c in range(size) if grid[r][c] is not None)
    covered_cells = set()
    for w in words_out:
        for i in range(w["length"]):
            r = w["row"] if w["is_across"] else w["row"] + i
            c = w["col"] + i if w["is_across"] else w["col"]
            covered_cells.add((r, c))
    if len(covered_cells) != letter_cells:
        raise CrosswordError(
            f"puzzle {puzzle_id}: {letter_cells - len(covered_cells)} celula(s) orfa(s) "
            f"detectada(s) -- bug interno do gerador"
        )

    return {
        "id": puzzle_id,
        "grid_w": size,
        "grid_h": size,
        "word_count": len(words_out),
        "words": words_out,
    }


# ---------------------------------------------------------------------------
# Emissao C
# ---------------------------------------------------------------------------

HEADER_COMMENT = """/*
 * GERADO por tools/generate_crossword.py a partir de
 * assets/crosswords/wordlists.json -- nao editar a mao. Reexecute o script
 * se a lista de palavras/dicas fonte mudar (ver assets/crosswords/README.md).
 */
"""


def emit_header(out_h: Path, guard: str):
    lines = [HEADER_COMMENT]
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append("#include <stdbool.h>")
    lines.append("#include <stddef.h>")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("/*")
    lines.append(" * Banco de quebra-cabecas do cruzadinha (D-11/JOGOS-04).")
    lines.append(" *")
    lines.append(" * Layout de grid: linha/coluna 0-based, is_across=true para uma entrada")
    lines.append(" * horizontal (esquerda->direita) e false para uma entrada vertical")
    lines.append(" * (cima->baixo). clue_number segue a convencao padrao de numeracao de")
    lines.append(" * cruzadinha -- uma celula que inicia tanto uma entrada horizontal quanto")
    lines.append(" * vertical compartilha o mesmo numero entre as duas.")
    lines.append(" */")
    lines.append(f"#define RATIMOS_CRUZADINHA_MAX_WORDS {MAX_WORDS}")
    lines.append(f"#define RATIMOS_CRUZADINHA_MAX_DIM {MAX_DIM}")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    uint8_t row;")
    lines.append("    uint8_t col;")
    lines.append("    uint8_t length;")
    lines.append("    bool is_across;")
    lines.append("    uint8_t clue_number;")
    lines.append("    char clue_text[96];")
    lines.append("    char answer[16];")
    lines.append("} ratimos_cruzadinha_word_t;")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    char id[16];")
    lines.append("    uint8_t grid_w;")
    lines.append("    uint8_t grid_h;")
    lines.append("    uint8_t word_count;")
    lines.append("    ratimos_cruzadinha_word_t words[RATIMOS_CRUZADINHA_MAX_WORDS];")
    lines.append("} ratimos_cruzadinha_puzzle_t;")
    lines.append("")
    lines.append("size_t ratimos_cruzadinha_puzzle_count(void);")
    lines.append("")
    lines.append("/* Copia o quebra-cabeca `index` para `out`. Retorna false (sem ler nada")
    lines.append(" * fora do array) quando o indice esta fora do banco. */")
    lines.append("bool ratimos_cruzadinha_get_puzzle(size_t index, ratimos_cruzadinha_puzzle_t * out);")
    lines.append("")
    lines.append(f"#endif /* {guard} */")
    lines.append("")
    out_h.parent.mkdir(parents=True, exist_ok=True)
    out_h.write_text("\n".join(lines), encoding="utf-8")


def c_string_literal(s: str) -> str:
    escaped = s.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def emit_source(out_c: Path, out_h_name: str, puzzles: list):
    lines = [HEADER_COMMENT]
    lines.append(f'#include "{out_h_name}"')
    lines.append("")
    lines.append("#include <string.h>")
    lines.append("")
    lines.append("static const ratimos_cruzadinha_puzzle_t s_puzzles[] = {")
    for pz in puzzles:
        lines.append("    {")
        lines.append(f'        {c_string_literal(pz["id"])},')
        lines.append(f'        {pz["grid_w"]}, {pz["grid_h"]}, {pz["word_count"]},')
        lines.append("        {")
        for w in pz["words"]:
            is_across = "true" if w["is_across"] else "false"
            lines.append(
                f'            {{ {w["row"]}, {w["col"]}, {w["length"]}, {is_across}, '
                f'{w["clue_number"]}, {c_string_literal(w["clue_text"])}, '
                f'{c_string_literal(w["answer"])} }},'
            )
        lines.append("        },")
        lines.append("    },")
    lines.append("};")
    lines.append("")
    lines.append("size_t ratimos_cruzadinha_puzzle_count(void)")
    lines.append("{")
    lines.append("    return sizeof(s_puzzles) / sizeof(s_puzzles[0]);")
    lines.append("}")
    lines.append("")
    lines.append("bool ratimos_cruzadinha_get_puzzle(size_t index, ratimos_cruzadinha_puzzle_t * out)")
    lines.append("{")
    lines.append("    if (!out || index >= ratimos_cruzadinha_puzzle_count()) {")
    lines.append("        return false;")
    lines.append("    }")
    lines.append("")
    lines.append("    memcpy(out, &s_puzzles[index], sizeof(*out));")
    lines.append("    return true;")
    lines.append("}")
    lines.append("")
    out_c.parent.mkdir(parents=True, exist_ok=True)
    out_c.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--in", dest="in_path", required=True, type=Path,
                     help="assets/crosswords/wordlists.json de entrada")
    ap.add_argument("--out-c", required=True, type=Path)
    ap.add_argument("--out-h", required=True, type=Path)
    ap.add_argument("--node-budget", type=int, default=DEFAULT_NODE_BUDGET,
                     help="teto de tentativas de posicionamento por quebra-cabeca antes de "
                          "falhar alto (padrao: %(default)s)")
    args = ap.parse_args()

    try:
        raw = json.loads(args.in_path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        print(f"[generate_crossword] {args.in_path} nao encontrado", file=sys.stderr)
        return 1

    puzzles = []
    try:
        for raw_puzzle in raw["puzzles"]:
            pz = build_puzzle(raw_puzzle, args.node_budget)
            puzzles.append(pz)
            print(f"[generate_crossword] {pz['id']}: {pz['grid_w']}x{pz['grid_h']}, "
                  f"{pz['word_count']} palavras posicionadas")
    except CrosswordError as exc:
        print(f"[generate_crossword] ERRO: {exc}", file=sys.stderr)
        return 1

    if len(puzzles) < 5:
        print(f"[generate_crossword] ERRO: banco final tem {len(puzzles)} quebra-cabecas, "
              f"minimo exigido e 5", file=sys.stderr)
        return 1

    emit_header(args.out_h, "RATIMOS_JOGOS_CRUZADINHA_PUZZLES_H")
    emit_source(args.out_c, args.out_h.name, puzzles)

    print(f"[generate_crossword] escrito {args.out_c} e {args.out_h} "
          f"({len(puzzles)} quebra-cabecas)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
