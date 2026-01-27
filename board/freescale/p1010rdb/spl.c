// SPDX-License-Identifier: GPL-2.0+
/* Copyright 2013 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <clock_legacy.h>
#include <console.h>
#include <env.h>
#include <env_internal.h>
#include <init.h>
#include <ns16550.h>
#include <malloc.h>
#include <mmc.h>
#include <nand.h>
#include <i2c.h>
#include <fsl_esdhc.h>
#include <spi_flash.h>
#include <asm/global_data.h>
#include "../common/spl.h"

DECLARE_GLOBAL_DATA_PTR;

phys_size_t get_effective_memsize(void)
{
	return CONFIG_SYS_L2_SIZE;
}
void board_init_r(gd_t *gd, ulong dest_addr);
void board_init_f(ulong bootflag)
{
	u32 plat_ratio;
	ccsr_gur_t *gur = (void *)CFG_SYS_MPC85xx_GUTS_ADDR;
	struct fsl_ifc ifc = {(void *)CFG_SYS_IFC_ADDR, (void *)NULL};

	console_init_f();
	printf("\n1 console_init_f();\n");
	/* Clock configuration to access CPLD using IFC(GPCM) */
	setbits_be32(&ifc.gregs->ifc_gcr, 1 << IFC_GCR_TBCTL_TRN_TIME_SHIFT);
	printf("2 setbits_be32(&ifc.gregs->ifc_gcr, 1 << IFC_GCR_TBCTL_TRN_TIME_SHIFT);\n ");
#ifdef CONFIG_TARGET_P1010RDB_PB
	setbits_be32(&gur->pmuxcr2, MPC85xx_PMUXCR2_GPIO01_DRVVBUS);
	printf("3setbits_be32(&gur->pmuxcr2, MPC85xx_PMUXCR2_GPIO01_DRVVBUS);\n");
#endif

	/* initialize selected port with appropriate baud rate */
	plat_ratio = in_be32(&gur->porpllsr) & MPC85xx_PORPLLSR_PLAT_RATIO;
	plat_ratio >>= 1;
	gd->bus_clk = get_board_sys_clk() * plat_ratio;

	ns16550_init((struct ns16550 *)CFG_SYS_NS16550_COM2,
		     gd->bus_clk / 16 / CONFIG_BAUDRATE);

#ifdef CONFIG_SPL_MMC_BOOT
	puts("\nSD boot...\n");
#elif defined(CONFIG_SPL_SPI_BOOT)
	puts("\nSPI Flash boot...\n");
#endif
	/* copy code to RAM and jump to it - this should not return */
	/* NOTE - code has to be copied out of NAND buffer before
	 * other blocks can be read.
	*/
	puts("4 before relocate\n");
	printf("MONITOR_BASE=%lx SPL_TEXT_BASE=%lx\n",
    (ulong)CONFIG_SYS_MONITOR_BASE, (ulong)CONFIG_SPL_TEXT_BASE);
	printf("board_init_f=%p\n", board_init_f);
	relocate_code(CONFIG_VAL(RELOC_STACK), 0, CONFIG_SPL_RELOC_TEXT_BASE); 
	
		puts("5 after relocate\n");
		puts("skip relocate -> jump board_init_r\n");
board_init_r(NULL, 0);

/* board_init_r가 리턴하면 안 되므로 여기서 멈춤 */
while (1) ;

}

void board_init_r(gd_t *gd, ulong dest_addr)
{
	/* Pointer is writable since we allocated a register for it */
	gd = (gd_t *)CONFIG_VAL(GD_ADDR);
	struct bd_info *bd;

	memset(gd, 0, sizeof(gd_t));
	bd = (struct bd_info *)(CONFIG_VAL(GD_ADDR) + sizeof(gd_t));
	memset(bd, 0, sizeof(struct bd_info));
	gd->bd = bd;

	arch_cpu_init();
	printf("arch_cpu_init();\n");
	get_clocks();
	printf("5get_clocks();\n");
	mem_malloc_init(CONFIG_VAL(RELOC_MALLOC_ADDR),
			CONFIG_VAL(RELOC_MALLOC_SIZE));
	gd->flags |= GD_FLG_FULL_MALLOC_INIT;

#ifndef CONFIG_SPL_NAND_BOOT
	env_init();
#endif
#ifdef CONFIG_SPL_MMC_BOOT
	mmc_initialize(bd);
#endif

	/* relocate environment function pointers etc. */
#ifdef CONFIG_SPL_NAND_BOOT
	nand_spl_load_image(CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE,
			    (uchar *)SPL_ENV_ADDR);
			    gd->env_addr  = (ulong)(SPL_ENV_ADDR);
	gd->env_valid = ENV_VALID;
#else
	env_relocate();
#endif

	i2c_init_all();

	dram_init();

#ifdef CONFIG_SPL_NAND_BOOT
	puts("\nTertiary program loader running in sram...");
#else
	puts("\nSecond program loader running in sram...");
#endif



#ifdef CONFIG_SPL_MMC_BOOT
	mmc_boot();
#elif defined(CONFIG_SPL_SPI_BOOT)
	fsl_spi_boot();
#elif defined(CONFIG_SPL_NAND_BOOT)

puts("\n[DBG] before nand_boot\n");
printf("[DBG] MONITOR_BASE=%08lx\n", (ulong)CONFIG_SYS_MONITOR_BASE);
/* 아래 매크로가 있으면 같이 */
printf("[DBG] NAND_U_BOOT_START=%08x\n", (u32)CFG_SYS_NAND_U_BOOT_START);
printf("[DBG] NAND_U_BOOT_DST  =%08x\n", (u32)CFG_SYS_NAND_U_BOOT_DST);
printf("[DBG] NAND_U_BOOT_SIZE =%08x\n", (u32)CFG_SYS_NAND_U_BOOT_SIZE);
	nand_boot();

	puts("[DBG] after nand_boot (should not return)\n");
while (1) ;
#endif
}