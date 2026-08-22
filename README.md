# 八猴 Toolbag 5 汉化

Marmoset Toolbag 5 简体中文运行时汉化补丁。

汉化通过 Font-only 内存 Hook 实现，不替换 Toolbag 原程序文件，也不修改其内部英文数据。

## 使用

运行：

```text
dist\安装八猴汉化.exe
```

安装器启动时会申请管理员权限：

1. 选择包含 `toolbag.exe` 的 Toolbag 5 安装目录。
2. 点击“安装汉化”或“拆卸汉化”。

安装后可通过桌面的“八猴5汉化版”快捷方式启动，也可以直接双击 `.tbscene` 场景文件。

拆卸会删除汉化文件、恢复 `.tbscene` 原版关联，并清理安装器创建的快捷方式和字体。

## 原理

运行时只 Hook 字体相关入口：

- 两个字体编译入口：将即将显示的英文替换为中文。
- 字体测量入口：按照中文文本计算 UI 宽度。

Toolbag 内部命令、枚举值和资源路径仍保持英文，避免汉化影响按钮及业务逻辑。

## 未翻译文本嗅探

在 Toolbag 位于前台时按一次 `F12`：

- 捕获 1.5 秒内出现的未翻译字体文本。
- 自动写入并定位：

```text
Toolbag 5\data\ChineseLocalizer\ChineseLocalizer_sniffer.json
```

未按 F12 时不会持续记录或写入磁盘。

## 从源码构建

需要 Visual Studio 2022 C++ 生成工具和 Python 3：

```bat
plugin\scripts\build.bat
python plugin\scripts\tools\embed_files.py
```

生成文件：

```text
dist\安装八猴汉化.exe
```

## 核心目录

```text
plugin\src\          Hook DLL、启动器和安装器源码
plugin\data\         中文字典和字体
plugin\scripts\      构建、安装及字典维护工具
plugin\resources\    程序图标
```

## 项目链接

- 作者：[Bilibili 神说要凑数汉化](https://space.bilibili.com/281243426)
- 仓库：[GitHub](https://github.com/iillya/Toolbag)

## 许可证

见 [LICENSE](LICENSE)。
