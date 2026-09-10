#!/usr/bin/env python3
"""
Cura o pool de palavras de 5 letras do termo/dueto/quarteto (JOGOS-03) a
partir do lexico bruto do repositorio `fserb/pt-br` (MIT, ~145 mil entradas
lexicais + pontuacoes ICF de frequencia) -- ver D-01b/02.1-CONTEXT.md e a
secao 1 de "Follow-up Research (Round 2)" em 02.1-RESEARCH.md.

Isto e trabalho de autoria com um esqueleto scriptado, nao uma instalacao de
dependencia: o lexico bruto precisa de curadoria (filtro de comprimento,
normalizacao de acentos, ranking por frequencia) MAIS uma revisao manual de
adequacao antes de virar o pool de respostas diarias que uma pessoa especifica
le todos os dias.

Layout esperado de --source (espelha o layout real do repo fserb/pt-br):
  <source>/lexico              -- uma palavra por linha, ja em minusculas
  <source>/icf                 -- CSV "palavra,pontuacao_icf" (menor = mais comum)
  <source>/listas/negativas    -- bloqueio automatico de baixo esforco (opcional)

Fluxo em duas fases (a fase manual acontece ENTRE as duas execucoes do script):
  1. Primeira execucao (arquivos txt ainda nao existem, ou --regenerate):
     le o lexico bruto, filtra, rankeia por ICF e escreve
     assets/wordlists/termo_answers.txt (candidato, ainda NAO revisado) e
     assets/wordlists/termo_accepted.txt.
  2. Um humano le termo_answers.txt inteiro e apaga qualquer entrada
     impropria (ver README.md do diretorio) -- esta e a "Manual quality pass"
     obrigatoria do plano 02.1-06, Tarefa 1.
  3. Execucoes seguintes (sem --regenerate) NAO tocam nos .txt -- apenas
     recompilam termo_words.c/.h a partir do que ja esta em disco, entao a
     curadoria manual da fase 2 nunca e sobrescrita por acidente. Isto e o
     que torna `--source <mesmo lexico> --out-c ... --out-h ...` idempotente
     (mesma entrada -> mesma saida) sem apagar uma revisao humana ja feita.

Uso:
  python3 tools/curate_termo_words.py --source <path-para-pt-br> \
      --out-c src/ratimos/apps/jogos/termo_words.c \
      --out-h src/ratimos/apps/jogos/termo_words.h \
      [--answers-max 600] [--regenerate]
"""
import argparse
import sys
import unicodedata
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
ANSWERS_TXT = REPO_ROOT / "assets" / "wordlists" / "termo_answers.txt"
ACCEPTED_TXT = REPO_ROOT / "assets" / "wordlists" / "termo_accepted.txt"

WORD_LEN = 5


def normalize_ascii(word: str) -> str:
    """Remove diacriticos portugueses (á->a, ç->c, ã->a, ...) via
    decomposicao NFKD + remocao das marcas combinantes -- a grade de letras e
    o teclado do jogo sao sem acento, entao 'aviao' e a forma unica que
    interessa ao motor mesmo que o lexico tenha 'avião' (a decisao de colapsar
    as duas na mesma entrada esta documentada aqui, nao e um acidente)."""
    decomposed = unicodedata.normalize("NFKD", word)
    stripped = "".join(ch for ch in decomposed if not unicodedata.combining(ch))
    return stripped.lower()


def read_lexico(path: Path):
    with path.open("r", encoding="utf-8") as f:
        return [line.strip() for line in f if line.strip()]


def read_icf(path: Path):
    """CSV 'palavra,pontuacao'. Retorna dict palavra(acentuada, como aparece
    no lexico) -> float. Entradas malformadas sao ignoradas silenciosamente
    (o arquivo real do fserb/pt-br nao tem nenhuma, mas um fallback de fonte
    poderia)."""
    scores = {}
    if not path.exists():
        return scores
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line:
                continue
            word, _, score = line.rpartition(",")
            if not word:
                continue
            try:
                scores[word] = float(score)
            except ValueError:
                continue
    return scores


def read_blocklist(path: Path):
    if not path.exists():
        return set()
    with path.open("r", encoding="utf-8") as f:
        return {line.strip().lower() for line in f if line.strip()}


