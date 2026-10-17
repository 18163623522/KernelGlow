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
| 3 | Task 3 待办 | 后续验收改用固定曝光启动参数（ShowFlag.EyeAdaptation 0 + r.Bloom.Quality 0） | ✅ 已落实 |
| 4 | Task 4 测试关卡 | `/Game/Maps/TestGlowMap`：5 发光体(强度2/5/10/20/50，前2个Stencil=1) + 灰盒参照 + 唯一出生点；构建脚本 `Saved/KernelGlowValidation/build_test_level.py` | ✅ 数值通过 |
| 5 | Task 4 BrightPass+金字塔 | `T4_brightpass_glow.png`：发光球体带柔和光晕，参照灰盒完全干净（阈值选择性生效） | ✅ 视觉通过 |
| 6 | Task 4 待办 | 地面过暗（中性曝光下默认光强不足），Task 5 前补 3 盏平行光改善场景可见度 | ⚠️ 记录在案 |
| 7 | Task 5 数学单测 | `KGTests.log`：KernelGlow.KernelMath.* 6/6 成功 0 失败（ChooseLevel/KernelLod/Extent/TapUVs/Normalize/ProceduralKernel+mips） | ✅ 数值通过 |
| 8 | Task 5 kernel 纹理 | 模块懒创建程序化 kernel（256²，CPU mip 链，AddToRoot 防 GC）；用户贴图路径优先加载 | ✅ 数值通过 |
| 9 | Task 4 待办闭环 | 测试关卡补 3 盏平行光（中性曝光下地面可见），关卡已重建 | ✅ 已落实 || 10 | Task 6 排错记录 | ① 5.7 后处理注入改 PrePostProcessPass + FSceneTextureUniformParameters.SceneColorTexture（RealBloom 5.7 同款，AfterDOF 委托仅前 2 帧触发弃用）；② Build.cs 需显式加 Renderer/Private+Internal 包含路径；③ -ExecCmds 末位命令会被吞、分号不支持——验证开关改走 ini [SystemSettings]（launch_kg.ps1 -Mode）；④ 渲染线程禁触设置 UObject（快照缓存）与 kernel 创建（预创建） | ✅ 已落实 |
| 11 | Task 6 中间证据 | `T6_diag_bright0add.png`（bright0 叠加=柔和光晕）、`T6_diag_deep.png`（深层叠加=鸣潮式大光晕+参照盒干净）、`T6_diag_convreplace3.png`（卷积输出=稀疏亮点，单尺度偏弱） | ✅ 视觉通过（管线贯通） || 12 | Task 7 三尺度+核心回填 | `T7_multiscale_on.png`（清晰白核+近距青白+中距蓝+Wide 低能量，鸣潮式分层光晕）；`T7_multiscale_off.png` 基线；A/B 像素差：光晕区 diff≈136/255、蓝染生效（B>R）、**参照灰盒区 diff 0.6/255（场景锐度保持）** | ✅ 视觉通过（M2+M3 核心目标达成） |