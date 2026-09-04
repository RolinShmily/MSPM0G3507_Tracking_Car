# 项目级智能体规范 (Project AGENTS.md)

> 本文件定义当前 MSPM0 Keil 工程特定的 AI 编码智能体操作约束与规范。

---

## 1. Git 操作严格禁令 (Git Command Ban)

- **严禁执行任何 Git 命令**：智能体在此工程中**绝对禁止**调用命令行执行任何 `git` 相关操作（包括但不限于 `git status`、`git add`、`git commit`、`git diff`、`git branch`、`git reset` 等）。
- **版本控制人工管理**：所有 Git 提交、分支及版本控制操作均由用户在 Windows / VS Code 端人工管理。
- **Git调用方式**: 可以用Windows的`PowerShell`来执行`git`相关命令。
---

## 2. 工程分层与文件规范 (Architecture & Hygiene)

- **分层设计**：
  - `empty.c`：主业务逻辑与主循环。
  - `BSP/`：板级支持包封装（`LED`、`Key`、`Tick` 等）。
  - `empty.syscfg` / `ti_msp_dl_config.c/.h`：SysConfig 硬件引脚与外设配置生成文件。
- **Keil 工程同步**：
  - 当在 `BSP/` 目录新增源文件或头文件时，需同步维护 `keil/*.uvprojx` 中的文件分组（Group）与包含路径（IncludePath）。
