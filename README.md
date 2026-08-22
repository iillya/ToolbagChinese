# 八猴 5 汉化补丁

适用于 Marmoset Toolbag 5 的简体中文界面补丁。

补丁通过运行时内存 Hook 翻译界面文字，不替换 `toolbag.exe`，也不修改 Toolbag 的场景、资源、配置或官方语言文件。安装后必须使用“八猴5汉化版”快捷方式启动，汉化才会生效。

> 本项目为非官方社区项目，与 Marmoset 无隶属或授权关系。“Marmoset Toolbag”及相关商标归其权利人所有。

## 功能特点

- 覆盖 Toolbag 5 的菜单、面板、属性、提示及常用资源文字。
- 采用显示层精确匹配，不改动程序内部使用的英文标识。
- 同时处理文字绘制与尺寸测量，降低中文溢出、截断和布局错位的概率。
- 内置中文字体；仅在 Toolbag 缺少该字体时安装。
- 提供自包含的图形化安装器，无须安装 Python 或 Visual Studio。
- 自动创建桌面和开始菜单快捷方式，并为 `.tbscene` 场景注册汉化启动入口。
- 支持按 `F12` 短时捕获未翻译文字，便于持续补全词典。
- 拆卸时恢复文件关联，并仅删除由本安装器添加的中文字体。

## 系统要求

- 64 位 Windows 系统。
- 已安装 Marmoset Toolbag 5。
- 安装和拆卸时需要管理员权限。
- Toolbag 安装目录必须包含 `toolbag.exe`。

## 下载与安装

发布版本只有一个文件：

```text
dist\安装八猴汉化.exe
```

安装步骤：

1. 退出 Toolbag。
2. 双击 `安装八猴汉化.exe`，并允许管理员权限。
3. 确认自动识别的 Toolbag 目录；如识别错误，手动选择正确的 `toolbag.exe`。
4. 单击“安装汉化”，等待安装和自检完成。
5. 通过桌面或开始菜单中的“八猴5汉化版”启动 Toolbag。

安装器会将以下文件写入 `Toolbag 5\data\ChineseLocalizer`：

```text
dictionary_zh.json
ToolbagChineseHook.dll
ToolbagChineseLauncher.exe
ChineseLocalizer_sniffer.json
```

如果 `Toolbag 5\data\gui\font\notosans_chinese.slug` 不存在，安装器还会补装内置中文字体。若 Toolbag 已自带同名字体，则保留原文件，不会覆盖。

> 直接运行原版 `toolbag.exe` 不会加载汉化。请使用“八猴5汉化版”快捷方式、汉化启动器，或双击已经关联的 `.tbscene` 场景文件。

## 更新与拆卸

更新汉化时，直接运行新版安装器并再次安装即可。安装器会先备份现有插件目录；如安装过程失败，会尽量回滚到原状态。

完整拆卸步骤：

1. 运行安装时使用的 `安装八猴汉化.exe`。
2. 选择同一个 Toolbag 安装目录。
3. 单击“拆卸汉化”。

拆卸操作会：

- 关闭对应目录下正在运行的 Toolbag 和汉化启动器。
- 删除 `data\ChineseLocalizer` 及旧版插件残留。
- 删除安装器创建的桌面和开始菜单快捷方式。
- 恢复安装前的 `.tbscene` 文件关联。
- 清理汉化运行日志。
- 仅在安装记录表明字体由本补丁添加时，删除 `notosans_chinese.slug`。

## 工作原理

启动器按以下顺序加载汉化：

1. 以挂起状态创建 `toolbag.exe` 进程。
2. 通过远程 `LoadLibraryW` 将 `ToolbagChineseHook.dll` 注入进程。
3. 等待 DLL 完成词典加载和 Hook 安装。
4. 收到就绪信号后恢复 Toolbag 主线程。

如果 DLL 加载或关键 Hook 安装失败，启动器会终止仍处于挂起状态的 Toolbag，并显示诊断信息，避免在不完整的汉化状态下继续运行。

### Font-only 翻译架构

Hook 只介入字体编译和测量路径：

| 入口 | 用途 |
| --- | --- |
| 两个 `Font::CompiledString` 入口 | 在文字提交绘制前替换界面文本。 |
| 字体测量入口 | 使用相同译文计算宽度和高度。 |
| `F12` 捕获入口 | 在限定时间内记录未命中的可显示英文字符串。 |

运行时通过 `mset::Text` 的 RTTI 和虚表定位 `Text::draw`，再根据调用关系识别字体编译与测量函数。Hook 使用内联跳转和 trampoline 保留原函数执行流程，因此不依赖固定的模块基址或单一 RVA。

