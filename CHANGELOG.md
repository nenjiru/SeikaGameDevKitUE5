# Changelog

## [0.1.0] - 2026-10-15

First release, for Unreal Engine 5.8. Released together with the agent setup (`SeikaGameDevAgentUE5.zip` and `install.ps1`).

- Events: one data asset per game event. `PDA_GameEvent` (no value), `PDA_GameEvent_Int`, `PDA_GameEvent_Float` and `PDA_GameEvent_String`, all children of `PDA_GameEventBase` (`Event Description`, and `On Any Raised` called for every `Raise` with the value as text)
- Listener components: `BPC_GameEventListener` (and `_Int`, `_Float`, `_String`) receive an event through `On Event` without bind nodes
- Play log: every PIE session is recorded to `Saved/PlayLog/<date-time>.jsonl` (session start and end, level loads, kit events, warnings, errors, Print String, and data asset values changed during PIE). Can be turned off in Project Settings
- Convention check (Python): menu Seika Game Dev Kit > 決まりを点検, and the Unreal MCP tool `SeikaKitTools.check_conventions`. Results are also written to `Saved/Conventions/latest.json`
- C++ is editor-only (one Editor module, `SeikaGameDevKitEditor`, with prebuilt Win64 binaries), so projects using the kit can be opened and packaged without a C++ compiler
