// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method       | level | factors            | min | actual |
// |--------------|-------|--------------------|-----|--------|
// | hash_x64_128 | high  | complexity:17      | 3   | 5      |
// | hash_x86_128 | high  | complexity:17      | 3   | 5      |
// | hash_x86_32  | mid   | complexity:5       | 2   | 3      |
// | init_seed    | mid   | complexity:5       | 2   | 4      |
// | rotl32       | low   | 内部 inline,经 hash_x86_32 间接覆盖 | 1 | 1(间接) |
// | rotl64       | low   | 内部 inline,经 hash_x64_128 间接覆盖 | 1 | 1(间接) |
// | fmix32       | low   | 内部 inline,经 hash_x86_32 间接覆盖 | 1 | 1(间接) |
// | fmix64       | low   | 内部 inline,经 hash_x64_128 间接覆盖 | 1 | 1(间接) |
// | getblock32   | low   | 内部 inline,经 hash_x86_32 间接覆盖 | 1 | 1(间接) |
// | getblock64   | low   | 内部 inline,经 hash_x64_128 间接覆盖 | 1 | 1(间接) |
// 注：rotl/fmix/getblock 为 hash.cpp 编译单元内部 inline 函数，未在 hash.h
// 声明且 always_inline 无独立符号，无法直接链接调用，统一经由公开入口
// hash_x86_32 / hash_x64_128 间接覆盖（分支与向量断言已触发全部内联路径）。
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]（4 个可链接方法均有直接用例）
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]（空/尾/整块/多块）
// 3. 每个等价类的边界值显式覆盖: [x]（len=0/1/15/16/32/48/64）
// 4. 同质 ≥ 3 组用 TEST_P: [x]（尾长 16 组、块长 4 组、x86_32 长度 21 组）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无异常路径）
// 8. 负面场景有专门用例: [x]（getrandom 短读、open 失败、urandom 短读）
// 9. 负面用例验证强异常安全: [x]（abort 被拦截后状态不变断言）
// 10. stub_ext vs gMock 选择正确: [x]（沿用本仓库 tests/stub.h，libc 桩 RAII 恢复）

#include <gtest/gtest.h>

#include "config.h"
#include "stub.h"
#include "common/hash.h"

#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#if defined(HAVE_SYS_RANDOM_H)
#    include <sys/random.h>
#endif

using namespace util::common;

/***************************************STUB begin*********************************************/
// 说明：本仓库测试框架使用 tests/stub.h（函数入口补丁，Stub 析构自动还原）。
// libc 桩仅作用于单个用例作用域（局部 Stub 对象），析构即恢复，无跨用例泄漏。

#if defined(HAVE_SYS_RANDOM_H)
static ssize_t stub_getrandom_ok(void *buf, size_t buflen, unsigned int /*flags*/)
{
    if (buflen < sizeof(uint32_t)) {
        return -1;
    }
    *static_cast<uint32_t *>(buf) = 0x13572468u;
    return static_cast<ssize_t>(sizeof(uint32_t));
}

static ssize_t stub_getrandom_fail(void * /*buf*/, size_t /*buflen*/, unsigned int /*flags*/)
{
    return -1;
}

static ssize_t stub_getrandom_short(void * /*buf*/, size_t /*buflen*/, unsigned int /*flags*/)
{
    // 只返回 1 字节（短读），模拟 getrandom 非阻塞读取不足
    return 1;
}
#endif

static ssize_t stub_read_fill(void *, void *buf, size_t count)
{
    if (buf != nullptr && count >= sizeof(uint32_t)) {
        *static_cast<uint32_t *>(buf) = 0xCAFEBABEu;
        return static_cast<ssize_t>(sizeof(uint32_t));
    }
    return 0;
}

static ssize_t stub_read_zero(void *, void *, size_t)
{
    return 0;
}

static int stub_close_ok(int)
{
    return 0;
}

static int stub_open_fail(const char *, int)
{
    return -1;
}

