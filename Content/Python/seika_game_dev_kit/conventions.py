"""決まりの点検。

プロジェクト直下の PROJECT_CONVENTIONS.md を読み、Content/ のファイルが決まりに合っているかを調べる（ファイルは変えない）。
- 置き場所：フォルダ構成の図（├─ │ └─）にないフォルダ、決まったフォルダの直下に置いたファイル
- 名前の文字：_Project/ の中で、半角の英字・数字・_・- 以外を使った名前
- 接頭辞：接頭辞の表にない名前、接頭辞とアセットの種類が合わないもの
- 種類と置き場所：Art/ の中の Blueprint、Maps/ の外のレベルなど（PROJECT_CONVENTIONS.md の文の決まり）
- リダイレクタ：移動の跡に残った転送用のファイル
結果は Saved/Conventions/latest.json にも書く。エージェントは MCP のツール（toolset.py）から呼ぶ。
"""

import datetime
import json
import os
import re

import unreal

# 図の中で「UE が作って使う」とされているフォルダ（Content/ の直下）
UE_FOLDERS = {'Developers', 'Collections', '__ExternalActors__', '__ExternalObjects__'}

# 名前に使える文字
NAME_PATTERN = re.compile(r'^[A-Za-z0-9_\-]+$')

ASSET_EXTENSIONS = ('.uasset', '.umap')

BLUEPRINT_CLASSES = {'Blueprint', 'WidgetBlueprint', 'AnimBlueprint'}


def _bp_type(data):
    return str(data.get_tag_value('BlueprintType') or '')


def _native_parent(data):
    return str(data.get_tag_value('NativeParentClass') or '')


def _is_bp(data, bp_type='BPTYPE_Normal'):
    return str(data.asset_class_path.asset_name) == 'Blueprint' and _bp_type(data) in (bp_type, '')


def _class_in(*names):
    return lambda data: str(data.asset_class_path.asset_name) in names


# 接頭辞ごとの「合うアセットの種類」。PROJECT_CONVENTIONS.md の表にない接頭辞は、種類を調べない
TYPE_RULES = {
    'BP_': lambda d: _is_bp(d) and 'Component' not in _native_parent(d) and 'DataAsset' not in _native_parent(d),
    'BPC_': lambda d: _is_bp(d) and 'Component' in _native_parent(d),
    'BPI_': lambda d: _is_bp(d, 'BPTYPE_Interface'),
    'BPL_': lambda d: _is_bp(d, 'BPTYPE_FunctionLibrary'),
    'WBP_': _class_in('WidgetBlueprint'),
    'ABP_': _class_in('AnimBlueprint'),
    'SM_': _class_in('StaticMesh'),
    'SK_': _class_in('SkeletalMesh'),
    'SKEL_': _class_in('Skeleton'),
    'PA_': _class_in('PhysicsAsset'),
    'AS_': _class_in('AnimSequence'),
    'AM_': _class_in('AnimMontage'),
    'BS_': _class_in('BlendSpace', 'BlendSpace1D', 'AimOffsetBlendSpace', 'AimOffsetBlendSpace1D'),
    'T_': _class_in('Texture2D', 'TextureCube', 'Texture2DArray', 'VolumeTexture', 'TextureRenderTarget2D'),
    'M_': _class_in('Material'),
    'MI_': _class_in('MaterialInstanceConstant'),
    'MF_': _class_in('MaterialFunction'),
    'PM_': _class_in('PhysicalMaterial'),
    'NS_': _class_in('NiagaraSystem'),
    'NE_': _class_in('NiagaraEmitter'),
    'A_': _class_in('SoundWave'),
    'MS_': _class_in('MetaSoundSource', 'MetaSoundPatch'),
    # Blueprint で作った型のデータアセットは、クラスの名前が「型の名前_C」になる
    'DA_': lambda d: str(d.asset_class_path.asset_name) in ('DataAsset', 'PrimaryDataAsset')
    or str(d.asset_class_path.asset_name).endswith('_C'),
    'PDA_': lambda d: _is_bp(d) and 'DataAsset' in _native_parent(d),
    'DT_': _class_in('DataTable', 'CompositeDataTable'),
    'CF_': _class_in('CurveFloat', 'CurveVector', 'CurveLinearColor'),
    'E_': _class_in('UserDefinedEnum'),
    'F_': _class_in('UserDefinedStruct'),
    'IA_': _class_in('InputAction'),
    'IMC_': _class_in('InputMappingContext'),
    'L_': _class_in('World'),
    'LI_': _class_in('World'),
}

