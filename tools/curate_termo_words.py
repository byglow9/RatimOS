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
  <source>/conjugações         -- uma forma verbal por linha (195.751 formas; o nome
                                  tem cedilha e til, em NFC ou NFD no disco)
  <source>/icf                 -- CSV "palavra,pontuacao_icf" (menor = mais comum)
  <source>/listas/negativas    -- bloqueio automatico de baixo esforco (opcional)

Plano 02.1-17 (UAT 2026-10-07): a lista de palpites ACEITOS deixou de ser so'
o lexico (quase so' lemas: "falou", "casas", "vamos" eram recusadas). Agora e'
a uniao de lexico + conjugações + formas do ICF ate' --icf-accept-max + plurais
regulares (ver `curate`). As RESPOSTAS continuam saindo so' do lexico e do
termo_answers.txt revisado a mao.

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
      [--answers-max 600] [--icf-accept-max 8.5] \
      [--regenerate | --sync-accepted | --sync-display]

  --sync-accepted : reescreve SO' termo_accepted.txt (uniao acima).
  --sync-display  : reescreve SO' termo_answers_display.txt (forma acentuada de
                    exibicao de cada resposta); DESCARTA edicoes manuais, que
                    precisam ser refeitas (ver assets/wordlists/README.md).
  (sem flag)      : so' recompila termo_words.c/.h a partir dos 3 .txt.
