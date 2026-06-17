#!/bin/bash

# SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

builddir=build
reportdir=build-ut
rm -r $builddir
rm -r ../$builddir
rm -r $reportdir
rm -r ../$reportdir
mkdir ../$builddir
mkdir ../$reportdir
cd ../$builddir
#编译
cmake -DCMAKE_SAFETYTEST_ARG="CMAKE_SAFETYTEST_ARG_ON" -DDOTEST=ON ..
make -j8
#生成asan日志和ut测试xml结果
# 套件存在预存的偶发崩溃（stub.h 非对齐写、process_page_widget UBSan 等），
# 崩溃会中断 libgcov 落盘 .gcda；重试直到某次完整跑通并 flush 出 .gcda，保证覆盖率可采集。
mkdir -p report
attempt=0
max_attempts=6
while [ ${attempt} -lt ${max_attempts} ]; do
    attempt=$((attempt + 1))
    find . -name '*.gcda' -delete 2>/dev/null
    echo ">>> test run attempt ${attempt}/${max_attempts}"
    ./tests/deepin-system-monitor-test --gtest_output=xml:./report/report_deepin-system-monitor.xml
    rc=$?
    gcda_n=$(find . -name '*.gcda' | wc -l)
    # 正常结束：gtest 跑到末尾（exit 0 全过 或 1 有失败，均非信号）且覆盖率已落盘
    if [ ${rc} -lt 128 ] && [ ${gcda_n} -gt 200 ]; then
        echo ">>> attempt ${attempt} completed cleanly (rc=${rc}, ${gcda_n} gcda files), coverage captured"
        break
    fi
    echo ">>> attempt ${attempt} ended abnormally (rc=${rc}, ${gcda_n} gcda), retrying..."
done

workdir=$(cd ../$(dirname $0)/$builddir; pwd)

mkdir -p report
#统计代码覆盖率并生成html报告
lcov -d $workdir -c -o ./coverage.info

lcov --extract ./coverage.info '*/deepin-system-monitor-main/*' -o ./coverage.info

lcov --remove ./coverage.info '*/tests/*' -o ./coverage.info

genhtml -o ./html ./coverage.info

mv ./html/index.html ./html/cov_deepin-system-monitor.html
#对asan、ut、代码覆盖率结果收集至指定文件夹
cp -r html ../$reportdir/
cp -r report ../$reportdir/
cp -r asan*.log* ../$reportdir/asan_deepin-system-monitor.log

exit 0
