# KernelGlow 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. 用户已授权自主连续执行：M1→M5 顺序推进，不需要中途确认；停止条件 = UI 符合约定（WinUI3 单面板 4 卡片）且效果接近鸣潮，并留下全部截图证据。

**Goal:** UE5.7 选择性 kernel 卷积辉光插件 KernelGlow：kernel 半径任意大不糊场景，独立于引擎 Bloom 且状态持久化，鸣潮式视觉。

**Architecture:** Runtime 模块（FSceneViewExtension + RDG GlobalShader：kernel 预处理→来源提取→金字塔→多尺度卷积→核心回填→HDR 加性合成）+ Editor 模块（WinUI3 风格 Slate 单面板）。设置 `config=Engine` 持久化。

**Tech Stack:** UE 5.7.x Win64 / C++20 / RDG / HLSL (usf) / Slate / UBT。

## Global Constraints（每任务隐含遵守）

- **唯一宿主**：`F:\Cache\AI\KernelGlow-Host` 只创建一次、全程复用；**禁止反复创建/更换工程**（用户 2026-10-17 指定）。
- **不碰用户现有工程**：AIK_*、HostProject、SmartCommentBoxTestProject、TrueGlow-Host*、MCPGameProject 等一律只读，不得修改（用户指定）。
- **宿主最小化**：有干扰的插件默认关闭；只启用 KernelGlow 与必要引擎插件（用户授权）。
- 插件源码在 `F:\Cache\AI\KernelGlow`（独立 git）；宿主 uproject 以相对路径 `../../KernelGlow/KernelGlow.uplugin` 引用。
- **视觉验收纪律**：数值/编译通过≠完成；每任务完成必须有截图，存 `F:\Cache\AI\KernelGlow-Host\Saved\KernelGlowValidation\<任务号>_<描述>.png`，并在 `docs/ACCEPTANCE.md` 登记。
- 不改引擎源码；锁定的引擎小版本在 M1 探测后记录到 ACCEPTANCE.md 支持矩阵。
- 每任务结束 git commit（插件仓库）；消息用中文 conventional commits。

## 关键接口约定（跨任务引用）

- 设置类 `UKernelGlowSettings`（Runtime，`config=Engine`）字段：
  `bool bEnabled=true; float OverallIntensity=1.2f; float Threshold=1.0f; int32 Quality=1;`(0低1高2电影)
  `FScaleLayer Scales[3]`：`bool bEnabled; float Radius; float Intensity; FLinearColor Tint;`
  默认 Radius = {0.06f, 0.16f, 0.42f}，Intensity = {0.8f, 0.35f, 0.0f}，Tint = 近青白/中蓝/深蓝。
  `FCoreBackfill Core{float Radius=3.0f; float Strength=0.6f;}`，`bool bCompositeHDR=true; float Gain=1.0f; int32 TapBudget=1;`(0=4x4,1=8x8,2=16x16) `bool bFastMode=false;`
- 数学工具 `FKgKernelMath`（可单测纯函数）：
  `static float ComputeLodForRadius(float RadiusScreenFrac, float BaseSizeX);`
  `static void NormalizeWeights(TArrayView<float> Weights);`(和归一)
- ViewExtension：`FKgViewExtension : FSceneViewExtensionBase`，订阅后处理通道注入；入口 `PostProcessPass_RenderThread` 系列（对齐 RealBloom 5.6/5.7 用法，执行时以其源码为准适配 5.7 真实签名）。
- 编辑器面板：`SKgPanelWindow`（SCompoundWidget），注册菜单 Window→KernelGlow。
- 截图：编辑器控制台 `HighResShot 1920x1080`，输出目录 `Saved/Screenshots/WindowsEditor/`，执行后移动归档到 KernelGlowValidation。

---

### Task 1: 唯一宿主 KernelGlow-Host（一次性）

