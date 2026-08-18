# 参与贡献

感谢你改进微信输入法候选框颜色控制台。这个项目优先考虑安全边界、可审查性、启动速度和低常驻资源占用。

## 开始之前

1. 先搜索现有 Issue，避免重复工作。
2. 较大的 UI 或架构重构请先创建 Issue，说明目标、资源开销和兼容策略。
3. 不要在 Issue、提交或截图中上传 `WeTypeSkin.ini`、日志、PID、内存地址、IP、用户名、个人路径、API Key 或 Token。

## 本地构建

准备 Windows x64、PowerShell 5.1+ 和支持 C++17 的 MinGW-w64，然后运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

输出位于 `build\WeTypeSkinLauncher.exe`。

## Pull Request 要求

- 每个 PR 只解决一个明确问题。
- 解释行为变化、测试环境和验证结果。
- UI 改动请附去敏后的前后截图。
- 不提交 `WeTypeSkin.ini`、EXE、OBJ、ZIP、日志或其他本机运行状态。
- 不增加遥测、网络请求、广告、账号系统或不必要的常驻依赖。
- 保留目标进程名称校验、匹配数量上限和签名不明确时拒绝写入三项安全边界。

## 兼容性报告

报告兼容问题时可以提供：

- Windows 大版本和架构
- 微信输入法公开版本号
- 程序显示的去敏状态名称
- 可复现步骤

请勿提供进程转储、真实 PID、内存地址、个人目录或包含输入内容的截图。

提交贡献即表示你同意贡献内容按项目的 MIT License 发布。

