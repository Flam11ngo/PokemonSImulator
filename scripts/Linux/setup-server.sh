#!/usr/bin/env bash
# =============================================================================
# PokemonSimulator — 一键部署脚本 (全新云服务器)
# =============================================================================
# 用法:
#   chmod +x scripts/setup-server.sh
#   sudo ./scripts/setup-server.sh              # 完整部署 (Docker + C++ 引擎 + 前端)
#   sudo ./scripts/setup-server.sh --minimal    # 仅 C++ 引擎 (无 Docker 大数据栈)
#   sudo ./scripts/setup-server.sh --dev        # 开发模式 (含前端 dev server)
#   sudo ./scripts/setup-server.sh --help       # 查看帮助
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# 颜色输出
# ---------------------------------------------------------------------------
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
BLUE='\033[0;34m'; CYAN='\033[0;36m'; NC='\033[0m'
log()  { echo -e "${GREEN}[INFO]${NC}  $(date '+%H:%M:%S') $*"; }
warn() { echo -e "${YELLOW}[WARN]${NC}  $(date '+%H:%M:%S') $*"; }
err()  { echo -e "${RED}[ERROR]${NC} $(date '+%H:%M:%S') $*"; }
info() { echo -e "${CYAN}[*]${NC}    $*"; }
hdr()  { echo -e "\n${BLUE}============================================================${NC}"; echo -e "${BLUE}  $*${NC}"; echo -e "${BLUE}============================================================${NC}"; }

# ---------------------------------------------------------------------------
# 参数解析
# ---------------------------------------------------------------------------
MODE="full"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --minimal) MODE="minimal" ;;
        --dev)     MODE="dev" ;;
        --help|-h)
            echo "用法: $0 [--minimal | --dev | --help]"
            echo ""
            echo "  (无参数)    完整部署: Docker 大数据栈 + C++ 引擎 + 前端 (Nginx)"
            echo "  --minimal   最小部署: 仅 C++ 引擎 (无 Docker, 无前端)"
            echo "  --dev       开发模式: Docker 基础服务 + C++ 引擎 (本地编译) + Node.js"
            echo "  --help      显示此帮助信息"
            exit 0
            ;;
        *) err "未知参数: $1"; exit 1 ;;
    esac
    shift
done

# ---------------------------------------------------------------------------
# 前置检查
# ---------------------------------------------------------------------------
hdr "PokemonSimulator 一键部署脚本"

if [[ $EUID -ne 0 ]]; then
    err "请使用 sudo 运行此脚本"
    exit 1
fi

# 获取实际用户 (即使通过 sudo 运行)
REAL_USER="${SUDO_USER:-$USER}"
REAL_HOME=$(eval echo "~$REAL_USER")
PROJECT_DIR="${REAL_HOME}/PokemonSimulator"

# 检测操作系统
if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    OS_NAME="$ID"
    OS_VERSION="$VERSION_ID"
else
    err "无法检测操作系统 (需要 /etc/os-release)"
    exit 1
fi

log "检测到操作系统: ${OS_NAME} ${OS_VERSION}"
log "模式: ${MODE}"
log "实际用户: ${REAL_USER}"
log "项目目录: ${PROJECT_DIR}"

case "$OS_NAME" in
    ubuntu|debian) log "Ubuntu/Debian 系统, 使用 apt 包管理器" ;;
    centos|rhel|fedora|rocky|almalinux)
        warn "RHEL 系系统 — 脚本仅支持 apt, 请手动安装依赖或使用 Ubuntu 22.04+"
        exit 1
        ;;
    *) warn "未测试的操作系统: $OS_NAME, 继续尝试 apt..." ;;
esac

# ---------------------------------------------------------------------------
# 阶段 1: 系统基础依赖
# ---------------------------------------------------------------------------
hdr "阶段 1/6: 安装系统基础依赖"

apt-get update -qq

# 基础工具链
log "安装基础工具链..."
apt-get install -y -qq \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    curl \
    wget \
    ca-certificates \
    gnupg \
    lsb-release \
    software-properties-common \
    unzip \
    jq \
    tmux \
    htop

# C++ 引擎编译依赖
log "安装 C++ 编译依赖..."
apt-get install -y -qq \
    libcurl4-openssl-dev \
    nlohmann-json3-dev

log "系统依赖安装完成 ✓"

# ---------------------------------------------------------------------------
# 阶段 2: Docker + Docker Compose (非 minimal 模式)
# ---------------------------------------------------------------------------
if [[ "$MODE" != "minimal" ]]; then
    hdr "阶段 2/6: 安装 Docker + Docker Compose"

    if command -v docker &>/dev/null; then
        log "Docker 已安装: $(docker --version)"
    else
        log "安装 Docker Engine..."
        # Docker 官方 GPG 密钥
        install -m 0755 -d /etc/apt/keyrings
        curl -fsSL https://download.docker.com/linux/${OS_NAME}/gpg \
            -o /etc/apt/keyrings/docker.asc
        chmod a+r /etc/apt/keyrings/docker.asc

        echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] \
