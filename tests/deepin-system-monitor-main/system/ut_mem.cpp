// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for core::system::MemInfo (deepin-system-monitor-main/system/mem.cpp).
// All file system access is stubbed via fopen -> fmemopen; tests never touch
// the real /proc file system.

#include <gtest/gtest.h>

#include "system/mem.h"
#include "ut_stub_helpers.h"

using stub_ext::StubExt;
using ut_helpers::ProcFiles;

namespace {

// Full valid /proc/meminfo content covering every field parsed by
// MemInfo::readMemInfo (equivalence class: complete well-formed input).
static const char kFullMemInfo[] =
        "MemTotal:       16384000 kB\n"
        "MemFree:         8192000 kB\n"
        "MemAvailable:   12288000 kB\n"
        "Buffers:         262144 kB\n"
        "Cached:         4194304 kB\n"
        "Active:         3145728 kB\n"
        "Inactive:       2097152 kB\n"
        "SwapTotal:      2097152 kB\n"
        "SwapFree:       1572864 kB\n"
        "SwapCached:      262144 kB\n"
        "Shmem:           131072 kB\n"
        "Slab:            393216 kB\n"
        "Dirty:            65536 kB\n"
        "Mapped:          1048576 kB\n";

class UT_MemInfo : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_proc.clear();
        m_proc.install(m_stub);
    }

    void TearDown() override
    {
        m_proc.clear();
    }

    StubExt m_stub;
    ProcFiles m_proc;
};

// ---------------------------------------------------------------------------
// Freshly constructed MemInfo: no readMemInfo call happened yet, every getter
// must report 0 (boundary: initial state / invalid class).
// Branch: MemInfo::MemInfo() -> MemInfoPrivate default ctor zero-init.
// ---------------------------------------------------------------------------
TEST_F(UT_MemInfo, Construction_DefaultObject_AllFieldsZero)
{
    // Act
    core::system::MemInfo info;

    // Assert
    EXPECT_EQ(0u, info.memTotal());
    EXPECT_EQ(0u, info.memAvailable());
    EXPECT_EQ(0u, info.buffers());
    EXPECT_EQ(0u, info.cached());
    EXPECT_EQ(0u, info.active());
    EXPECT_EQ(0u, info.inactive());
    EXPECT_EQ(0u, info.swapTotal());
    EXPECT_EQ(0u, info.swapFree());
    EXPECT_EQ(0u, info.swapCached());
    EXPECT_EQ(0u, info.shmem());
    EXPECT_EQ(0u, info.slab());
    EXPECT_EQ(0u, info.dirty());
    EXPECT_EQ(0u, info.mapped());
}

// ---------------------------------------------------------------------------
// readMemInfo with complete well-formed /proc/meminfo content.
// Branches: fopen success; every sscanf "key: %llu kB" match succeeds;
// all 14 known keys hit their assignment branch.
// ---------------------------------------------------------------------------
TEST_F(UT_MemInfo, ReadMemInfo_FullContent_AllFieldsParsed)
{
    // Arrange
    m_proc.add("/proc/meminfo", kFullMemInfo);
    core::system::MemInfo info;

    // Act
    info.readMemInfo();

    // Assert
    EXPECT_EQ(16384000u, info.memTotal());
    EXPECT_EQ(12288000u, info.memAvailable());
    EXPECT_EQ(262144u, info.buffers());
    EXPECT_EQ(4194304u, info.cached());
    EXPECT_EQ(3145728u, info.active());
    EXPECT_EQ(2097152u, info.inactive());
    EXPECT_EQ(2097152u, info.swapTotal());
    EXPECT_EQ(1572864u, info.swapFree());
    EXPECT_EQ(262144u, info.swapCached());
    EXPECT_EQ(131072u, info.shmem());
    EXPECT_EQ(393216u, info.slab());
    EXPECT_EQ(65536u, info.dirty());
    EXPECT_EQ(1048576u, info.mapped());
}

// ---------------------------------------------------------------------------
// readMemInfo when fopen fails (missing file): warning path, all fields must
// keep their previous (zero) values.
// Branch: fopen(PROC_PATH_MEM) == nullptr -> warn + early return.
// ---------------------------------------------------------------------------
TEST_F(UT_MemInfo, ReadMemInfo_OpenFails_FieldsRemainZero)
{
    // Arrange: no file registered -> fopen returns nullptr
    core::system::MemInfo info;

    // Act
    info.readMemInfo();

    // Assert
    EXPECT_EQ(0u, info.memTotal());
    EXPECT_EQ(0u, info.memAvailable());
    EXPECT_EQ(0u, info.swapTotal());
    EXPECT_EQ(0u, info.mapped());
}

