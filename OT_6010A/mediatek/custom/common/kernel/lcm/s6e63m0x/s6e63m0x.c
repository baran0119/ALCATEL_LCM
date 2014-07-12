/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
#define LCM_DEBUG		1

#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
    #ifdef LCD_DEBUG
        #define LCM_DEBUG(format, ...)   printf("uboot s6e63m0x" format "\n", ## __VA_ARGS__)
    #else
        #define LCM_DEBUG(format, ...)
    #endif

#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt6577_gpio.h>
    #ifdef LCD_DEBUG
        #define LCM_DEBUG(format, ...)   printf("uboot s6e63m0x" format "\n", ## __VA_ARGS__)
    #else
        #define LCM_DEBUG(format, ...)
    #endif
#else
    #include <mach/mt_gpio.h>
    #include <linux/string.h>
    #ifdef LCD_DEBUG
        #define LCM_DEBUG(format, ...)   printk("kernel s6e63m0x" format "\n", ## __VA_ARGS__)
    #else
        #define LCM_DEBUG(format, ...)
    #endif
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
#define LSCE_GPIO_PIN   (GPIO47)   		//
#define LSCK_GPIO_PIN   (GPIO51)               //clk
#define LSDA_GPIO_PIN   (GPIO52)  		//DIO
#define LSDI_GPIO_PIN    (GPIO52)

#define FRAME_WIDTH  (480)
#define FRAME_HEIGHT (800)
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define UDELAY(n)	(lcm_util.udelay(n))
#define MDELAY(n)	(lcm_util.mdelay(n))

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define SET_GPIO_OUT(n, v)	(lcm_util.set_gpio_out((n), (v)))


#define SET_LSCE_LOW   SET_GPIO_OUT(LSCE_GPIO_PIN, 0)
#define SET_LSCE_HIGH  SET_GPIO_OUT(LSCE_GPIO_PIN, 1)
#define SET_LSCK_LOW   SET_GPIO_OUT(LSCK_GPIO_PIN, 0)
#define SET_LSCK_HIGH  SET_GPIO_OUT(LSCK_GPIO_PIN, 1)
#define SET_LSDA_LOW   SET_GPIO_OUT(LSDA_GPIO_PIN, 0)
#define SET_LSDA_HIGH  SET_GPIO_OUT(LSDA_GPIO_PIN, 1)
#define GET_SDI     mt_get_gpio_in(LSDI_GPIO_PIN)

#define SET_SWIRE_LOW   SET_GPIO_OUT(SWIRE_GPIO_PIN, 0)
#define SET_SWIRE_HIGH  SET_GPIO_OUT(SWIRE_GPIO_PIN, 1)

#define    ARRAY_OF(x)      ((int)(sizeof(x)/sizeof(x[0])))

static int acl_enable = 0;

//gamma backlight setting
#define GAMMA_DATA_COUNT 22
#define MAX_GAMMA_LEVEL  21
static const unsigned int s6e63m0_300[] = {
	0x18, 0x08, 0x24, 0x70, 0x6e, 0x4e, 0xbc,
	0xc0, 0xaf, 0xb3, 0xb8, 0xa5, 0xc5, 0xc7,
	0xbb, 0x00, 0xb9, 0x00, 0xb8, 0x00, 0xfc
};

static const unsigned int s6e63m0_290[] = {
	0x18, 0x08, 0x24, 0x71, 0x70, 0x50, 0xbd,
	0xc1, 0xb0, 0xb2, 0xb8, 0xa4, 0xc6, 0xc7,
	0xbb, 0x00, 0xb6, 0x00, 0xb6, 0x00, 0xfa
};

static const unsigned int s6e63m0_280[] = {
	0x18, 0x08, 0x24, 0x6e, 0x6c, 0x4d, 0xbe,
	0xc3, 0xb1, 0xb3, 0xb8, 0xa5, 0xc6, 0xc8,
	0xbb, 0x00, 0xb4, 0x00, 0xb3, 0x00, 0xf7
};