https://download.docker.com/linux/${OS_NAME} $(lsb_release -cs) stable" \
            > /etc/apt/sources.list.d/docker.list

        apt-get update -qq
        apt-get install -y -qq \
            docker-ce \
            docker-ce-cli \
            containerd.io \
            docker-buildx-plugin \
            docker-compose-plugin

        # 将用户加入 docker 组
        usermod -aG docker "$REAL_USER"
        log "Docker 安装完成 ✓"
        warn "用户 '${REAL_USER}' 已加入 docker 组 — 重新登录后生效"
    fi

    # 验证 docker compose 可用
    if docker compose version &>/dev/null; then
        log "Docker Compose 可用: $(docker compose version)"
    else
        err "Docker Compose 不可用, 请检查安装"
        exit 1
    fi
else
    hdr "阶段 2/6: 跳过 (minimal 模式)"
fi

# ---------------------------------------------------------------------------
# 阶段 3: Python 3.12 + Node.js (按模式)
# ---------------------------------------------------------------------------
hdr "阶段 3/6: 安装 Python & Node.js 运行时"

# Python 3.12
PYTHON_NEEDED=false
if [[ "$MODE" == "dev" ]]; then
    PYTHON_NEEDED=true
fi

if $PYTHON_NEEDED || [[ "$MODE" == "minimal" ]]; then
    if command -v python3.12 &>/dev/null; then
        log "Python 3.12 已安装: $(python3.12 --version)"
    else
        log "安装 Python 3.12..."
        apt-get install -y -qq python3.12 python3.12-venv python3-pip || {
            warn "系统仓库无 Python 3.12, 尝试 deadsnakes PPA..."
            add-apt-repository -y ppa:deadsnakes/ppa
            apt-get update -qq
            apt-get install -y -qq python3.12 python3.12-venv python3-pip
        }
        log "Python 3.12 安装完成 ✓"
    fi
fi

# Node.js 20 LTS
NODE_NEEDED=false
if [[ "$MODE" == "dev" ]] || [[ "$MODE" == "full" ]]; then
    NODE_NEEDED=true
fi

if $NODE_NEEDED; then
    if command -v node &>/dev/null; then
        log "Node.js 已安装: $(node --version)"
    else
        log "安装 Node.js 20 LTS..."
        curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
        apt-get install -y -qq nodejs
        log "Node.js 安装完成: $(node --version) ✓"
    fi

    # npm
    if command -v npm &>/dev/null; then
        log "npm 已安装: $(npm --version)"
    fi
fi

# ---------------------------------------------------------------------------
# 阶段 4: 克隆/更新项目代码
# ---------------------------------------------------------------------------
hdr "阶段 4/6: 获取项目代码"

if [[ -d "$PROJECT_DIR/.git" ]]; then
    log "项目目录已存在, 更新代码..."
    cd "$PROJECT_DIR"
    git fetch origin
    git reset --hard origin/main
    log "代码已更新到最新 ✓"
else
    log "克隆项目仓库..."
    if [[ -d "$PROJECT_DIR" ]]; then
        warn "目录已存在但不是 git 仓库, 将备份后重新克隆..."
        mv "$PROJECT_DIR" "${PROJECT_DIR}.bak.$(date +%Y%m%d%H%M%S)"
    fi

    # TODO: 替换为实际的 git 仓库地址
    git clone https://github.com/Flam11ngo/PokemonSimulator.git "$PROJECT_DIR" || {
        # 如果网络克隆失败, 尝试从本地复制 (适用于 rsync 部署场景)
        warn "GitHub 克隆失败, 请手动放置代码到 ${PROJECT_DIR}"
        warn "或设置 GIT_REPO_URL 环境变量指定仓库地址"
        if [[ -n "${GIT_REPO_URL:-}" ]]; then
            git clone "$GIT_REPO_URL" "$PROJECT_DIR"
        else
            err "无法获取代码, 请检查网络或手动操作"
            exit 1
        fi
    }
    log "代码克隆完成 ✓"
fi

cd "$PROJECT_DIR"
chown -R "$REAL_USER:$REAL_USER" "$PROJECT_DIR"

# ---------------------------------------------------------------------------
# 阶段 5: 编译 C++ 引擎
# ---------------------------------------------------------------------------
hdr "阶段 5/6: 编译 C++ 对战引擎"

if [[ "$MODE" != "minimal" ]]; then
    # Docker 模式: 用 Docker 构建
    log "使用 Docker 构建 C++ 引擎..."
    docker compose -f docker/docker-compose.yml build battle-engine
    log "C++ 引擎 Docker 镜像构建完成 ✓"
