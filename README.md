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
| (none yet) | | | |

When the editor starts, `LogSeikaGameDevKit` writes the loaded kit version to the Output Log.

## License

[CC0 1.0](LICENSE)
