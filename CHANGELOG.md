<!--
SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Change Log

Deepin system monitor: a more user-friendly system monitor.

## [Unreleased]

### Features

* Add dedicated GPU detail page and wire left-panel GPU click to real GPU details.
* Add NPU detection/aggregation paths and show GPU/NPU usage in detail view.
* Improve GPU engine metrics collection, including 3D/compute/copy/video engine naming.

### Improvements

* Improve integrated vs external GPU classification logic (shared-memory iGPU handling).
* Refine left-panel monitor layout with CPU/GPU same-row split and dynamic hide when GPU is not present.
* Update GPU detail memory fields/ordering to align with current detail-page design.

### Build & Test

* Refresh Qt6 test compatibility in multiple suites and fix missing test-link sources.
* Update build docs with required test dependencies (`libgtest-dev`, `libgmock-dev`).

## [6.5.38] - 2026-02-05

* fix: handle failure in resolving executable path for process ID
* chore: update version to 6.5.38

## [6.5.37] - 2025-11-27

* chore: update compiler flags for security enhancements

## [6.5.36] - 2025-11-06

* refactor: service safety rectification
* fix: fix service security issues

## [6.5.35] - 2025-10-14

* fix: can not get dconfig value issue
* chore: update version to 6.5.35

## [6.5.34] - 2025-10-10

* fix: add XDG_DATA_HOME to XDG_DATA_DIRS
* chore: update version to 6.5.34

## [6.5.33] - 2025-09-18

* fix: fix coredump issue

## [6.5.32] - 2025-09-05

* feat: change CPU processor details page content

## [6.5.31] - 2025-08-29

* fix: high CPU and empty info issue on dkapture model

## [6.5.30] - 2025-08-21

* maintenance release

## [6.5.29] - 2025-08-14

* chore: add Lao language translations in policy files

## [6.5.28] - 2025-08-13

* chore: add support for Lao language translation

## [6.5.27] - 2025-08-07

* chore: add new Lao translation file

## [6.5.26] - 2025-07-31

* feat: add average CPU frequency calculation and display

## [6.5.25] - 2025-07-31

* fix: corrected string formatting in system protection settings

## [6.5.24] - 2025-07-11

* chore: add more logs

## [6.5.23] - 2025-07-09

* chore: update deepin-manual resources
