"""Unreal MCP から呼べるキットのツール（ToolsetRegistry のツールセット）。

ToolsetRegistry プラグイン（Unreal MCP の仕組み）がないプロジェクトでは、登録しない（キットはそのまま使える）。
"""

import json

import unreal

from seika_game_dev_kit import conventions

try:
    import toolset_registry
    from toolset_registry.registration import Registration
except ImportError:
    toolset_registry = None
    Registration = None


_registration = None

if toolset_registry is not None:

    @unreal.uclass()
    class SeikaKitTools(unreal.ToolsetDefinition):
        """Tools of the Seika Game Dev Kit (the course kit): checks that the project follows PROJECT_CONVENTIONS.md."""

        @toolset_registry.tool_call
        @staticmethod
        def check_conventions() -> str:
            """Checks Content/ against PROJECT_CONVENTIONS.md (folder tree, name characters, prefixes and asset types, where each kind goes). Read-only; no files are changed.

            The result is also written to Saved/Conventions/latest.json.

            Returns:
                JSON with checked_at, issue_count and issues (kind, path, message). kind is one of placement, name, prefix, prefix_type, folder_rule, redirector. Messages are in Japanese.
            """
            return json.dumps(conventions.run(), ensure_ascii=False)

        @toolset_registry.tool_call
        @staticmethod
        def reload_kit_python() -> str:
            """Reloads the kit's Python modules after they were edited (for kit development only).

            Returns:
                A short confirmation.
            """
            import seika_game_dev_kit
            toolset_registry.reload_module(seika_game_dev_kit)
            return 'reloaded'

    _registration = Registration([SeikaKitTools])


def register():
    if _registration is not None:
        _registration.register()