else
    # 原生编译
    log "原生编译 C++ 引擎 (Release + Tests)..."
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
    cmake --build build
    log "C++ 引擎编译完成 ✓"

    log "运行测试..."
    ctest --test-dir build --output-on-failure && log "全部测试通过 ✓" || warn "部分测试未通过, 请检查"
fi

# ---------------------------------------------------------------------------
# 阶段 6: 启动服务
# ---------------------------------------------------------------------------
hdr "阶段 6/6: 启动服务"

case "$MODE" in
    full)
        log "构建前端静态文件..."
        cd "$PROJECT_DIR/frontend"
        npm install --no-audit --no-fund
        npm run build
        cd "$PROJECT_DIR"

        log "启动全部 Docker 服务 (含前端 Nginx)..."
        docker compose -f docker/docker-compose.yml --profile frontend up -d

        # 等待服务就绪
        log "等待基础设施就绪..."
        sleep 10

        # 初始化 Kafka topics
        log "初始化 Kafka topics..."
        docker compose -f docker/docker-compose.yml up kafka-init 2>/dev/null || true

        # 初始化 HDFS 目录
        log "初始化 HDFS 目录..."
        docker exec pokemon-namenode hdfs dfs -mkdir -p /user/pokemon/raw/battles 2>/dev/null || true
        docker exec pokemon-namenode hdfs dfs -mkdir -p /user/pokemon/raw/events 2>/dev/null || true
        docker exec pokemon-namenode hdfs dfs -mkdir -p /user/pokemon/analytics/pokemon_usage 2>/dev/null || true
        docker exec pokemon-namenode hdfs dfs -mkdir -p /user/pokemon/analytics/move_usage 2>/dev/null || true
        docker exec pokemon-namenode hdfs dfs -mkdir -p /user/pokemon/analytics/type_matchups 2>/dev/null || true
        docker exec pokemon-namenode hdfs dfs -mkdir -p /user/pokemon/analytics/player_stats 2>/dev/null || true
        docker exec pokemon-namenode hdfs dfs -chmod -R 777 /user/pokemon 2>/dev/null || true
        log "HDFS 目录初始化完成 ✓"

        log "全部服务启动完成!"
        info ""
        info "  前端:      http://<服务器IP>"
        info "  API:       http://<服务器IP>:8000/docs"
        info "  HDFS UI:   http://<服务器IP>:9870"
        info "  Kafka:     <服务器IP>:9092"
        info "  PostgreSQL: <服务器IP>:5432"
        ;;

    dev)
        log "启动 Docker 基础设施 (不含前端)..."
        docker compose -f docker/docker-compose.yml up -d
        sleep 8

        log "初始化 Kafka topics..."
        docker compose -f docker/docker-compose.yml up kafka-init 2>/dev/null || true

        log "安装 Python API 依赖..."
        cd "$PROJECT_DIR/api-server"
        python3.12 -m pip install -r ../docker/api-server/requirements.txt

        log "安装前端依赖..."
        cd "$PROJECT_DIR/frontend"
        npm install --no-audit --no-fund

        cd "$PROJECT_DIR"
        log "开发环境就绪!"
        info ""
        info "  启动 API Server:  make api"
        info "  启动前端 dev:     make frontend-dev"
        info "  构建 C++ 引擎:    make build"
        info "  运行测试:         make test"
        info ""
        info "  服务端口:"
        info "    Kafka:       localhost:9092"
        info "    PostgreSQL:  localhost:5432 (pokemon/pokemon123)"
        info "    HDFS UI:     http://localhost:9870"
        info "    API (make api 后): http://localhost:8000/docs"
        info "    前端 (make frontend-dev 后): http://localhost:5173"
        ;;

    minimal)
        log "最小部署完成!"
        info ""
        info "  C++ 引擎已编译到: ${PROJECT_DIR}/build/PokemonSimulator"
        info "  运行测试:         cd ${PROJECT_DIR} && ctest --test-dir build --output-on-failure"
        info "  命令行对战:       ${PROJECT_DIR}/build/PokemonSimulator --help"
        info "  招式测试:         ${PROJECT_DIR}/build/PokemonSimulator --run-move-tests"
        info "  道具测试:         ${PROJECT_DIR}/build/PokemonSimulator --run-item-tests"
        info "  守护进程模式:     ${PROJECT_DIR}/build/PokemonSimulator --daemon"
        ;;
esac

# ---------------------------------------------------------------------------
# 完成
# ---------------------------------------------------------------------------
hdr "部署完成!"
log "模式: ${MODE}"
log "项目目录: ${PROJECT_DIR}"
log "耗时: ${SECONDS} 秒"

if [[ "$MODE" != "minimal" ]]; then
    warn "注意: 如使用了 Docker, 请重新登录以使 docker 组生效, 或执行: newgrp docker"
fi
