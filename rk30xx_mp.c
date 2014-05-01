/*-
 * Copyright (c) 2014 Ganbold Tsagaankhuu <ganbold@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/smp.h>

#include <machine/smp.h>
#include <machine/fdt.h>
#include <machine/intr.h>

#define	SCU_PHYSBASE			0x1013c000
#define	SCU_SIZE			0x100

#define	SCU_CONTROL_REG			0x00
#define	SCU_CONTROL_ENABLE		(1 << 0)
#define	SCU_STANDBY_EN			(1 << 5)
#define	SCU_CONFIG_REG			0x04
#define	SCU_CONFIG_REG_NCPU_MASK	0x03
#define	SCU_CPUPOWER_REG		0x08
#define	SCU_INV_TAGS_REG		0x0c

#define	SCU_FILTER_START_REG		0x10
#define	SCU_FILTER_END_REG		0x14
#define	SCU_SECURE_ACCESS_REG		0x18
#define	SCU_NONSECURE_ACCESS_REG	0x1c

#define	IMEM_PHYSBASE			0x10080000
//#define	IMEM_SIZE			0x10
//#define	IMEM_SIZE			0x8000
#define	IMEM_SIZE			0x50

#define	PMU_PHYSBASE			0x20004000
#define	PMU_SIZE			0x100
#define	PMU_PWRDN_CON			0x08

void
platform_mp_init_secondary(void)
{

	gic_init_secondary();
}

void
platform_mp_setmaxid(void)
{
//	bus_space_handle_t scu;
//	int hwcpu, ncpu;
//	uint32_t val;

	/* If we've already set the global vars don't bother to do it again. */
	if (mp_ncpus != 0)
		return;
/*
	if (bus_space_map(fdtbus_bs_tag, SCU_PHYSBASE, SCU_SIZE, 0, &scu) != 0)
		panic("Couldn't map the SCU\n");

	val = bus_space_read_4(fdtbus_bs_tag, scu, SCU_CONFIG_REG);
	hwcpu = (val & SCU_CONFIG_REG_NCPU_MASK) + 1;
	bus_space_unmap(fdtbus_bs_tag, scu, SCU_SIZE);

	ncpu = hwcpu;
	TUNABLE_INT_FETCH("hw.ncpu", &ncpu);
	if (ncpu < 1 || ncpu > hwcpu)
		ncpu = hwcpu;

	mp_ncpus = ncpu;
	mp_maxid = ncpu - 1;
*/
	mp_ncpus = 4;
	mp_maxid = 3;
}

int
platform_mp_probe(void)
{

	/* I think platform_mp_setmaxid must get called first, but be safe. */
	if (mp_ncpus == 0)
		platform_mp_setmaxid();

	return (mp_ncpus > 1);
}

void    
platform_mp_start_ap(void)
{
	bus_space_handle_t scu;
	bus_space_handle_t imem;
	bus_space_handle_t pmu;

	uint32_t val;
	int i;

	if (bus_space_map(fdtbus_bs_tag, SCU_PHYSBASE, SCU_SIZE, 0, &scu) != 0)
		panic("Couldn't map the SCU\n");
	if (bus_space_map(fdtbus_bs_tag, IMEM_PHYSBASE, IMEM_SIZE, 0, &imem) != 0)
		panic("Couldn't map the IMEM\n");
	if (bus_space_map(fdtbus_bs_tag, PMU_PHYSBASE, PMU_SIZE, 0, &pmu) != 0)
		panic("Couldn't map the PMU\n");

	/* First make sure that all cores except the first are really off */
	val = bus_space_read_4(fdtbus_bs_tag, pmu, PMU_PWRDN_CON);
	for (i = 1; i < mp_ncpus; i++)
		val |= 1 << i;
	bus_space_write_4(fdtbus_bs_tag, pmu, PMU_PWRDN_CON, val);

	/* Write start address */
	bus_space_write_4(fdtbus_bs_tag, imem, 0,
	    pmap_kextract((vm_offset_t)mpentry));

	cpu_idcache_wbinv_all();
	cpu_l2cache_wbinv_all();

	/* Enable the SCU power domain */
	val = bus_space_read_4(fdtbus_bs_tag, pmu, PMU_PWRDN_CON);
	val &= (1 << 4);
	bus_space_write_4(fdtbus_bs_tag, pmu, PMU_PWRDN_CON, val);

	val = bus_space_read_4(fdtbus_bs_tag, scu, SCU_CONTROL_REG);
	bus_space_write_4(fdtbus_bs_tag, scu, SCU_CONTROL_REG, 
	    val | SCU_STANDBY_EN);

	/* Enable the SCU */
	val = bus_space_read_4(fdtbus_bs_tag, scu, SCU_CONTROL_REG);
	bus_space_write_4(fdtbus_bs_tag, scu, SCU_CONTROL_REG, 
	    val | SCU_CONTROL_ENABLE);

	/* Start all cores */
	for (i = 1; i < mp_ncpus; i++) {
		val = bus_space_read_4(fdtbus_bs_tag, pmu, PMU_PWRDN_CON);
		val &= (1 << i);
		bus_space_write_4(fdtbus_bs_tag, pmu, PMU_PWRDN_CON, val);
	}

	armv7_sev();

	bus_space_unmap(fdtbus_bs_tag, scu, SCU_SIZE);
	bus_space_unmap(fdtbus_bs_tag, imem, IMEM_SIZE);
	bus_space_unmap(fdtbus_bs_tag, pmu, PMU_SIZE);
}

void
platform_ipi_send(cpuset_t cpus, u_int ipi)
{

	pic_ipi_send(cpus, ipi);
}