static const unsigned int s6e63m0_270[] = {
	0x18, 0x08, 0x24, 0x71, 0x6c, 0x50, 0xbd,
	0xc3, 0xb0, 0xb4, 0xb8, 0xa6, 0xc6, 0xc9,
	0xbb, 0x00, 0xb2, 0x00, 0xb1, 0x00, 0xf4
};

static const unsigned int s6e63m0_260[] = {
	0x18, 0x08, 0x24, 0x74, 0x6e, 0x54, 0xbd,
	0xc2, 0xb0, 0xb5, 0xba, 0xa7, 0xc5, 0xc9,
	0xba, 0x00, 0xb0, 0x00, 0xae, 0x00, 0xf1
};

static const unsigned int s6e63m0_250[] = {
	0x18, 0x08, 0x24, 0x74, 0x6d, 0x54, 0xbf,
	0xc3, 0xb2, 0xb4, 0xba, 0xa7, 0xc6, 0xca,
	0xba, 0x00, 0xad, 0x00, 0xab, 0x00, 0xed
};

static const unsigned int s6e63m0_240[] = {
	0x18, 0x08, 0x24, 0x76, 0x6f, 0x56, 0xc0,
	0xc3, 0xb2, 0xb5, 0xba, 0xa8, 0xc6, 0xcb,
	0xbb, 0x00, 0xaa, 0x00, 0xab, 0x00, 0xe9
};

static const unsigned int s6e63m0_230[] = {
	0x18, 0x08, 0x24, 0x75, 0x6f, 0x56, 0xbf,
	0xc3, 0xb2, 0xb6, 0xbb, 0xa8, 0xc7, 0xcb,
	0xbc, 0x00, 0xa8, 0x00, 0xa6, 0x00, 0xe6
};

static const unsigned int s6e63m0_220[] = {
	0x18, 0x08, 0x24, 0x78, 0x6f, 0x58, 0xbf,
	0xc4, 0xb3, 0xb5, 0xbb, 0xa9, 0xc8, 0xcc,
	0xbc, 0x00, 0xa6, 0x00, 0xa3, 0x00, 0xe2
};

static const unsigned int s6e63m0_210[] = {
	0x18, 0x08, 0x24, 0x79, 0x6d, 0x57, 0xc0,
	0xc4, 0xb4, 0xb7, 0xbd, 0xaa, 0xc8, 0xcc,
	0xbd, 0x00, 0xa2, 0x00, 0xa0, 0x00, 0xdd
};

static const unsigned int s6e63m0_200[] = {
	0x18, 0x08, 0x24, 0x79, 0x6d, 0x58, 0xc1,
	0xc4, 0xb4, 0xb6, 0xbd, 0xaa, 0xca, 0xcd,
	0xbe, 0x00, 0x9f, 0x00, 0x9d, 0x00, 0xd9
};
static const unsigned int s6e63m0_190[] = {
	0x18, 0x08, 0x24, 0x7a, 0x6d, 0x59, 0xc1,
	0xc5, 0xb4, 0xb8, 0xbd, 0xac, 0xc9, 0xce,
	0xbe, 0x00, 0x9d, 0x00, 0x9a, 0x00, 0xd5
};

static const unsigned int s6e63m0_180[] = {
	0x18, 0x08, 0x24, 0x7b, 0x6d, 0x5b, 0xc0,
	0xc5, 0xb3, 0xba, 0xbe, 0xad, 0xca, 0xce,
	0xbf, 0x00, 0x99, 0x00, 0x97, 0x00, 0xd0
};

static const unsigned int s6e63m0_170[] = {
	0x18, 0x08, 0x24, 0x7c, 0x6d, 0x5c, 0xc0,
	0xc6, 0xb4, 0xbb, 0xbe, 0xad, 0xca, 0xcf,
	0xc0, 0x00, 0x96, 0x00, 0x94, 0x00, 0xcc
};

static const unsigned int s6e63m0_160[] = {
	0x18, 0x08, 0x24, 0x7f, 0x6e, 0x5f, 0xc0,
	0xc6, 0xb5, 0xba, 0xbf, 0xad, 0xcb, 0xcf,
	0xc0, 0x00, 0x94, 0x00, 0x91, 0x00, 0xc8
};

