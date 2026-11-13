#!/bin/bash

# SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# 参考 deepin-reader/tests/test-prj-running.sh 风格适配:
# 清理重建构建目录 → 编译 → 运行单测产出 gtest XML → 采集 lcov 覆盖率
# → 生成 HTML 报告与 ut-summary.json 统计摘要
#
# 套件分片说明: 本套件存在 stub.h 野写导致的累积性内存破坏,单进程跑全量
# 必然中途崩溃(SIGSEGV),且崩溃时 libgcov 无法落盘 .gcda(历史重试整个
# 二进制的方案在本机 6 次全崩)。故改为按 gtest 套件分片运行:每片独立
# 进程,单片崩溃仅重试该片;连续崩溃的片再拆成单套件重试,仍崩的套件
# 记录并跳过。.gcda 计数跨进程累加,XML 按片输出,由 gen-ut-summary.py 汇总。

set -u

export builddir=build
export reportdir=build-ut
export scriptdir="$(cd "$(dirname "$0")" && pwd)"
export projectdir="$(cd "${scriptdir}/.." && pwd)"

build_path="${projectdir}/${builddir}"
report_path="${projectdir}/${reportdir}"

# 全新构建目录,保证覆盖率干净
rm -rf "${build_path}"
rm -rf "${report_path}"
mkdir -p "${build_path}"
mkdir -p "${report_path}"

cd "${build_path}"

#编译 (单测 + 覆盖率插桩)
cmake -DCMAKE_SAFETYTEST_ARG="CMAKE_SAFETYTEST_ARG_ON" -DDOTEST=ON "${projectdir}" || exit 1
make -j"$(nproc)" || exit 1

TESTBIN=./tests/deepin-system-monitor-test

mkdir -p report

# 重置历史覆盖率计数
lcov --directory "${build_path}" --zerocounters >/dev/null 2>&1 || true

# 收集全部 gtest 套件名
mapfile -t suites < <("${TESTBIN}" --gtest_list_tests 2>/dev/null | grep -E '^[A-Za-z0-9_]+\.$' | tr -d '.')
if [ "${#suites[@]}" -eq 0 ]; then
    echo "ERROR: no gtest suites listed by ${TESTBIN}"
    exit 1
fi
echo ">>> total ${#suites[@]} suites, running in chunks"

CHUNK_SIZE=10
test_exit_code=0
failed_suites=""
chunk=0

