# KernelGlow 验收登记（ACCEPTANCE）

宿主：`F:\Cache\AI\KernelGlow-Host`（唯一宿主，全程复用，不修改用户现有工程）。
截图归档：`F:\Cache\AI\KernelGlow-Host\Saved\KernelGlowValidation\`。

## 支持矩阵

| 项 | 值 |
|---|---|
| 引擎 | UE 5.7.3（Changelist 50162420，`E:\Softwave\UE\UE_5.7`，安装版） |
| 平台 | Win64 / D3D12 / SM5 |
| GPU | NVIDIA（RTX 40 系，具体型号 M5 性能验收时记录） |
| 验证方式 | `UnrealEditor-Cmd.exe <host> -game /Engine/Maps/Templates/Template_Default -ExecCmds="kg.AutoScreenshot 1 kg.AutoQuit 1 [kg.ForceDisable 0|1]"`，插件 3 秒延迟自截图后自退出 |

## 关键实现决策记录

| 日期 | 决策 | 原因 |
|---|---|---|
| 2026-10-17 | 后处理注入走 `SubscribeToPostProcessingPass`，弃用 `PrePostProcessPass_RenderThread` | 5.7 中 `FPostProcessingInputs::SceneTextures` 变为 uniform buffer，无法取裸 SceneColor |
| 2026-10-17 | HDR 槽位选 **AfterDOF**（BL_SceneColorAfterDOF），非 MotionBlur 槽 | MotionBlur 槽按帧裁剪（静止相机常 disabled，订阅时序不可靠）；AfterDOF 恒订阅，且在曝光计量之后、tonemap 前 |
| 2026-10-17 | AfterTonemap 槽位保留给 `bCompositeHDR=false` 风格化路径 | 设计文档 5 节的合成时机选项 |
| 2026-10-17 | GlobalShader 需要 `CFLAG_AllowTypedUAVLoads` | UAV typed load 编译要求（D3D11 兼容旗标，D3D12 必需） |
| 2026-10-17 | uplugin 不写 `EngineVersion` 字段 | 引擎分支串报 5.7.0 而二进制为 5.7.3，字段会触发不兼容模态框阻塞 -game 启动 |
| 2026-10-17 | 安装版引擎上项目自产 Game exe 会卡死在 ShaderCodeLibrary | 验证一律走 `UnrealEditor-Cmd -game` |
| 2026-10-17 | 新增 `kg.AutoScreenshot / kg.AutoQuit / kg.ForceDisable` 验证管线 cvar | ExecCmds 的 HighResShot 在第 1 帧执行早于注入生效帧；插件侧延迟自截图解决时序 |

## 验收记录

| # | 任务 | 证据 | 结论 |
|---|---|---|---|
| 1 | Task 1-2 骨架 | UBT Editor/Development 双目标 Succeeded（日志 UBT Log.txt） | ✅ 数值通过 |
| 2 | Task 3 注入链路 | `T3_injection_green.png`（整屏纯绿，mean RGB=[0.8, 227, 0.2]，ON/OFF 同帧时像素差法确证注入生效） | ✅ 视觉通过 |
| 3 | Task 3 待办 | placeholder 1.35× tint 的 ON/OFF 差被自动曝光补偿（同帧时差≈0），真实管线的亮度 A/B 验收需**固定曝光**（Task 6 落实） | ⚠️ 记录在案 |