# 接頭辞ごとの置き場所（_Project/ の中のフォルダ）。PROJECT_CONVENTIONS.md の文の決まりから。Sandbox/ は調べない
PLACE_RULES = {
    'L_': ('Maps',),
    'LI_': ('Art/Backgrounds', 'Maps'),
    'WBP_': ('Blueprints/UI',),
    'ABP_': ('Blueprints',),
    'IA_': ('Input',),
    'IMC_': ('Input',),
    'DA_': ('Data',),
    'PDA_': ('Data',),
    'DT_': ('Data',),
    'CF_': ('Data',),
    'PM_': ('Data',),
}


class FolderNode:
    """フォルダ構成の図の1つのフォルダ。children が空なら、中は自由。"""

    def __init__(self, name):
        self.name = name
        self.children = {}


def parse_folder_tree(text):
    """PROJECT_CONVENTIONS.md のフォルダ構成の図（Content から始まる ├─ │ └─ の図）を読む。"""
    root = None
    stack = []
    in_code = False
    for line in text.splitlines():
        if line.strip().startswith('```'):
            if in_code and root is not None:
                break
            in_code = not in_code
            continue
        if not in_code:
            continue
        if root is None:
            if line.strip().split(' ')[0] == 'Content':
                root = FolderNode('Content')
                stack = [root]
            continue
        match = re.search(r'[├└]─\s*(\S+)', line)
        if not match:
            continue
        # 深さ ＝ 名前の前にある「│」の数 ＋ 1
        depth = line[:match.start()].count('│') + 1
        node = FolderNode(match.group(1))
        del stack[depth:]
        stack[depth - 1].children[node.name] = node
        stack.append(node)
    return root


def parse_prefixes(text):
    """接頭辞の表（| `BP_` | ... |）を読む。長いものから並べる（BPC_ を BP_ より先に当てるため）。"""
    prefixes = re.findall(r'^\|\s*`([A-Za-z0-9]+_)`\s*\|', text, re.MULTILINE)
    return sorted(set(prefixes), key=len, reverse=True)


def _issue(kind, path, message):
    return {'kind': kind, 'path': path, 'message': message}


def run():
    """点検して、結果の辞書を返す（latest.json にも書く）。"""
    project_dir = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
    content_dir = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
    conventions_path = os.path.join(project_dir, 'PROJECT_CONVENTIONS.md')
    issues = []

    if not os.path.isfile(conventions_path):
        result = {'checked_at': _now(), 'error': 'PROJECT_CONVENTIONS.md がない', 'issue_count': 0, 'issues': []}
        _write_result(result)
        return result

    with open(conventions_path, encoding='utf-8') as f:
        text = f.read()
    tree = parse_folder_tree(text)
    prefixes = parse_prefixes(text)
    if tree is None:
        result = {'checked_at': _now(), 'error': 'フォルダ構成の図が読めない', 'issue_count': 0, 'issues': []}
        _write_result(result)
        return result

    registry = unreal.AssetRegistryHelpers.get_asset_registry()

    for dirpath, dirnames, filenames in os.walk(content_dir):
        rel_dir = os.path.relpath(dirpath, content_dir).replace('\\', '/')
        parts = [] if rel_dir == '.' else rel_dir.split('/')

        # UE が作って使うフォルダの中は見ない
        if parts and parts[0] in UE_FOLDERS:
            dirnames[:] = []
            continue

        node = _find_node(tree, parts)
        in_project = bool(parts) and parts[0] == '_Project'
        in_sandbox = len(parts) >= 2 and parts[0] == '_Project' and parts[1] == 'Sandbox'

        # 置き場所：中が決まっているフォルダ（図に子がある）に、図にないフォルダやファイルがないか
        if node is not None and node.children:
            for d in dirnames:
                if d in node.children or (not parts and d in UE_FOLDERS):
                    continue
                issues.append(_issue('placement', _game_path(parts + [d]),
                                     f'フォルダ構成の図にないフォルダ（{"/".join(["Content"] + parts)} の中に作れるのは {", ".join(node.children)}）'))
            for name in filenames:
                if name.endswith(ASSET_EXTENSIONS):
                    issues.append(_issue('placement', _game_path(parts + [_stem(name)]),
                                         f'{"/".join(["Content"] + parts)} の直下にはファイルを置かない（下のフォルダに置く）'))
            # 図にないフォルダの中は、それ自体を1件として、中までは見ない
            dirnames[:] = [d for d in dirnames if d in node.children]

        if parts and parts[0] == '_ThirdParty':
            dirnames[:] = []
            continue

        if not in_project:
            continue

        # 名前の文字（フォルダ）
        if parts and not NAME_PATTERN.match(parts[-1]):
            issues.append(_issue('name', _game_path(parts), '名前に使えない文字がある（半角の英字・数字・_・- だけにする）'))

        for name in filenames:
            if not name.endswith(ASSET_EXTENSIONS):
                continue
            stem = _stem(name)
            path = _game_path(parts + [stem])
            data = _asset_data(registry, path)
            # 移動の跡に残った転送用のファイル（名前や置き場所は調べない）
            if data is not None and str(data.asset_class_path.asset_name) == 'ObjectRedirector':
                issues.append(_issue('redirector', path,
                                     'リダイレクタ（移動の跡に残った転送用のファイル）。フォルダを右クリックして Update Redirector References で片付ける'))
                continue
            if not NAME_PATTERN.match(stem):
                issues.append(_issue('name', path, '名前に使えない文字がある（半角の英字・数字・_・- だけにする）'))
                continue
            prefix = next((p for p in prefixes if stem.startswith(p)), None)
            if prefix is None:
                issues.append(_issue('prefix', path, '接頭辞の表にない名前（種類の接頭辞を付ける）'))
                continue
            rule = TYPE_RULES.get(prefix)
            if data is not None and rule is not None and not rule(data):
                issues.append(_issue('prefix_type', path,
                                     f'接頭辞 {prefix} とアセットの種類（{_describe(data)}）が合わない'))
            if in_sandbox or len(parts) < 2:
                continue
            sub = '/'.join(parts[1:])
            # Art/ の中は見た目だけ
            if parts[1] == 'Art' and data is not None and str(data.asset_class_path.asset_name) in BLUEPRINT_CLASSES:
                issues.append(_issue('folder_rule', path, 'Art/ には Blueprint を置かない（Blueprints/ に置く）'))
                continue
            places = PLACE_RULES.get(prefix)
            if places and not any(sub == p or sub.startswith(p + '/') for p in places):
                issues.append(_issue('folder_rule', path,
                                     f'{prefix} のアセットは {" か ".join(places)} に置く'))

    result = {'checked_at': _now(), 'issue_count': len(issues), 'issues': issues}
    _write_result(result)
    return result