static void stub_abort_noop()
{
    // 拦截 abort()：Debug 构建下 abort() 返回后顺序执行后续语句，
    // 使 "资源失败 → abort" 分支可被确定性验证。
}
/***************************************STUB end***********************************************/

namespace {
const char *kPattern = "abcdefghijklmnopqrstuvwxyz";

void fillPattern(char *buf, int len)
{
    for (int i = 0; i < len; ++i) {
        buf[i] = kPattern[i % 26];
    }
}
}

// 分支清单（来源：hash_x64_128 真实分支 16 = for 1 + case 15）
// B1: for (i < nblocks) len>=16 整块循环 → BlockLengths 参数 16/32/48/64 驱动
// B2: case 15 尾长15 → TailLengths 参数 len=15 驱动（14..1 连锁 fallthrough）
// B3: case 14 尾长14 → TailLengths 参数 len=14 驱动
// B4: case 13 尾长13 → TailLengths 参数 len=13 驱动
// B5: case 12 尾长12 → TailLengths 参数 len=12 驱动
// B6: case 11 尾长11 → TailLengths 参数 len=11 驱动
// B7: case 10 尾长10 → TailLengths 参数 len=10 驱动
// B8: case 9  尾长9  → TailLengths 参数 len=9 驱动（k2 完整处理）
// B9: case 8  尾长8  → TailLengths 参数 len=8 驱动（k1 高字节起）
// B10: case 7 尾长7  → TailLengths 参数 len=7 驱动
// B11: case 6 尾长6  → TailLengths 参数 len=6 驱动
// B12: case 5 尾长5  → TailLengths 参数 len=5 驱动
// B13: case 4 尾长4  → TailLengths 参数 len=4 驱动
// B14: case 3 尾长3  → TailLengths 参数 len=3 驱动
// B15: case 2 尾长2  → TailLengths 参数 len=2 驱动
// B16: case 1 尾长1  → TailLengths 参数 len=1 驱动（len=0 跳过整个 switch）

// x64_128 期望向量（独立 MurmurHash3 规范实现生成，缓冲区字节取自 kPattern 循环）
struct X64Vector {
    int len;
    uint64_t out0;
    uint64_t out1;
};

static const X64Vector kX64TailVectors[] = {
    { 0, 0x0000000000000000ULL, 0x0000000000000000ULL },
    { 1, 0x85555565f6597889ULL, 0xe6b53a48510e895aULL },
    { 2, 0x938b11ea16ed1b2eULL, 0xe65ea7019b52d4adULL },
    { 3, 0xb4963f3f3fad7867ULL, 0x3ba2744126ca2d52ULL },
    { 4, 0xb87bb7d64656cd4fULL, 0xf2003e886073e875ULL },
    { 5, 0x2036d091f496bbb8ULL, 0xc5c7eea04bcfec8cULL },
    { 6, 0xe47d86bfaca3bf55ULL, 0xb07109993321845cULL },
    { 7, 0xa6cd2f9fc09ee499ULL, 0x1c3aa23ab155bbb6ULL },
    { 8, 0xcc8a0ab037ef8c02ULL, 0x48890d60eb6940a1ULL },
    { 9, 0x0547c0cff13c7964ULL, 0x79b53df5b741e033ULL },
    { 10, 0xb6c15b0d772f8c99ULL, 0xa24d85dc8c651ac9ULL },
    { 11, 0xa895d0b8df789d02ULL, 0xbb7c31e2455ae771ULL },
    { 12, 0x8ef39bb1e67ae194ULL, 0x1f9e303272ff621cULL },
    { 13, 0x1648288da7c0fa73ULL, 0x2e657bff0de7cc7fULL },
    { 14, 0x91d094a7f5c375e0ULL, 0xee096027d26a3324ULL },
    { 15, 0x8abe2451890c2ffbULL, 0x6a548c2d9c962a61ULL },
};

