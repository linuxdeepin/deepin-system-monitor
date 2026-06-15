# SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 单元测试构建 + 覆盖率报告生成脚本
# 用法：从 tests/ 目录执行 bash cmake-lcov-test.sh

utdir=build-ut
rm -rf $utdir
rm -rf ../$utdir
mkdir -p ../$utdir
cd ../$utdir

# NOTE: 必须 -DDOTEST=ON 才会编译 tests/ 子目录（顶层 CMakeLists option(DOTEST) 默认 OFF）
cmake -DCMAKE_BUILD_TYPE=Debug -DDOTEST=ON ..
make -j16

workdir=$(pwd)

# 清理上次遗留的 .gcda 计数文件，避免二进制重编后 libgcov "different checksum" 报错污染覆盖率
find . -name "*.gcda" -delete

# 运行测试（offscreen 平台避免无显示环境下的 GUI 崩溃）；生成 JUnit XML 报告
QT_QPA_PLATFORM=offscreen ./tests/deepin-system-monitor-test --gtest_output=xml:./report/report.xml

mkdir -p report
# 采集 gcov 覆盖率数据
lcov -d $workdir -c -o ./report/coverage.info
# 仅保留主程序源码覆盖率，剔除测试代码与三方库
lcov --extract ./report/coverage.info '*/deepin-system-monitor-main/*' -o ./report/coverage.info
lcov --remove ./report/coverage.info '*/tests/*' '*/3rdparty/*' -o ./report/coverage.info

# 生成 HTML 报告
genhtml -o ./report ./report/coverage.info

# 打印总体覆盖率摘要
lcov --summary ./report/coverage.info 2>&1 | grep -E "lines|functions"

exit 0