def curate(source: Path, answers_max: int):
    lexico_path = source / "lexico"
    icf_path = source / "icf"
    blocklist_path = source / "listas" / "negativas"

    if not lexico_path.exists():
        raise FileNotFoundError(
            f"{lexico_path} nao encontrado -- --source deve apontar para um "
            f"checkout/download do fserb/pt-br (ou do fallback VeroBP/hunspell, "
            f"ver docstring)"
        )

    raw_entries = read_lexico(lexico_path)
    icf_scores = read_icf(icf_path)
    blocklist = read_blocklist(blocklist_path)

    # normalized_5letter -> (best_icf_score, chosen_original_entry)
    candidates = {}
    dropped_capitalized = 0
    dropped_non_letter = 0
    dropped_blocklist = 0
    collapsed_duplicates = 0

    for entry in raw_entries:
        # (c) descarta qualquer coisa capitalizada na fonte (nome proprio).
        # O lexico do fserb/pt-br ja vem 100% minusculo (verificado nesta
        # curadoria) -- este filtro fica aqui como uma salvaguarda generica
        # para uma fonte de fallback que capitalize nomes proprios.
        if entry != entry.lower():
            dropped_capitalized += 1
            continue

        normalized = normalize_ascii(entry)

        # (b) mantem so entradas de exatamente 5 letras APOS normalizar.
        if len(normalized) != WORD_LEN:
            continue

        # (c) descarta qualquer coisa com caractere fora de a-z apos a
        # normalizacao (numeros, hifen, apostrofo -- nenhuma entrada do
        # lexico principal tem isso, mas uma fonte de fallback pode ter).
        if not normalized.isalpha() or not normalized.isascii():
            dropped_non_letter += 1
            continue

        # (c) bloqueio automatico de baixo esforco contra a lista
        # `listas/negativas` do proprio fserb/pt-br -- NAO substitui a
        # revisao manual obrigatoria (Tarefa 1, passo 2), apenas remove os
        # casos mais obvios antes mesmo de um humano olhar a lista.
        if entry in blocklist or normalized in blocklist:
            dropped_blocklist += 1
            continue

        score = icf_scores.get(entry, float("inf"))  # sem score = tratado como raro
        if normalized in candidates:
            collapsed_duplicates += 1
            existing_score, existing_entry = candidates[normalized]
            if score < existing_score:
                candidates[normalized] = (score, entry)
        else:
            candidates[normalized] = (score, entry)

    # accepted-guess list: toda a lista filtrada, ordenada alfabeticamente
    # (a ordenacao e o que permite ratimos_termo_is_accepted_guess() usar
    # busca binaria no array C gerado).
    accepted = sorted(candidates.keys())

    # answer pool: as --answers-max entradas mais comuns (menor pontuacao
    # ICF primeiro), com a palavra normalizada como desempate deterministico
    # para que a curadoria seja reproduzivel para uma fonte fixa.
    ranked = sorted(candidates.items(), key=lambda kv: (kv[1][0], kv[0]))
    answers = [normalized for normalized, _ in ranked[:answers_max]]
    answers_sorted_for_output = sorted(answers)  # ordem estavel no arquivo p/ revisao humana

    stats = {
        "raw_entries": len(raw_entries),
        "dropped_capitalized": dropped_capitalized,
        "dropped_non_letter": dropped_non_letter,
        "dropped_blocklist": dropped_blocklist,
        "collapsed_duplicates": collapsed_duplicates,
        "accepted_count": len(accepted),
        "answers_count": len(answers_sorted_for_output),
    }
    return answers_sorted_for_output, accepted, stats


