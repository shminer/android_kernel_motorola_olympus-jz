/*
 * arch/arm/mach-tegra/board-olympus-pinmux.c
 *
 * Copyright 2013: Olympus Kernel Project
 * <http://forum.xda-developers.com/showthread.php?t=2016837>
 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <mach/pinmux.h>

#include "gpio-names.h"

/* Setting the drive strength of pins
 * hsm: Enable High speed mode (ENABLE/DISABLE)
 * Schimit: Enable/disable schimit (ENABLE/DISABLE)
 * drive: low power mode (DIV_1, DIV_2, DIV_4, DIV_8)
 * pulldn_drive - drive down (falling edge) - Driver Output Pull-Down drive
 *                strength code. Value from 0 to 31.
 * pullup_drive - drive up (rising edge)  - Driver Output Pull-Up drive
 *                strength code. Value from 0 to 31.
 * pulldn_slew -  Driver Output Pull-Up slew control code  - 2bit code
 *                code 11 is least slewing of signal. code 00 is highest
 *                slewing of the signal.
 *                Value - FASTEST, FAST, SLOW, SLOWEST
 * pullup_slew -  Driver Output Pull-Down slew control code -
 *                code 11 is least slewing of signal. code 00 is highest
 *                slewing of the signal.
 *                Value - FASTEST, FAST, SLOW, SLOWEST
 */
#define SET_DRIVE(_name, _hsm, _schmitt, _drive, _pulldn_drive, _pullup_drive, _pulldn_slew, _pullup_slew) \
	{                                               \
		.pingroup = TEGRA_DRIVE_PINGROUP_##_name,   \
		.hsm = TEGRA_HSM_##_hsm,                    \
		.schmitt = TEGRA_SCHMITT_##_schmitt,        \
		.drive = TEGRA_DRIVE_##_drive,              \
		.pull_down = TEGRA_PULL_##_pulldn_drive,    \
		.pull_up = TEGRA_PULL_##_pullup_drive,		\
		.slew_rising = TEGRA_SLEW_##_pulldn_slew,   \
		.slew_falling = TEGRA_SLEW_##_pullup_slew,	\
	}

#define DEFAULT_DRIVE(_name)					\
	{							\
		.pingroup = TEGRA_DRIVE_PINGROUP_##_name,	\
		.hsm = TEGRA_HSM_DISABLE,			\
		.schmitt = TEGRA_SCHMITT_ENABLE,		\
		.drive = TEGRA_DRIVE_DIV_1,			\
		.pull_down = TEGRA_PULL_31,			\
		.pull_up = TEGRA_PULL_31,			\
		.slew_rising = TEGRA_SLEW_SLOWEST,		\
		.slew_falling = TEGRA_SLEW_SLOWEST,		\
	}
#if 0
static __initdata struct tegra_drive_pingroup_config olympus_drive_pinmux[] = {
	DEFAULT_DRIVE(VI1),
	DEFAULT_DRIVE(SDIO1),
	SET_DRIVE(DAP2, DISABLE, ENABLE, DIV_1, 46, 46, SLOWEST, SLOWEST),
	SET_DRIVE(DAP3, DISABLE, ENABLE, DIV_1, 46, 46, SLOWEST, SLOWEST),
	SET_DRIVE(DBG, DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),  /* I2C1 */
	SET_DRIVE(DDC, DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),  /* I2C2-1 */
	SET_DRIVE(AT1, DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),  /* I2C2-2 */
	SET_DRIVE(VI2, DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),  /* I2C3 */
	SET_DRIVE(AO1, DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),  /* DVC */
};
#endif

static __initdata struct tegra_drive_pingroup_config olympus_drive_pinmux[] = {
	{TEGRA_DRIVE_PINGROUP_DBG,     TEGRA_HSM_DISABLE, TEGRA_SCHMITT_ENABLE,  TEGRA_DRIVE_DIV_1, TEGRA_PULL_31, TEGRA_PULL_31, TEGRA_SLEW_SLOWEST, TEGRA_SLEW_SLOWEST},
	{TEGRA_DRIVE_PINGROUP_VI1,     TEGRA_HSM_DISABLE, TEGRA_SCHMITT_ENABLE,  TEGRA_DRIVE_DIV_1, TEGRA_PULL_31, TEGRA_PULL_31, TEGRA_SLEW_SLOWEST, TEGRA_SLEW_SLOWEST},
	{TEGRA_DRIVE_PINGROUP_VI2,     TEGRA_HSM_DISABLE, TEGRA_SCHMITT_ENABLE,  TEGRA_DRIVE_DIV_1, TEGRA_PULL_31, TEGRA_PULL_31, TEGRA_SLEW_SLOWEST, TEGRA_SLEW_SLOWEST},
	{TEGRA_DRIVE_PINGROUP_SDIO1,   TEGRA_HSM_DISABLE, TEGRA_SCHMITT_ENABLE,  TEGRA_DRIVE_DIV_1, TEGRA_PULL_31, TEGRA_PULL_31, TEGRA_SLEW_SLOWEST, TEGRA_SLEW_SLOWEST},
	{TEGRA_DRIVE_PINGROUP_DDC,     TEGRA_HSM_DISABLE, TEGRA_SCHMITT_ENABLE,  TEGRA_DRIVE_DIV_1, TEGRA_PULL_31, TEGRA_PULL_31, TEGRA_SLEW_SLOWEST, TEGRA_SLEW_SLOWEST},
};

