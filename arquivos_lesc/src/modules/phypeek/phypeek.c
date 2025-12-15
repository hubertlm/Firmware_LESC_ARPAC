/****************************************************************************
 *
 *   Copyright (c) 2012-2022 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <px4_platform_common/log.h>
#include <px4_platform_common/module.h>
#include <nuttx/net/mii.h>
#include <arch/board/board.h>
#include <stdint.h>

#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/imxrt_enet.h"
//#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/imxrt_enet.h"
//#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/imxrt_enet.c"
#include <nuttx/net/mii.h>
#include <arch/board/board.h>
#include <syslog.h>

#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/imxrt_memorymap.h"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/imxrt_iomuxc.h"

//#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/imxrt_clockconfig_ver2.c"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/imxrt_clockconfig_ver2.h"

#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/imxrt_lcd.h"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/imxrt_pmu.h"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/imxrt_memorymap.h"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/imxrt_iomuxc.h"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/rt117x/imxrt117x_osc.h"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/rt117x/imxrt117x_pll.h"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/rt117x/imxrt117x_anadig.h"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/rt117x/imxrt117x_ocotp.h"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/rt117x/imxrt117x_gpc.h"
#include "../../../platforms/nuttx/NuttX/nuttx/arch/arm/src/common/arm_internal.h"


#ifndef ENET_EIR_MII
#define ENET_EIR_MII (1 << 23)
#endif

/* MII Management Frame Register */

#define ENET_MMFR_DATA_SHIFT         (0)       /* Bits 0-15: Management frame data */
#define ENET_MMFR_DATA_MASK          (0xffff << ENET_MMFR_DATA_SHIFT)
#define ENET_MMFR_TA_SHIFT           (16)      /* Bits 16-17: Turn around */
#define ENET_MMFR_TA_MASK            (3 << ENET_MMFR_TA_SHIFT)
#define ENET_MMFR_RA_SHIFT           (18)      /* Bits 18-22: Register address */
#define ENET_MMFR_RA_MASK            (31 << ENET_MMFR_RA_SHIFT)
#define ENET_MMFR_PA_SHIFT           (23)      /* Bits 23-27: PHY address */
#define ENET_MMFR_PA_MASK            (31 << ENET_MMFR_PA_SHIFT)
#define ENET_MMFR_OP_SHIFT           (28)      /* Bits 28-29: Operation code */
#define ENET_MMFR_OP_MASK            (3 << ENET_MMFR_OP_SHIFT)
#  define ENET_MMFR_OP_WRNOTMII      (0 << ENET_MMFR_OP_SHIFT) /* Write frame, not MII compliant */
#  define ENET_MMFR_OP_WRMII         (1 << ENET_MMFR_OP_SHIFT) /* Write frame, MII management frame */
#  define ENET_MMFR_OP_RDMII         (2 << ENET_MMFR_OP_SHIFT) /* Read frame, MII management frame */
#  define ENET_MMFR_OP_RDNOTMII      (3 << ENET_MMFR_OP_SHIFT) /* Read frame, not MII compliant */

#define ENET_MMFR_ST_SHIFT           (30)      /* Bits 30-31: Start of frame delimiter */
#define ENET_MMFR_ST_MASK            (3 << ENET_MMFR_ST_SHIFT)


#define PHY_PAGE_SEL_REG 31
#define PHY_RMSR_REG     16
#define PHY_RMSR_VALUE   0x7FFB
#define RTL8201F_PHYADDR 3

static void imxrt_phy_write(uint8_t phyaddr, uint8_t regaddr, uint16_t value)
{
    uint32_t command =
        ((1 << ENET_MMFR_ST_SHIFT) & ENET_MMFR_ST_MASK) |
        (ENET_MMFR_OP_WRMII & ENET_MMFR_OP_MASK) |
        ((phyaddr << ENET_MMFR_PA_SHIFT) & ENET_MMFR_PA_MASK) |
        ((regaddr << ENET_MMFR_RA_SHIFT) & ENET_MMFR_RA_MASK) |
        ((2 << ENET_MMFR_TA_SHIFT) & ENET_MMFR_TA_MASK) |
        (value & ENET_MMFR_DATA_MASK);

    int timeout = 10000;
    while ((getreg32(IMXRT_ENET_EIR) & ENET_EIR_MII) && --timeout);
    if (timeout == 0) {
        PX4_ERR("Timeout aguardando EIR limpar (escrita)");
        return;
    }

    putreg32(command, IMXRT_ENET_MMFR);

    timeout = 10000;
    while (!(getreg32(IMXRT_ENET_EIR) & ENET_EIR_MII) && --timeout);
    if (timeout == 0) {
        PX4_ERR("Timeout aguardando resposta do PHY na escrita");
        return;
    }

    putreg32(ENET_EIR_MII, IMXRT_ENET_EIR);
}