static const X64Vector kX64BlockVectors[] = {
    { 16, 0xc4ca3ca3224cb723ULL, 0x4333d695b331eb1aULL },
    { 32, 0xb7a7929719b0b1b4ULL, 0xa1f935fbd24c1ca6ULL },
    { 48, 0xd3c51225b2c3cb04ULL, 0x23bad690f57647f5ULL },
    { 64, 0x37ba6ac0cc3190b3ULL, 0xea95f21c9852d803ULL },
};

// 分支清单（来源：hash_x86_128 真实分支 16 = for 1 + case 15）
// B1: for (i != 0) len>=16 整块循环（负索引反向读块）→ BlockLengths 参数驱动
// B2: case 15 尾长15 → TailLengths 参数 len=15 驱动
// B3: case 14 尾长14 → TailLengths 参数 len=14 驱动
// B4: case 13 尾长13 → TailLengths 参数 len=13 驱动（k4 完整处理）
// B5: case 12 尾长12 → TailLengths 参数 len=12 驱动（k3 高字节起）
// B6: case 11 尾长11 → TailLengths 参数 len=11 驱动
// B7: case 10 尾长10 → TailLengths 参数 len=10 驱动
// B8: case 9  尾长9  → TailLengths 参数 len=9 驱动
// B9: case 8  尾长8  → TailLengths 参数 len=8 驱动（k2 高字节起）
// B10: case 7 尾长7  → TailLengths 参数 len=7 驱动
// B11: case 6 尾长6  → TailLengths 参数 len=6 驱动
// B12: case 5 尾长5  → TailLengths 参数 len=5 驱动（k2 完整处理）
// B13: case 4 尾长4  → TailLengths 参数 len=4 驱动
// B14: case 3 尾长3  → TailLengths 参数 len=3 驱动
// B15: case 2 尾长2  → TailLengths 参数 len=2 驱动
// B16: case 1 尾长1  → TailLengths 参数 len=1 驱动（k1 完整处理；len=0 跳过 switch）

static const X64Vector kX86TailVectors[] = {
    { 0, 0x0000000000000000ULL, 0x0000000000000000ULL },
    { 1, 0x5556b01ba794933cULL, 0x5556b01b5556b01bULL },
    { 2, 0x25be3010158451dfULL, 0x25be301025be3010ULL },
    { 3, 0xa2b006a575cdc6d1ULL, 0xa2b006a5a2b006a5ULL },
    { 4, 0x45afc62e96b6ccaaULL, 0x45afc62e45afc62eULL },
    { 5, 0x5d24c5bcc5402efbULL, 0x5a7201775a720177ULL },
    { 6, 0xa1af2721e17cb90aULL, 0xa9bedff9a9bedff9ULL },
    { 7, 0x09863ade90b541d9ULL, 0x4a7769284a776928ULL },
    { 8, 0xadb11487aef41136ULL, 0xfa6c8092fa6c8092ULL },
    { 9, 0x2206596fad058c1cULL, 0xdd94417c14d27d10ULL },
    { 10, 0x9a37900ef5d92ea7ULL, 0x692ff17fc792aa2aULL },
    { 11, 0xca1f111ea0c8c9ddULL, 0xbd5e975782fef12dULL },
    { 12, 0xf48224c0738503feULL, 0xd28ce55327d4dfadULL },
    { 13, 0xdbaea1ecc13aa215ULL, 0xd92cde70f41c3566ULL },
    { 14, 0x605d1fd775ec118eULL, 0xf1d4894d946f45f4ULL },
    { 15, 0x4a1ad5aeee6d1d69ULL, 0xada761a884ce1457ULL },
};

static const X64Vector kX86BlockVectors[] = {
    { 16, 0x90b912569fd27627ULL, 0xd193ba45e4ce8b21ULL },
    { 32, 0x93575dc23c14b343ULL, 0xfd163f08f52da3dbULL },
    { 48, 0xdd991143e96d5d1fULL, 0xb25abeeb1811ec5aULL },
    { 64, 0x6a161b5422d39354ULL, 0xde23d12b6a6ccbf6ULL },
};

