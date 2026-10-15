"""エディタのメインメニューに「Seika Game Dev Kit」を足す。"""

import unreal

MENU_NAME = 'LevelEditor.MainMenu.SeikaGameDevKit'


def register():
    menus = unreal.ToolMenus.get()
    main_menu = menus.find_menu('LevelEditor.MainMenu')
    if main_menu is None:
        return
    if menus.find_menu(MENU_NAME) is None:
        main_menu.add_sub_menu('SeikaGameDevKit', '', 'SeikaGameDevKit', 'Seika Game Dev Kit', 'ゲーム制作演習のキット')
    menu = menus.find_menu(MENU_NAME)
    if menu is None:
        return

    entry = unreal.ToolMenuEntry(
        name='CheckConventions',
        type=unreal.MultiBlockType.MENU_ENTRY,
    )
    entry.set_label('決まりを点検')
    entry.set_tool_tip('PROJECT_CONVENTIONS.md の置き場所と名前の決まりに合っているかを調べる（ファイルは変えない）')
    entry.set_string_command(
        unreal.ToolMenuStringCommandType.PYTHON,
        '',
        'import seika_game_dev_kit.conventions as c; c.run_from_menu()')
    menu.add_menu_entry('Conventions', entry)
    menus.refresh_all_widgets()
