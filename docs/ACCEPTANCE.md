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
| 11 | Task 6 中间证据 | `T6_diag_bright0add.png`（bright0 叠加=柔和光晕）、`T6_diag_deep.png`（深层叠加=鸣潮式大光晕+参照盒干净）、`T6_diag_convreplace3.png`（卷积输出=稀疏亮点，单尺度偏弱） | ✅ 视觉通过（管线贯通） || 12 | Task 7 三尺度+核心回填 | `T7_multiscale_on.png`（清晰白核+近距青白+中距蓝+Wide 低能量，鸣潮式分层光晕）；`T7_multiscale_off.png` 基线；A/B 像素差：光晕区 diff≈136/255、蓝染生效（B>R）、**参照灰盒区 diff 0.6/255（场景锐度保持）** | ✅ 视觉通过（M2+M3 核心目标达成） || 13 | Task 8 来源系统 | 全屏亮度源 ✓（T7 系列截图）；RT 源接口已留（Settings.SourceRT）；**Custom Stencil 白名单：基础设施完成（ini 解析 ✓ stencilNum=1、BrightPass 掩码、可视化工具 kg.DebugSource=6），但 PrePostProcessPass 阶段 CustomStencil SRV 读出无效数据（D32S8 与 X24_G8 重解释不兼容，`T8_diag_stencil2.png` 噪点证据）——标记为实验性已知问题，后续换 Tonemap 后钩子或独立 stencil pass 解决** | ⚠️ 部分通过（已知问题记录） || 14 | Task 10 面板 | `T10_panel_editor.png`（编辑器内浮窗：InfoBar/开关/滑杆/kernel 选择条/三折叠卡/页脚）、`T10_panel_expanded.png`（展开态：尺度半径/强度滑杆）——布局与 WinUI3 原型一致，强调色青蓝，自绘开关/折叠卡/选择条 | ✅ 视觉通过（M4 UI 达成） |
| 15 | Task 9 持久化 | 面板显示 亮度阈值 0.5 系 DefaultEngine.ini 读回（跨会话）；总开关默认开；SaveConfig 每次改动写回 | ✅ 数值通过 || 16 | Task 11 三目标编译 | Editor ✓ / Development(Game) ✓ / Shipping ✓（Shipping 修复 UTexture2D MipGenSettings 访问后通过） | ✅ 数值通过 |
| 17 | Task 11 最终 A/B | `T11_final_on.png` / `T11_final_off.png`：ON vs OFF 像素差 39.8% 像素 |d|>8，平均 R+10.8/B-2.9（暖核+蓝晕），发光定位正确；kernel mip 链在 MipGen 行移除后仍有效 | ✅ 视觉通过 |
| 18 | 停止条件核对 | ① UI = 约定 WinUI3 单面板（T10 系列）✅ ② 效果接近鸣潮（T7/T11 系列：清晰核心+分层光晕+克制 Wide）✅ ③ 全程截图归档 KernelGlowValidation/ ✅ | ✅ 达成，停止 |

## 已知问题（诚实边界）

1. **Custom Stencil 白名单（实验性）**：PrePostProcessPass 阶段 CustomStencil SRV 读出无效数据（D32S8/X24_G8 重解释问题，证据 T8_diag_stencil2.png）。基础设施（ini 解析/掩码/可视化）已就绪，v1 以全屏亮度源交付；后续可换 AfterTonemap 钩子或独立 stencil 提取 pass。
2. 曝光计联动：注入点在曝光计量之后（无呼吸），但 HDR 合成的辉光能量参与后续 TSR 历史——连续运动场景的时序稳定性待 M5 后实测（静态/慢速场景已验证）。
3. 用户 ini 曾被验证脚本以 ASCII 编码写入导致中文注释损坏（已重写为纯 ASCII；正式用户配置不受影响）。| 19 | 过夜迭代 IT1 | 高斯衰减 kernel（中心亮斑+平滑边缘替代硬边圆盘）：`IT1_gaussian.png` 光散布自然度显著提升 | ✅ 视觉通过 |
| 20 | 过夜迭代 IT2 | 合成端轻度径向色散（RGB 微偏移）+ 抖动去色带：`IT2_chroma_dither.png` 边缘冷色散自然、暗部无色带 | ✅ 视觉通过 |
| 21 | UI 对比度+动画（用户反馈修复） | 未选中按钮 10%底+26%描边高对比两态；开关滑块渐变动画；折叠卡滑入淡入动画；自绘滑杆填充轨道（编译通过，`T10_panel_expanded_v2.png` 展开态布局与原型一致） | ✅ 视觉通过 |