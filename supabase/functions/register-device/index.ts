// Phase 2 Plan 1: register-device Edge Function (D-06/D-07/D-08, T-2-05).
//
// Admin-secret-gated device registration. Mints an opaque random token
// (D-08 -- not a JWT, trivial to generate/validate in C on the ESP32 later
// and trivially revocable by deleting the devices row), hashes it with
// SHA-256 before persisting (never stores the plaintext), and returns the
// plaintext token exactly once in this response body -- it is never logged
// or written anywhere else in the system (T-2-02).
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
  // T-2-05: the admin-secret check is the FIRST action -- return 401 before
  // any database access is attempted if the header is missing or wrong.
  const providedSecret = req.headers.get("X-Admin-Secret");
  const expectedSecret = Deno.env.get("ADMIN_REGISTRATION_SECRET");

  if (!expectedSecret || !providedSecret || providedSecret !== expectedSecret) {
    return new Response(JSON.stringify({ error: "unauthorized" }), {
      status: 401,
      headers: { "Content-Type": "application/json" },
    });
  }

  let label = "unlabeled-device";
  try {
    const body = await req.json();
    if (body && typeof body.label === "string" && body.label.trim() !== "") {
      label = body.label;
    }
  } catch {
    // No/invalid JSON body is fine -- label falls back to the default above.
  }

  // D-08: opaque random token, never a JWT.
  const token = crypto.randomUUID();
  const tokenHash = await sha256Hex(token);

  // SUPABASE_URL / SUPABASE_SERVICE_ROLE_KEY are auto-injected by the
  // platform -- never set these manually as Edge Function secrets.
  const admin = createClient(
    Deno.env.get("SUPABASE_URL")!,
    Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!,
  );

  const { data, error } = await admin
    .from("devices")
    .insert({ token_hash: tokenHash, label })
    .select("id")
    .single();

  if (error || !data) {
    return new Response(JSON.stringify({ error: "registration failed" }), {
      status: 500,
      headers: { "Content-Type": "application/json" },
    });
  }

  // The plaintext token appears here, in this one response, and nowhere
  // else in the system, ever.
  return new Response(
    JSON.stringify({ device_id: data.id, token }),
    { status: 201, headers: { "Content-Type": "application/json" } },
  );
});
