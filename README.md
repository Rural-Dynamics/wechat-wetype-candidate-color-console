# WeType Candidate Color Patcher (Unofficial)

一个轻量的 Windows 原生工具，用于调整微信输入法（WeType）浅色和深色模式下的候选项选中颜色。程序使用 Win32/C++ 编写，不依赖 WebView 或常驻运行时。

> [!IMPORTANT]
> 本项目是非官方实验工具，与腾讯、微信或微信输入法团队没有关联，也未获得其认可。微信、WeChat 和相关标识是其各自权利人的商标。本项目不包含腾讯源代码、二进制文件或官方图标。

## 工作原理与风险

程序查找本机的 `wetype_renderer.exe`，读取其可写私有内存区域，并将已知候选框颜色签名替换为用户选择的颜色。为了访问目标进程，后台任务可能需要以当前用户的最高权限运行。

- 输入法更新后，内存签名可能变化，届时补丁可能失效。
- 找不到唯一、兼容的签名时，程序会停止写入并在配置文件中记录状态。
- Windows Defender、SmartScreen 或其他安全软件可能对未签名且会访问其他进程内存的程序发出警告。
- 请自行评估风险；使用前建议关闭重要文档并保留输入法的安装包或恢复方式。

## 已测试环境

- Windows 11 x64
- 微信输入法 `2.1.2.11`

其他版本并未保证兼容。若版本不匹配，请不要反复强制运行。

## 普通用户使用

1. 从 GitHub Releases 下载便携版 ZIP 并完整解压。
2. 双击 `WeTypeSkinLauncher.exe` 打开设置窗口。
3. 选择浅色/深色模式候选项颜色，然后应用。
4. 如需登录后自动恢复颜色，以管理员身份运行 `install-wetype-native-task.ps1`。
5. 如需停止常驻监控，在程序中关闭服务；如需彻底移除任务，以管理员身份运行 `uninstall-wetype-native-task.ps1`。

程序首次运行后会在同目录创建 `WeTypeSkin.ini`。该文件包含当前进程 PID、临时内存地址和运行状态，不应公开分享。

## 从源码构建

要求：Windows x64、PowerShell 5.1+、支持 C++17 的 MinGW-w64（推荐 [w64devkit](https://github.com/skeeto/w64devkit)）。确保 `g++.exe` 和 `windres.exe` 位于 `PATH`。

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

输出文件位于 `build\WeTypeSkinLauncher.exe`。

对应的手动构建命令：

```powershell
windres .\WeTypeSkin.rc -O coff -o .\build\WeTypeSkin.res.o
g++ -std=c++17 -O2 -Wall -Wextra -municode -mwindows -static -static-libgcc -static-libstdc++ -s .\WeTypeSkinLauncher.cpp .\build\WeTypeSkin.res.o -o .\build\WeTypeSkinLauncher.exe -lgdiplus -ltaskschd -lole32 -loleaut32 -luuid -lversion -lcomdlg32 -lgdi32
```

## 配置

`WeTypeSkin.example.ini` 展示了可公开的配置项：

- `targets.dark`：深色模式候选项颜色，默认浅灰 `D4D4D4`
- `targets.light`：浅色模式候选项颜色，默认深灰 `3A3A3A`
- `signatures.*`：用于识别原始颜色的签名
- `max_matches`：允许匹配的最大数量，避免误写
- `poll_ms`：等待输入法进程出现时的轮询间隔

## 隐私

本程序没有网络通信、遥测、账号或数据上传功能。它不会收集输入内容。运行状态仅写入本地 `WeTypeSkin.ini`。

## 许可证

[MIT](LICENSE)

