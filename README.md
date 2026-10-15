# Seika Game Dev Kit UE5

A genre-agnostic set of behind-the-scenes components for Unreal Engine 5.8, built to work with **Seika Game Dev Agent** — the Claude Code agent setup for a university game development course.
Before building new Blueprints, the agent checks the component list below and reuses a component when one fits.

Code comments and editor UI text are in Japanese, because the students read them.

This is the Unreal Engine counterpart of [SeikaGameDevKit](https://github.com/nenjiru/SeikaGameDevKit) for Unity. It is under development.

## Installation

The kit is a project plugin. Put this repository at `Plugins/SeikaGameDevKitUE5/` in your project.
Prebuilt binaries for Win64 are included, so no C++ compiler is needed to open the project.

The kit is read-only for students. Report bugs and missing features to the instructor.

### Packaging without a C++ compiler

The kit's C++ code is editor-only, so it does not make packaging require a compiler.
Engine plugins that are disabled by default and have runtime modules do. If you enable such plugins only for editor use (for example `ModelContextProtocol` and `PythonScriptPlugin`), limit them to the editor in the `.uproject`:

```json
{
	"Name": "ModelContextProtocol",
	"Enabled": true,
	"TargetAllowList": [ "Editor" ]
}
```

## Components

| Component | Module | Purpose | Usage |
|---|---|---|---|
| Events | Content `/SeikaGameDevKitUE5/Events/` | Blueprints notify each other of game events without direct references. One data asset per event. Listener components receive them without bind nodes | [Events](#events) |
| Play log | `SeikaGameDevKitEditor` | Automatically records what happened every time you play in the editor (PIE) | [Play log](#play-log) |
| Convention check | Python `seika_game_dev_kit.conventions` | Lists files that break the rules in `PROJECT_CONVENTIONS.md` (placement, names, prefixes and asset types, where each kind goes, redirectors) | [Convention check](#convention-check) |

When the editor starts, `LogSeikaGameDevKit` writes the loaded kit version to the Output Log.

## Events

Create one event asset per game event ("ball drained", "score changed", ...). Senders and receivers connect simply by referencing the same asset in the Details panel.

| Type | Value | Use for |
|---|---|---|
| `PDA_GameEvent` | none | Something happened ("ball drained") |
| `PDA_GameEvent_Int` | Integer | A count or score ("score changed" → 120) |
| `PDA_GameEvent_Float` | Float | A ratio or amount ("health changed" → 0.75) |
| `PDA_GameEvent_String` | String | A name or message ("stage cleared" → "Stage2") |

All types are children of `PDA_GameEventBase`.

- Create: Content Browser → right-click → Miscellaneous > Data Asset, and pick one of the types above (in the kit's `Events` folder). Name it `DA_<Event>` (e.g. `DA_BallDrained`), put it in `Data/Events/`, and describe the event in `Event Description`
- Reference: add a variable of the same type (Object Reference, e.g. `PDA Game Event Int`) to the Blueprint, turn on Instance Editable, and set it to the event asset (as the default value, or per placed actor in the level)
- Send: call `Raise` on the variable (with a value: `Raise` takes `Value`)
- Receive: in `BeginPlay`, call `Bind Event to On Raised` on the variable and connect a custom event to its `Event` pin (drag from the pin and choose Add Custom Event, so the event gets the matching `Value` input). Every bound receiver is called each time the event is raised
- Receive without binding: use a listener component instead (see below)
- Bindings of actors that were destroyed (including those from the previous PIE session) are skipped automatically. Bind again in `BeginPlay` every time
- Any type: `On Any Raised` (on `PDA_GameEventBase`) is called for every `Raise`, with the value as text (`ValueText`, empty for `PDA_GameEvent`). It is meant for behind-the-scenes components such as a play log, not for game logic
### Listener components

Receive an event without building the bind nodes. There is one component per event type.

| Component | Event type | `On Event` gives |
|---|---|---|
| `BPC_GameEventListener` | `PDA_GameEvent` | nothing |
| `BPC_GameEventListener_Int` | `PDA_GameEvent_Int` | `Value` (Integer) |
| `BPC_GameEventListener_Float` | `PDA_GameEvent_Float` | `Value` (Float) |
| `BPC_GameEventListener_String` | `PDA_GameEvent_String` | `Value` (String) |

1. In the Blueprint's Components panel, Add → the listener component for the event type
2. Select the component and set `Game Event` in the Details panel (per placed actor is also possible)
3. In the Details panel's Events section, click + next to `On Event`. The event node appears in the Event Graph; connect what should happen
   - The Events section appears only in the Blueprint editor (select the component in its Components panel), not in the Details panel of an actor placed in the level
   - When the event node already exists, the button next to `On Event` jumps to it instead of showing +
- The component binds in `BeginPlay` and unbinds in `EndPlay`. Add one component per event you want to receive
- If `Game Event` is empty, it prints a warning with the actor name when play starts

### Other value types

For another value type (a struct, an actor, ...), make it in the game project (not in the kit):

- Event type: a child Blueprint of `PDA_GameEventBase` with an `On Raised` dispatcher that has the value as input, and a `Raise` function that calls `Notify Raised` (with the value as text) and then `On Raised`
- Listener component (optional): an Actor Component with the same structure as `BPC_GameEventListener_Int`, with `Game Event` and `On Event` changed to the new type

## Play log

Nothing to set up. Once the kit is installed, every PIE session is recorded.

- Location: `Saved/PlayLog/<date-time>.jsonl` in the project (the newest 20 files are kept; `Saved/` is ignored by git)
- One JSON object per line. Every line has `kind`, `t` (seconds since PIE started) and `frame` (frames since PIE started)

| kind | When | Fields |
|---|---|---|
| `session_start` | PIE starts (before `BeginPlay`) | `date`, `unreal` (engine version), `level`, `mode` (`simulate` only when simulating) |
| `level_loaded` | A level is opened during PIE (Open Level, ...) | `level` |
| `event` | A kit event is raised | `name` (the event asset), `value` (as text; omitted for events without a value) |
| `log` | A warning or error (any category), or a Print String | `level` (`log`, `warning`, `error`), `category`, `message` |
| `summary` | The same entry occurred more than 10 times in one second | `of` (what), `second`, `total`, `omitted`, `last` (last value) |
| `session_end` | PIE stops | `duration`, `events` (number of raised events) |

- When the same entry (same event, or same log message) occurs more than 10 times in one second, the rest of that second is folded into one `summary` line. It is written with the next line, or when PIE stops
- Other logs (engine information) and `LogModelContextProtocol` (Unreal MCP) are not recorded
- Kit events loaded during PIE are recorded too. Bindings are removed when PIE stops
- To disable it, turn off Project Settings > Plugins > Seika Game Dev Kit > プレイを記録する (stored in `Config/DefaultEditor.ini`)
- Packaged games are not recorded
- When the editor is in the background, UE may slow it down, so `frame` grows slowly

## Convention check

Menu Seika Game Dev Kit > 決まりを点検 checks whether `Content/` follows `PROJECT_CONVENTIONS.md` at the project root (no files are changed). It needs the Python Editor Script Plugin (`PythonScriptPlugin`).

- What is checked (only inside `Content/_Project/`, except placement)
  - Placement: folders that are not in the folder tree, and files placed directly in a folder whose subfolders are defined. The tree is read from the diagram (`├─` `│` `└─`) in `PROJECT_CONVENTIONS.md`. Anything under a folder whose contents are not defined is allowed. The folders UE creates (`Developers`, `Collections`, `__ExternalActors__`, `__ExternalObjects__`) are allowed
  - Names: names may only use ASCII letters, digits, `_` and `-`
  - Prefixes: names without a prefix from the prefix table, and prefixes that do not match the asset type (e.g. `SM_` on something that is not a static mesh; `BP_` / `BPC_` / `PDA_` are told apart by the parent class). Prefixes in the table that the kit does not know are only checked for presence
  - Where each kind goes: Blueprints inside `Art/`; `L_` outside `Maps/`; `WBP_` outside `Blueprints/UI/`; `ABP_` outside `Blueprints/`; `IA_` / `IMC_` outside `Input/`; `DA_` / `PDA_` / `DT_` / `CF_` / `PM_` outside `Data/`; `LI_` outside `Art/Backgrounds/` and `Maps/`
  - Redirectors left after moving assets
- `Sandbox/` is checked for names and prefixes only. `_ThirdParty/` and plugins are not checked
- Results: a summary dialog, one warning per issue in the Output Log, and the assets are selected in the Content Browser. The result is also written to `Saved/Conventions/latest.json`
- For the agent: Unreal MCP toolset `seika_game_dev_kit.toolset.SeikaKitTools`, tool `check_conventions` (returns the same JSON). It is registered only when the `ToolsetRegistry` plugin (part of Unreal MCP) is enabled
- The rules for each prefix's asset type and folder are in the kit (`Content/Python/seika_game_dev_kit/conventions.py`, `TYPE_RULES` and `PLACE_RULES`). Update them together with `PROJECT_CONVENTIONS.md`

## License

[CC0 1.0](LICENSE)