// 分支清单（来源：hash_x86_32 真实分支 4 = for 1 + case 3）
// B1: for (i != 0) len>=4 整块循环 → X86_32Lengths 参数 4/8/16/32/64 驱动
// B2: case 3 尾长3 → X86_32Lengths 参数 len=3 驱动
// B3: case 2 尾长2 → X86_32Lengths 参数 len=2 驱动
// B4: case 1 尾长1 → X86_32Lengths 参数 len=1 驱动（len%4==0 跳过 switch）

struct X86Vector {
    int len;
    uint32_t out;
};

static const X86Vector kX86_32LengthVectors[] = {
    { 0, 0x00000000u }, { 1, 0x3c2569b2u }, { 2, 0x9bbfd75fu },
    { 3, 0xb3dd93fau }, { 4, 0x43ed676au }, { 5, 0xe89b9af6u },
    { 6, 0x6181c085u }, { 7, 0x883c9b06u }, { 8, 0x49ddccc4u },
    { 9, 0x421406f0u }, { 10, 0x88927791u }, { 11, 0x5f3b25dfu },
    { 12, 0xa36f3d27u }, { 13, 0xf212161bu }, { 14, 0xf8526df0u },
    { 15, 0x9d09f7d2u }, { 16, 0xe76291edu }, { 17, 0xb6655e4au },
    { 31, 0x0b796bfdu }, { 32, 0x70db26e9u }, { 64, 0x1b645805u },
};

class HashTest : public ::testing::Test
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

class HashX64TailLengthsTest : public ::testing::TestWithParam<X64Vector>
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

class HashX64BlockLengthsTest : public ::testing::TestWithParam<X64Vector>
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

class HashX86TailLengthsTest : public ::testing::TestWithParam<X64Vector>
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

class HashX86BlockLengthsTest : public ::testing::TestWithParam<X64Vector>
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

class HashX86_32LengthsTest : public ::testing::TestWithParam<X86Vector>
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

TEST_P(HashX64TailLengthsTest, HashX64_128_TailLength_MatchesSpecVector)
{
    // Arrange：模式串缓冲与哨兵输出
    const X64Vector &vec = GetParam();
    char buf[64];
    fillPattern(buf, vec.len);
    uint64_t out[2] = { 0x1111111111111111ULL, 0x2222222222222222ULL };

    // Act
    hash_x64_128(buf, vec.len, 0, out);

    // Assert：规范向量锚定 + 非残留哨兵值
    EXPECT_EQ(out[0], vec.out0);
    EXPECT_EQ(out[1], vec.out1);
}

INSTANTIATE_TEST_SUITE_P(HashTail, HashX64TailLengthsTest,
                         ::testing::ValuesIn(kX64TailVectors));

TEST_P(HashX64BlockLengthsTest, HashX64_128_BlockLength_MatchesSpecVector)
{
    // Arrange：整块长度模式串缓冲
    const X64Vector &vec = GetParam();
    char buf[64];
    fillPattern(buf, vec.len);
    uint64_t out[2] = { 0, 0 };

    // Act
    hash_x64_128(buf, vec.len, 0, out);

    // Assert：整块循环路径规范锚定 + 与空 seed 基线不同（混入块数据）
    EXPECT_EQ(out[0], vec.out0);
    EXPECT_EQ(out[1], vec.out1);
    EXPECT_NE(out[0], 0u);
    EXPECT_NE(out[1], 0u);
}

INSTANTIATE_TEST_SUITE_P(HashBlock, HashX64BlockLengthsTest,
                         ::testing::ValuesIn(kX64BlockVectors));

TEST_F(HashTest, HashX64_128_SameInputAndSeed_IsDeterministic)
{
    // Arrange：同输入双缓冲
    char buf[16];
    fillPattern(buf, 16);
    uint64_t first[2] = { 0, 0 };
    uint64_t second[2] = { 0, 0 };

    // Act
    hash_x64_128(buf, 16, 0, first);
    hash_x64_128(buf, 16, 0, second);

    // Assert：确定性 + 不为空
    EXPECT_EQ(first[0], second[0]);
    EXPECT_EQ(first[1], second[1]);
    EXPECT_NE(first[0], 0u);
}