**Files:** Create `F:\Cache\AI\KernelGlow-Host\KernelGlowHost.uproject`、`Config\DefaultEngine.ini`、`Config\DefaultGame.ini`、`Config\DefaultInput.ini`、`README.md`

- [ ] 探测 UE 5.7 安装路径（`C:\Program Files\Epic Games\UE_5.7` 或由 TrueGlow-Host 的编译产物/注册表推断），记录完整版本号
- [ ] 写 uproject：`EngineAssociation` 探测值；Plugins 仅预留（本任务不引用插件）
- [ ] 写 DefaultEngine.ini：D3D12 RHI、默认后处理；显式禁用干扰项：`[/Script/Engine.RendererSettings]` 保持默认，`[Plugins]` 不启用额外插件
- [ ] 验证：目录结构完整，不启动编辑器（无插件无可验证视觉）
- [ ] Commit（宿主不建 git，生成物目录；以插件仓库 docs 记录路径）

### Task 2: 插件骨架 + UBT 三目标编译通过

**Files:** Create `KernelGlow.uplugin`、`Source/KernelGlow/{KernelGlow.Build.cs, Private/KernelGlowModule.cpp, Public/KernelGlow.h}`、`Source/KernelGlowEditor/{KernelGlowEditor.Build.cs, Private/KernelGlowEditorModule.cpp}`、`Source/KernelGlow/Private/KgViewExtension.{h,cpp}`（空注入）、`Source/KernelGlow/Public/KernelGlowSettings.h`（完整设置类）

- [ ] 按"关键接口约定"写设置类全字段（config=Engine，含 `PostEditChangeProperty` 重载空实现）
- [ ] ViewExtension 骨架：注册到 FSceneViewExtensions，全部回调空实现，仅日志一次注册成功
- [ ] Build：`"$(UE57)\Engine\Build\BatchFiles\Build.bat" KernelGlowEditor Win64 Development -project="F:\Cache\AI\KernelGlow-Host\KernelGlowHost.uproject" -WaitMutex`
- [ ] 期望：退出码 0、0 error；再编 Development(Game) 目标
- [ ] Commit `feat: 插件骨架（Runtime+Editor 模块、设置类、ViewExtension 注册）`

### Task 3: 渲染注入打通 + 编辑器启动 + 截图链路 + 视口策略

**Files:** Modify `KgViewExtension.{h,cpp}`（注入一个可辨识的 placeholder：拷贝 scene color 并加 5% 绿 tint）；Create `docs/ACCEPTANCE.md`

- [ ] 实现 SubscribeToPostProcessingPass / PrePostProcessPass（对照 RealBloom `RealBloom.cpp:138-160` 的 5.6 用法适配 5.7 签名）；视口过滤：SceneCapture 视口 fail-closed 跳过；材质/Niagara 预览视口跳过（按 ViewFamily/Viewport 类型判断，执行时按 5.7 API 定）
- [ ] RDG placeholder pass：AddPass 拷贝+乘 (1,1.05,1) 写回
- [ ] 启动：`UnrealEditor-Cmd.exe host.uproject -game` 后台？→ 首选编辑器窗口模式 `UnrealEditor.exe host.uproject`（后台进程），等 60s，`HighResShot` 经由 `-ExecCmds` 不可行（需运行时触发）→ **截图链路**：编辑器自动化优先（unreal-mcp 若本会话可用则 call_tool 截图；否则 `-ExecCmds="HighResShot 1920x1080"` 在启动时截首帧；再不行用 Python commandlet 截图）。任一路径拿到 PNG 即可，归档 `T3_placeholder_tint.png`
- [ ] 验证：截图画面偏绿（证明注入成功）；确认引擎 Bloom 默认开着而画面正常（此时插件只是 tint）
- [ ] Commit `feat: ViewExtension 注入打通（placeholder tint + 视口过滤）` + ACCEPTANCE.md 登记