"""
import argparse
import sys
import unicodedata
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
ANSWERS_TXT = REPO_ROOT / "assets" / "wordlists" / "termo_answers.txt"
ACCEPTED_TXT = REPO_ROOT / "assets" / "wordlists" / "termo_accepted.txt"
DISPLAY_TXT = REPO_ROOT / "assets" / "wordlists" / "termo_answers_display.txt"

WORD_LEN = 5

# Limiar ICF default da lista aceita (plano 02.1-17): o MENOR valor limpo que
# faz entrar "novas" (ICF 8,48), a unica palavra da sonda que nao vem do lexico
# nem das conjugações e que o ICF conhece. ("pesos", "gatos" e "bolos" nao
# estao no ICF em nenhuma forma: entram pelos plurais regulares.)
DEFAULT_ICF_ACCEPT_MAX = 8.5

# Conjunto PT-BR que as fontes do jogo (ratimos_font_title_16) tem glifo --
# o mesmo de tools/convert_title_font.sh. Forma de exibicao so' usa estes.
DISPLAY_ACCENTED_LOWER = "áàãâéêíóôõúç"
DISPLAY_ALLOWED_UPPER = set("ABCDEFGHIJKLMNOPQRSTUVWXYZ") | set(DISPLAY_ACCENTED_LOWER.upper())


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


def find_conjugacoes(source: Path):
    """Acha o arquivo 'conjugações' (cedilha e til) em NFC ou NFD no disco."""
    target = unicodedata.normalize("NFC", "conjugações")
    for entry in sorted(source.iterdir()):
        if unicodedata.normalize("NFC", entry.name) == target:
            return entry
    return None


def read_conjugacoes(path: Path):
    """Uma forma verbal por linha (formato real conferido: 'abafa', 'abafado',
    'abafai', ...), ja em minusculas. Retorna a lista de formas."""
    if path is None or not path.exists():
        return []
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


def five_letter_ascii(entry: str):
    """Forma normalizada de `entry` se ela tem exatamente 5 letras a-z, senao
    None. Capitalizadas (nome proprio) tambem dao None."""
    if entry != entry.lower():
        return None
    normalized = normalize_ascii(entry)
    if len(normalized) != WORD_LEN or not normalized.isalpha() or not normalized.isascii():
        return None
    return normalized


def curate(source: Path, answers_max: int, icf_accept_max: float = DEFAULT_ICF_ACCEPT_MAX):
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
    conjugacoes = read_conjugacoes(find_conjugacoes(source))

    # normalized_5letter -> (best_icf_score, chosen_original_entry)
    candidates = {}
    dropped_capitalized = 0
    dropped_non_letter = 0
    dropped_blocklist = 0
    collapsed_duplicates = 0
    # Formas normalizadas bloqueadas pela lista de negativas: ficam FORA do
    # pool de respostas, mas DENTRO dos palpites aceitos (plano 02.1-14).
    blocked_guesses = set()

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
        #
        # Plano 02.1-14: o bloqueio vale SO' pro pool de respostas (o que a
        # jogadora le todo dia). Aplicado tambem aos palpites, ele recusava
        # palavras comuns que ela digita ("peste", "bagre", "burro", "droga",
        # "fenda"...) e o enter parecia nao funcionar. Bloqueada aqui = nunca
        # resposta, mas sempre um palpite valido.
        if entry in blocklist or normalized in blocklist:
            dropped_blocklist += 1
            blocked_guesses.add(normalized)
            continue

        score = icf_scores.get(entry, float("inf"))  # sem score = tratado como raro
        if normalized in candidates:
            collapsed_duplicates += 1
            existing_score, existing_entry = candidates[normalized]
            if score < existing_score:
                candidates[normalized] = (score, entry)
        else:
            candidates[normalized] = (score, entry)

    # --- Lista aceita ampliada (plano 02.1-17) -------------------------------
    # O lexico e' quase so' lemas: "falou", "comeu", "vamos", "casas" ficavam
    # de fora e a jogadora perdia a tentativa com palavra comum flexionada.
    # Fontes (todas do mesmo repositorio MIT, mesmo filtro: so' a-z apos
    # normalizar_ascii; hifen, enclise e numeros ficam fora):
    #   1. lexico (candidates + blocked_guesses, acima);
    #   2. conjugações (195.751 formas verbais);
    #   3. icf: formas de corpus (plurais, femininos) com pontuacao <=
    #      icf_accept_max;
    #   4. plurais regulares: lexico de 4 letras terminado em a/e/o + "s"
    #      ("pesos", "gatos", "bolos" nao existem em nenhuma forma no ICF);
    # Nomes proprios e estrangeirismos que o ICF trouxer sao aceitaveis como
    # PALPITE (nunca resposta: as respostas vem so' do termo_answers.txt).
    from_conj = set()
    for entry in conjugacoes:
        n = five_letter_ascii(entry)
        if n:
            from_conj.add(n)

    from_icf = set()
    for entry, score in icf_scores.items():
        if score <= icf_accept_max:
            n = five_letter_ascii(entry)
            if n:
                from_icf.add(n)

    from_plural = set()
    for entry in raw_entries:
        if entry != entry.lower():
            continue
        n = normalize_ascii(entry)
        if len(n) == WORD_LEN - 1 and n.isalpha() and n.isascii() and n[-1] in "aeo":
            from_plural.add(n + "s")

    lexical = set(candidates.keys()) | blocked_guesses
    accepted = sorted(lexical | from_conj | from_icf | from_plural)

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
        "lexical_count": len(lexical),
        "conjugation_count": len(from_conj),
        "icf_count": len(from_icf),
        "plural_count": len(from_plural),
        "accepted_count": len(accepted),
        "answers_count": len(answers_sorted_for_output),
    }
    return answers_sorted_for_output, accepted, stats


def curate_display(source: Path, answers):
    """Forma acentuada de exibicao de cada resposta (plano 02.1-17).

    Representante = entre todas as entradas de lexico + conjugações + icf que
    normalizam para a resposta, a de MENOR ICF; empate: a do lexico, depois a
    ordem alfabetica. Grava so' quando o representante tem acento E todo
    caractere esta no conjunto PT-BR das fontes. Retorna [(ascii, EXIBICAO)]
    ordenado por ascii.
    """
    icf_scores = read_icf(source / "icf")
    wanted = set(answers)
    best = {}  # ascii -> (icf, fonte, entrada)

    def consider(entry, rank):
        if entry != entry.lower():
            return
        n = normalize_ascii(entry)
        if n not in wanted:
            return
        key = (icf_scores.get(entry, float("inf")), rank, entry)
        if n not in best or key < best[n]:
            best[n] = key

    for entry in read_lexico(source / "lexico"):
        consider(entry, 0)
    for entry in read_conjugacoes(find_conjugacoes(source)):
        consider(entry, 1)
    for entry in icf_scores:
        consider(entry, 2)

    pairs = []
    skipped = []
    for ascii_word in sorted(wanted):
        if ascii_word not in best:
            continue
        rep = best[ascii_word][2]
        if rep == ascii_word:
            continue  # representante sem acento: sai em ASCII maiusculo
        if not all(ch in DISPLAY_ACCENTED_LOWER or ("a" <= ch <= "z") for ch in rep):
            skipped.append((ascii_word, rep))
            continue
        pairs.append((ascii_word, rep.upper()))
    return pairs, skipped


def validate_display(pairs, answers):
    """O gerador falha (ValueError) se a exibicao nao normaliza de volta para a
    chave ASCII ou usa caractere fora de A-Z + ACENTOS PT-BR das fontes."""
    answer_set = set(answers)
    for ascii_word, shown in pairs:
        if ascii_word not in answer_set:
            raise ValueError(f"exibicao de {ascii_word!r}: nao e' uma resposta")
        if normalize_ascii(shown) != ascii_word:
            raise ValueError(f"exibicao {shown!r} nao normaliza para {ascii_word!r}")
        if len(shown) != WORD_LEN:
            raise ValueError(f"exibicao {shown!r} nao tem {WORD_LEN} letras")
        bad = [ch for ch in shown if ch not in DISPLAY_ALLOWED_UPPER]
        if bad:
            raise ValueError(f"exibicao {shown!r} tem caractere fora do conjunto PT-BR: {bad}")


def read_display_txt(path: Path):
    pairs = []
    if not path.exists():
        return pairs
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line:
            continue
        ascii_word, _, shown = line.partition("\t")
        pairs.append((ascii_word.strip(), shown.strip()))
    return sorted(pairs)


def c_string_literal(text: str) -> str:
    """Literal C 100% ASCII: caracteres nao ASCII viram escapes \\xNN (bytes
    UTF-8). Um escape hex engole todo digito hex seguinte, entao o literal e'
    fechado e reaberto quando o proximo caractere e' um digito hex."""
    out = '"'
    prev_hex = False
    for ch in text:
        if ord(ch) < 128:
            if prev_hex and ch in "0123456789abcdefABCDEF":
                out += '" "'
            out += ch
            prev_hex = False
        else:
            for byte in ch.encode("utf-8"):
                out += f"\\x{byte:02x}"
            prev_hex = True
    return out + '"'