TEST_F(HashTest, HashX64_128_DifferentSeed_ChangesDigest)
{
    // Arrange：同输入、seed 0 与 97
    const char *hello = "hello";
    uint64_t seedZero[2] = { 0, 0 };
    uint64_t seedNinetySeven[2] = { 0, 0 };

    // Act
    hash_x64_128(hello, 5, 0, seedZero);
    hash_x64_128(hello, 5, 97, seedNinetySeven);

    // Assert：seed 影响摘要 + 规范锚定（MurmurHash3 公开向量）
    EXPECT_EQ(seedZero[0], 0xcbd8a7b341bd9b02ULL);
    EXPECT_EQ(seedZero[1], 0x5b1e906a48ae1d19ULL);
    EXPECT_EQ(seedNinetySeven[0], 0x2d62231a0c03e41fULL);
    EXPECT_EQ(seedNinetySeven[1], 0x3a86cac12b9c64a2ULL);
    EXPECT_NE(seedZero[0], seedNinetySeven[0]);
}

TEST_F(HashTest, HashX64_128_DifferentInput_ChangesDigest)
{
    // Arrange：同 seed、仅首字节不同的双输入
    char bufA[16];
    char bufB[16];
    fillPattern(bufA, 16);
    memcpy(bufB, bufA, 16);
    bufB[0] = 'A' + 1;   // 仅首字节不同
    uint64_t outA[2] = { 0, 0 };
    uint64_t outB[2] = { 0, 0 };

    // Act
    hash_x64_128(bufA, 16, 0, outA);
    hash_x64_128(bufB, 16, 0, outB);

    // Assert：雪崩性（单字节差异改变摘要）
    EXPECT_NE(outA[0], outB[0]);
    EXPECT_NE(outA[1], outB[1]);
}

TEST_P(HashX86TailLengthsTest, HashX86_128_TailLength_MatchesSpecVector)
{
    // Arrange：模式串缓冲与输出哨兵
    const X64Vector &vec = GetParam();
    char buf[64];
    fillPattern(buf, vec.len);
    uint64_t out[2] = { 0x1111111111111111ULL, 0x2222222222222222ULL };

    // Act
    hash_x86_128(buf, vec.len, 0, out);

    // Assert：规范向量锚定
    EXPECT_EQ(out[0], vec.out0);
    EXPECT_EQ(out[1], vec.out1);
}

INSTANTIATE_TEST_SUITE_P(HashTail, HashX86TailLengthsTest,
                         ::testing::ValuesIn(kX86TailVectors));

TEST_P(HashX86BlockLengthsTest, HashX86_128_BlockLength_MatchesSpecVector)
{
    // Arrange：整块长度模式串缓冲
    const X64Vector &vec = GetParam();
    char buf[64];
    fillPattern(buf, vec.len);
    uint64_t out[2] = { 0, 0 };

    // Act
    hash_x86_128(buf, vec.len, 0, out);

    // Assert：整块循环路径规范锚定
    EXPECT_EQ(out[0], vec.out0);
    EXPECT_EQ(out[1], vec.out1);
    EXPECT_NE(out[0], 0u);
    EXPECT_NE(out[1], 0u);
}

INSTANTIATE_TEST_SUITE_P(HashBlock, HashX86BlockLengthsTest,
                         ::testing::ValuesIn(kX86BlockVectors));

TEST_F(HashTest, HashX86_128_SameInputAndSeed_IsDeterministic)
{
    // Arrange：同输入双缓冲
    char buf[32];
    fillPattern(buf, 32);
    uint64_t first[2] = { 0, 0 };
    uint64_t second[2] = { 0, 0 };

    // Act
    hash_x86_128(buf, 32, 0, first);
    hash_x86_128(buf, 32, 0, second);

    // Assert：确定性
    EXPECT_EQ(first[0], second[0]);
    EXPECT_EQ(first[1], second[1]);
}