### Task 4: 测试关卡（发光体矩阵）+ 软阈值 BrightPass + 金字塔

**Files:** Create `Shaders/Private/KgBrightPass.usf`、`Shaders/Private/KgDownsample.usf`；`Private/KgRenderPipeline.{h,cpp}`（pass 编排骨架）；宿主 `Content/Maps/TestGlowMap.umap`（Python commandlet 生成）

- [ ] Python commandlet 建图：地面 + 5 发光球（ emissive 强度 2/5/10/20/50、半径 0.3/0.5/1/1.5/2m，两个标 Stencil=1）+ 未标记参照灰盒 + 相机；存 TestGlowMap
- [ ] BrightPass CS：`soft_threshold(col, t, knee)`（膝部=0.5t），输出 f16
- [ ] Downsample CS：13-tap box（Karis 平均可选关）
- [ ] 编译 + 启动 + 截图：`T4_brightpass_mask.png`（调试视图直接输出 bright pass，应只见发光球）
- [ ] Commit

### Task 5: kernel 预处理（能量归一化 + mip 链）+ 数学单测

**Files:** Create `Shaders/Private/KgKernelPreprocess.usf`、`Public/KgKernelMath.h`、`Private/KgKernelMath.cpp`、`Private/Tests/KgKernelMathTests.cpp`

- [ ] TDD：先写 TEST（ComputeLodForRadius：radius 0.06/base 2560→期望 lod；NormalizeWeights：和=1±1e-5；零输入回退均匀权重）→ 跑失败 → 实现 → 跑通过（automation：`UnrealEditor-Cmd host.uproject -ExecCmds="Automation RunTests KernelGlow" -TestExit="Automation Test Queue Empty"`，断言退出码与日志 0 fail）
- [ ] GPU pass：kernel 贴图（暂用程序化圆盘生成 CS）→ 归一化（权重和=1）→ 生成 mip 链（自定义 downsample 保能量）
- [ ] 截图：`T5_kernel_mips.png`（调试视图输出 kernel mip0）
- [ ] Commit `feat: kernel 预处理与归一化数学（TDD）`

### Task 6: 单尺度卷积 + HDR 合成 + M2 视觉验收 ★

**Files:** Create `Shaders/Private/KgConvolve.usf`、`Shaders/Private/KgComposite.usf`；编排接入管线

- [ ] ConvolveCS：每像素 8×8 tap（宏 TAP_BUDGET），tap 位置=kernel 纹理均匀网格，按 ComputeLodForRadius 选 mip 采样权重，输入=对应金字塔级 bright pass；权重和恒一（CPU 侧预归一后按 mip 复检）
- [ ] CompositeCS：上采样（双线性）加回 scene color（HDR、Tonemap 前）；Gain/OverallIntensity 应用
- [ ] 曝光豁免决策落地：合成 slot 保证不影响眼适应直方图（执行时验证：开关插件，对比 -ExecCmds="r.EyeAdaptation.ExponentialTransitionDistance" 无关，肉眼截图亮度无呼吸）
- [ ] **M2 视觉验收**：`T6_single_scale_ON.png` / `T6_single_scale_OFF.png`（开关插件同机位）；标记者光晕、场景锐利
- [ ] Commit `feat: 单尺度卷积合成（M2 视觉通过）`

### Task 7: 三尺度 + 核心回填（鸣潮式）

**Files:** Modify 管线编排循环 3 尺度（半径 0.06/0.16/0.42，强度/染色按设置类）；Create `Shaders/Private/KgCoreBackfill.usf`

- [ ] 尺度循环复用金字塔对应级 + ConvolveCS（参数化 radius/tint/intensity）
- [ ] 核心回填：全分辨率 3px 小核（双 pass 横/纵或 9-tap）加性回填
- [ ] 截图：`T7_scales_off.png` / `T7_scales_default.png` / `T7_scales_wide_boost.png`（尺度3 强度 0.3 半径 0.8——验证大半径仍不糊场景）
- [ ] 对比 `T7_scales_wide_boost.png` 与 OFF：未标记区域目测锐度一致；归档
- [ ] Commit `feat: 三尺度卷积+核心回填（鸣潮式分层）`

