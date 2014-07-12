#ifdef BUILD_LK
#include <string.h>
#else
#include <linux/string.h>
#endif

#ifdef BUILD_UBOOT
#include <asm/arch/mt6577_gpio.h>

#elif defined BUILD_LK
#include <platform/mt_gpio.h>
#else
#include <linux/kernel.h>
#include <mach/mt_gpio.h>
#endif
#include "lcm_drv.h"

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(800)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0x00   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[120];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
	
	/*
	Note :

	Data ID will depends on the following rule.
	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/
	/*
	{0xFF,	4,	{0xAA, 0x55, 0xA5, 0x80}},
	{0xF7,	15,	{0x63, 0x40, 0x00, 0x00, 0x00, 0x01, 
				 0xC4, 0xA2, 0x00, 0x02,0x64,0x54,0x48,
				 0x00, 0xD0}},
				 
	{0xFF,	4,	{0xAA, 0x55, 0xA5, 0x00}},
	*/
	{0xF0,	5,	{0x55, 0xAA, 0x52, 0x08, 0x01}}, //Page 1
	{0xBC,	3,	{0x00, 0x58, 0x1A}}, //#VGMP/VGSP 4.5V/0V
	{0xBD,	3,	{0x00, 0x58, 0x1A}}, //#VGMN/VGSN -4.5V/0V
	{0xBE,	2,	{0x00, 0x48}}, //#VCOM, 0x64

	{0xD1,	52,	{0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x16,0x00,
				 0x29,0x00,0x54,0x00,0x85,0x00,0xD6,0x01,0x13,
				 0x01,0x6A,0x01,0xA8,0x01,0xFF,0x02,0x3F,0x02,
				 0x41,0x02,0x7B,0x02,0xB6,0x02,0xD6,0x02,0xFA,
				 0x03,0x12,0x03,0x30,0x03,0x43,0x03,0x5B,0x03,
				 0x6C,0x03,0x85,0x03,0xB9,0x03,0xFD}},

	{0xD2,	52,	{0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x16,0x00,
				 0x29,0x00,0x54,0x00,0x85,0x00,0xD6,0x01,0x13,
				 0x01,0x6A,0x01,0xA8,0x01,0xFF,0x02,0x3F,0x02,
				 0x41,0x02,0x7B,0x02,0xB6,0x02,0xD6,0x02,0xFA,
				 0x03,0x12,0x03,0x30,0x03,0x43,0x03,0x5B,0x03,
				 0x6C,0x03,0x85,0x03,0xB9,0x03,0xFD}},

	{0xD3,	52,	{0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x16,0x00,
				 0x29,0x00,0x54,0x00,0x85,0x00,0xD6,0x01,0x13,
				 0x01,0x6A,0x01,0xA8,0x01,0xFF,0x02,0x3F,0x02,
				 0x41,0x02,0x7B,0x02,0xB6,0x02,0xD6,0x02,0xFA,
				 0x03,0x12,0x03,0x30,0x03,0x43,0x03,0x5B,0x03,
				 0x6C,0x03,0x85,0x03,0xB9,0x03,0xFD}},

	{0xD4,	52,	{0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x16,0x00,
				 0x29,0x00,0x54,0x00,0x85,0x00,0xD6,0x01,0x13,
				 0x01,0x6A,0x01,0xA8,0x01,0xFF,0x02,0x3F,0x02,
				 0x41,0x02,0x7B,0x02,0xB6,0x02,0xD6,0x02,0xFA,
				 0x03,0x12,0x03,0x30,0x03,0x43,0x03,0x5B,0x03,
				 0x6C,0x03,0x85,0x03,0xB9,0x03,0xFD}},

	{0xD5,	52,	{0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x16,0x00,
				 0x29,0x00,0x54,0x00,0x85,0x00,0xD6,0x01,0x13,
				 0x01,0x6A,0x01,0xA8,0x01,0xFF,0x02,0x3F,0x02,
				 0x41,0x02,0x7B,0x02,0xB6,0x02,0xD6,0x02,0xFA,
				 0x03,0x12,0x03,0x30,0x03,0x43,0x03,0x5B,0x03,
				 0x6C,0x03,0x85,0x03,0xB9,0x03,0xFD}},

	{0xD6,	52,	{0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x16,0x00,
				 0x29,0x00,0x54,0x00,0x85,0x00,0xD6,0x01,0x13,
				 0x01,0x6A,0x01,0xA8,0x01,0xFF,0x02,0x3F,0x02,
				 0x41,0x02,0x7B,0x02,0xB6,0x02,0xD6,0x02,0xFA,
				 0x03,0x12,0x03,0x30,0x03,0x43,0x03,0x5B,0x03,
				 0x6C,0x03,0x85,0x03,0xB9,0x03,0xFD}},

	{0xB0,	3,	{0x0D, 0x0D, 0x0D}}, //#AVDD Set AVDD 5.2V
	{0xB6,	3,	{0x44, 0x44, 0x44}}, //#AVDD ratio
	//{0xB8,	3,	{0x26, 0x26, 0x26}}, //#VCL ratio

	{0xB1,	3,	{0x0D, 0x0D, 0x0D}}, //#AVEE  -5.2V
	{0xB7,	3,	{0x35, 0x35, 0x35}}, //#AVEE ratio
	{0xBA,	3,	{0x16, 0x16, 0x16}}, //#VGL_REG -10V
	{0xB9,	3,	{0x37, 0x37, 0x37}}, //#VGH ratio

	{0xF0,	5,	{0x55, 0xAA, 0x52, 0x08, 0x00}},//Page 0

	{0xB1,	1,	{0xFC}},//#Display control
	{0xB4,	1,	{0x10}},
	{0xB6,	1,	{0x04}},//#Source hold time
	{0xB7,	2,	{0x00,0x00}}, //Set Gate EQ
	{0xB8,	4,	{0x01, 0x07, 0x07, 0x07}},//Set Source EQ
	{0xBC,	3,	{0x04, 0x04, 0x04}},//Inversion Control
	{0xBD,	5,	{0x01, 0x84, 0x07, 0x31, 0x00}},
	{0xBE,	5,	{0x01, 0x84, 0x07, 0x31, 0x00}},
	{0xBF,	5,	{0x01, 0x84, 0x07, 0x31, 0x00}},

	{0x35,	1,	{0x00}},//TE ON

	{0x3A,	1,	{0x77}},//16.7M color

	{0xF0,	5,	{0x55, 0xAA, 0x52, 0x08, 0x00}},//Page 0
	{0xC7,	1,	{0x02}},
	{0xC9,	1,	{0x11}},//#Timing control 4H w/ 4-delay
	{0xCA,	12,	{0x01,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0x08,0x08,0x00,0x00}},
	{0xB1,	2,	{0xfc,0x00}},
	//only for mackup
	{0x11,	1,	{0x11}},//Sleep Out
	{REGFLAG_DELAY, 150, {}},
	{0x29,	1,	{0x29}},//Display On
	{REGFLAG_DELAY, 30, {}},

	//{0x21,	1,	{0x00}},//Start GRAM write  Enter_inversion_mode

	{0xF0,	5,	{0x55, 0xAA, 0x52, 0x08, 0x01}}, //Page 1
	{0xFF,	4,	{0xAA, 0x55, 0xA5, 0x80}},
	{0xF7,	15,	{0x63, 0x40, 0x00, 0x00, 0x00, 0x01,
				 0xC4, 0xA2, 0x00, 0x02,0x64,0x54,0x48,
				 0x00, 0xD0}},	
	{0xFF,	4,	{0xAA, 0x55, 0xA5, 0x00}},

	{0xF0,	5,	{0x55, 0xAA, 0x52, 0x08, 0x00}},//Page 0

	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.
	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 0, {0x00}},
    {REGFLAG_DELAY, 100, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},
	{REGFLAG_DELAY, 30, {}},
    // Sleep Mode On
	{0x10, 0, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
				//MDELAY(10);//soso add or it will fail to send register
       	}
    }
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
	
	params->type   = LCM_TYPE_DSI;
	
	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	
	
	params->dsi.mode   = SYNC_EVENT_VDO_MODE;
	params->dsi.lcm_ext_te_monitor = TRUE;
	
	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq	= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding 	= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format		= LCM_DSI_FORMAT_RGB888;
	
	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;
	
	// Video mode setting		
	params->dsi.intermediat_buffer_num = 2;
	
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	
	params->dsi.vertical_sync_active    = 2;
	params->dsi.vertical_backporch	    = 50;//50
	params->dsi.vertical_frontporch 	= 20;//20
	params->dsi.vertical_active_line	= FRAME_HEIGHT; 
	
	params->dsi.horizontal_sync_active	= 2;
	params->dsi.horizontal_backporch	= 100;
	params->dsi.horizontal_frontporch	= 100;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	// Bit rate calculation
	params->dsi.pll_div1=54;		//fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
	params->dsi.pll_div2=2; 		// div2=0~15: fout=fvo/(2*div2)

}