def emit_c_sources(answers, accepted, out_c: Path, out_h: Path, source_note: str):
    guard = "RATIMOS_JOGOS_TERMO_WORDS_H"

    header_comment = f"""/*
 * GERADO por tools/curate_termo_words.py -- nao editar a mao. Reexecute o
 * script se {source_note} mudar.
 *
 * Fonte: lexico fserb/pt-br (https://github.com/fserb/pt-br), licenca MIT.
 * Pool de respostas ({len(answers)} palavras) ranqueado por pontuacao ICF
 * (frequencia inversa de corpus) e adicionalmente aprovado numa revisao
 * manual de adequacao (ver assets/wordlists/README.md) -- nunca confie
 * apenas no filtro de frequencia para conteudo lido todo dia por uma
 * pessoa especifica.
 */
"""

    h_lines = [header_comment]
    h_lines.append(f"#ifndef {guard}")
    h_lines.append(f"#define {guard}")
    h_lines.append("")
    h_lines.append("#include <stdbool.h>")
    h_lines.append("#include <stddef.h>")
    h_lines.append("")
    h_lines.append(f"#define RATIMOS_TERMO_WORD_LEN {WORD_LEN}")
    h_lines.append("")
    h_lines.append("/* Quantidade de palavras no pool de respostas diarias. */")
    h_lines.append("size_t ratimos_termo_answer_count(void);")
    h_lines.append("")
    h_lines.append("/* Palavra de resposta no indice `index` (0-based), minuscula, sem")
    h_lines.append(" * acento, RATIMOS_TERMO_WORD_LEN caracteres. NULL se index estiver")
    h_lines.append(" * fora de [0, ratimos_termo_answer_count()). */")
    h_lines.append("const char * ratimos_termo_answer_at(size_t index);")
    h_lines.append("")
    h_lines.append("/* Verdadeiro quando `word` (RATIMOS_TERMO_WORD_LEN caracteres, aceita")
    h_lines.append(" * qualquer caixa) esta na lista mais ampla de palpites validos --")
    h_lines.append(" * comparacao case-insensitive via busca binaria sobre um array")
    h_lines.append(" * ordenado. Retorna false para NULL, para comprimento errado, ou para")
    h_lines.append(" * qualquer caractere fora de a-z/A-Z. */")
    h_lines.append("bool ratimos_termo_is_accepted_guess(const char * word);")
    h_lines.append("")
    h_lines.append(f"#endif /* {guard} */")
    h_lines.append("")
    out_h.parent.mkdir(parents=True, exist_ok=True)
    out_h.write_text("\n".join(h_lines), encoding="utf-8")

    def emit_word_array(name: str, words):
        lines = [f"static const char {name}[][RATIMOS_TERMO_WORD_LEN + 1] = {{"]
        for w in words:
            lines.append(f'    "{w}",')
        lines.append("};")
        return "\n".join(lines)

    c_lines = [header_comment]
    c_lines.append(f'#include "{out_h.name}"')
    c_lines.append("")
    c_lines.append("#include <ctype.h>")
    c_lines.append("#include <string.h>")
    c_lines.append("")
    c_lines.append(emit_word_array("s_termo_answers", answers))
    c_lines.append("")
    c_lines.append(emit_word_array("s_termo_accepted", accepted))
    c_lines.append("")
    c_lines.append("size_t ratimos_termo_answer_count(void)")
    c_lines.append("{")
    c_lines.append("    return sizeof(s_termo_answers) / sizeof(s_termo_answers[0]);")
    c_lines.append("}")
    c_lines.append("")
    c_lines.append("const char * ratimos_termo_answer_at(size_t index)")
    c_lines.append("{")
    c_lines.append("    if (index >= ratimos_termo_answer_count()) {")
    c_lines.append("        return NULL;")
    c_lines.append("    }")
    c_lines.append("    return s_termo_answers[index];")
    c_lines.append("}")
    c_lines.append("")
    c_lines.append("bool ratimos_termo_is_accepted_guess(const char * word)")
    c_lines.append("{")
    c_lines.append("    if (!word) {")
    c_lines.append("        return false;")
    c_lines.append("    }")
    c_lines.append("")
    c_lines.append("    char lower[RATIMOS_TERMO_WORD_LEN + 1];")
    c_lines.append("    size_t len = strlen(word);")
    c_lines.append("    if (len != RATIMOS_TERMO_WORD_LEN) {")
    c_lines.append("        return false;")
    c_lines.append("    }")
    c_lines.append("    for (size_t i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {")
    c_lines.append("        unsigned char c = (unsigned char) word[i];")
    c_lines.append("        if (!isalpha(c)) {")
    c_lines.append("            return false;")
    c_lines.append("        }")
    c_lines.append("        lower[i] = (char) tolower(c);")
    c_lines.append("    }")
    c_lines.append("    lower[RATIMOS_TERMO_WORD_LEN] = '\\0';")
    c_lines.append("")
    c_lines.append("    size_t lo = 0;")
    c_lines.append("    size_t hi = sizeof(s_termo_accepted) / sizeof(s_termo_accepted[0]);")
    c_lines.append("    while (lo < hi) {")
    c_lines.append("        size_t mid = lo + (hi - lo) / 2;")
    c_lines.append("        int cmp = strcmp(lower, s_termo_accepted[mid]);")
    c_lines.append("        if (cmp == 0) {")
    c_lines.append("            return true;")
    c_lines.append("        } else if (cmp < 0) {")
    c_lines.append("            hi = mid;")
    c_lines.append("        } else {")
    c_lines.append("            lo = mid + 1;")
    c_lines.append("        }")
    c_lines.append("    }")
    c_lines.append("    return false;")
    c_lines.append("}")
    c_lines.append("")
    out_c.parent.mkdir(parents=True, exist_ok=True)
    out_c.write_text("\n".join(c_lines), encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--source", required=True, type=Path,
                     help="diretorio de um checkout/download do fserb/pt-br (ou fallback com o mesmo layout)")
    ap.add_argument("--out-c", required=True, type=Path)
    ap.add_argument("--out-h", required=True, type=Path)
    ap.add_argument("--answers-max", type=int, default=600)
    ap.add_argument("--regenerate", action="store_true",
                     help="reescreve assets/wordlists/termo_answers.txt e termo_accepted.txt a "
                          "partir do --source, DESCARTANDO qualquer revisao manual ja feita. Sem "
                          "esta flag, se os .txt ja existirem em disco eles sao usados como estao "
                          "(preserva a curadoria manual) e so os arquivos C sao recompilados.")
    args = ap.parse_args()

    source = args.source.resolve()

    need_regenerate = args.regenerate or not ANSWERS_TXT.exists() or not ACCEPTED_TXT.exists()

    if need_regenerate:
        answers, accepted, stats = curate(source, args.answers_max)

        ANSWERS_TXT.parent.mkdir(parents=True, exist_ok=True)
        ANSWERS_TXT.write_text("\n".join(answers) + "\n", encoding="utf-8")
        ACCEPTED_TXT.write_text("\n".join(accepted) + "\n", encoding="utf-8")

        print(f"[curate_termo_words] fonte: {source}")
        print(f"[curate_termo_words] entradas brutas no lexico: {stats['raw_entries']}")
        print(f"[curate_termo_words] descartadas (capitalizadas): {stats['dropped_capitalized']}")
        print(f"[curate_termo_words] descartadas (caractere invalido pos-normalizacao): {stats['dropped_non_letter']}")
        print(f"[curate_termo_words] descartadas (bloqueio automatico listas/negativas): {stats['dropped_blocklist']}")
        print(f"[curate_termo_words] colapsadas por normalizacao de acento (mesma forma final): {stats['collapsed_duplicates']}")
        print(f"[curate_termo_words] pool de respostas (candidato, PRE revisao manual): {stats['answers_count']} palavras -> {ANSWERS_TXT}")
        print(f"[curate_termo_words] lista de palpites aceitos: {stats['accepted_count']} palavras -> {ACCEPTED_TXT}")
        print("[curate_termo_words] PROXIMO PASSO OBRIGATORIO: leia termo_answers.txt "
              "inteiro e apague qualquer entrada impropria antes de confiar nele como "
              "pool de respostas final (ver assets/wordlists/README.md).")
    else:
        print(f"[curate_termo_words] {ANSWERS_TXT} e {ACCEPTED_TXT} ja existem -- usando como "
              f"estao (curadoria manual preservada). Use --regenerate para redescartar tudo e "
              f"comecar do zero a partir de {source}.")

    answers = [line.strip() for line in ANSWERS_TXT.read_text(encoding="utf-8").splitlines() if line.strip()]
    accepted = [line.strip() for line in ACCEPTED_TXT.read_text(encoding="utf-8").splitlines() if line.strip()]

    # Uniao de seguranca: toda resposta valida tem de ser tambem um palpite
    # aceito (senao o proprio jogo rejeitaria a resposta do dia como "palavra
    # invalida" quando o jogador a digitasse por sorte) -- a lista de
    # respostas e sempre subconjunto do lexico filtrado, entao isto so
    # protege contra uma edicao manual que tenha digitado algo novo em
    # termo_answers.txt sem tambem estar em termo_accepted.txt.
    accepted_set = set(accepted)
    missing = [w for w in answers if w not in accepted_set]
    if missing:
        accepted = sorted(accepted_set | set(missing))

    emit_c_sources(answers, accepted, args.out_c, args.out_h, source_note="o lexico fonte")

    print(f"[curate_termo_words] escrito {args.out_c} e {args.out_h} "
          f"({len(answers)} respostas, {len(accepted)} palpites aceitos)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