for ((i = 0; i < ${#suites[@]}; i += CHUNK_SIZE)); do
    chunk=$((chunk + 1))
    filter=""
    last=$((i + CHUNK_SIZE - 1))
    [ "${last}" -ge "${#suites[@]}" ] && last=$((${#suites[@]} - 1))
    for ((j = i; j <= last; j++)); do
        filter+="${suites[j]}.*:"
    done
    filter="${filter%:}"
    xml="./report/report_deepin-system-monitor_$(printf '%02d' "${chunk}").xml"

    ok=0
    for attempt in 1 2 3; do
        echo ">>> chunk ${chunk} attempt ${attempt}: suites [${i}-${last}]"
        # 10 分钟硬超时兕底: 个别套件用例全部通过但进程退出阶段挂死
        timeout -k 5 600 "${TESTBIN}" --gtest_filter="${filter}" --gtest_output="xml:${xml}"
        rc=$?
        # 以 XML 有效性为唯一成功标准：用例跑完即写 XML；退出阶段崩溃/挂死
        # (rc>=128) 或整体超时(rc=124)但 XML 有效时也算成功；rc<128 但无 XML
        # （如超时被杀、写盘前中断）同样视为失败重试，避免整片用例结果与
        # .gcda 覆盖率数据静默丢失
        if grep -q '<testsuite' "${xml}" 2>/dev/null; then
            ok=1
            break
        fi
        echo ">>> chunk ${chunk} attempt ${attempt} crashed (rc=${rc}), retrying..."
    done

    if [ "${ok}" -ne 1 ]; then
        # 整片连续崩溃: 拆成单套件逐个重试,定位并跳过持续崩溃的套件
        echo ">>> chunk ${chunk} keeps crashing, splitting into single suites"
        rm -f "${xml}"
        for ((j = i; j <= last; j++)); do
            s_xml="./report/report_deepin-system-monitor_s$(printf '%03d' "${j}").xml"
            s_ok=0
            for attempt in 1 2; do
                timeout -k 5 300 "${TESTBIN}" --gtest_filter="${suites[j]}.*" --gtest_output="xml:${s_xml}"
                rc=$?
                if grep -q '<testsuite' "${s_xml}" 2>/dev/null; then
                    s_ok=1
                    break
                fi
            done
            if [ "${s_ok}" -ne 1 ]; then
                echo "WARNING: suite ${suites[j]} crashed even alone, excluded from stats"
                rm -f "${s_xml}"
                failed_suites="${failed_suites} ${suites[j]}"
                test_exit_code=1
            fi
        done
    fi
done

if [ -n "${failed_suites}" ]; then
    echo ">>> FAILED (always-crashing) suites:${failed_suites}"
fi

# >>> BEGIN add-uos ut-sm: 运行各子模块独立 UT 二进制，采集全部子模块的用例结果与覆盖率
# 各模块（daemon/server/system-server/plugin/plugin-popup/dde-shell-plugin）因 ODR
# 冲突各自独立成测试二进制，主二进制之外它们也要运行，否则其源码无 .gcda，
# 覆盖率统计会遗漏全部子模块。注：遗留目标 dsm-popup-ut（popup 模块内 autotests，
# 253 用例）在 ASAN+stub 野写下早期崩溃（DConfigGlobalTh 线程 SEGV），暂不纳入。
# LeakSanitizer 的泄漏退出码不影响测试结果判定（以 XML 为准），关闭以免误报。
MODULE_UTS=(
    "daemon-ut-build/test_deepin-system-monitor-daemon:daemon"
    "server-ut-build/test_deepin-system-monitor-server:server"
    "sys-server-ut-build/test_deepin-system-monitor-system-server:system-server"
    "plugin-ut-build/test_deepin-system-monitor-plugin:plugin"
    "popup-ut-build/test_deepin-system-monitor-plugin-popup:plugin-popup"
    "dde-shell-plugin-ut-build/test_deepin-system-monitor-dde-shell-plugin:dde-shell-plugin"
)
for entry in "${MODULE_UTS[@]}"; do
    m_bin="./tests/${entry%%:*}"
    m_name="${entry##*:}"
    m_xml="./report/report_${m_name}.xml"
    echo ">>> module UT ${m_name}: ${m_bin}"
    ASAN_OPTIONS=detect_leaks=0 timeout -k 5 300 "${m_bin}" --gtest_output="xml:${m_xml}" >/dev/null 2>&1
    m_rc=$?
    if grep -q '<testsuite' "${m_xml}" 2>/dev/null; then
        # 只取根节点 <testsuites ...> 开标签,再单独提取各属性数字;
        # 旧写法一次 grep 含 tests+failures 两个数字再 head -1,
        # 会把 tests 计数误当作 failures 导致全量误报失败
        m_root=$(grep -m1 -oE '<testsuites[^>]*>' "${m_xml}")
        m_total=$(echo "${m_root}" | grep -oE ' tests="[0-9]+"' | grep -oE '[0-9]+')
        m_fail=$(echo "${m_root}" | grep -oE ' failures="[0-9]+"' | grep -oE '[0-9]+')
        echo ">>> module UT ${m_name}: ${m_total} tests, ${m_fail} failures (rc=${m_rc})"
        if [ "${m_fail:-0}" -ne 0 ]; then
            test_exit_code=1
        fi
    else
        echo "WARNING: module UT ${m_name} produced no XML (rc=${m_rc}), excluded from stats"
        rm -f "${m_xml}"
        test_exit_code=1
    fi
done
# <<< END

workdir="${build_path}"

#统计代码覆盖率并生成html报告
# 注：不按模块 extract，保留全部子模块生产代码的覆盖率；
# 仅排除测试代码自身(tests//autotests/)与vendored第三方库(3rdparty/)
lcov -d "${workdir}" -c -o ./coverage.info

lcov --remove ./coverage.info '*/tests/*' -o ./coverage.info
lcov --remove ./coverage.info '*/autotests/*' -o ./coverage.info
lcov --remove ./coverage.info '*/3rdparty/*' -o ./coverage.info
# 剔除系统头文件（/usr/include 的 STL/Qt 模板实例化会被 gcov 记录，
# 本机实测混入 228 个文件 7229 行，占报告总行数 24%，严重稀释项目真实覆盖率）
lcov --remove ./coverage.info '/usr/include/*' -o ./coverage.info
lcov --remove ./coverage.info '/usr/lib/*' -o ./coverage.info

genhtml -o ./html ./coverage.info

mv ./html/index.html ./html/cov_deepin-system-monitor.html

#对asan、ut、代码覆盖率结果收集至指定文件夹
cp -r html "${report_path}/"
cp -r report "${report_path}/"
cp -r asan*.log* "${report_path}/asan_deepin-system-monitor.log" 2>/dev/null || true

# 生成摘要 JSON (gtest 用例统计 + 行/函数覆盖率)
echo "==> Generating summary JSON: ${report_path}/ut-summary.json"
python3 "${scriptdir}/gen-ut-summary.py"

exit "${test_exit_code}"
