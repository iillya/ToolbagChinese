# 八猴 Toolbag 5 汉化插件

当前只保留一个版本：

- `ToolbagChineseHook.dll`：完整 3-Hook 汉化版
- `dllmain.cpp`：对应源码
- `dictionary.txt`：汉化字典
- `__main__.py`：插件入口
- `inject_verify.py`：启动注入脚本

## 使用

插件方式：放入 Toolbag 插件目录，运行 ChineseLocalizer。

启动注入方式：

```text
python inject_verify.py
```

注意：该版本能正常加载并显示中文，但打开纹理界面可能闪退。

## 字典格式

```text
英文;中文
```