def emit_c_sources(answers, accepted, display, out_c: Path, out_h: Path, source_note: str):
    guard = "RATIMOS_JOGOS_TERMO_WORDS_H"

    header_comment = f"""/*
 * GERADO por tools/curate_termo_words.py -- nao editar a mao. Reexecute o
 * script se {source_note} mudar.
 *
 * Fonte: fserb/pt-br (https://github.com/fserb/pt-br), licenca MIT.
 * Pool de respostas ({len(answers)} palavras) ranqueado por pontuacao ICF
 * (frequencia inversa de corpus) e adicionalmente aprovado numa revisao
 * manual de adequacao (ver assets/wordlists/README.md) -- nunca confie
 * apenas no filtro de frequencia para conteudo lido todo dia por uma
 * pessoa especifica.
 *
 * Plano 02.1-17: a lista de palpites aceitos ({len(accepted)} palavras) inclui
 * flexoes comuns (lexico + conjugacoes + formas do ICF + plurais regulares) e
 * a tabela de exibicao ({len(display)} formas acentuadas, revisadas a mao)
 * mostra a resposta com acento no fim de jogo ("aviao" vira A-V-I-A-com-til-O).
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
    h_lines.append("/* Tamanho minimo do buffer de ratimos_termo_display_form(): 5 code points")
    h_lines.append(" * UTF-8 de ate' 2 bytes + NUL, com folga. */")
    h_lines.append("#define RATIMOS_TERMO_DISPLAY_MAX 16")
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
    h_lines.append(" * ordenado. A entrada e' sem acento por construcao (teclado sem")
    h_lines.append(" * acento): 'aurea' equivale a 'aurea' acentuada na lista. Retorna false")
    h_lines.append(" * para NULL, para comprimento errado, ou para qualquer caractere fora de")
    h_lines.append(" * a-z/A-Z (inclusive bytes UTF-8). */")
    h_lines.append("bool ratimos_termo_is_accepted_guess(const char * word);")
    h_lines.append("")
    h_lines.append("/* Forma de EXIBICAO de uma resposta no fim de jogo, em MAIUSCULAS e UTF-8")
    h_lines.append(" * (\"aviao\" sai com til no A). `word`: 5 letras a-z (qualquer caixa). Sem")
    h_lines.append(" * entrada na tabela, copia o ASCII em maiusculas. A saida sempre tem 5")
    h_lines.append(" * code points. false para NULL, comprimento != 5, caractere fora de a-z")
    h_lines.append(" * ou `out_len` < RATIMOS_TERMO_DISPLAY_MAX. */")
    h_lines.append("bool ratimos_termo_display_form(const char * word, char * out, size_t out_len);")
    h_lines.append("")
    h_lines.append("/* Acesso a tabela de formas acentuadas (teste de glifos das fontes). */")
    h_lines.append("size_t ratimos_termo_display_count(void);")
    h_lines.append("const char * ratimos_termo_display_at(size_t i);")
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
    c_lines.append("/* Chave ASCII minuscula -> forma acentuada UTF-8 (escapes hex: a fonte e'")
    c_lines.append(" * sempre ASCII). Ordenada pela chave (busca binaria). */")
    c_lines.append("static const struct {")
    c_lines.append("    char key[RATIMOS_TERMO_WORD_LEN + 1];")
    c_lines.append("    const char * form;")
    c_lines.append("} s_termo_display[] = {")
    for key, shown in display:
        c_lines.append(f'    {{ "{key}", {c_string_literal(shown)} }},')
    if not display:
        c_lines.append('    { "", "" },')
    c_lines.append("};")
    c_lines.append(f"#define TERMO_DISPLAY_COUNT {len(display)}")
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
    c_lines.append("/* Minuscula de uma palavra de 5 letras a-z/A-Z; false em qualquer outra")
    c_lines.append(" * coisa (NULL, tamanho errado, byte nao ASCII). */")
    c_lines.append("static bool termo_lowercase_word(const char * word, char lower[RATIMOS_TERMO_WORD_LEN + 1])")
    c_lines.append("{")
    c_lines.append("    if (!word || strlen(word) != RATIMOS_TERMO_WORD_LEN) {")
    c_lines.append("        return false;")
    c_lines.append("    }")
    c_lines.append("    for (size_t i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {")
    c_lines.append("        unsigned char c = (unsigned char) word[i];")
    c_lines.append("        if (c >= 128 || !isalpha(c)) {")
    c_lines.append("            return false;")
    c_lines.append("        }")
    c_lines.append("        lower[i] = (char) tolower(c);")
    c_lines.append("    }")
    c_lines.append("    lower[RATIMOS_TERMO_WORD_LEN] = '\\0';")
    c_lines.append("    return true;")
    c_lines.append("}")
    c_lines.append("")
    c_lines.append("bool ratimos_termo_is_accepted_guess(const char * word)")
    c_lines.append("{")
    c_lines.append("    char lower[RATIMOS_TERMO_WORD_LEN + 1];")
    c_lines.append("    if (!termo_lowercase_word(word, lower)) {")
    c_lines.append("        return false;")
    c_lines.append("    }")
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
    c_lines.append("bool ratimos_termo_display_form(const char * word, char * out, size_t out_len)")
    c_lines.append("{")
    c_lines.append("    char lower[RATIMOS_TERMO_WORD_LEN + 1];")
    c_lines.append("    if (!out || out_len < RATIMOS_TERMO_DISPLAY_MAX || !termo_lowercase_word(word, lower)) {")
    c_lines.append("        return false;")
    c_lines.append("    }")
    c_lines.append("")
    c_lines.append("    size_t lo = 0;")
    c_lines.append("    size_t hi = TERMO_DISPLAY_COUNT;")
    c_lines.append("    while (lo < hi) {")
    c_lines.append("        size_t mid = lo + (hi - lo) / 2;")
    c_lines.append("        int cmp = strcmp(lower, s_termo_display[mid].key);")
    c_lines.append("        if (cmp == 0) {")
    c_lines.append("            size_t n = strlen(s_termo_display[mid].form);")
    c_lines.append("            if (n + 1 > out_len) {")
    c_lines.append("                return false;")
    c_lines.append("            }")
    c_lines.append("            memcpy(out, s_termo_display[mid].form, n + 1);")
    c_lines.append("            return true;")
    c_lines.append("        } else if (cmp < 0) {")
    c_lines.append("            hi = mid;")
    c_lines.append("        } else {")
    c_lines.append("            lo = mid + 1;")
    c_lines.append("        }")
    c_lines.append("    }")
    c_lines.append("")
    c_lines.append("    for (size_t i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {")
    c_lines.append("        out[i] = (char) toupper((unsigned char) lower[i]);")
    c_lines.append("    }")
    c_lines.append("    out[RATIMOS_TERMO_WORD_LEN] = '\\0';")
    c_lines.append("    return true;")
    c_lines.append("}")
    c_lines.append("")
    c_lines.append("size_t ratimos_termo_display_count(void)")
    c_lines.append("{")
    c_lines.append("    return TERMO_DISPLAY_COUNT;")
    c_lines.append("}")
    c_lines.append("")
    c_lines.append("const char * ratimos_termo_display_at(size_t i)")
    c_lines.append("{")
    c_lines.append("    return i < TERMO_DISPLAY_COUNT ? s_termo_display[i].form : NULL;")
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
    ap.add_argument("--sync-accepted", action="store_true",
                     help="reescreve SO' termo_accepted.txt a partir do --source (a lista "
                          "de palpites e' 100%% derivada do lexico, sem revisao manual); "
                          "termo_answers.txt, que tem a revisao manual, nao e' tocado.")
    ap.add_argument("--icf-accept-max", type=float, default=DEFAULT_ICF_ACCEPT_MAX,
                     help="limiar de pontuacao ICF (menor = mais comum) ate' o qual uma forma de "
                          "corpus com 5 letras entra na lista de palpites aceitos "
                          f"(default {DEFAULT_ICF_ACCEPT_MAX}, calibrado no plano 02.1-17).")
    ap.add_argument("--sync-display", action="store_true",
                     help="reescreve SO' assets/wordlists/termo_answers_display.txt (forma "
                          "acentuada de exibicao de cada resposta) a partir do --source. DESCARTA "
                          "edicoes manuais: refaca a revisao de assets/wordlists/README.md.")
    ap.add_argument("--regenerate", action="store_true",
                     help="reescreve assets/wordlists/termo_answers.txt e termo_accepted.txt a "
                          "partir do --source, DESCARTANDO qualquer revisao manual ja feita. Sem "
                          "esta flag, se os .txt ja existirem em disco eles sao usados como estao "
                          "(preserva a curadoria manual) e so os arquivos C sao recompilados.")
    args = ap.parse_args()

    source = args.source.resolve()

    need_regenerate = args.regenerate or not ANSWERS_TXT.exists() or not ACCEPTED_TXT.exists()

    if need_regenerate:
        answers, accepted, stats = curate(source, args.answers_max, args.icf_accept_max)

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
        print(f"[curate_termo_words]   fontes: lexico {stats['lexical_count']}, conjugacoes {stats['conjugation_count']}, "
              f"icf<={args.icf_accept_max} {stats['icf_count']}, plurais regulares {stats['plural_count']} (com sobreposicao)")
        print("[curate_termo_words] PROXIMO PASSO OBRIGATORIO: leia termo_answers.txt "
              "inteiro e apague qualquer entrada impropria antes de confiar nele como "
              "pool de respostas final (ver assets/wordlists/README.md).")
    elif args.sync_accepted:
        _, accepted_new, stats = curate(source, args.answers_max, args.icf_accept_max)
        ACCEPTED_TXT.write_text("\n".join(accepted_new) + "\n", encoding="utf-8")
        print(f"[curate_termo_words] lista de palpites aceitos sincronizada: "
              f"{stats['accepted_count']} palavras -> {ACCEPTED_TXT} (respostas intactas)")
        print(f"[curate_termo_words]   fontes: lexico {stats['lexical_count']}, conjugacoes {stats['conjugation_count']}, "
              f"icf<={args.icf_accept_max} {stats['icf_count']}, plurais regulares {stats['plural_count']} (com sobreposicao)")
    else:
        print(f"[curate_termo_words] {ANSWERS_TXT} e {ACCEPTED_TXT} ja existem -- usando como "
              f"estao (curadoria manual preservada). Use --regenerate para redescartar tudo e "
              f"comecar do zero a partir de {source}.")

    answers = [line.strip() for line in ANSWERS_TXT.read_text(encoding="utf-8").splitlines() if line.strip()]

    if args.sync_display:
        pairs, skipped = curate_display(source, answers)
        validate_display(pairs, answers)
        DISPLAY_TXT.write_text("".join(f"{k}\t{v}\n" for k, v in pairs), encoding="utf-8")
        print(f"[curate_termo_words] formas de exibicao sincronizadas: {len(pairs)} -> {DISPLAY_TXT} "
              f"(DESCARTA edicoes manuais; refaca a revisao do README)")
        for ascii_word, rep in skipped:
            print(f"[curate_termo_words]   ignorada (caractere fora do conjunto PT-BR): {ascii_word} -> {rep}")

    if not DISPLAY_TXT.exists():
        print(f"[curate_termo_words] ERRO: {DISPLAY_TXT} nao existe -- rode com --sync-display uma vez.",
              file=sys.stderr)
        return 1
    display = read_display_txt(DISPLAY_TXT)
    try:
        validate_display(display, answers)
    except ValueError as err:
        print(f"[curate_termo_words] ERRO: {DISPLAY_TXT}: {err}", file=sys.stderr)
        return 1

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

    emit_c_sources(answers, accepted, display, args.out_c, args.out_h, source_note="o lexico fonte")

    print(f"[curate_termo_words] escrito {args.out_c} e {args.out_h} "
          f"({len(answers)} respostas, {len(accepted)} palpites aceitos, {len(display)} formas de exibicao)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
