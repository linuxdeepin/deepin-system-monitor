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
# 以 connect(gApp, ..., lambda) 无 context object 方式注册连接，窗口析构后连接仍
# 存活，后续 backgroundTaskStateChanged 发射时触发对已释放对象的调⽤（Q_ASSERT
# /段错误）。全量单次运行会在预置的 UT_LinglongController 用例中中途 abort，
# 导致 gcov 退出钩子不执行、全部 gcda 丢失。因此分组运行：先跑其余全部用例，
# 再单独跑 LinglongController 组，两次均正常退出后 lcov 合并。
GROUP1_FILTER="-UT_LinglongController.*"
GROUP2_FILTER="UT_LinglongController.*"
mkdir -p "$workdir/report"
if [ -n "${ONLY:-}" ]; then
    # 指定 ONLY 时按原样单次运行（调用方自行控制范围）
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 \
        LSAN_OPTIONS=suppressions="$TEST_DIR_NAME/lsan_suppressions.txt" \
        "$GTEST_BIN" --gtest_output=xml:"$workdir/report/report.xml" \
        --gtest_filter="${ONLY}"
else
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 \
        LSAN_OPTIONS=suppressions="$TEST_DIR_NAME/lsan_suppressions.txt" \
        "$GTEST_BIN" --gtest_output=xml:"$workdir/report/report_g1.xml" \
        --gtest_filter="${GROUP1_FILTER}"
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 \
        LSAN_OPTIONS=suppressions="$TEST_DIR_NAME/lsan_suppressions.txt" \
        "$GTEST_BIN" --gtest_output=xml:"$workdir/report/report_g2.xml" \
        --gtest_filter="${GROUP2_FILTER}"
    # 合并两份 gtest XML 供 CI/报告使用（取 g1 的套件头 + 两组全部用例）
    python3 - "$workdir/report" <<'PYEOF'
import sys, xml.etree.ElementTree as ET
reportdir = sys.argv[1]
t1 = ET.parse(f"{reportdir}/report_g1.xml").getroot()
t2 = ET.parse(f"{reportdir}/report_g2.xml").getroot()
merged = ET.Element(t1.tag, dict(t1.attrib))
for attr in ("tests", "failures", "disabled", "errors", "time", "timestamp"):
    if attr in t1.attrib or attr in t2.attrib:
        merged.set(attr, str(float(t1.get(attr, 0)) + float(t2.get(attr, 0))))
for root in (t1, t2):
    for child in root:
        merged.append(child)
ET.ElementTree(merged).write(f"{reportdir}/report.xml", encoding="utf-8", xml_declaration=True)
import os
os.remove(f"{reportdir}/report_g1.xml")
os.remove(f"{reportdir}/report_g2.xml")
PYEOF
fi

# ---------- step 4: lcov capture + extract ----------
lcov -d "$workdir" -c -o "$workdir/coverage.info" --ignore-errors mismatch,empty,unused
lcov --extract "$workdir/coverage.info" '*/deepin-system-monitor-main/*' -o "$workdir/coverage.info"
lcov --remove "$workdir/coverage.info" '*/tests/*' '*/3rdparty/*' -o "$workdir/coverage.info"

# ---------- step 5: genhtml ----------
genhtml -o "$workdir/report/html" "$workdir/coverage.info" >/dev/null

# ---------- step 6: ut-summary.json ----------
projectdir="$PROJECT_ROOT" builddir="$BUILDDIR_NAME" reportdir="$BUILDDIR_NAME/report" \
    python3 "$TEST_DIR_NAME/gen-ut-summary.py"

echo "[SUCCESS] coverage html: $workdir/report/html"
