# KernelGlow

UE 5.7 选择性 kernel 卷积辉光插件：只有被标记的高光参与卷积，kernel 半径任意大也不影响场景清晰度。独立于引擎 Bloom，设置持久化，鸣潮式视觉（清晰核心 + 彩色局部辉光 + 克制的大范围扩散）。

## 文档

- [设计文档](docs/2026-10-17-kernelglow-design.md)
- [实现计划](docs/plans/2026-10-17-kernelglow-implementation.md)
- [验收记录](docs/ACCEPTANCE.md)
- [插件包下载与安装](packages/README.md)

## 快速开始

1. 从 `packages/` 下载安装 ZIP（含 SHA-256）
2. 解压到工程 `Plugins/` 目录
3. 窗口 → KernelGlow 打开辉光面板

## 分支

- `main` — 文档与发布
- `feat/plugin-core` — 插件源码（当前开发线）
