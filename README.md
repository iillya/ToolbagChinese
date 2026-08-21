# 八猴 Toolbag 5 汉化插件

在**不修改 Toolbag 任何原始文件**的前提下，通过运行时内存 Hook 将 Marmoset Toolbag 5 的英文界面翻译为中文。

## 一、交付物（单文件）

最终交付物只有一个文件：

```
dist\安装八猴汉化.exe   （约 45 MB，自包含单文件安装器）
```

用户拿到后**只需这一个 exe**，不需要任何其它文件、不需要安装 Python / Visual Studio：

- 双击 → 弹一次管理员确认（点“是”）
- 弹出窗口选择：**“是”安装汉化** 或 **“否”卸载汉化**（取消=退出）
  - 安装：自动定位 Toolbag、安装插件/双字典/中文字体、自动备份原文件、安装自检
  - 卸载：关闭 Toolbag、删除插件目录、**还原原版字体**、清理运行时日志
- 全程**图形界面**，无命令行、无乱码，完成/失败都会弹窗提示

安装后需通过插件目录里的 `ToolbagChineseLauncher.exe` 启动 Toolbag 才生效。

## 二、工作原理

采用**三层显示翻译 + 一个兜底捕获**，全程只在内存中替换显示文本，不改动 Toolbag 的资产、路径、搜索索引或配置原文：

| 层 | 作用 |
|---|---|
| `Text::setText` | 翻译普通控件的主文字/辅助文字，并按中文宽度计算布局 |
| `Menu::MenuItem` | 只翻译菜单项标题，不修改命令、回调或快捷键 |
| `Font::CompiledString` | 翻译不经过 `Text` 的自绘文字（Slug 字体），并参与字形测量与绘制 |
| GDI/USER32 兜底 | 捕获 `ExtTextOutW / DrawTextW / SetWindowTextW/A`，把未命中字典的文字记入日志，保证“显示出来的文本不漏” |

三个引擎入口均通过**机器码结构特征**定位（不依赖固定地址，便于跨版本）。字典采用**完整英文字符串精确匹配**，缺条目时保留英文。

## 三、目录结构

```
Toolbag/
├─ dist\安装八猴汉化.exe     ← 最终交付物（唯一）
├─ plugin\                   ← 插件全部源码与资产
│  ├─ src\                   C++ 源码（dllmain / launcher / installer）
│  ├─ scripts\               构建、安装、字典维护、逆向分析工具
│  │   ├─ build.bat  install.ps1
│  │   ├─ dictionary\        字典维护脚本（扫描/合并/覆盖率/翻译）
│  │   ├─ tools\             embed_files.py（打包）、dump_ui_text.ps1（UI 转储）
│  │   └─ reverse\           逆向分析工具（RTTI/虚表/符号定位）
│  ├─ data\                  主字典（dictionary.txt 主 UI + dictionary_assets.txt 素材）
│  └─ assets\                插件资产（__main__.py、deng_ui.slug 中文字体）
├─ build\                    本地编译中间件（可再生成，已 gitignore）
└─ README.md  LICENSE  .gitignore  .gitattributes
```

## 四、从源码构建

前置：安装 **Visual Studio 2022 生成工具（含 C++ 生成工具）**（`build.bat` 会自动探测 vswhere）。

```bat
:: 1) 编译 DLL、启动器、原始安装器（中间产物输出到 build\）
plugin\scripts\build.bat

:: 2) 把所有文件嵌入，生成最终单文件安装器
python plugin\scripts\tools\embed_files.py
::    -> dist\安装八猴汉化.exe
```

## 五、字典维护（plugin\data\）

字典是纯文本，格式为 `英文;中文`，每行一条：

```text
Translate;平移
Decal;贴花
```

主字典与素材字典分开维护，便于独立更新：

- `plugin\data\dictionary.txt` —— 主 UI 字典（界面标签、设置、官方帮助文本）
- `plugin\data\dictionary_assets.txt` —— 素材/库名称字典

> 所有维护脚本都已指向 `plugin\data\`；修改字典后重新运行 `embed_files.py` 即可打进 exe。

### 常用维护命令（在仓库根目录执行）

```powershell
# 1) 全量扫描 Toolbag 的 UI 文本（exe + 官方语言文件 + 数据文件）
python plugin\scripts\dictionary\scan_all_text.py

# 2) 合并所有来源，生成待翻译清单
python plugin\scripts\dictionary\merge_captured.py

# 3) 覆盖率 / 缺口报告（最终质检）
python plugin\scripts\dictionary\report_coverage.py

# 4) 机器翻译回填（可选，需自行安装 argostranslate）
python plugin\scripts\dictionary\translate_merge.py [candidates_file]

# 5) 运行时 UI 树转储（需先用启动器运行 Toolbag）
powershell -ExecutionPolicy Bypass -File plugin\scripts\tools\dump_ui_text.ps1
powershell -ExecutionPolicy Bypass -File plugin\scripts\tools\dump_ui_text.ps1 -ExpandMenus
```

> 质检标准：以“显示时未命中字典”为准——只要运行时缺失日志不再出现新串，即说明实际运行时已无遗漏。

## 六、运行时诊断（可选）

- 缺失文本日志：`%LOCALAPPDATA%\Marmoset Toolbag 5\ChineseLocalizer_missing.tsv`
- 详细追踪日志：`%LOCALAPPDATA%\Marmoset Toolbag 5\ChineseLocalizer_trace.tsv`

追踪默认关闭。如需开启，在插件安装目录创建 `trace.enabled` 文件后重新启动 Toolbag。

## 七、常见问题

- **杀毒软件误报**：DLL 注入式汉化偶尔会被误报，请将插件目录加入信任白名单。
- **找不到 Toolbag**：安装器会自动定位（常见路径 + 注册表），找不到会弹窗让你手动选择 `toolbag.exe`。
- **如何完全还原**：用 `安装八猴汉化.exe` 选 `2` 卸载即可（删除插件目录、还原原版字体、清理日志）。

## 八、许可证

见 `LICENSE`。
