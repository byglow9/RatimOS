/*
 * Suite do manifesto de progressao (PROGRESSAO-01, D-06a) -- pina a
 * resolucao threshold->estagio, o estado final e o invariante estrutural
 * aditivo-seguro que garante que uma futura linha 7 nao exige nenhuma
 * mudanca de codigo de renderizacao. Roda via PlatformIO native Unity, sem
 * LVGL/SDL, contra o manifesto v1 real compilado em progression.c.
 */
#include <string.h>
#include <unity.h>

#include "ratimos/progression.h"

/* Helper interno com linkage externa (nao declarado em progression.h) --
 * mesmo padrao de ratimos_storage_tracks_read_title_or_default() em
 * tracks.c: existe so para a suite poder apontar o mesmo codigo de
 * resolucao para um manifesto local diferente do compilado em producao. */
extern size_t ratimos_progression_stage_index_in(const ratimos_progression_manifest_t * manifest,
                                                  uint16_t shared_completions);

void setUp(void) {}
void tearDown(void) {}

void test_stage_index_at_and_below_first_threshold_resolve_to_stage_zero(void)
{
    TEST_ASSERT_EQUAL_UINT(0, ratimos_progression_stage_index(0));
    TEST_ASSERT_EQUAL_UINT(0, ratimos_progression_stage_index(5));
}

void test_stage_index_at_six_resolves_to_stage_one(void)
{
    TEST_ASSERT_EQUAL_UINT(1, ratimos_progression_stage_index(6));
}

void test_stage_index_at_twenty_nine_resolves_to_stage_four(void)
{
    TEST_ASSERT_EQUAL_UINT(4, ratimos_progression_stage_index(29));
}

void test_stage_index_at_thirty_resolves_to_stage_five(void)
{
    TEST_ASSERT_EQUAL_UINT(5, ratimos_progression_stage_index(30));
}

void test_stage_index_clamps_past_final_threshold(void)
{
    /* 9999 nunca le alem de stage_count nem de RATIMOS_PROGRESSION_MAX_STAGES
     * -- resolve para o ultimo estagio populado (5), nunca para lixo. */
    TEST_ASSERT_EQUAL_UINT(5, ratimos_progression_stage_index(9999));
}

void test_stage_asset_id_returns_third_stage_asset_id(void)
{
    TEST_ASSERT_EQUAL_STRING("castle_stage_02_muros_canteiro",
                             ratimos_progression_stage_asset_id(12));
}

void test_target_completions_is_thirty(void)
{
    TEST_ASSERT_EQUAL_UINT16(30, ratimos_progression_target_completions());
}

void test_is_complete_boundary(void)
{
    TEST_ASSERT_FALSE(ratimos_progression_is_complete(29));
    TEST_ASSERT_TRUE(ratimos_progression_is_complete(30));
    TEST_ASSERT_TRUE(ratimos_progression_is_complete(31));
}

void test_unlock_asset_id_present_for_every_game(void)
{
    for (int i = 0; i < (int) RATIMOS_GAME_COUNT; i++) {
        const char * id = ratimos_progression_unlock_asset_id((ratimos_game_kind_t) i);
        TEST_ASSERT_NOT_NULL(id);
        TEST_ASSERT_TRUE(strlen(id) > 0);
    }
    TEST_ASSERT_EQUAL_STRING("unlock_conexo_portao",
                             ratimos_progression_unlock_asset_id(RATIMOS_GAME_CONEXO));
}

void test_unlock_asset_id_out_of_range_returns_null(void)
{
    TEST_ASSERT_NULL(ratimos_progression_unlock_asset_id((ratimos_game_kind_t) RATIMOS_GAME_COUNT));
    TEST_ASSERT_NULL(ratimos_progression_unlock_asset_id((ratimos_game_kind_t) -1));
}

/* Invariante estrutural: thresholds estritamente crescentes, asset_id
 * sempre nao-vazio e unico, stage_count nunca maior que o array fisico.
 * Esta e a asercao que fica vermelha se uma futura linha de estagio for
 * acrescentada fora de ordem ou com um id duplicado/vazio. */
void test_manifest_structural_invariants(void)
{
    const ratimos_progression_manifest_t * m = ratimos_progression_manifest();

    TEST_ASSERT_TRUE(m->stage_count <= RATIMOS_PROGRESSION_MAX_STAGES);

    for (size_t i = 0; i < m->stage_count; i++) {
        TEST_ASSERT_TRUE(strlen(m->stages[i].asset_id) > 0);

        if (i > 0) {
            TEST_ASSERT_TRUE(m->stages[i].threshold > m->stages[i - 1].threshold);
        }

        for (size_t j = 0; j < i; j++) {
            TEST_ASSERT_TRUE(strcmp(m->stages[i].asset_id, m->stages[j].asset_id) != 0);
        }
    }
}

/* Prova de D-06a: apontar o MESMO resolvedor para um manifesto local com uma
 * setima linha muda apenas a resolucao de um contador alem do antigo teto
 * (30) -- toda resolucao dentro dos 6 estagios originais permanece
 * identica, provando que crescer stage_count e uma mudanca de dado, nunca
 * de codigo de renderizacao. */
void test_growing_stage_count_changes_only_high_counter_resolution(void)
{
    ratimos_progression_manifest_t local = *ratimos_progression_manifest();
    local.stage_count = 7;
    local.stages[6].threshold = 36;
    strncpy(local.stages[6].asset_id, "castle_stage_06_extra", sizeof(local.stages[6].asset_id) - 1);
    local.stages[6].asset_id[sizeof(local.stages[6].asset_id) - 1] = '\0';

    TEST_ASSERT_EQUAL_UINT(ratimos_progression_stage_index(0), ratimos_progression_stage_index_in(&local, 0));
    TEST_ASSERT_EQUAL_UINT(ratimos_progression_stage_index(29), ratimos_progression_stage_index_in(&local, 29));
    TEST_ASSERT_EQUAL_UINT(ratimos_progression_stage_index(30), ratimos_progression_stage_index_in(&local, 30));

    /* So um contador alem do teto antigo (30) alcanca o setimo estagio novo
     * -- a UNICA mudanca de comportamento introduzida pela linha extra. */
    TEST_ASSERT_EQUAL_UINT(6, ratimos_progression_stage_index_in(&local, 36));
    TEST_ASSERT_EQUAL_UINT(6, ratimos_progression_stage_index_in(&local, 9999));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_stage_index_at_and_below_first_threshold_resolve_to_stage_zero);
    RUN_TEST(test_stage_index_at_six_resolves_to_stage_one);
    RUN_TEST(test_stage_index_at_twenty_nine_resolves_to_stage_four);
    RUN_TEST(test_stage_index_at_thirty_resolves_to_stage_five);
    RUN_TEST(test_stage_index_clamps_past_final_threshold);
    RUN_TEST(test_stage_asset_id_returns_third_stage_asset_id);
    RUN_TEST(test_target_completions_is_thirty);
    RUN_TEST(test_is_complete_boundary);
    RUN_TEST(test_unlock_asset_id_present_for_every_game);
    RUN_TEST(test_unlock_asset_id_out_of_range_returns_null);
    RUN_TEST(test_manifest_structural_invariants);
    RUN_TEST(test_growing_stage_count_changes_only_high_counter_resolution);
    return UNITY_END();
}