def run_from_menu():
    """エディタのメニューから点検する。結果を出力ログと、まとめの画面と、コンテンツブラウザで見せる。"""
    result = run()
    if result.get('error'):
        unreal.EditorDialog.show_message('決まりの点検', f'点検できなかった：{result["error"]}', unreal.AppMsgType.OK)
        return
    issues = result['issues']
    for issue in issues:
        unreal.log_warning(f'[決まりの点検] {issue["path"]}：{issue["message"]}')
    if not issues:
        unreal.EditorDialog.show_message('決まりの点検', '決まりから外れたファイルはありませんでした。', unreal.AppMsgType.OK)
        return
    lines = [f'・{i["path"]}：{i["message"]}' for i in issues[:15]]
    if len(issues) > 15:
        lines.append(f'ほか {len(issues) - 15} 件（すべては出力ログ）')
    unreal.EditorDialog.show_message(
        '決まりの点検',
        f'決まりから外れたファイルが {len(issues)} 件ありました。\n\n' + '\n'.join(lines),
        unreal.AppMsgType.OK)
    # 外れたアセットをコンテンツブラウザで選ぶ
    asset_paths = [i['path'] for i in issues if unreal.EditorAssetLibrary.does_asset_exist(i['path'])]
    if asset_paths:
        unreal.EditorAssetLibrary.sync_browser_to_objects(asset_paths)


def result_path():
    saved = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())
    return os.path.join(saved, 'Conventions', 'latest.json')


def _write_result(result):
    path = result_path()
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(result, f, ensure_ascii=False, indent=2)


def _find_node(tree, parts):
    node = tree
    for part in parts:
        if not node.children:
            return None
        node = node.children.get(part)
        if node is None:
            return None
    return node


def _describe(data):
    """アセットの種類を、人が読める形にする（Blueprint は親のクラスも添える）。"""
    class_name = str(data.asset_class_path.asset_name)
    if class_name == 'Blueprint':
        bp_type = _bp_type(data).replace('BPTYPE_', '')
        parent = _native_parent(data).rstrip("'").split('.')[-1]
        if bp_type and bp_type != 'Normal':
            return f'Blueprint の {bp_type}'
        return f'Blueprint、親は {parent}' if parent else 'Blueprint'
    return class_name


def _asset_data(registry, game_path):
    datas = registry.get_assets_by_package_name(game_path)
    return datas[0] if datas else None


def _game_path(parts):
    return '/Game/' + '/'.join(parts) if parts else '/Game'


def _stem(filename):
    return os.path.splitext(filename)[0]


def _now():
    return datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
