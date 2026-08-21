# 八猴 Toolbag 5 汉化插件

当前版本采用三层显示翻译，不修改 Toolbag 的资产、路径、搜索索引或配置原文：

- `Text`：翻译普通控件的主文字和辅助文字，并让布局按中文宽度计算。
- `MenuItem`：只翻译菜单项标题，不修改命令、回调或快捷键。
- `Font::CompiledString`：翻译不经过 `Text` 的自绘文字，并参与字形测量与绘制。

三个入口均通过机器码结构特征定位；字典采用完整英文字符串匹配，缺少条目时保留英文。

## 目录

- `src/`：DLL 与启动器源码。
- `dist/`：可安装文件、字典与中文字体。
- `scripts/`：构建、安装、字典维护及逆向分析工具。
- `build/`：本地编译中间文件。
- `backups/`、`reports/`：历史备份与分析报告。

## 构建与安装

运行 `scripts\build.bat` 构建，再以管理员权限运行 `scripts\install_plugin.ps1` 安装。

安装后请通过插件目录中的 `ToolbagChineseLauncher.exe` 启动 Toolbag。插件菜单入口只负责提示正确的启动方式，不支持运行中注入。

## 字典格式

```text
英文;中文
```

运行时追踪默认关闭。如需诊断，在插件安装目录创建 `trace.enabled` 后重新启动 Toolbag。
