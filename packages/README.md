# KernelGlow 插件包下载与安装

## 当前包

| 文件 | 说明 |
|---|---|
| `KernelGlow-0.1.0-UE5.7-Win64-Editor.zip` | UE 5.7.3 Win64 编辑器预编译安装包（含源码） |

SHA-256：`fb5249b954b69d2d11542cdef60e1ddf415c30c035e1a6c3fd4edb8c9b8c5db1`

## 安装步骤

1. 解压 ZIP，得到 `KernelGlow` 文件夹（含 `.uplugin`）
2. 将 `KernelGlow` 文件夹复制到目标工程的 `Plugins/` 目录下
3. 打开（或重启）工程，插件管理器中确认 KernelGlow 已启用
4. 顶部菜单 **窗口 → KernelGlow**（或 **工具** 菜单）打开辉光面板

## 支持范围

- 引擎：UE 5.7.x Win64 / D3D12 / SM5+
- 仅编辑器 + Development/Shipping 运行时（本包为编辑器预编译）
- 不含移动端 / 其他平台二进制

## 回滚

删除工程 `Plugins/KernelGlow` 文件夹即可完全卸载；工程配置中的
`[/Script/KernelGlow.KernelGlowSettings]` 段可一并删除。