static const unsigned int s6e63m0_150[] = {
	0x18, 0x08, 0x24, 0x80, 0x6e, 0x5f, 0xc1,
	0xc6, 0xb6, 0xbc, 0xc0, 0xae, 0xcc, 0xd0,
	0xc2, 0x00, 0x8f, 0x00, 0x8d, 0x00, 0xc2
};

static const unsigned int s6e63m0_140[] = {
	0x18, 0x08, 0x24, 0x80, 0x6c, 0x5f, 0xc1,
	0xc6, 0xb7, 0xbc, 0xc1, 0xae, 0xcd, 0xd0,
	0xc2, 0x00, 0x8c, 0x00, 0x8a, 0x00, 0xbe
};

static const unsigned int s6e63m0_130[] = {
	0x18, 0x08, 0x24, 0x8c, 0x6c, 0x60, 0xc3,
	0xc7, 0xb9, 0xbc, 0xc1, 0xaf, 0xce, 0xd2,
	0xc3, 0x00, 0x88, 0x00, 0x86, 0x00, 0xb8
};

static const unsigned int s6e63m0_120[] = {
	0x18, 0x08, 0x24, 0x82, 0x6b, 0x5e, 0xc4,
	0xc8, 0xb9, 0xbd, 0xc2, 0xb1, 0xce, 0xd2,
	0xc4, 0x00, 0x85, 0x00, 0x82, 0x00, 0xb3
};

static const unsigned int s6e63m0_110[] = {
	0x18, 0x08, 0x24, 0x86, 0x6a, 0x60, 0xc5,
	0xc7, 0xba, 0xbd, 0xc3, 0xb2, 0xd0, 0xd4,
	0xc5, 0x00, 0x80, 0x00, 0x7e, 0x00, 0xad
};

static const unsigned int s6e63m0_100[] = {
	0x18, 0x08, 0x24, 0x86, 0x69, 0x60, 0xc6,
	0xc8, 0xba, 0xbf, 0xc4, 0xb4, 0xd0, 0xd4,
	0xc6, 0x00, 0x7c, 0x00, 0x7a, 0x00, 0xa7
};

unsigned int *gamma_backlight[MAX_GAMMA_LEVEL] =
{
	(unsigned int *)&s6e63m0_100,
	(unsigned int *)&s6e63m0_110,
	(unsigned int *)&s6e63m0_120,
	(unsigned int *)&s6e63m0_130,
	(unsigned int *)&s6e63m0_140,
	(unsigned int *)&s6e63m0_150,
	(unsigned int *)&s6e63m0_160,
	(unsigned int *)&s6e63m0_170,
	(unsigned int *)&s6e63m0_180,
	(unsigned int *)&s6e63m0_190,
	(unsigned int *)&s6e63m0_200,
	(unsigned int *)&s6e63m0_210,
	(unsigned int *)&s6e63m0_220,
	(unsigned int *)&s6e63m0_230,
	(unsigned int *)&s6e63m0_240,
	(unsigned int *)&s6e63m0_250,
	(unsigned int *)&s6e63m0_260,
	(unsigned int *)&s6e63m0_270,
	(unsigned int *)&s6e63m0_280,
	(unsigned int *)&s6e63m0_290,
	(unsigned int *)&s6e63m0_300,
};
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

/*
*It used index to indentify command or data tyep.1:data.0:cmd.
*/
static  void spi_send(unsigned int index,  unsigned int data)
{
    unsigned int i;

    SET_LSCE_LOW;
    UDELAY(10);

    SET_LSCK_LOW;
    UDELAY(10);
    SET_GPIO_OUT(LSDA_GPIO_PIN, index);
    UDELAY(10);
    SET_LSCK_HIGH;

    for (i = 0; i < 8; i++)
    {
        if( data & 0x80 ) {
            SET_LSDA_HIGH;
        } else {
            SET_LSDA_LOW;
        }
        data <<= 1;
        UDELAY(10);
        SET_LSCK_LOW;
        UDELAY(10);
	SET_LSCK_HIGH;
        UDELAY(10);
    }

    SET_LSCE_HIGH;
}

