# fwx.ko Build for iStoreOS 24.10

为 iStoreOS 24.10.8 (kernel 6.6.144, x86_64) 交叉编译 fanchmwrt 的 fwx 内核模块。

## 编译方式

使用 GitHub Actions 云端编译，**无需本地 Linux 环境**。采用 OpenWrt 24.10.8 预编译 SDK，编译时间约 **5-10 分钟**（非完整固件编译的 1-2 小时）。

## 快速开始

### 第一步：创建 GitHub 仓库

1. 打开 https://repo.new
2. 仓库名随意，比如 `fwx-build`
3. **不要**勾选 "Add a README"、".gitignore" 或 "license"
4. 点击 "Create repository"
5. 复制仓库地址（类似 `https://github.com/你的用户名/fwx-build.git`）

### 第二步：推送代码

**方法 A：双击运行 `push-to-github.bat`**（推荐 Windows 用户）
- 双击项目目录中的 `push-to-github.bat`
- 按提示粘贴仓库地址
- 会弹出 GitHub 登录窗口（Git Credential Manager）

**方法 B：手动命令行**
```bash
cd fwx-build
git remote add origin https://github.com/你的用户名/fwx-build.git
git push -u origin main
```

### 第三步：触发编译

1. 在浏览器打开你的 GitHub 仓库页面
2. 点击顶部 **Actions** 标签
3. 左侧选择 **"Build fwx.ko for iStoreOS 24.10"**
4. 点击右侧 **"Run workflow"** 按钮
5. 分支选 `main`，点击绿色 **"Run workflow"**
6. 等待约 5-10 分钟

### 第四步：下载编译结果

1. 编译完成后（绿色对勾），点击该次运行
2. 页面底部 **Artifacts** 区域下载 `fwx-ko-istoreos-24.10.8-kernel6.6.144`
3. 解压得到：
   - `fwx.ko` — 内核模块
   - `kmod-fwx_*.ipk` — OpenWrt 安装包（如果生成了）
   - `build_fwx.log` — 编译日志
   - `INSTALL.txt` — 安装说明

### 第五步：在路由器上测试

```bash
# 上传到路由器
scp fwx.ko root@192.168.9.1:/tmp/

# SSH 登录
ssh root@192.168.9.1

# 加载模块
insmod /tmp/fwx.ko

# 验证
lsmod | grep fwx
dmesg | tail -20 | grep fwx
ls -la /proc/net/af_active_app /proc/net/af_active_host

# 如果一切正常，查看活跃应用
cat /proc/net/af_active_app
cat /proc/net/af_active_host

# 卸载
rmmod fwx
```

如果 `insmod` 报错 `invalid module format` 或 `exec format error`，说明 vermagic 不匹配，请检查 build log 中的 vermagic 信息。

## 目标环境

| 项目 | 值 |
|------|-----|
| 系统 | iStoreOS 24.10.8 (2026073111) |
| 内核 | 6.6.144 SMP mod_unload |
| 架构 | x86_64 |
| 编译器 | GCC 13.3.0 + musl (OpenWrt SDK 24.10.8) |

## 项目结构

```
fwx-build/
├── .github/workflows/build-fwx.yml   # GitHub Actions 编译流程
├── .gitattributes                     # 强制 LF 行尾（Makefile 必需）
├── .gitignore
├── package/fcm/fwx/
│   ├── Makefile                       # OpenWrt 内核包 Makefile
│   └── src/                           # fwx 内核模块源码（25 个文件）
│       ├── fwx.h                      # 版本 1.0.3，netlink ID 29
│       ├── fwx_main.c                 # 核心：netfilter hook、procfs、netlink
│       ├── fwx_conntrack.c            # 连接跟踪
│       ├── fwx_app_filter.c           # DPI 应用识别（AC 自动机）
│       ├── fwx_client.c               # 客户端管理
│       ├── fwx_client_fs.c            # 客户端文件系统
│       ├── fwx_mac_filter.c           # MAC 过滤
│       ├── fwx_config.c               # 配置管理
│       ├── fwx_log.c                  # 日志
│       ├── fwx_mac.c                  # MAC 工具
│       ├── fwx_utils.c                # 工具函数
│       ├── k_json.c                   # JSON 解析
│       └── regexp.c                   # 正则表达式
├── push-to-github.bat                 # Windows 一键推送脚本
└── README.md
```

## 技术说明

fwx 是 fanchmwrt 项目的内核模块，负责：
- 在 PRE_ROUTING 挂载 netfilter hook（优先级 CONNTRACK+1）
- 通过自维护 conntrack 哈希表追踪连接
- DPI 深度包检测（HTTP Host / HTTPS SNI / QUIC / DNS）
- 通过 AC 自动机匹配应用特征
- 通过 netlink（ID 29）与用户空间守护进程 fwxd 通信
- procfs 接口：`/proc/net/af_active_app`、`/proc/net/af_active_host`

**注意**：内核模块本身只提供数据采集能力，显示应用名还需要：
1. OAF 特征库（从 fanchmwrt 云端下发）
2. 用户空间守护进程 fwxd（数据存储和 ubus API）
3. LuCI 前端界面

本项目仅完成第一步——编译并验证内核模块可加载。