static void lcm_init_for_old_glass(void)
{
	unsigned int data_array[64];

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(120);//Must > 120ms
    
	//IsFirstBoot = KAL_TRUE;

//******* for NT35512_TD-TNWV4003-35 ************//

//*************Enable CMD2 Page1  *******************//
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x00043902;
	data_array[1]=0x1A8C00BC;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;
	data_array[1]=0x1A8C00BD;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00033902;
	data_array[1]=0x003500BE;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D1;
	data_array[2]=0x00270000;//00270013
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02CB019B;
	data_array[7]=0x024E0216;
	data_array[8]=0x027F024F;
	data_array[9]=0x02CF02B3;
	data_array[10]=0x030103EE;
	data_array[11]=0x032A031B;
	data_array[12]=0x03500340;
	data_array[13]=0x03A80367;
	data_array[14]=0x000000F8;//000000E8
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D2;
	data_array[2]=0x00270000;//00270013
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02CB019B;
	data_array[7]=0x024E0216;
	data_array[8]=0x027F024F;
	data_array[9]=0x02CF02B3;
	data_array[10]=0x030103EE;
	data_array[11]=0x032A031B;
	data_array[12]=0x03500340;
	data_array[13]=0x03A80367;
	data_array[14]=0x000000F8;//000000E8
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D3;
	data_array[2]=0x00270000;
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02CB019B;
	data_array[7]=0x024E0216;
	data_array[8]=0x027F024F;
	data_array[9]=0x02CF02B3;
	data_array[10]=0x030103EE;
	data_array[11]=0x032A031B;
	data_array[12]=0x03500340;
	data_array[13]=0x03A80367;
	data_array[14]=0x000000F8;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D4;
	data_array[2]=0x00270000;
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02CB019B;
	data_array[7]=0x024E0216;
	data_array[8]=0x027F024F;
	data_array[9]=0x02CF02B3;
	data_array[10]=0x030103EE;
	data_array[11]=0x032A031B;
	data_array[12]=0x03500340;
	data_array[13]=0x03A80367;
	data_array[14]=0x000000F8;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D5;
	data_array[2]=0x00270000;
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02CB019B;
	data_array[7]=0x024E0216;
	data_array[8]=0x027F024F;
	data_array[9]=0x02CF02B3;
	data_array[10]=0x030103EE;
	data_array[11]=0x032A031B;
	data_array[12]=0x03500340;
	data_array[13]=0x03A80367;
	data_array[14]=0x000000F8;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D6;
	data_array[2]=0x00270000;
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02CB019B;
	data_array[7]=0x024E0216;
	data_array[8]=0x027F024F;
	data_array[9]=0x02CF02B3;
	data_array[10]=0x030103EE;
	data_array[11]=0x032A031B;
	data_array[12]=0x03500340;
	data_array[13]=0x03A80367;
	data_array[14]=0x000000F8;
	dsi_set_cmdq(data_array, 15, 1);

//************* AVDD: manual  *******************//
	data_array[0]=0x00043902;
	data_array[1]=0x000000B0;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;
	data_array[1]=0x242424B6;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;
	data_array[1]=0x343434B8;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;//AVEE voltage, Set AVEE -6V
	data_array[1]=0x000000B1;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;//AVEE: manual, -6V
	data_array[1]=0x252525B7;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;//VGL(LVGL)
	data_array[1]=0x262626BA;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;//VGH: Clamp Enable
	data_array[1]=0x343434B9;
	dsi_set_cmdq(data_array, 2, 1);

// ********************  EABLE CMD2 PAGE 0 **************//

	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000008;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0xFCB11500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x10B41500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x07B61500;//#Source hold time
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x00033902;//Set Gate EQ
	data_array[1]=0x007171B7;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00053902;//Set Source EQ
	data_array[1]=0x0A0A01B8;
	data_array[2]=0x0000000A;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x00043902;//Inversion: Column inversion (NVT)
	data_array[1]=0x050505BC;//0x000000BC
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00063902;//Display Timing
	data_array[1]=0x078401BD;
	data_array[2]=0x00000031;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x00063902;//Display Timing
	data_array[1]=0x078401BE;
	data_array[2]=0x00000031;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x00063902;//Display Timing
	data_array[1]=0x078401BF;
	data_array[2]=0x00000031;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x00351500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x11111500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x29291500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x773A1500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x02C71500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x11C91500;//#Timing control 4H w/ 4-delay
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x00211500;
	dsi_set_cmdq(data_array, 1, 1);

//*************Enable CMD2 Page1  *******************//
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = 0xA555AAFF;
	data_array[2] = 0x00000080;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00103902;
	data_array[1] = 0x004063F7;
	data_array[2] = 0xC4010000;
	data_array[3] = 0x640200A2;
	data_array[4] = 0xD0004854;//0x54,0x48,0x00,0xD0
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0x00053902;
	data_array[1] = 0xA555AAFF;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);