static __initdata struct tegra_pingroup_config olympus_pinmux[] = {
	{TEGRA_PINGROUP_ATA,             TEGRA_MUX_GMI,			TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_ATB,             TEGRA_MUX_SDIO4,		TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},  //emmc + sdcard detect
	{TEGRA_PINGROUP_ATC,             TEGRA_MUX_GMI,			TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_ATD,             TEGRA_MUX_GMI,			TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_ATE,             TEGRA_MUX_GMI,			TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_CDEV1,           TEGRA_MUX_OSC,			TEGRA_PUPD_NORMAL,    TEGRA_TRI_TRISTATE}, //audio
	{TEGRA_PINGROUP_CDEV2,           TEGRA_MUX_OSC,			TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_CRTP,            TEGRA_MUX_CRT,			TEGRA_PUPD_NORMAL,    TEGRA_TRI_TRISTATE},
	{TEGRA_PINGROUP_CSUS,            TEGRA_MUX_VI_SENSOR_CLK,	TEGRA_PUPD_NORMAL,    TEGRA_TRI_TRISTATE}, //camera/flash
	{TEGRA_PINGROUP_DAP1,            TEGRA_MUX_DAP1,		TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_TRISTATE}, // audio output
	{TEGRA_PINGROUP_DAP2,            TEGRA_MUX_DAP2,		TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_TRISTATE}, // audio input
	{TEGRA_PINGROUP_DAP3,            TEGRA_MUX_DAP3,		TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_TRISTATE}, // audio output on call??
	{TEGRA_PINGROUP_DAP4,            TEGRA_MUX_DAP4,		TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_TRISTATE}, // audio input on call??
	{TEGRA_PINGROUP_DDC,             TEGRA_MUX_I2C2,		TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_DTA,             TEGRA_MUX_VI,			TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_DTB,             TEGRA_MUX_VI,			TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},  //camera
	{TEGRA_PINGROUP_DTC,             TEGRA_MUX_VI,			TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_DTD,             TEGRA_MUX_VI,			TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},  //camera
	{TEGRA_PINGROUP_DTE,             TEGRA_MUX_VI,			TEGRA_PUPD_NORMAL,	TEGRA_TRI_NORMAL},  //camera/flash/modem
	{TEGRA_PINGROUP_DTF,             TEGRA_MUX_I2C3,		TEGRA_PUPD_PULL_DOWN,	TEGRA_TRI_NORMAL}, //
	{TEGRA_PINGROUP_GMA,             TEGRA_MUX_SDIO4,		TEGRA_PUPD_NORMAL,	TEGRA_TRI_NORMAL}, //mmc
	{TEGRA_PINGROUP_GMB,             TEGRA_MUX_GMI,			TEGRA_PUPD_NORMAL,	TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_GMC,             TEGRA_MUX_UARTD,		TEGRA_PUPD_PULL_UP,	TEGRA_TRI_NORMAL}, //modem uart
	{TEGRA_PINGROUP_GMD,             TEGRA_MUX_GMI,			TEGRA_PUPD_NORMAL,	TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_GME,             TEGRA_MUX_SDIO4,		TEGRA_PUPD_NORMAL,	TEGRA_TRI_NORMAL}, //emmc
	{TEGRA_PINGROUP_GPU,             TEGRA_MUX_GMI,			TEGRA_PUPD_PULL_UP,	TEGRA_TRI_NORMAL},  //bluetooth
	{TEGRA_PINGROUP_GPU7,            TEGRA_MUX_RTCK,		TEGRA_PUPD_NORMAL,	TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_GPV,             TEGRA_MUX_PCIE,		TEGRA_PUPD_NORMAL,	TEGRA_TRI_NORMAL}, // something cpcap/usb? in cm7
	{TEGRA_PINGROUP_HDINT,           TEGRA_MUX_HDMI,		TEGRA_PUPD_NORMAL,	TEGRA_TRI_NORMAL}, //HDMI video
	{TEGRA_PINGROUP_I2CP,            TEGRA_MUX_I2C1,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_IRRX,            TEGRA_MUX_UARTA,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL}, //cpcap whisperer
	{TEGRA_PINGROUP_IRTX,            TEGRA_MUX_UARTA,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL}, //cpcap whisperer
	{TEGRA_PINGROUP_KBCA,            TEGRA_MUX_KBC,			TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_KBCB,            TEGRA_MUX_KBC,			TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL}, //hall sensor (not Olympus)
	{TEGRA_PINGROUP_KBCC,            TEGRA_MUX_KBC,			TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_KBCD,            TEGRA_MUX_KBC,			TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_KBCE,            TEGRA_MUX_KBC,			TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_KBCF,            TEGRA_MUX_KBC,			TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LCSN,            TEGRA_MUX_DISPLAYA,		TEGRA_PUPD_PULL_UP,   TEGRA_TRI_TRISTATE},
	{TEGRA_PINGROUP_LD0,             TEGRA_MUX_DISPLAYA,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},  //backlight
	{TEGRA_PINGROUP_LD1,             TEGRA_MUX_DISPLAYA,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD10,            TEGRA_MUX_RSVD4,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD11,            TEGRA_MUX_RSVD4,		TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD12,            TEGRA_MUX_DISPLAYA,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},  //touchscreen
	{TEGRA_PINGROUP_LD13,            TEGRA_MUX_DISPLAYA,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},  //touchscreen
	{TEGRA_PINGROUP_LD14,            TEGRA_MUX_DISPLAYA,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD15,            TEGRA_MUX_DISPLAYA,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},  //backlight
	{TEGRA_PINGROUP_LD16,            TEGRA_MUX_DISPLAYA,		TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD17,            TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD2,             TEGRA_MUX_RSVD4,         TEGRA_PUPD_PULL_UP,	TEGRA_TRI_NORMAL}, //akm8975 irq
	{TEGRA_PINGROUP_LD3,             TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD4,             TEGRA_MUX_RSVD4,         TEGRA_PUPD_PULL_DOWN,	TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD5,             TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD6,             TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD7,             TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD8,             TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LD9,             TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LDC,             TEGRA_MUX_RSVD4,         TEGRA_PUPD_PULL_UP,   TEGRA_TRI_TRISTATE},
	{TEGRA_PINGROUP_LDI,             TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LHP0,            TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},  //fingerprint
	{TEGRA_PINGROUP_LHP1,            TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_TRISTATE}, //console on jack (for dev atrix)
	{TEGRA_PINGROUP_LHP2,            TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LHS,             TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LM0,             TEGRA_MUX_SPI3,          TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},  //cpcap/modem?
	{TEGRA_PINGROUP_LM1,             TEGRA_MUX_RSVD4,         TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LPP,             TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LPW0,            TEGRA_MUX_SPI3,	TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL}, //cpcap/modem?
	{TEGRA_PINGROUP_LPW1,            TEGRA_MUX_DISPLAYA,	TEGRA_PUPD_PULL_UP,   TEGRA_TRI_TRISTATE},
	{TEGRA_PINGROUP_LPW2,            TEGRA_MUX_SPI3,	TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL}, //cpcap/modem?
	{TEGRA_PINGROUP_LSC0,            TEGRA_MUX_RSVD4,	TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LSC1,            TEGRA_MUX_SPI3,	TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL}, //cpcap/modem?
	{TEGRA_PINGROUP_LSCK,            TEGRA_MUX_SAFE,	TEGRA_PUPD_PULL_UP,   TEGRA_TRI_TRISTATE},
	{TEGRA_PINGROUP_LSDA,            TEGRA_MUX_SAFE,	TEGRA_PUPD_PULL_UP,   TEGRA_TRI_TRISTATE},
	{TEGRA_PINGROUP_LSDI,            TEGRA_MUX_SAFE,      	TEGRA_PUPD_PULL_UP,   TEGRA_TRI_TRISTATE},
	{TEGRA_PINGROUP_LSPI,            TEGRA_MUX_DISPLAYA,      TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LVP0,            TEGRA_MUX_RSVD4,         TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LVP1,            TEGRA_MUX_RSVD4,         TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_LVS,             TEGRA_MUX_RSVD4,         TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_OWC,             TEGRA_MUX_OWR,           TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},  //1-wire (battery)
	{TEGRA_PINGROUP_PMC,             TEGRA_MUX_PWR_ON,        TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_PTA,             TEGRA_MUX_SAFE,         TEGRA_PUPD_NORMAL,    TEGRA_TRI_TRISTATE},
	{TEGRA_PINGROUP_RM,              TEGRA_MUX_I2C1,           TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},  //touchscreen
	{TEGRA_PINGROUP_SDB,             TEGRA_MUX_SDIO3,         TEGRA_PUPD_PULL_UP,	TEGRA_TRI_NORMAL},  //sdcard
	{TEGRA_PINGROUP_SDC,             TEGRA_MUX_SDIO3,         TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},  //sdcard
	{TEGRA_PINGROUP_SDD,             TEGRA_MUX_SDIO3,         TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},  //sdcard
	{TEGRA_PINGROUP_SDIO1,           TEGRA_MUX_SDIO1,         TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},  //wifi
	{TEGRA_PINGROUP_SLXA,            TEGRA_MUX_SAFE,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_SLXC,            TEGRA_MUX_SPDIF,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL}, //audio codec
	{TEGRA_PINGROUP_SLXD,            TEGRA_MUX_SPDIF,         TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL}, //audio codec
	{TEGRA_PINGROUP_SLXK,            TEGRA_MUX_SAFE,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_SPDI,            TEGRA_MUX_RSVD2,         TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_SPDO,            TEGRA_MUX_RSVD2,         TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},  //akm8975 reset
	{TEGRA_PINGROUP_SPIA,            TEGRA_MUX_SPI2,          TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},  //spi2 cpcap
	{TEGRA_PINGROUP_SPIB,            TEGRA_MUX_SPI2,          TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},  //spi2 cpcap
	{TEGRA_PINGROUP_SPIC,            TEGRA_MUX_SPI2,          TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},  //spi2 cpcap
	{TEGRA_PINGROUP_SPID,            TEGRA_MUX_SPI1,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},  //spi1 (fp/modem)
	{TEGRA_PINGROUP_SPIE,            TEGRA_MUX_SPI1,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},  //spi1 (fp/modem)
	{TEGRA_PINGROUP_SPIF,            TEGRA_MUX_SPI1,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},  //spi1 (fp/modem)
	{TEGRA_PINGROUP_SPIG,            TEGRA_MUX_SPI2,          TEGRA_PUPD_PULL_DOWN, TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_SPIH,            TEGRA_MUX_SPI2,          TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},  //hall sensor (not Olympus)
	{TEGRA_PINGROUP_UAA,             TEGRA_MUX_SAFE,       TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},  // mipi_hs
	{TEGRA_PINGROUP_UAB,             TEGRA_MUX_SAFE,       TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},  // mipi_hs
	{TEGRA_PINGROUP_UAC,             TEGRA_MUX_OWR,         TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL}, // modem/kxtf9
	{TEGRA_PINGROUP_UAD,             TEGRA_MUX_IRDA,          TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL}, //jtag serial
	{TEGRA_PINGROUP_UCA,             TEGRA_MUX_UARTC,         TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL}, //bluetooth
	{TEGRA_PINGROUP_UCB,             TEGRA_MUX_UARTC,         TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL}, //bluetooth
	{TEGRA_PINGROUP_UDA,             TEGRA_MUX_RSVD2,         TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_CK32,            TEGRA_MUX_NONE,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_DDRC,            TEGRA_MUX_NONE,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_PMCA,            TEGRA_MUX_NONE,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_PMCB,            TEGRA_MUX_NONE,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_PMCC,            TEGRA_MUX_NONE,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_PMCD,            TEGRA_MUX_NONE,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_PMCE,            TEGRA_MUX_NONE,          TEGRA_PUPD_PULL_UP,   TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_XM2C,            TEGRA_MUX_NONE,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
	{TEGRA_PINGROUP_XM2D,            TEGRA_MUX_NONE,          TEGRA_PUPD_NORMAL,    TEGRA_TRI_NORMAL},
};

static struct tegra_gpio_table gpio_table[] = {
};


void __init olympus_pinmux_init(void)
{
	tegra_pinmux_config_table(olympus_pinmux, ARRAY_SIZE(olympus_pinmux));
	tegra_drive_pinmux_config_table(olympus_drive_pinmux,
		ARRAY_SIZE(olympus_drive_pinmux));

	tegra_gpio_config(gpio_table, ARRAY_SIZE(gpio_table));
}
