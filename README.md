# 🚀 HOJ CLI

**新一代 HOJ (Hulu Online Judge) 命令行客户端** *由 [GeneLuo](https://github.com/gene-2012) 及其贡献者团队开发。*

[![License](https://img.shields.io/github/license/gene-2012/hoj-cli)](https://github.com/gene-2012/hoj-cli/blob/main/LICENSE)
[![Release](https://img.shields.io/github/v/release/gene-2012/hoj-cli)](https://github.com/gene-2012/hoj-cli/releases)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

---

## 简介

`hoj-cli` 是专为 HOJ 平台设计的轻量化工具，旨在让开发者无需离开终端即可完成题目提交、状态查询等操作。它支持跨平台使用，并提供灵活的配置管理。

## 快速开始

### 1. 安装方式

#### 方式 A：二进制安装 (推荐)
前往 [📦Releases](https://github.com/GeneLuo/hoj-cli/releases) 页面下载对应系统的构建产物，解压并将可执行文件添加至系统路径（PATH）即可。

#### 方式 B：源码编译
需要 **C++ 17** 或更高版本（强烈建议使用 **C++ 20**）。

```bash
git clone https://github.com/gene-2012/hoj-cli
cd hoj-cli

# Linux / macOS 编译
g++ -std=c++20 main.cpp -o hoj-cli

# Windows 编译 (需链接网络库)
g++ -std=c++20 main.cpp -o hoj-cli -lws2_32

```

### 2. 身份验证与配置

在使用 CLI 前，您需要配置有效的 `JSESSIONID` 以完成登录。

#### 配置文件优先级

`hoj-cli` 会按以下顺序寻找配置文件：

1. **工作区配置** (推荐)：当前目录下的 `./.hojcli.json`
2. **全局配置**：用户目录下的 `~/.config/hojcli/user.json`

#### 登录指令

```bash
# 推荐：使用 JSESSIONID 登录 (通过 --local 存入工作区)
hoj-cli login --jsessionid <YOUR_SESSION_ID> --local

# 备选：使用账号密码登录
hoj-cli login --username <USERNAME> --password <PASSWORD> --local

```

> [!WARNING]
> **说明**：由于账号密码登录涉及复杂的汉字编码校验，目前可能无法正常验证。**建议优先使用 JSESSIONID 方法**。此外，Session 会定期失效，届时请重新执行登录。


### 3. 常用操作指令

| 功能 | 命令示例 |
| --- | --- |
| **获取帮助** | `hoj-cli --help` |
| **查看登录状态** | `hoj-cli whoami` |
| **提交代码题目** | `hoj-cli submit <problem_id> -l <language> -f <code_file>` |


## 如何贡献

我们非常欢迎社区的贡献！如果您希望改进 `hoj-cli`，请遵循以下流程：

1. **Fork** 本仓库。
2. **创建特性分支**：`git checkout -b feature/AmazingFeature`。
3. **遵循规范**：
* 命名风格：请务必使用 **K&R** 风格。
* 可读性：保持代码逻辑清晰，并添加必要的注释。


4. **提交 Pull Request**。

## 📝 许可证

本项目基于 [MIT License](https://www.google.com/search?q=LICENSE) 协议开源。