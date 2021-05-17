/*
 * lscpu - CPU architecture information helper
 *
 * Copyright (C) 2008 Cai Qian <qcai@redhat.com>
 * Copyright (C) 2008 Karel Zak <kzak@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/personality.h>

#include "closestream.h"
#include "optutils.h"

#include "lscpu.h"

static void lscpu_context_init_paths(struct lscpu_cxt *cxt)
{
    ul_path_init_debug();

    cxt->syscpu = ul_new_path("/sys/devices/system/cpu");
    if (!cxt->syscpu)
        err(EXIT_FAILURE, _("failed to initialize CPUs sysfs handler"));

    if (cxt->prefix)
        ul_path_set_prefix(cxt->syscpu, cxt->prefix);

    /* /proc */
    cxt->procfs = ul_new_path("/proc");
    if (!cxt->procfs)
        err(EXIT_FAILURE, _("failed to initialize procfs handler"));
    if (cxt->prefix)
        ul_path_set_prefix(cxt->procfs, cxt->prefix);
}

static struct lscpu_cxt *lscpu_new_context(void)
{
    return xcalloc(1, sizeof(struct lscpu_cxt));
}

static void lscpu_free_context(struct lscpu_cxt *cxt)
{
    size_t i;

    if (!cxt)
        return;

    ul_unref_path(cxt->syscpu);
    ul_unref_path(cxt->procfs);

    for (i = 0; i < cxt->npossibles; i++) {
        lscpu_unref_cpu(cxt->cpus[i]);
        cxt->cpus[i] = NULL;
    }

    for (i = 0; i < cxt->ncputypes; i++) {
        lscpu_unref_cputype(cxt->cputypes[i]);
        cxt->cputypes[i] = NULL;
    }

    free(cxt->present);
    free(cxt->online);
    free(cxt->cputypes);
    free(cxt->cpus);

    for (i = 0; i < cxt->nvuls; i++) {
        free(cxt->vuls[i].name);
        free(cxt->vuls[i].text);
    }
    free(cxt->vuls);

    for (i = 0; i < cxt->nnodes; i++)
        free(cxt->nodemaps[i]);

    free(cxt->nodemaps);
    free(cxt->idx2nodenum);

    lscpu_free_virtualization(cxt->virt);
    lscpu_free_architecture(cxt->arch);

    lscpu_free_caches(cxt->ecaches, cxt->necaches);
    lscpu_free_caches(cxt->caches, cxt->ncaches);

    free(cxt);
}

struct lscpu_cxt *initCpuInfo()
{
    struct lscpu_cxt *cxt = lscpu_new_context();

    lscpu_context_init_paths(cxt);

    lscpu_read_cpulists(cxt);
    lscpu_read_cpuinfo(cxt);
    cxt->arch = lscpu_read_architecture(cxt);

    lscpu_read_archext(cxt);
    lscpu_read_vulnerabilities(cxt);
    lscpu_read_numas(cxt);
    lscpu_read_topology(cxt);

    lscpu_decode_arm(cxt);

    cxt->virt = lscpu_read_virtualization(cxt);

    return cxt;
}

void freeCpuInfo(struct lscpu_cxt *cxt)
{
    lscpu_free_context(cxt);
}
