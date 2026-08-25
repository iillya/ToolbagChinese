# 八猴5汉化版

Marmoset Toolbag 5 简体中文运行时汉化补丁。

补丁仅在内存中处理字体编译与测量，不修改 `toolbag.exe`，也不改写
Toolbag 内部的英文命令、枚举值和资源路径。

## 使用方法

1. 从 Releases 下载并运行 `安装八猴汉化.exe`。
2. 选择包含 `toolbag.exe` 的 Toolbag 5 安装目录。
3. 点击“安装汉化”。
4. 通过桌面的“八猴5汉化版”快捷方式启动。

安装器需要管理员权限。安装和拆卸始终作用于界面中选择的 Toolbag目录，
因此也支持非默认安装位置。

首次双击 `.tbscene` 时，Windows可能要求确认默认应用。选择“八猴5汉化版”
并点击“始终”即可；这是 Windows 10/11 的默认应用保护机制。

## 拆卸

重新运行安装器，选择对应的 Toolbag目录，然后点击“拆卸汉化”。拆卸会：

- 删除汉化 DLL、启动器、字典和场景图标；
- 删除安装器创建的快捷方式；
- 恢复安装前的 `.tbscene` 文件关联；
- 在替换过字体时恢复原版 `segoeui.slug`。

## 实现原理

启动器以挂起状态创建 Toolbag，注入 Hook DLL，安装成功后再恢复主线程。
运行时只 Hook 字体相关入口：

- 字体编译入口：把即将显示的英文替换为中文；
- 字体测量入口：按中文文本重新计算 UI宽度。

Hook DLL 使用 RTTI、控制流分析和 Zydis x64 指令解码器定位入口，并按完整
指令边界生成 trampoline。入口地址、虚表槽和函数序言均不按版本写死。

已静态验证 Toolbag `5.0.0`、`5.0.1`、`5.0.2` 和 `5.0.3`。

## 中文字体

如果目标版本包含 `notosans_chinese.slug`，安装器保留原字体配置。

如果缺少该字体，安装器会：

1. 将原版 `segoeui.slug` 备份为 `segoeui.slug.ChineseLocalizer.backup`；
2. 安装带中文字符的 `segoeui.slug`；
3. 重复安装时保留第一次创建的备份；
4. 拆卸时恢复原版字体。

## 未翻译文本嗅探

Toolbag 位于前台时按一次 `F12`：

- 捕获随后 1.5 秒内经过字体入口的未翻译文本；
- 自动保存并定位到：

```text
Toolbag 5\ChineseLauncher\ChineseLocalizer_sniffer.json
```

未按 F12 时不会持续记录或写入磁盘。

## 从源码构建

需要 Visual Studio 2022 C++生成工具和 Python 3：

```bat
source\build.bat
```

生成文件：

```text
dist\安装八猴汉化.exe
```

## 目录

```text
source\               Hook DLL、启动器、安装器和正式构建入口
translations\         正式中文字典
fonts\                兼容字体
icon\                 程序图标和 .tbscene 文档图标
scripts\              安装器运行脚本
third_party\          Zydis x64 指令解码器
```

## 作者与仓库

- 作者：[Bilibili 神说要凑数](https://space.bilibili.com/281243426)
- 仓库：[GitHub](https://github.com/iillya/Toolbag)

## 许可证

本项目许可证见 [LICENSE](LICENSE)。Zydis 使用 MIT许可证，详见
`third_party/zydis/LICENSE`。