// ********************  EABLE CMD2 PAGE 0 **************//
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000008;
	dsi_set_cmdq(data_array, 3, 1);
}

static void lcm_init_for_new_glass(void)
{
	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}
static void lcm_init(void)
{
	static int id1 = 0xff, id2 = 0xff;

	if (id1 == 0xff && id2 == 0xff){
		id1 = mt_get_gpio_in(GPIO50);
		id2 = mt_get_gpio_in(GPIO47);
	}

	if (id1 == 0 && id2 == 1)
		lcm_init_for_old_glass();
	else if (id1 == 0 && id2 == 0)
		lcm_init_for_new_glass();
}

static void lcm_suspend(void)
{
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(150);
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void)
{
	lcm_init();
	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	MDELAY(10);

    return TRUE;
}


static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0, ret = 0;
	unsigned char buffer[3];
	unsigned int array[16];
	unsigned int data_array[64];
	static int checktimes = 1;

	if (checktimes == 3)
		return 1;
	checktimes++;  // checktimes should be less than 3 times, there is an error for LCD (NO LCD or LCD is broken).

	SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(120);


	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(data_array, 3, 1);

	array[0] = 0x00033700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xC5, buffer, 3);
	
	id = buffer[1]; //we only need ID

#ifdef BUILD_UBOOT
	printf("\n\n\n\n[soso]35512 >> %s, id0 = 0x%08x,id1 = 0x%08x,id2 = 0x%08x\n", __func__, buffer[0],buffer[1],buffer[2]);
#endif

	if(0x55 == buffer[0] && 0x12 == buffer[1])
		ret = 1;

	return ret;
}

LCM_DRIVER nt35512_smartiii_lcm_drv = 
{
    .name			= "nt35512_smartiii",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.esd_recover   = lcm_esd_recover,
	.compare_id     = lcm_compare_id,
};


