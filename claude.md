# CLAUDE.md

## How to work on this project

**This project exists to learn systems C++, networking, concurrency, and
low-level OS work by hand. Producing working code quickly is not the goal.**

Therefore:

- **Never write, edit, or create files.** Do not use file-editing tools on this
  repository under any circumstances, including when asked to "just fix it" or
  when the change seems trivial.
- **Explain first, then show.** Lead with what the problem is, what the options
  are, and why one is better. The code comes after the reasoning, not instead
  of it.
- **Show pseudocode or short illustrative snippets in chat only.** Enough to
  convey structure and the specific API calls involved. Not a complete
  implementation to be pasted in.
- **Prefer pointing at the right primitive over writing the usage.** "You want
  `svcWaitSynchronization` here, and here's why the naive approach deadlocks" is
  more useful than a finished function.
- **Do not pre-empt design decisions.** When there's a real choice — framing
  format, threading model, API shape — lay out the tradeoffs and let the
  decision be made rather than picking one and moving on.
- **Say when something is worth struggling with.** Some bugs here are the point
  of the exercise. Debugging a partial read, a deadlock, or a wrong Lua stack
  index teaches more than being handed the fix.

Explaining an error, unblocking a stuck build, or clarifying an unfamiliar API
is always welcome. Writing the implementation is not.

The parts to be especially hands-off with, because they are the reason the
project exists: the threading model, the wire protocol, and any rendering code.
Setup, build configuration, and third-party boilerplate (OpenSSL context setup,
JSON handling on the proxy) can be discussed more freely.

## Project

A voice-driven "vibe coding" tool that runs on a Nintendo 3DS. The user speaks a
request; an LLM generates a complete Lua script for a small 2D game; the script
runs immediately on the console.

- **Top screen** — the running game.
- **Bottom screen** — chat log (LLM prose) and status.
- **Input** — microphone, transcribed off-device. Physical buttons and D-pad are
  the primary input for generated games; touch is secondary.

Two binaries, both C++:

- `console/` — the 3DS app. devkitARM, libctru, citro2d, embedded Lua 5.4.
- `proxy/` — a server on the developer's Mac. Holds the API key, terminates TLS,
  transcribes audio, talks to the LLM.

The 3DS speaks plain HTTP/TCP over LAN to the proxy. It never talks to the
internet directly — the console's SSL module cannot negotiate with modern
endpoints, and this is deliberate, not a limitation to work around.

## Non-goals

Do not add these unless explicitly asked:

- Compiling code on-device. Lua is the only runtime; there is no other path.
- Tool-calling / structured output APIs. The Lua block _is_ the structured output.
- A widget toolkit, layout engine, or HTML rendering. Rejected during design.
- 3D rendering, a software rasterizer, or stereoscopic output. Possible later,
  out of scope now.
- Sharing/multiplayer/server-hosted games. Later phase.
- `std::filesystem` on the 3DS — it is unreliable. Use libctru FS or `fopen`.
- Protobuf, nlohmann on the wire, or any dependency on the 3DS↔proxy link.

## Architecture

```
[3DS] --raw PCM / text--> [proxy] --HTTPS--> [transcription API]
                                  --HTTPS--> [LLM API]
      <--prose + lua-----
```

The proxy is the only component with internet access, secrets, or TLS.

### Wire protocol (3DS ↔ proxy)

Hand-rolled and binary. Length-prefixed frames:

```
[u32 length][u8 type][payload]
```

Little-endian, explicit `u32`/`u8` types, no packed structs shared across the
two builds. Duplicate the message definitions in each binary rather than sharing
a header — the compilers, alignment, and assumptions differ.

Message types: `AUDIO`, `TEXT_REQUEST`, `TRANSCRIPT`, `PROSE`, `SCRIPT`,
`ERROR_REPORT`.

Include a protocol version byte in the handshake from day one.

## The Lua API

The single most important design constraint: **keep it small**. Around 15
functions, all of which fit in the system prompt alongside worked examples.
Reliability of first-try generation depends on this more than on anything else
in the project.

Generated scripts define `update(dt)` and `draw()`.

```
-- drawing
draw_rect(x, y, w, h, color)
draw_sprite(spr, x, y)
draw_text(x, y, str)
clear(color)

-- input
btn(id)          -- held
btn_pressed(id)  -- edge-triggered this frame
touch()          -- returns x, y or nil

-- helpers
collide(ax, ay, aw, ah, bx, by, bw, bh)  -- AABB
button(x, y, w, h, label)                 -- immediate-mode, returns true on tap
rand(min, max)

-- state
state  -- global table, conventional home for game state
```

Screen is 400x240 (top). Coordinates are fixed; there is no responsive layout
and no need for one.

Version the API. Each generated script declares the API version it targets.

### Sandboxing