### 翻译规则

- 词典采用完整英文字符串精确匹配，不进行模糊替换。
- 匹配时保留原字符串首尾空白，防止控件对齐发生变化。
- 未命中的文本保持英文，不影响 Toolbag 原有行为。
- `None` 和 `NULL` 等哨兵值不翻译，避免破坏下拉框或空值逻辑。
- 翻译只影响最终显示文本，不改变场景数据和内部英文键值。

### 中文字体

`notosans_chinese.slug` 是 Toolbag 可识别的中文字体资源。Hook 不会硬编码加载该文件，而是依赖 Toolbag 自身的字体发现和回退机制：

- 字体不存在时，安装器将内置文件复制到 `data\gui\font`。
- 字体已经存在时，安装器不覆盖它。
- 拆卸时，只有安装器曾添加该字体，才会将其删除。

## 版本兼容性

补丁具备跨 Toolbag 5 小版本适配能力，但不能保证所有新版本都可直接使用。

相比固定地址 Hook，RTTI、虚表和调用结构定位对程序地址变化更有适应性；但下列内部实现一旦改变，仍可能需要重新适配：

- `mset::Text` 的虚表布局或 `Text::draw` 所在槽位。
- 字体编译函数的开头指令结构。
- 字体测量函数的函数签名或开头指令结构。
- Toolbag 的字体系统、渲染流程或模块组织方式。

Toolbag 更新后，建议先保留当前安装器，再进行验证。若汉化启动器提示 Hook 安装失败，请直接运行原版 `toolbag.exe` 使用 Toolbag，并等待补丁更新；原程序文件不会因此受损。

## 未翻译词条捕获

在汉化版 Toolbag 中按一次 `F12`，补丁会执行一次约 1.5 秒的短时捕获：

1. 清空本次捕获缓存。
2. 记录字体路径中未命中词典的可显示英文字符串。
3. 将结果写入 `data\ChineseLocalizer\ChineseLocalizer_sniffer.json`。
4. 捕获结束后，在资源管理器中选中该文件。

非捕获期间，嗅探逻辑只进行状态检查，不分配捕获字符串、不加捕获锁，也不写入磁盘。单次最多记录 10,000 条结果。

> `F12` 只会捕获触发期间实际绘制的文字。请先打开需要检查的窗口、菜单或面板，再按键捕获；复杂界面应分多次检查。

## 常见问题

### 安装器找不到 Toolbag

手动选择 Toolbag 安装目录中的 `toolbag.exe`。不要选择汉化启动器，也不要只选择 `data` 子目录。

### 通过原版快捷方式启动后仍是英文

这是预期行为。原版快捷方式直接启动 `toolbag.exe`，不会注入汉化 DLL。请改用“八猴5汉化版”快捷方式。

### 双击 `.tbscene` 没有进入汉化版

重新运行安装器完成关联注册。如 Windows 仍保留旧的默认打开方式，请在系统的“默认应用”中将 `.tbscene` 选择为“八猴5汉化版”。

### 部分文字仍为英文

通常表示该字符串尚未收入词典，或当前上下文中的原文与已有词条不完全一致。打开对应界面后按 `F12`，将生成的 `ChineseLocalizer_sniffer.json` 用于补充词典。

### 中文显示为方框、空白或乱码

检查以下文件是否存在：

```text
Toolbag 5\data\gui\font\notosans_chinese.slug
```

若文件缺失，关闭 Toolbag 后重新运行安装器。若文件存在但仍异常，可能是当前 Toolbag 小版本修改了字体加载或渲染流程。

### 启动器提示“汉化 Hook 安装失败”

这通常意味着当前 Toolbag 版本的内部结构已经变化，或安全软件阻止了 DLL 注入。可先尝试：

1. 确认启动器、DLL 和词典位于同一个 `data\ChineseLocalizer` 目录。
2. 将该目录加入安全软件信任列表后重试。
3. 运行原版 `toolbag.exe`，确认 Toolbag 本身能够正常启动。
4. 提交 Toolbag 的完整版本号和启动器显示的诊断信息。

### 安全软件报告风险

补丁使用进程挂起、远程内存写入和 DLL 注入来实现运行时汉化，这类行为可能触发启发式检测。请仅从可信发布渠道获取安装器；如你不接受这种实现方式，请勿安装。

## 从源码构建

开发环境：

- Visual Studio 2022 或 Visual Studio 2022 Build Tools。
- “使用 C++ 的桌面开发”组件，包含 x64 MSVC 工具链和 Windows SDK。
- Python 3，用于生成自包含安装器。

