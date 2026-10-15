# Changelog

## [Unreleased]

- Events: `PDA_GameEvent`, a data asset type with `Raise` and the `On Raised` dispatcher. One asset per event
- Events with a value: `PDA_GameEvent_Int`, `PDA_GameEvent_Float` and `PDA_GameEvent_String`
- Listener components: `BPC_GameEventListener` (and `_Int`, `_Float`, `_String`) receive an event through `On Event` without bind nodes
- `PDA_GameEventBase`, the parent of all event types, with `Event Description` and `On Any Raised` (called for every `Raise` with the value as text)
- Plugin skeleton with one Editor module, `SeikaGameDevKitEditor`. C++ is editor-only, so projects using the kit can be packaged without a C++ compiler