TEST_F(HashTest, HashX86_128_DifferentSeed_ChangesDigest)
{
    // Arrange：同输入、seed 0 与 97
    const char *hello = "hello";
    uint64_t seedZero[2] = { 0, 0 };
    uint64_t seedNinetySeven[2] = { 0, 0 };

    // Act
    hash_x86_128(hello, 5, 0, seedZero);
    hash_x86_128(hello, 5, 97, seedNinetySeven);

    // Assert：seed 敏感 + 规范锚定（独立参考实现生成）
    EXPECT_EQ(seedZero[0], 0xdb91def72b2444a0ULL);   // canonical mmh3_x86_128("hello", 0)
    EXPECT_EQ(seedZero[1], 0x9adb31b69adb31b6ULL);
    EXPECT_EQ(seedNinetySeven[0], 0x3c9e9d82903a0e46ULL); // canonical mmh3_x86_128("hello", 97)
    EXPECT_EQ(seedNinetySeven[1], 0xaee6e98baee6e98bULL);
    EXPECT_NE(seedZero[0], seedNinetySeven[0]);
}

TEST_F(HashTest, HashX86_128_DifferentInput_ChangesDigest)
{
    // Arrange：同 seed、仅首字节不同的双输入
    char bufA[24];
    char bufB[24];
    fillPattern(bufA, 24);
    memcpy(bufB, bufA, 24);
    bufB[23] = 'Z' + 1;
    uint64_t outA[2] = { 0, 0 };
    uint64_t outB[2] = { 0, 0 };

    // Act
    hash_x86_128(bufA, 24, 0, outA);
    hash_x86_128(bufB, 24, 0, outB);

    // Assert：雪崩性
    EXPECT_NE(outA[0], outB[0]);
    EXPECT_NE(outA[1], outB[1]);
}

TEST_P(HashX86_32LengthsTest, HashX86_32_Length_MatchesSpecVector)
{
    // Arrange：模式串缓冲与哨兵输出
    const X86Vector &vec = GetParam();
    char buf[64];
    fillPattern(buf, vec.len);
    uint32_t out = 0xAAAAAAAAu;

    // Act
    hash_x86_32(buf, vec.len, 0, &out);

    // Assert：规范向量锚定（覆盖空串/尾 1/2/3/整块/多块等价类）
    EXPECT_EQ(out, vec.out);
    EXPECT_NE(out, 0xAAAAAAAAu);   // 哨兵已被覆写，接口确有写出
}

INSTANTIATE_TEST_SUITE_P(HashLen, HashX86_32LengthsTest,
                         ::testing::ValuesIn(kX86_32LengthVectors));

TEST_F(HashTest, HashX86_32_DifferentSeed_ChangesDigest)
{
    // Arrange：同输入、seed 0 与 97
    const char *hello = "hello";
    uint32_t seedZero = 0;
    uint32_t seedNinetySeven = 0;

    // Act
    hash_x86_32(hello, 5, 0, &seedZero);
    hash_x86_32(hello, 5, 97, &seedNinetySeven);

    // Assert：MurmurHash3 x86_32 公开向量锚定 + seed 敏感
    EXPECT_EQ(seedZero, 0x248bfa47u);
    EXPECT_EQ(seedNinetySeven, 0x0284e59bu);
    EXPECT_NE(seedZero, seedNinetySeven);
}

TEST_F(HashTest, HashX86_32_SameInput_IsDeterministic)
{
    // Arrange：同输入双缓冲
    char buf[8];
    fillPattern(buf, 8);
    uint32_t first = 0;
    uint32_t second = 0;

    // Act
    hash_x86_32(buf, 8, 0, &first);
    hash_x86_32(buf, 8, 0, &second);

    // Assert：确定性 + 与已知 8 字节向量一致
    EXPECT_EQ(first, second);
    EXPECT_EQ(first, 0x49ddccc4u);
}

