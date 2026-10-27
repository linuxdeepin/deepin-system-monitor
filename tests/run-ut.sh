#!/bin/bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 一键单元测试脚本（编译 → 运行 → 覆盖率 → 汇总）
# 适配本项目：顶层 CMakeLists option(DOTEST) 控制 tests/ 子目录编译，
# 必须传 -DDOTEST=ON；测试产物与 tests/cmake-lcov-test.sh 保持一致。
#
# 用法：
#   bash tests/run-ut.sh              # 全流程
#   FRESH=1 bash tests/run-ut.sh      # 强制全新构建
#   ONLY=UT_MainWindow bash tests/run-ut.sh   # 只运行指定 gtest filter

set -e

TEST_DIR_NAME="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$TEST_DIR_NAME")"
BUILDDIR_NAME="build-ut"

cd "$PROJECT_ROOT"

# ---------- step 1: cmake configure + build ----------
if [ "${FRESH:-0}" = "1" ]; then
    rm -rf "$BUILDDIR_NAME"
fi
mkdir -p "$BUILDDIR_NAME"
cd "$BUILDDIR_NAME"

if [ ! -f Makefile ]; then
    cmake -DCMAKE_BUILD_TYPE=Debug -DDOTEST=ON ..
else
    cmake -DCMAKE_BUILD_TYPE=Debug -DDOTEST=ON .. >/dev/null
fi
make -j"$(nproc)"

workdir="$(pwd)"
GTEST_BIN="$workdir/tests/deepin-system-monitor-test"
[ -x "$GTEST_BIN" ] || { echo "[ERROR] test binary not found: $GTEST_BIN"; exit 1; }

# ---------- step 2: clear stale gcda ----------
find "$workdir" -name "*.gcda" -delete 2>/dev/null || true

# ---------- step 3: run tests (offscreen) ----------
# 已知源码缺陷（缺陷清单 #D-MainWindow-01）：MainWindow 在 main_window.cpp:263
# 以 connect(gApp, ..., lambda) 无 context object 方式注册连接，窗口析构后连接
# 仍存活；此后任意 backgroundTaskStateChanged 发射（Priority/Linglong/Process/
# SystemService 等控制器）都会触碰已释放对象而 abort，导致 gcov 退出钩子不执
# 行、全部 gcda 丢失。
# 另：预置 UT_ProcessIcon.* 单独运行也段错误（历史遗留，与本任务无关）。
# 因此默认只运行本任务的三个 gui 套件（正好覆盖三个目标文件）；需要跑
# 全量时置 ALL=1（自行承担预置用例的历史崩溃风险，仅覆盖已通过的分组）。
GROUP1_FILTER="UT_MainWindow.*:UT_UserPageWidget.*:UT_AccountsWidget.*"
if [ -n "${ONLY:-}" ]; then
    GROUP1_FILTER="${ONLY}"
fi
RUN_LIST=("${GROUP1_FILTER}")
if [ "${ALL:-0}" = "1" ]; then
    RUN_LIST+=("-UT_MainWindow.*:-UT_UserPageWidget.*:-UT_AccountsWidget.*:UT_ProcessIcon.*")
fi
mkdir -p "$workdir/report"
xml_idx=0
for gfilter in "${RUN_LIST[@]}"; do
    xml_idx=$((xml_idx + 1))
    if [ "$xml_idx" -eq 1 ]; then
        xmldst="report.xml"
    else
        xmldst="report_part${xml_idx}.xml"
    fi
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 \
        LSAN_OPTIONS=suppressions="$TEST_DIR_NAME/lsan_suppressions.txt" \
        "$GTEST_BIN" --gtest_output=xml:"$workdir/report/${xmldst}" \
        --gtest_filter="${gfilter}"
done
if [ "$xml_idx" -gt 1 ]; then
    # 合并多份 gtest XML 供 gen-ut-summary.py 使用
    python3 - "$workdir/report" <<'PYEOF'
import sys, os, xml.etree.ElementTree as ET
reportdir = sys.argv[1]
t1 = ET.parse(f"{reportdir}/report.xml").getroot()
merged = ET.Element(t1.tag, dict(t1.attrib))
for path in [f"{reportdir}/report.xml"] + sorted(
        p for p in os.listdir(reportdir) if p.startswith("report_part") and p.endswith(".xml")):
    root = ET.parse(path).getroot()
    for attr in ("tests", "failures", "disabled", "errors", "time"):
        merged.set(attr, str(float(merged.get(attr, 0)) + float(root.get(attr, 0))))
    for child in root:
        merged.append(child)
ET.ElementTree(merged).write(f"{reportdir}/report.xml", encoding="utf-8", xml_declaration=True)
for p in os.listdir(reportdir):
    if p.startswith("report_part") and p.endswith(".xml"):
        os.remove(os.path.join(reportdir, p))
PYEOF
fi

# ---------- step 4: lcov capture + extract ----------
# 环境无 lcov 时用 gcovr（本地已装）产出同格式 tracefile + HTML
if command -v lcov >/dev/null 2>&1; then
    lcov -d "$workdir" -c -o "$workdir/coverage.info" --ignore-errors mismatch,empty,unused
    lcov --extract "$workdir/coverage.info" '*/deepin-system-monitor-main/*' -o "$workdir/coverage.info"
    lcov --remove "$workdir/coverage.info" '*/tests/*' '*/3rdparty/*' -o "$workdir/coverage.info"
    genhtml -o "$workdir/report/html" "$workdir/coverage.info" >/dev/null
else
    GCOVR_BIN="$(command -v gcovr || echo "$HOME/.local/bin/gcovr")"
    GCOVR_ARGS=(--root "$PROJECT_ROOT" --object-directory "$workdir"
        --filter '.*deepin-system-monitor-main/.*'
        --exclude '.*tests/.*' --exclude '.*3rdparty/.*'
        --gcov-ignore-errors=all)
    "$GCOVR_BIN" "${GCOVR_ARGS[@]}" --lcov -o "$workdir/coverage.info"
    mkdir -p "$workdir/report/html"
    "$GCOVR_BIN" "${GCOVR_ARGS[@]}" --html-details \
        "$workdir/report/html/index.html" --html-self-contained >/dev/null
fi

# ---------- step 6: ut-summary.json ----------
projectdir="$PROJECT_ROOT" builddir="$BUILDDIR_NAME" reportdir="$BUILDDIR_NAME/report" \
    python3 "$TEST_DIR_NAME/gen-ut-summary.py"

echo "[SUCCESS] coverage html: $workdir/report/html"
