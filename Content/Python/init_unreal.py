"""エディタの起動時に UE が読む（Python Editor Script Plugin）。キットのメニューと MCP のツールを登録する。"""

import unreal

try:
    from seika_game_dev_kit import menu, toolset

    menu.register()
    toolset.register()
except Exception as error:  # キットの Python が動かなくても、エディタは起動させる
    unreal.log_warning(f'[Seika Game Dev Kit] Python の部品を登録できませんでした：{error}')
