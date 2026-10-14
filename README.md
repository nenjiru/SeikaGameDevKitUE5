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
| Events | Content `/SeikaGameDevKitUE5/Events/` | Blueprints notify each other of game events without direct references. One data asset per event | [Events](#events) |

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
- Bindings of actors that were destroyed (including those from the previous PIE session) are skipped automatically. Bind again in `BeginPlay` every time
- Any type: `On Any Raised` (on `PDA_GameEventBase`) is called for every `Raise`, with the value as text (`ValueText`, empty for `PDA_GameEvent`). It is meant for behind-the-scenes components such as a play log, not for game logic
- Other value types (a struct, an actor, ...): make a new child Blueprint of `PDA_GameEventBase` with an `On Raised` dispatcher that has the value as input, and a `Raise` function that calls `Notify Raised` (with the value as text) and then `On Raised`

## License

[CC0 1.0](LICENSE)