在仓库根目录执行：

```bat
plugin\scripts\build.bat
python plugin\scripts\tools\embed_files.py
```

第一条命令会生成：

```text
build\ToolbagChineseHook.dll
build\ToolbagChineseLauncher.exe
build\ChineseInstaller.exe
```

第二条命令会把安装脚本、词典、字体、Hook DLL 和启动器追加到原始安装器，最终生成：

```text
dist\安装八猴汉化.exe
```

`build.bat` 会通过 `vswhere` 自动查找 Visual Studio 2022 的 x64 C++ 工具链。普通用户使用发布版安装器时，不需要上述开发环境。

## 词典维护

主词典位于 `plugin\data\dictionary_zh.json`，使用 `sp-translation-v1` 结构：

```json
{
  "$schema": "sp-translation-v1",
  "id": "toolbag-zh",
  "language": "zh-CN",
  "translations": {
    "Translate": "平移",
    "Decal": "贴花"
  }
}
```

维护要求：

- JSON 必须保持 UTF-8 编码和合法语法。
- 英文键必须与 Toolbag 实际显示字符串完全一致。
- 译文应结合 Toolbag 的建模、烘焙、材质、渲染和摄影语境，不应只按字面翻译。
- 中文正文使用全角标点；数值、单位、快捷键、变量名及代码符号保留行业通用格式。
- 不要添加路径、内部符号、调试文本或不会显示的资源标识。
- 不要翻译 `None`、`NULL` 等可能参与程序逻辑的哨兵值。
- 修改词典后，需重新运行 `embed_files.py`，新词条才会进入单文件安装器。

### 全量扫描与覆盖率检查

默认 Toolbag 路径为 `C:\Program Files\Marmoset\Toolbag 5`。如安装在其他位置，请先设置 `TOOLBAG_DIR`：

```powershell
$env:TOOLBAG_DIR = 'D:\Marmoset\Toolbag 5'
```

然后在仓库根目录执行：

```powershell
python plugin\scripts\dictionary\scan_all_text.py
powershell -ExecutionPolicy Bypass -File plugin\scripts\tools\dump_ui_text.ps1
python plugin\scripts\dictionary\merge_captured.py
python plugin\scripts\dictionary\report_coverage.py
```

各步骤用途如下：

| 工具 | 用途 |
| --- | --- |
| `scan_all_text.py` | 扫描 `toolbag.exe`、官方英文文本及 `data` 目录中的候选字符串。 |
| `dump_ui_text.ps1` | 通过 Windows UI Automation 导出当前可见界面文字。 |
| `merge_captured.py` | 合并静态扫描、界面导出和 `F12` 捕获结果，并排除词典已有项。 |
| `report_coverage.py` | 生成覆盖率报告和待补充词条清单。 |

需要展开菜单进行界面导出时，可使用：

```powershell
powershell -ExecutionPolicy Bypass -File plugin\scripts\tools\dump_ui_text.ps1 -ExpandMenus
```

扫描器的筛选结果只是候选集，不等于可直接入库的翻译。合并前必须逐条确认它确实属于用户可见界面，并结合实际控件、功能模块和上下文审校译文。

## 项目结构

```text
Toolbag\
├─ README.md
├─ LICENSE
├─ dist\
│  └─ 安装八猴汉化.exe
├─ plugin\
│  ├─ data\
│  │  ├─ dictionary_zh.json
│  │  └─ notosans_chinese.slug
│  ├─ resources\
│  │  └─ 安装器与启动器图标资源
│  ├─ scripts\
│  │  ├─ build.bat
│  │  ├─ install.ps1
│  │  ├─ dictionary\
│  │  ├─ reverse\
│  │  └─ tools\
│  └─ src\
│     ├─ dllmain.cpp
│     ├─ installer.cpp
│     └─ launcher.cpp
└─ build\
   └─ 本地构建产物
```

## 参与改进

提交问题或词典修改时，请尽量附上：

- Toolbag 的完整版本号。
- 出现问题的功能模块和操作步骤。
- 英文原文、当前译文及建议译文。
- 能体现上下文的界面截图。
- 必要时附上 `ChineseLocalizer_sniffer.json`；提交前请检查其中是否含有不希望公开的信息。

涉及 Hook 失效时，请同时提供启动器的完整诊断信息。不要上传 Toolbag 的商业程序文件或受版权保护的资源。

## 许可证

本项目依据 GNU General Public License v3.0 发布，完整条款见 [LICENSE](LICENSE)。