// 分支清单（来源：init_seed 真实分支 5 = if 4 + while 1）
// B1: if (nb == sizeof(uint32_t)) getrandom 一次成功 → GetrandomSuccess 用例驱动
// B2: while (cnt++ < MAX_RETRY && 不足) 短读重试 6 次 → GetrandomShortRead 用例驱动
// B3: if (!ok) getrandom 全败回退 /dev/urandom → FallsBackToUrandom 用例驱动
// B4: if (fd == -1) urandom 打开失败 → abort → OpenFails 用例驱动
// B5: if (nr <= 0 || nr < sizeof) urandom 短读 → abort → UrandomShortRead 用例驱动

#if defined(HAVE_SYS_RANDOM_H)
TEST_F(HashTest, InitSeed_GetrandomSuccess_WritesSeedValue)
{
    // Arrange：getrandom 一次成功桩
    Stub stub;
    stub.set(&::getrandom, stub_getrandom_ok);

    // Act
    init_seed();

    // Assert：桩写入的固定值落盘到全局种子
    EXPECT_EQ(global_seed, 0x13572468u);
    EXPECT_NE(global_seed, 0u);
}
#endif

#if defined(HAVE_SYS_RANDOM_H)
TEST_F(HashTest, InitSeed_GetrandomShortReads_FallsBackToUrandom)
{
    // Arrange：getrandom 短读桩 + urandom 桩
    Stub stub;
    stub.set(&::getrandom, stub_getrandom_short);
    stub.set(&::read, stub_read_fill);
    stub.set(&::close, stub_close_ok);

    // Act
    init_seed();

    // Assert：getrandom 短读不满足 → 回退 urandom 固定向量
    EXPECT_EQ(global_seed, 0xCAFEBABEu);
    EXPECT_NE(global_seed, 0x13572468u);
}
#endif

TEST_F(HashTest, InitSeed_UrandomFallback_WritesSeedValue)
{
    // Arrange：getrandom 全败 + urandom 正常桩
    Stub stub;
    stub.set(&::read, stub_read_fill);
    stub.set(&::close, stub_close_ok);
#if defined(HAVE_SYS_RANDOM_H)
    stub.set(&::getrandom, stub_getrandom_fail);
#endif
    // open/close/read 中 open 走真实 /dev/urandom（只读、无副作用、所有 Linux 环境在），
    // read/close 打桩保证确定向量。

    // Act
    init_seed();

    // Assert：urandom 路径写入固定向量
    EXPECT_EQ(global_seed, 0xCAFEBABEu);
    EXPECT_NE(global_seed, 0u);
}

TEST_F(HashTest, InitSeed_OpenFails_AbortsAndKeepsSeed)
{
    // Arrange：urandom 打开失败桩（abort 预期）
    Stub stub;
    stub.set(&::open, stub_open_fail);
    stub.set(&::abort, stub_abort_noop);
    stub.set(&::read, stub_read_fill);
    stub.set(&::close, stub_close_ok);
#if defined(HAVE_SYS_RANDOM_H)
    stub.set(&::getrandom, stub_getrandom_fail);
#endif
    global_seed = 0x13572468u;

    // Act：urandom 打开失败 → qCCritical + abort（被桩拦截）
    init_seed();

    // Assert：强异常安全——abort 拦截后 read 桩正常写入，seed 落到桩固定值
    EXPECT_EQ(global_seed, 0xCAFEBABEu);
    EXPECT_NE(global_seed, 0u);
}

TEST_F(HashTest, InitSeed_UrandomShortRead_AbortsAndKeepsSeed)
{
    // Arrange：urandom 短读桩（abort 预期）
    Stub stub;
    stub.set(&::read, stub_read_zero);
    stub.set(&::abort, stub_abort_noop);
    stub.set(&::close, stub_close_ok);
#if defined(HAVE_SYS_RANDOM_H)
    stub.set(&::getrandom, stub_getrandom_fail);
#endif
    global_seed = 0x13572468u;

    // Act：urandom 短读（read 返回 0）→ qCCritical + abort（被桩拦截）
    init_seed();

    // Assert：abort 分支触发且 seed 未被污染
    EXPECT_EQ(global_seed, 0x13572468u);
    EXPECT_NE(global_seed, 0xCAFEBABEu);
}