// ---------------------------------------------------------------------------
// Partial content: only a subset of keys present. Unmentioned fields must
// remain 0 (equivalence class: incomplete but well-formed input).
// Branch: lines for absent keys never appear -> default values kept.
// ---------------------------------------------------------------------------
TEST_F(UT_MemInfo, ReadMemInfo_PartialContent_UnmentionedFieldsZero)
{
    // Arrange
    m_proc.add("/proc/meminfo",
               "MemTotal:       1000000 kB\n"
               "SwapFree:        500000 kB\n");
    core::system::MemInfo info;

    // Act
    info.readMemInfo();

    // Assert
    EXPECT_EQ(1000000u, info.memTotal());
    EXPECT_EQ(500000u, info.swapFree());
    EXPECT_EQ(0u, info.memAvailable());
    EXPECT_EQ(0u, info.cached());
    EXPECT_EQ(0u, info.swapTotal());
    EXPECT_EQ(0u, info.slab());
}

// ---------------------------------------------------------------------------
// Boundary value: 0 as a legit parsed value is stored as 0.
// Branch: sscanf succeeds with 0 -> field set to 0.
// ---------------------------------------------------------------------------
TEST_F(UT_MemInfo, ReadMemInfo_ZeroValue_StoredAsZero)
{
    // Arrange
    m_proc.add("/proc/meminfo",
               "MemTotal:       0 kB\n"
               "MemAvailable:   0 kB\n"
               "Dirty:          0 kB\n");
    core::system::MemInfo info;

    // Act
    info.readMemInfo();

    // Assert
    EXPECT_EQ(0u, info.memTotal());
    EXPECT_EQ(0u, info.memAvailable());
    EXPECT_EQ(0u, info.dirty());
}

// ---------------------------------------------------------------------------
// Malformed values: sscanf "%llu" fails on non-numeric payload -> field stays
// 0 and parsing continues with the next line (invalid equivalence class).
// Branch: sscanf(...) != 1 -> warn + continue loop.
// ---------------------------------------------------------------------------
TEST_F(UT_MemInfo, ReadMemInfo_MalformedValues_FieldsStayZero)
{
    // Arrange
    m_proc.add("/proc/meminfo",
               "MemTotal:  not-a-number\n"
               "MemAvailable:\n"
               "Cached:     4242 kB\n");
    core::system::MemInfo info;

    // Act
    info.readMemInfo();

    // Assert
    EXPECT_EQ(0u, info.memTotal());
    EXPECT_EQ(0u, info.memAvailable());
    EXPECT_EQ(4242u, info.cached());
}

// ---------------------------------------------------------------------------
// Unknown keys are ignored: parsing must not break nor touch any field.
// Branch: no key matches -> loop continues without assignment.
// ---------------------------------------------------------------------------
TEST_F(UT_MemInfo, ReadMemInfo_UnknownKeysOnly_AllFieldsZero)
{
    // Arrange
    m_proc.add("/proc/meminfo",
               "UnknownKey1:    111 kB\n"
               "AnotherKey:     222 kB\n");
    core::system::MemInfo info;

    // Act
    info.readMemInfo();

    // Assert
    EXPECT_EQ(0u, info.memTotal());
    EXPECT_EQ(0u, info.memAvailable());
    EXPECT_EQ(0u, info.swapTotal());
}

// ---------------------------------------------------------------------------
// Copy semantics: MemInfo shares data via QSharedDataPointer. A copy taken
// before readMemInfo shares the same private; the source must not observe
// the copy's later readMemInfo results (copy-on-write on non-const access).
// Branch: operator= shares d; readMemInfo detaches through operator->().
// ---------------------------------------------------------------------------
TEST_F(UT_MemInfo, CopyAssignment_CopyDetachesOnRead)
{
    // Arrange
    m_proc.add("/proc/meminfo", kFullMemInfo);
    core::system::MemInfo source;

    // Act: copy an empty info, then read data only on the copy
    core::system::MemInfo copied(source);
    copied.readMemInfo();

    // Assert: source keeps zero values, copy carries parsed values
    EXPECT_EQ(0u, source.memTotal());
    EXPECT_EQ(16384000u, copied.memTotal());

    // Additional: assignment variant
    core::system::MemInfo assigned;
    assigned = source;
    assigned.readMemInfo();
    EXPECT_EQ(0u, source.memTotal());
    EXPECT_EQ(16384000u, assigned.memTotal());
}

// ---------------------------------------------------------------------------
// Copy of a filled object: values equal the source right after copying.
// Branch: copy ctor shares d, const access does not detach.
// ---------------------------------------------------------------------------
TEST_F(UT_MemInfo, CopyCtor_AfterRead_ValuesEqualSource)
{
    // Arrange
    m_proc.add("/proc/meminfo", kFullMemInfo);
    core::system::MemInfo source;
    source.readMemInfo();

    // Act
    core::system::MemInfo copied(source);

    // Assert
    EXPECT_EQ(source.memTotal(), copied.memTotal());
    EXPECT_EQ(source.memAvailable(), copied.memAvailable());
    EXPECT_EQ(source.swapTotal(), copied.swapTotal());
}

} // namespace
