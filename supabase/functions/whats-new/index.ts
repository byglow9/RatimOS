// Phase 2 Plan 2: whats-new Edge Function (D-09/D-10/D-12, T-2-02/T-2-07).
//
// Device-bearer-token-scoped read of a device's pending synced content.
// Mirrors register-device's admin-secret-gated pattern (Plan 1) but
// authenticates via the device's own opaque bearer token instead of the
// admin secret: extract Authorization header -> hash the token with the
// exact same SHA-256 call register-device uses to mint it -> look up
// devices by token_hash -> query content_items scoped by the resolved
// device_id AND delivered_at IS NULL (D-12).
//
// This endpoint is READ-ONLY -- it never writes delivered_at. Marking a
// row delivered is Phase 7's job, once local persistence can confirm the
// sync actually succeeded (RESEARCH.md Open Question 2). Re-calling this
// endpoint with the same token returns the same pending items every time
// during this phase.
//
// verify_jwt = false is set in supabase/config.toml for this function --
// Supabase's platform JWT gateway must never run here, since our tokens
// are not Supabase-issued JWTs (RESEARCH.md Pitfall 2).
import { createClient } from "npm:@supabase/supabase-js@2";

async function sha256Hex(input: string): Promise<string> {
  const data = new TextEncoder().encode(input);
  const digest = await crypto.subtle.digest("SHA-256", data);
  return Array.from(new Uint8Array(digest))
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

Deno.serve(async (req) => {
  // T-2-07: extract and validate the Authorization header BEFORE any
  // database access -- missing/malformed header returns 401 immediately.
  const authHeader = req.headers.get("Authorization");
  if (!authHeader || !authHeader.startsWith("Bearer ")) {
    return new Response(JSON.stringify({ error: "missing bearer token" }), {
      status: 401,
      headers: { "Content-Type": "application/json" },
    });
  }

  // T-2-02: the raw token is only ever hashed for lookup, never logged.
  const token = authHeader.slice("Bearer ".length);
  const tokenHash = await sha256Hex(token);

  // SUPABASE_URL / SUPABASE_SERVICE_ROLE_KEY are auto-injected by the
  // platform -- never set these manually as Edge Function secrets.
  const admin = createClient(
    Deno.env.get("SUPABASE_URL")!,
    Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!,
  );

  const { data: device, error: deviceErr } = await admin
    .from("devices")
    .select("id")
    .eq("token_hash", tokenHash)
    .maybeSingle();

  // A missing row or DB error both return 401 (D-08/D-09) before any
  // content_items query ever runs.
  if (deviceErr || !device) {
    return new Response(JSON.stringify({ error: "invalid token" }), {
      status: 401,
      headers: { "Content-Type": "application/json" },
    });
  }

  // T-2-07: device_id comes exclusively from the resolved token lookup
  // above -- never from a client-supplied parameter -- so no request can
  // ask for another device's content_items by ID.
  const { data: items, error: itemsErr } = await admin
    .from("content_items")
    .select("id, title, type, content_date, url")
    .eq("device_id", device.id)
    .is("delivered_at", null);

  if (itemsErr) {
    return new Response(JSON.stringify({ error: "query failed" }), {
      status: 500,
      headers: { "Content-Type": "application/json" },
    });
  }

  return new Response(JSON.stringify({ items }), {
    status: 200,
    headers: { "Content-Type": "application/json" },
  });
});
