# Phase 2: Sync Protocol, Security Model & Cloud Backend - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-08-27
**Phase:** 2-Sync Protocol, Security Model & Cloud Backend
**Areas discussed:** Setup do Supabase, Cliente de protocolo em C ou script, Emissão do token por dispositivo, Escopo de conteúdo nesta fase, Como a RLS de verdade vê o token do dispositivo, Tabela ota_releases nesta fase

---

## Setup do Supabase

| Option | Description | Selected |
|--------|-------------|----------|
| Já tenho conta, ainda sem projeto | You already use Supabase but will create a new project for RatimOS now | |
| Não tenho conta ainda | Need to create account and project from scratch | ✓ |
| Já tenho projeto Supabase pronto | A RatimOS-specific Supabase project already exists | |

**User's choice:** Não tenho conta ainda

| Option | Description | Selected |
|--------|-------------|----------|
| Arquivo .env local (git-ignorado) | Recommended: a .env at repo root, gitignored, never committed | ✓ |
| Variáveis de ambiente do sistema | Exported in shell/user profile, no file in repo | |
| Você decide | Delegate the choice | |

**User's choice:** Arquivo .env local (git-ignorado)

| Option | Description | Selected |
|--------|-------------|----------|
| Deixar pra execução (recomendado) | Record the decision now; actual account/project creation happens during phase execution | |
| Criar agora, nesta conversa | Stop context discussion and walk through account/project creation right now | ✓ |

**User's choice:** Criar agora, nesta conversa

**Notes:** User created a real Supabase account and project live during this session (org "byglow9's Org", project "RatimOS", Free plan, region South America/São Paulo). Guided step by step: account creation → org creation → project creation with explicit security checkbox choices → retrieved Project URL and Publishable key. Secret/service_role key deliberately never shared with Claude — user was instructed to keep it local-only.

Security checkboxes chosen during project creation (not asked as a separate AskUserQuestion, guided directly given security context already established):
- Enable Data API: kept checked
- Automatically expose new tables: unchecked (Supabase's own recommendation, aligns with explicit-RLS-per-table intent)
- Enable automatic RLS: checked (fail-closed default — new tables get RLS forced on)

---

## Cliente de protocolo em C ou script

| Option | Description | Selected |
|--------|-------------|----------|
| C no native_sim (recomendado) | New src/sync/ module running in the existing native_sim environment, reusable by Phase 7 | ✓ |
| Script Python/Node descartável | Faster now, but throwaway — Phase 7 writes the real C client from scratch | |

**User's choice:** C no native_sim (recomendado)

| Option | Description | Selected |
|--------|-------------|----------|
| Você decide (recomendado) | Let research pick — likely libcurl on PC behind an abstraction interface, swapped for ESP32 HTTPClient/NetworkClientSecure in Phase 7 without changing call sites | ✓ |
| libcurl explicitamente | Decide libcurl now, no research needed | |

**User's choice:** Você decide (recomendado)

| Option | Description | Selected |
|--------|-------------|----------|
| src/sync/ (recomendado) | New sibling directory of board/ and storage/, signaling shared network/auth infrastructure | ✓ |
| Você decide | Delegate exact folder structure | |

**User's choice:** src/sync/ (recomendado)

---

## Emissão do token por dispositivo

| Option | Description | Selected |
|--------|-------------|----------|
| Endpoint mínimo de registro (recomendado) | An Edge Function/endpoint like POST /devices creates a devices row and returns a token — the same mechanism Phase 6's BLE provisioning will call later | ✓ |
| Inserido direto no banco via SQL | Manual INSERT in Supabase's SQL editor for a fixed-token test device — faster now, doesn't prove the token-issuance flow Phase 6 needs | |

**User's choice:** Endpoint mínimo de registro (recomendado)

| Option | Description | Selected |
|--------|-------------|----------|
| Protegido por chave de admin (recomendado) | Only someone with a secret admin key can register a new device/token | ✓ |
| Aberto por enquanto | No extra protection this phase — simpler to test but anyone who finds the URL can mint a token | |

**User's choice:** Protegido por chave de admin (recomendado)

| Option | Description | Selected |
|--------|-------------|----------|
| String opaca aleatória (recomendado) | UUID/32+ byte random token stored (hashed) in devices.token — simple to generate/validate in C later, trivially revocable | ✓ |
| JWT assinado | Signed token with claims (device_id, exp) — more "standard" but needs JWT parsing/verification in C on ESP32 later, and revocation before expiry is harder | |

**User's choice:** String opaca aleatória (recomendado)

---

## Escopo de conteúdo nesta fase

| Option | Description | Selected |
|--------|-------------|----------|
| Só metadados (recomendado) | content_items carries title/type/date/optional-url; real Storage upload deferred to Phase 7 | ✓ |
| Já testar upload real no Storage | Get ahead of Phase 7 now by uploading a real test file and returning its real URL | |

**User's choice:** Só metadados (recomendado)

| Option | Description | Selected |
|--------|-------------|----------|
| Seed SQL fixo (recomendado) | A fixed SQL seed script with 3-4 fictitious items, run manually in Supabase's SQL editor, mirroring Phase 1's fixture-count convention | ✓ |
| Inserir via endpoint também | A second minimal endpoint (also admin-key-protected) just for inserting test content items | |

**User's choice:** Seed SQL fixo (recomendado)

| Option | Description | Selected |
|--------|-------------|----------|
| Coluna delivered_at em content_items (recomendado) | Simple nullable timestamp; null = pending. Fits the single-real-device nature of this project | ✓ |
| Tabela de relação dispositivo×conteúdo | More flexible for a hypothetical future multi-device case, but added complexity that will likely never be exercised | |

**User's choice:** Coluna delivered_at em content_items (recomendado)

---

## Como a RLS de verdade vê o token do dispositivo

| Option | Description | Selected |
|--------|-------------|----------|
| Tudo via Edge Functions (recomendado) | All device access goes through Edge Functions that validate the token manually and use service-role internally; RLS is defense-in-depth against direct publishable-key access | ✓ |
| Token vira claim de um JWT customizado | Exchange the opaque token for a signed JWT with a device_id claim, used directly against PostgREST with RLS reading the real claim — more robust, more complex to build/maintain | |

**User's choice:** Tudo via Edge Functions (recomendado)

---

## Tabela ota_releases nesta fase

| Option | Description | Selected |
|--------|-------------|----------|
| Só estrutura, vazia (recomendado) | Table exists with correct columns but zero rows — avoids inventing fictitious firmware data before Phase 8 decides the real shape | ✓ |
| Já semear uma linha de teste | Insert a fictitious firmware release row even though nothing consumes it yet | |

**User's choice:** Só estrutura, vazia (recomendado)

---

## Claude's Discretion

- Exact HTTP/TLS library for the native_sim C client (likely libcurl behind an abstraction interface) — confirm during research.
- Exact `ota_releases` column list/types beyond version/url/released_at — Phase 8 may refine further.
- Exact Edge Function route names and request/response JSON shapes for registration and "what's new".

## Deferred Ideas

- Real file upload/download to Supabase Storage — Phase 7 (Cloud Content Sync).
- BLE-based device provisioning UX — Phase 6 (WiFi Provisioning & Settings); this phase's registration endpoint is the mechanism Phase 6 will call.
- OTA release consumption/rollback logic — Phase 8 (OTA & Concurrent Integration Stress Test).
- Multi-device support — explicitly out of scope for the entire project (one-off personal gift device).
