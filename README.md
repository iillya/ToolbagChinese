# 八猴 5 简体中文补丁

当前仅提供 **Inno Setup 安装器**，文件名为 `ToolbagChineseInstaller.exe`。安装、更新、迁移及卸载说明见 [安装指南](docs/installer.md)。卸载请使用 Windows“已安装的应用”，不再使用旧安装器。

适用于 Windows 版 **Marmoset Toolbag 5** 的非官方简体中文补丁。安装后通过“Toolbag 中文版”启动，即可使用中文界面，同时保留 Toolbag 原有的命令、快捷键和工程数据。

## 下载与安装

普通用户只需下载 Releases 中的 `ToolbagChineseInstaller.exe`。

1. 保存工程并关闭 Toolbag。
2. 运行安装器；出现系统权限提示时选择“是”。
3. 选择包含 `toolbag.exe` 的 Toolbag 5 安装目录。
4. 按向导点击“安装”。
5. 安装完成后，从桌面或开始菜单打开“Toolbag 中文版”。

常见的默认目录：

```text
C:\Program Files\Marmoset\Toolbag 5
```

如果 Toolbag 安装在其他位置，直接选择实际目录即可。不要选择 `ChineseLauncher` 子目录。

## 如何启动

请使用“Toolbag 中文版”快捷方式启动。直接运行原版 `toolbag.exe` 不会加载汉化，这是正常现象。

如果快捷方式不可用，也可以直接运行：

```text
Toolbag安装目录\ChineseLauncher\ToolbagChineseLauncher.exe
```

汉化文件统一放在原软件目录的 `ChineseLauncher` 文件夹内，不会替换 `toolbag.exe`。

## 打开 `.tbscene` 文件

安装器会把中文启动器注册为 Toolbag 场景的打开方式。首次双击 `.tbscene` 文件时，Windows 可能仍会要求确认默认应用：

1. 选择“Toolbag 中文版”；
2. 点击“始终”。

之后双击场景文件即可通过中文启动器打开。卸载汉化时，安装器会恢复安装前的文件关联。

## 汉化范围

当前补丁主要翻译 Toolbag 自绘界面中的菜单、面板、按钮、选项、工具提示和常用提示文字。

汉化只改变运行时的显示文字。Toolbag 内部使用的英文命令、枚举值和资源路径保持不变，因此不会向 `.tbscene` 写入中文命令，也不会改变材质、模型、渲染或导出数据。

少量动态内容、插件界面或尚未收录的词条仍可能显示英文。个别固定尺寸区域可能因中文较长而出现拥挤或截断。

## 中文字体

所有受支持的 Toolbag 版本统一使用补丁内置的简体中文字体（阿里巴巴普惠体 2.0 Regular Slug，补丁内文件名为 `ToolbagChineseFont.slug`）。安装器会替换 Toolbag 的字体目录里以下三个字体（哪个存在就替换哪个，并备份原字体）：`notosans_chinese.slug`、`segoeui.slug`、`selawik.slug`。替换前会备份原字体，重复安装不会覆盖上一次备份，卸载汉化时会自动恢复原字体。

## 更新汉化

安装新版前先关闭 Toolbag，然后运行新版安装器并再次按向导点击“安装”。安装器会检查必要文件，并在失败时尽量恢复安装前状态。

## 卸载与恢复原版

保存工程并退出 Toolbag，在 Windows“已安装的应用”中卸载“Toolbag 中文补丁”。只删除补丁拥有的文件与快捷方式，保留修改过的词典、额外文件及备份，不删除用户工程。产品关联与字体的恢复规则见 [安装指南](docs/installer.md)。

## 常见问题

### 安装后仍然是英文

- 确认启动的是“Toolbag 中文版”，不是原版快捷方式。
- 确认 Toolbag 目录下存在 `ChineseLauncher` 文件夹。
- 关闭全部 Toolbag 进程，再使用最新版安装器覆盖安装。

### 安装器提示目录无效

请选择直接包含 `toolbag.exe` 的 Toolbag 5 文件夹，不要选择其上级目录或 `ChineseLauncher`。

### 提示 Toolbag 正在运行或文件被占用

保存并正常退出 Toolbag。如果仍有后台进程，请先保存并正常退出后重试，安装器不会强行关闭进程。

### 启动器提示 Hook 安装失败

启动器会停止本次启动，以免 Toolbag 进入不完整状态。请重新安装汉化，并检查安全软件是否隔离了 `ToolbagChineseHook.dll`。

### 安全软件报警

中文启动器需要在 Toolbag 启动时加载汉化 DLL，少数安全软件可能将这种行为误判为注入。请只从可信来源下载；如有疑虑，可检查源码或自行构建。

### 双击场景仍由原版 Toolbag 打开

在 Windows 的“打开方式”中选择“Toolbag 中文版”，并点击“始终”。Windows 10/11 可能不会允许安装器直接替用户决定默认应用。

### 中文不完整、翻译不准确或排版异常

提交反馈时请附上 Toolbag 完整版本号、问题界面的完整截图和复现步骤。安装过程若出现错误，提示框会给出安装错误日志的保存位置，也请一并提供该日志。

## 兼容性说明

- 支持 Windows x64 和 Marmoset Toolbag 5。
- 已对 Toolbag 5.0.0、5.0.1、5.0.2、5.0.3 做过静态兼容验证。
- 不同小版本的界面可能存在差异，部分新词条可能暂时显示英文。
- Toolbag 更新内部文字渲染方式后，可能需要等待补丁适配。
- 汉化不参与材质计算、烘焙、渲染和导出，不会改变这些功能的数据结果。

## 从源码构建

本节仅供开发者使用。需要 Visual Studio 2022 C++ 生成工具和 Python 3。

```bat
source\build.bat
```

生成的单文件安装器位于 `dist\ToolbagChineseInstaller.exe`。

```text
source\          Hook DLL、启动器、安装器和构建入口
translations\    正式中文字典
fonts\           中文兼容字体
icon\            程序与场景文件图标
scripts\         安装和恢复脚本
third_party\     第三方依赖
build\           本地构建文件与编译结果
dist\            可发布安装器
```

## 作者、许可与声明

- 作者：[Bilibili 神说要凑数](https://space.bilibili.com/281243426)
- 项目仓库：[GitHub](https://github.com/iillya/ToolbagChinese)

项目代码许可信息见 [LICENSE](LICENSE)。Zydis 使用 MIT 许可证，详见 `third_party/zydis/LICENSE`。内置 Slug 字体由本机安装的 Alibaba PuHuiTi 2.0 Regular 转换生成，字体版权归 Alibaba (China) Co., Ltd. 所有。Marmoset Toolbag 是 Marmoset LLC 的产品；本项目是非官方社区汉化补丁，与 Marmoset LLC 没有隶属关系。