static __inline void Write_com(unsigned int cmd)
{
    spi_send(0, cmd);
}

static __inline void Write_register(unsigned int data)
{
    spi_send(1, data);
}

static void init_lcm_registers(void)
{
	Write_com(0xf0);
	Write_register(0x5a);
	Write_register(0x5a);

	Write_com(0xf8);
	Write_register(0x01);
	Write_register(0x27);
	Write_register(0x27);
	Write_register(0x07);
	Write_register(0x07);
	Write_register(0x54);
	Write_register(0x9f);
	Write_register(0x63);
	Write_register(0x86);
	Write_register(0x1a);
	Write_register(0x33);
	Write_register(0x0d);
	Write_register(0x00);
	Write_register(0x00);

	Write_com(0xf2);
	Write_register(0x02);
	Write_register(0x03);
	Write_register(0x1c);
	Write_register(0x10);
	Write_register(0x10);

	Write_com(0xf7);
	Write_register(0x00);
	Write_register(0x00);
	Write_register(0x00);

	Write_com(0xfa);
	Write_register(0x02);
	Write_register(0x18);
	Write_register(0x08);
	Write_register(0x24);
	Write_register(0x70);
	Write_register(0x6e);
	Write_register(0x4e);
	Write_register(0xbc);
	Write_register(0xc0);
	Write_register(0xaf);
	Write_register(0xb3);
	Write_register(0xb8);
	Write_register(0xa5);
	Write_register(0xc5);
	Write_register(0xc7);
	Write_register(0xbb);
	Write_register(0x00);
	Write_register(0xb9);
	Write_register(0x00);
	Write_register(0xb8);
	Write_register(0x00);
	Write_register(0xfc);

	Write_com(0xfa);
	Write_register(0x03);

	Write_com(0xf6);
	Write_register(0x00);
	Write_register(0x8e);
	Write_register(0x07);

	Write_com(0xb3);
	Write_register(0x6c);

	Write_com(0xb5);
	Write_register(0x2c);
	Write_register(0x12);
	Write_register(0x0c);
	Write_register(0x0a);
	Write_register(0x10);
	Write_register(0x0e);
	Write_register(0x17);
	Write_register(0x13);
	Write_register(0x1f);
	Write_register(0x1a);
	Write_register(0x2a);
	Write_register(0x24);
	Write_register(0x1f);
	Write_register(0x1b);
	Write_register(0x1a);
	Write_register(0x17);
	Write_register(0x2b);
	Write_register(0x26);
	Write_register(0x22);
	Write_register(0x20);
	Write_register(0x3a);
	Write_register(0x34);
	Write_register(0x30);
	Write_register(0x2c);
	Write_register(0x29);
	Write_register(0x26);
	Write_register(0x25);
	Write_register(0x23);
	Write_register(0x21);
	Write_register(0x20);
	Write_register(0x1e);
	Write_register(0x1e);

	Write_com(0xb6);
	Write_register(0x00);
	Write_register(0x00);
	Write_register(0x11);
	Write_register(0x22);
	Write_register(0x33);
	Write_register(0x44);
	Write_register(0x44);
	Write_register(0x44);
	Write_register(0x55);
	Write_register(0x55);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);

	Write_com(0xb7);
	Write_register(0x2c);
	Write_register(0x12);
	Write_register(0x0c);
	Write_register(0x0a);
	Write_register(0x10);
	Write_register(0x0e);
	Write_register(0x17);
	Write_register(0x13);
	Write_register(0x1f);
	Write_register(0x1a);
	Write_register(0x2a);
	Write_register(0x24);
	Write_register(0x1f);
	Write_register(0x1b);
	Write_register(0x1a);
	Write_register(0x17);
	Write_register(0x2b);
	Write_register(0x26);
	Write_register(0x22);
	Write_register(0x20);
	Write_register(0x3a);
	Write_register(0x34);
	Write_register(0x30);
	Write_register(0x2c);
	Write_register(0x29);
	Write_register(0x26);
	Write_register(0x25);
	Write_register(0x23);
	Write_register(0x21);
	Write_register(0x20);
	Write_register(0x1e);
	Write_register(0x1e);

	Write_com(0xb8);
	Write_register(0x00);
	Write_register(0x00);
	Write_register(0x11);
	Write_register(0x22);
	Write_register(0x33);
	Write_register(0x44);
	Write_register(0x44);
	Write_register(0x44);
	Write_register(0x55);
	Write_register(0x55);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);

	Write_com(0xb9);
	Write_register(0x2c);
	Write_register(0x12);
	Write_register(0x0c);
	Write_register(0x0a);
	Write_register(0x10);
	Write_register(0x0e);
	Write_register(0x17);
	Write_register(0x13);
	Write_register(0x1f);
	Write_register(0x1a);
	Write_register(0x2a);
	Write_register(0x24);
	Write_register(0x1f);
	Write_register(0x1b);
	Write_register(0x1a);
	Write_register(0x17);
	Write_register(0x2b);
	Write_register(0x26);
	Write_register(0x22);
	Write_register(0x20);
	Write_register(0x3a);
	Write_register(0x34);
	Write_register(0x30);
	Write_register(0x2c);
	Write_register(0x29);
	Write_register(0x26);
	Write_register(0x25);
	Write_register(0x23);
	Write_register(0x21);
	Write_register(0x20);
	Write_register(0x1e);
	Write_register(0x1e);

	Write_com(0xba);
	Write_register(0x00);
	Write_register(0x00);
	Write_register(0x11);
	Write_register(0x22);
	Write_register(0x33);
	Write_register(0x44);
	Write_register(0x44);
	Write_register(0x44);
	Write_register(0x55);
	Write_register(0x55);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);
	Write_register(0x66);

	Write_com(0xc1);
	Write_register(0x4d);
	Write_register(0x96);
	Write_register(0x1d);
	Write_register(0x00);
	Write_register(0x00);
	Write_register(0x01);
	Write_register(0xdf);
	Write_register(0x00);
	Write_register(0x00);
	Write_register(0x03);
	Write_register(0x1f);
	Write_register(0x00);
	Write_register(0x00);
	Write_register(0x00);
	Write_register(0x00);
	Write_register(0x00);
	Write_register(0x01);
	Write_register(0x08);
	Write_register(0x0f);
	Write_register(0x16);
	Write_register(0x1d);
	Write_register(0x24);
	Write_register(0x2a);
	Write_register(0x31);
	Write_register(0x38);
	Write_register(0x3f);
	Write_register(0x46);

	if(acl_enable == 1) {
		Write_com(0xc0);
		Write_register(0x01);
	} else {
		Write_com(0xc0);
		Write_register(0x00);
	}

	Write_com(0xb2);
	Write_register(0x10);
	Write_register(0x10);
	Write_register(0x0b);
	Write_register(0x05);

	Write_com(0xb1);
	Write_register(0x0b);

	Write_com(0x11);

}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void config_gpio(void)
{
    	lcm_util.set_gpio_mode(LSCE_GPIO_PIN, GPIO_MODE_00);
    	lcm_util.set_gpio_mode(LSCK_GPIO_PIN, GPIO_MODE_00);
    	lcm_util.set_gpio_mode(LSDA_GPIO_PIN, GPIO_MODE_00);

    	lcm_util.set_gpio_dir(LSCE_GPIO_PIN, GPIO_DIR_OUT);
    	lcm_util.set_gpio_dir(LSCK_GPIO_PIN, GPIO_DIR_OUT);
    	lcm_util.set_gpio_dir(LSDA_GPIO_PIN, GPIO_DIR_OUT);

	lcm_util.set_gpio_pull_enable(LSCE_GPIO_PIN, GPIO_PULL_DISABLE);
	lcm_util.set_gpio_pull_enable(LSCK_GPIO_PIN, GPIO_PULL_DISABLE);
	lcm_util.set_gpio_pull_enable(LSDA_GPIO_PIN, GPIO_PULL_DISABLE);

	SET_LSCE_HIGH;
	SET_LSCK_HIGH;
	SET_LSDA_HIGH;

	MDELAY(100);

}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DPI;
	params->ctrl   = LCM_CTRL_GPIO;
	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	params->dpi.mipi_pll_clk_ref  = 0;
	params->dpi.mipi_pll_clk_div1 = 42;
	params->dpi.mipi_pll_clk_div2 = 10;
	params->dpi.dpi_clk_div       = 2;
	params->dpi.dpi_clk_duty      = 1;

	params->dpi.clk_pol           = LCM_POLARITY_FALLING;
	params->dpi.de_pol            = LCM_POLARITY_FALLING;
	params->dpi.vsync_pol         = LCM_POLARITY_FALLING;
	params->dpi.hsync_pol         = LCM_POLARITY_FALLING;

	params->dpi.hsync_pulse_width = 2;
	params->dpi.hsync_back_porch  = 14;   //16
	params->dpi.hsync_front_porch = 16;
	params->dpi.vsync_pulse_width = 2;
	params->dpi.vsync_back_porch  = 1;   //3
	params->dpi.vsync_front_porch = 28;

	params->dpi.format            = LCM_DPI_FORMAT_RGB888;
	params->dpi.rgb_order         = LCM_COLOR_ORDER_RGB;
	params->dpi.is_serial_output  = 0;

	params->dpi.intermediat_buffer_num = 2;
	params->dpi.io_driving_current = LCM_DRIVING_CURRENT_4MA;

}