static uint16_t imxrt_phy_read(uint8_t phyaddr, uint8_t regaddr)
{
    uint32_t command =
        ((1 << ENET_MMFR_ST_SHIFT) & ENET_MMFR_ST_MASK) |
        (ENET_MMFR_OP_RDMII & ENET_MMFR_OP_MASK) |
        ((phyaddr << ENET_MMFR_PA_SHIFT) & ENET_MMFR_PA_MASK) |
        ((regaddr << ENET_MMFR_RA_SHIFT) & ENET_MMFR_RA_MASK) |
        ((2 << ENET_MMFR_TA_SHIFT) & ENET_MMFR_TA_MASK);

    int timeout = 10000;
    while ((getreg32(IMXRT_ENET_EIR) & ENET_EIR_MII) && --timeout);
    if (timeout == 0) {
        PX4_ERR("Timeout aguardando EIR limpar (leitura)");
        return 0xFFFF;
    }

    putreg32(command, IMXRT_ENET_MMFR);

    timeout = 10000;
    while (!(getreg32(IMXRT_ENET_EIR) & ENET_EIR_MII) && --timeout);
    if (timeout == 0) {
        PX4_ERR("Timeout aguardando resposta do PHY no MDIO");
        return 0xFFFF;
    }

    putreg32(ENET_EIR_MII, IMXRT_ENET_EIR);
    return (uint16_t)(getreg32(IMXRT_ENET_MMFR) & ENET_MMFR_DATA_MASK);
}

static void print_binary16(uint16_t value)
{
	char bin[17];
	bin[16] = '\0'; // null terminator

	for (int i = 15; i >= 0; --i) {
		bin[15 - i] = (value & (1 << i)) ? '1' : '0';
	}

	PX4_INFO("  Bin: %s", bin);
}

int phypeek_main(int argc, char *argv[])
{
    PX4_INFO("Inicializando ENET e tentando comunicação com o PHY...");
    up_mdelay(100);

   int phyaddr = 3;
        uint16_t phyid1 = imxrt_phy_read(phyaddr, MII_PHYID1);
        uint16_t phyid2 = imxrt_phy_read(phyaddr, MII_PHYID2);

        PX4_INFO("Leitura endereço %d: ID1=0x%04x ID2=0x%04x", phyaddr, phyid1, phyid2);

        if ((phyid1 != 0xFFFF && phyid1 != 0x0000) && (phyid2 != 0xFFFF && phyid2 != 0x0000)) {
            PX4_INFO("PHY encontrado no endereço %d", phyaddr);

	    uint16_t val;
	    uint8_t reg;
            
            PX4_INFO("Basic Control Register:");
            val = imxrt_phy_read(phyaddr, 0);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 0, val);
            print_binary16(val);
            
            PX4_INFO("Basic Status Register:");
            val = imxrt_phy_read(phyaddr, 1);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 1, val);
            print_binary16(val);            
            
            PX4_INFO("PHY Identifier 1:");
            val = imxrt_phy_read(phyaddr, 2);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 2, val);
            print_binary16(val);
            
            PX4_INFO("PHY Identifier 2:");
            val = imxrt_phy_read(phyaddr, 3);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 3, val);
            print_binary16(val);
            
            PX4_INFO("Auto-Negotiation Advertisement Register:");
            val = imxrt_phy_read(phyaddr, 4);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 4, val);
            print_binary16(val);
            
            PX4_INFO("Auto-Negotiation Link Partner Ability Register:");
            val = imxrt_phy_read(phyaddr, 5);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 5, val);
            print_binary16(val);
            
            PX4_INFO("Auto-Negotiation Expansion Register:");
            val = imxrt_phy_read(phyaddr, 6);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 6, val);
            print_binary16(val);
            
            PX4_INFO("Auto-Negotiation Next Page TX:");
            val = imxrt_phy_read(phyaddr, 7);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 7, val);
            print_binary16(val);
            
            PX4_INFO("Auto-Negotiation Next Page RX:");
            val = imxrt_phy_read(phyaddr, 8);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 8, val);
            print_binary16(val);
            
            PX4_INFO("Register 24 Power Saving Mode Register (PSMR):");
            val = imxrt_phy_read(phyaddr, 24);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 24, val);

            PX4_INFO("Register 28 Fiber Mode and Loopback Register:");
            val = imxrt_phy_read(phyaddr, 28);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 28, val);
            
            PX4_INFO("Register 30 Interrupt Indicators and SNR Display Register");
            val = imxrt_phy_read(phyaddr, 30);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 30, val);
            
            PX4_INFO("Register 31 Page Select Register");
            val = imxrt_phy_read(phyaddr, 31);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 28, val);
            
            PX4_INFO("Page 7 Register 16 RMII Mode Setting Register (RMSR)");
            imxrt_phy_write(phyaddr, 31, 7);
            val = imxrt_phy_read(phyaddr, 16);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 16, val);
            print_binary16(val);
            
            imxrt_phy_write(phyaddr, 31, 7);
            val = imxrt_phy_read(phyaddr, 31);
            
            reg = 16;
            val = imxrt_phy_read(phyaddr, reg);
            PX4_INFO("  Reg 0x%02X: 0x%04X", 16, val);
        }

    return 0;
}