### Task 8: 来源系统（Stencil 白名单 + RT + 全屏亮源开关）

**Files:** Create `Shaders/Private/KgSourceMask.usf`；Modify 管线输入合成；设置类加来源字段（`TArray<int32> StencilValues; UTextureRenderTarget2D* SourceRT; bool bUseFullScreenLuminance=false;`）

- [ ] SourceMaskCS：GBuffer CustomStencil 白名单 ∪ 外部 RT ∪（可选）全屏亮源，输出 mask
- [ ] BrightPass 输入改为 mask 加权
- [ ] 截图：`T8_stencil_only.png`（仅标记者发光，未标记灰盒无光晕）——**根治性验收**；`T8_rt_source.png`（RT 注入）
- [ ] Commit

### Task 9: 持久化契约验证

- [ ] 改设置→重启编辑器→截图 `T9_persisted.png`（面板/画面保持）；关卡切换验证
- [ ] 引擎 Bloom 开/关一致性：`r.Bloom.Quality 0` + PostProcess 关 bloom 前后，插件输出对比截图 `T9_engine_bloom_on/off.png`（目测逐像素一致，另跑像素差脚本：Python 读 PNG，非光晕区域平均差 < 2/255）
- [ ] Commit `feat: 持久化与独立性契约验证`

### Task 10: WinUI3 Slate 面板（M4 UI 验收）★

**Files:** Create `Source/KernelGlowEditor/Private/SKgPanelWindow.cpp`（+h）、`Private/KgPanelStyle.cpp`（Fluent 配色/圆角 brush）、3 自绘控件（折叠卡片 SKgExpanderCard / 开关 SKgToggleSwitch / SKgSelectorBar）

- [ ] 按 `docs/ui-proto-kernelglow-panel.html` 实现：4 卡片（首卡常开：启用/强度/阈值/kernel 槽+程序化形状；光晕形状折叠；来源折叠；高级折叠），折叠头摘要文本，InfoBar 契约文案，强调色 #0FA7E6，圆角 8
- [ ] 绑定 UKernelGlowSettings（改动即生效 + SaveConfig）
- [ ] 截图：`T10_panel_default.png`、`T10_panel_expanded.png`、`T10_panel_narrow.png`（停靠窄宽 320px）、`T10_panel_dpi150.png`
- [ ] 对照原型逐块核对（布局/文案/配色），差异记录 ACCEPTANCE.md
- [ ] Commit `feat: WinUI3 风格设置面板（M4 UI 视觉通过）`

### Task 11: M5 全清单验收 + 性能 + 交付

- [ ] 设计文档第 9 节 11 条视觉验收逐项过图，截图编号入 ACCEPTANCE.md（含 kernel 0.06 vs 0.5 屏耗时对比 <5%）
- [ ] 三目标编译终验（Editor/Development/Shipping）
- [ ] `docs/ACCEPTANCE.md` 完整登记 + 支持矩阵（引擎版本/GPU/分辨率）
- [ ] 最终汇报：所有截图路径列表给用户
- [ ] Commit `docs: M5 验收登记`

## Self-Review 结论

- 规格覆盖：渲染管线(T4-T8)、契约(T9)、UI(T10)、验收纪律(每任务)、性能(T11)、风险盯守项(任务内步骤)——无遗漏任务。
- 类型一致性：设置类字段与接口约定一致；Shader 文件名与编排引用一致。
- 执行适配说明：Task 3 截图链路与 Task 6 合成 slot 依赖 5.7 真实 API/可用工具，允许在"期望行为不变"前提下选择具体实现路径，选择记录进 ACCEPTANCE.md。