static void lcm_init(void)
{

	 config_gpio();

	 SET_RESET_PIN(0);
	 MDELAY(20);
	 SET_RESET_PIN(1);
	 MDELAY(50);

	 init_lcm_registers();

}

static void lcm_suspend(void)
{
	Write_com(0x28);
	Write_com(0x10);

	MDELAY(120);

	return;
}

static void lcm_resume(void)
{
	 SET_RESET_PIN(0);
	 MDELAY(20);
	 SET_RESET_PIN(1);
	 MDELAY(50);

	 init_lcm_registers();
	 return;
}

static unsigned int lcm_compare_id(void)
{
	return 1;
}
static void lcm_setbacklight (unsigned int level)
{
	int i;
	static int old_level = -1;

	LCM_DEBUG("%s backlight level: %d\n", __func__, level); // leibo


	if(level < 0 || level > 255)
		return -1;

	level = level / 12;  /* 0 <= level <= 20*/
	if(level > (ARRAY_OF(gamma_backlight)-1 ))
		level = ARRAY_OF(gamma_backlight) -1;

	if (old_level == level){ /* no need update */
		return;
	}

	if (level == 0) {
		Write_com(0x28);
	} else {
		/* disable gamma update. */
		Write_com(0xfa);
		Write_register(0x02);

		/* write gamma data setting. */
		for (i = 0 ; i < GAMMA_DATA_COUNT; i++) {
			Write_register(gamma_backlight[level][i]);
		}

		/* update gamma table. */
		Write_com(0xfa);
		Write_register(0x03);

		if(!old_level) {
			/*when send 0x11 since now,process scheduling speed time is 250ms,so it's not nedd MDELAY anymore*/
			//MDELAY(120);
			Write_com(0x29);
		}
	}

	old_level = level;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------

LCM_DRIVER s6e63m0x_lcm_drv =
{
	.name		  = "s6e63m0x",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.set_backlight  = lcm_setbacklight,
};

/*jrd add for LCD ACL function,PR350502*/
int set_acl_function(unsigned int enable){
	if(enable == 1) {
		acl_enable = 1;
		Write_com(0xc0);
		Write_register(0x01);
	} else {
		acl_enable= 0;
		Write_com(0xc0);
		Write_register(0x00);
	}
}

int get_acl_status(void) {
	return acl_enable;
}
/*end*/