Build each `lua_State` without `io`, `os`, or `package`. Scripts are untrusted
by design — eventually they will come from other users.

Install an instruction-count hook (`lua_sethook`) that errors out after a
per-frame budget. A generated `while true do end` must surface as a chat
message, not a console hang.

## Execution model

One generation = one fresh `lua_State`.

```cpp
lua_State* L = luaL_newstate();
luaL_openlibs(L);          // then strip io/os/package
register_api(L);
luaL_loadbuffer(L, src, len, "game");  // compile errors surface here
lua_pcall(L, 0, 0, 0);                 // top level defines update/draw
```

Per frame: `lua_getglobal` + `lua_pcall` for `update`, then `draw`. **Never**
`lua_call` — always `pcall`.

State does not persist across regenerations. A new script restarts the game.
This is intentional; do not add serialization/restore.

Any error — compile or runtime — is captured as a string and sent back to the
LLM as the next turn, along with the current script. This feedback loop is the
core of the tool.

## LLM interaction

- Prompt for: a short conversational reply (2–3 sentences), then a single
  fenced ```lua block containing the **complete** script.
- Emphasize _complete_. Partial scripts and "the rest is unchanged" are the
  primary failure mode.
- Edits are full-script rewrites with the current script supplied as context.
  Scripts are small; diffs are not worth it.
- Parse: prose is everything outside the fence, script is everything inside.
  Handle zero fences (a valid conversational turn — not an error) and multiple
  fences (take the last).
- Keep prose short. The bottom screen is 320x240 and shares space with status.
- History lives on the proxy as a `std::vector<Message>`. Trim by keeping the
  system prompt and the last N turns; drop the middle.
- Have the proxy run a cleanup pass on transcripts against the known API name
  list before they reach the main prompt — transcription mangles identifiers.

Use `nlohmann/json` on the proxy. Do not hand-roll a JSON parser; that effort
belongs on the wire protocol, where both ends are owned.

## C++ conventions

C++20. devkitARM ships GCC 14; the full language is available.

- **RAII over every init/exit pair.** `camInit`/`camExit`, `micInit`/`micExit`,
  `romfsInit`, `lua_State*` via `unique_ptr` with a `lua_close` deleter. Camera
  and mic are initialized lazily and torn down on mode exit — leaking a service
  handle on an early return is the failure this prevents.
- **Never let a C++ exception cross a Lua binding boundary.** Lua uses `longjmp`
  and will skip destructors. Catch inside the binding, return a Lua error.
- `std::thread` for keeping network I/O off the render loop. A mutex-guarded
  queue between the network thread and the main loop; the `lua_State` is touched
  by exactly one thread, ever.

## Portability layer

Core logic must build and run natively on macOS, not just on the 3DS. Keep the
Lua bindings, game runtime, protocol code, and any rendering logic free of
libctru. Platform-specific code (framebuffer, buttons, mic, camera) lives behind
a thin interface with an SDL2 implementation for the Mac.

Three tiers, and code should be structured to support all three:

- **Mac native (SDL2)** — logic, fast iteration, real debugger, sanitizers.
- **Azahar emulator** — integration, graphics, input, chat UI. Note: Citra is
  discontinued; Azahar is the maintained fork.
- **Hardware** — anything with a sensor, and every performance measurement.
  Emulator timing does not reflect the ARM11 and is meaningless for profiling.

## Build order

Do not reorder these. Each step should run before the next begins.

1. Proxy: socket server, LLM call over HTTPS, hardcoded prompt, prints response.
2. 3DS ↔ proxy link: framing both ends, echo test.
3. Lua embed on Mac native: run a hardcoded script, bindings stubbed to stdout.
4. End-to-end ugly version: one button, hardcoded request, generated script
   renders via citro2d on hardware. **This is the milestone that matters.**
5. Mic: capture to buffer → dump to SD → verify offline. Then stream to proxy.
6. Chat UI on the bottom screen.
7. Error-feedback loop.
8. Prompt and API iteration. Expect to redesign the Lua API at least twice
   after observing real generations.

Keep `swkbd` text input available behind a button combo for development, even
though it is not in the shipped UX — prompt iteration means sending the same
request many times, and speaking each one is impractical.

## Working notes

- **libctru is the source of truth.** Headers are in
  `/opt/devkitpro/libctru/include/`. Training data on 3DS homebrew is thin and
  frequently wrong. Verify camera, mic, and service APIs against the headers
  rather than reproducing remembered snippets.
- No debugger on hardware. `consoleInit(GFX_BOTTOM, NULL)` and `printf`, or
  `3dslink -s` for stdout over the network.
- `3dslink` over WiFi for deploys. Do not swap the SD card to iterate.
- Camera output is RGB565 and can be blitted to the framebuffer without
  conversion. Encoding to PNG happens on the proxy, never on the console.
