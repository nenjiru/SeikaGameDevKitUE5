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

- Create: Content Browser → right-click → Miscellaneous > Data Asset, and pick `PDA_GameEvent` (in the kit's `Events` folder). Name it `DA_<Event>` (e.g. `DA_BallDrained`), put it in `Data/Events/`, and describe the event in `Event Description`
- Reference: add a variable of type `PDA Game Event` (Object Reference) to the Blueprint, turn on Instance Editable, and set it to the event asset (as the default value, or per placed actor in the level)
- Send: call `Raise` on the variable
- Receive: in `BeginPlay`, call `Bind Event to On Raised` on the variable and connect a custom event to its `Event` pin. Every bound receiver is called each time the event is raised
- Bindings of actors that were destroyed (including those from the previous PIE session) are skipped automatically. Bind again in `BeginPlay` every time
- Events have no value yet

## License

[CC0 1.0](LICENSE)
