@echo off
chcp 65001 >nul
echo ============================================
echo   fwx-build 推送到 GitHub 并触发编译
echo ============================================
echo.

cd /d "%~dp0"

echo [1/4] 初始化 Git 仓库...
if not exist .git (
    git init
    git branch -M main
)

echo.
echo [2/4] 添加文件...
git add .
git status

echo.
echo [3/4] 提交...
git commit -m "Add fwx kernel module build for iStoreOS 24.10 (kernel 6.6.144)" --allow-empty

echo.
echo [4/4] 推送到 GitHub...
echo.
echo 请选择操作方式：
echo   1. 如果还没有创建 GitHub 仓库：
echo      - 打开 https://repo.new 创建一个新仓库（名字随意，比如 fwx-build）
echo      - 不要勾选 README/license/gitignore
echo      - 创建后复制仓库地址，粘贴到下方
echo.
echo   2. 如果已经有仓库地址，直接粘贴到下方
echo.
set /p REPO_URL="请输入 GitHub 仓库地址 (例如 https://github.com/你的用户名/fwx-build.git): "

if "%REPO_URL%"=="" (
    echo 错误：未输入仓库地址
    pause
    exit /b 1
)

git remote remove origin 2>nul
git remote add origin "%REPO_URL%"
git push -u origin main --force

echo.
echo ============================================
echo   推送完成！
echo ============================================
echo.
echo 接下来：
echo   1. 打开浏览器访问你的 GitHub 仓库页面
echo   2. 点击顶部的 "Actions" 标签
echo   3. 左侧选择 "Build fwx.ko for iStoreOS 24.10"
echo   4. 点击右侧 "Run workflow" 按钮
echo   5. 分支选 main，点击绿色 "Run workflow"
echo.
echo 编译大约需要 1-2 小时（首次构建工具链）
echo 完成后在 Actions 运行页面底部下载 artifact
echo.
pause
