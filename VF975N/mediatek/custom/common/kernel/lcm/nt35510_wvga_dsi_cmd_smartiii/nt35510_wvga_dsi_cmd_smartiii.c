
#ifdef BUILD_LK
#include <string.h>
#else
#include <linux/string.h>
#ifndef BUILD_UBOOT
#include <linux/module.h>
#include <linux/printk.h>

#endif
#endif

#include "lcm_drv.h"


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(800)
//#define LCM_ID       (0x55)
#define LCM_ID       (0x01)
#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#define LCM_DSI_CMD_MODE									1

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
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
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

	{0xF0,	5,	{0x55, 0xaa, 0x52,0x08,0x01}},
	{0xBc,	3,	{0x00, 0x78, 0x1a}},
	{0xBd,	3,	{0x00, 0x78,0x1a}},
	{0xBe,	2,	{0x00, 0x4e}},
	{0xD1,	52, {0x00, 0x00, 0x00, 0x13,
				 0x00, 0x27, 0x00, 0x46,
				 0x00, 0x6a, 0x00, 0xa4,
				 0x00, 0xd5, 0x01, 0x1e,
				 0x01, 0x53, 0x01, 0x9b,
				 0x01, 0xcb, 0x02, 0x16,
				 0x02, 0x4e, 0x02, 0x4f,
				 0x02, 0x7f, 0x02, 0xb3,
				 0x02, 0xcf, 0x02, 0xee,
				 0x03, 0x01, 0x03, 0x1b,
				 0x03, 0x2a, 0x03, 0x40,
				 0x03, 0x50, 0x03, 0x67,
				 0x03, 0xa8, 0x03, 0xd8}},
	{0xD2,	52, {0x00, 0x00, 0x00, 0x13,
				 0x00, 0x27, 0x00, 0x46,
				 0x00, 0x6a, 0x00, 0xa4,
				 0x00, 0xd5, 0x01, 0x1e,
				 0x01, 0x53, 0x01, 0x9b,
				 0x01, 0xcb, 0x02, 0x16,
				 0x02, 0x4e, 0x02, 0x4f,
				 0x02, 0x7f, 0x02, 0xb3,
				 0x02, 0xcf, 0x02, 0xee,
				 0x03, 0x01, 0x03, 0x1b,
				 0x03, 0x2a, 0x03, 0x40,
				 0x03, 0x50, 0x03, 0x67,
				 0x03, 0xa8, 0x03, 0xd8}},
	{0xD3,	52, {0x00, 0x00, 0x00, 0x13,
				 0x00, 0x27, 0x00, 0x46,
				 0x00, 0x6a, 0x00, 0xa4,
				 0x00, 0xd5, 0x01, 0x1e,
				 0x01, 0x53, 0x01, 0x9b,
				 0x01, 0xcb, 0x02, 0x16,
				 0x02, 0x4e, 0x02, 0x4f,
				 0x02, 0x7f, 0x02, 0xb3,
				 0x02, 0xcf, 0x02, 0xee,
				 0x03, 0x01, 0x03, 0x1b,
				 0x03, 0x2a, 0x03, 0x40,
				 0x03, 0x50, 0x03, 0x67,
				 0x03, 0xa8, 0x03, 0xd8}},
	{0xD4,	52, {0x00, 0x00, 0x00, 0x13,
				 0x00, 0x27, 0x00, 0x46,
				 0x00, 0x6a, 0x00, 0xa4,
				 0x00, 0xd5, 0x01, 0x1e,
				 0x01, 0x53, 0x01, 0x9b,
				 0x01, 0xcb, 0x02, 0x16,
				 0x02, 0x4e, 0x02, 0x4f,
				 0x02, 0x7f, 0x02, 0xb3,
				 0x02, 0xcf, 0x02, 0xee,
				 0x03, 0x01, 0x03, 0x1b,
				 0x03, 0x2a, 0x03, 0x40,
				 0x03, 0x50, 0x03, 0x67,
				 0x03, 0xa8, 0x03, 0xd8}},
	{0xD5,	52, {0x00, 0x00, 0x00, 0x13,
				 0x00, 0x27, 0x00, 0x46,
				 0x00, 0x6a, 0x00, 0xa4,
				 0x00, 0xd5, 0x01, 0x1e,
				 0x01, 0x53, 0x01, 0x9b,
				 0x01, 0xcb, 0x02, 0x16,
				 0x02, 0x4e, 0x02, 0x4f,
				 0x02, 0x7f, 0x02, 0xb3,
				 0x02, 0xcf, 0x02, 0xee,
				 0x03, 0x01, 0x03, 0x1b,
				 0x03, 0x2a, 0x03, 0x40,
				 0x03, 0x50, 0x03, 0x67,
				 0x03, 0xa8, 0x03, 0xd8}},
	{0xD6,	52, {0x00, 0x00, 0x00, 0x13,
				 0x00, 0x27, 0x00, 0x46,
				 0x00, 0x6a, 0x00, 0xa4,
				 0x00, 0xd5, 0x01, 0x1e,
				 0x01, 0x53, 0x01, 0x9b,
				 0x01, 0xcb, 0x02, 0x16,
				 0x02, 0x4e, 0x02, 0x4f,
				 0x02, 0x7f, 0x02, 0xb3,
				 0x02, 0xcf, 0x02, 0xee,
				 0x03, 0x01, 0x03, 0x1b,
				 0x03, 0x2a, 0x03, 0x40,
				 0x03, 0x50, 0x03, 0x67,
				 0x03, 0xa8, 0x03, 0xd8}},
	{0xB0,	3,	{0x00, 0x00, 0x00}},
	{0xB6,	3,	{0x36, 0x36, 0x36}},
	{0xB8,	3,	{0x26, 0x26, 0x26}},
	{0xB1,	3,	{0x00, 0x00, 0x00}},
	{0xB7,	3,	{0x26, 0x26, 0x26}},
	{0xBa,	3,	{0x16, 0x16, 0x16}},
	{0xB9,	3,	{0x34, 0x34, 0x34}},
	{0xf0,	5,	{0x55, 0xaa, 0x52, 0x08, 0x00}},
	{0xB1, 	1,	{0xcc}},
	{0xB4,	1,	{0x10}},
	{0xFF, 	4,	{0xaa,0x55,0x25,0x01}},
	{0xF9, 	11,	{0x14,0x00,0x0d,0x1a,
					0x26,0x33,0x40,0x4d,
					0x5a,0x66,0x73}},
	{0xB6,	1,	{0x07}},
	{0xB7,	2,	{0x71, 0x71}},
	{0xB8,	4,	{0x01, 0x0a, 0x0a,0x0a}},
	{0xBC,	3,	{0x05, 0x05, 0x05}},
	{0xBD,	5,	{0x01, 0x84, 0x07,0x31,0x00}},
	{0xBe,	5,	{0x01, 0x84, 0x07,0x31,0x00}},
	{0xBf,	5,	{0x01, 0x84, 0x07,0x31,0x00}},
	{0x35,	1,	{0x00}},
	{0x3a,	1,	{0x77}},
	{0xf0,	5,	{0x55, 0xaa, 0x52, 0x08, 0x00}},
	{0xC7,	1,	{0x02}},
	{0xc9,	5,	{0x11, 0x00, 0x00, 0x00, 0x00}},
	{0x21,	1,	{0x00}},
	{0xf0,	5,	{0x55, 0xaa, 0x52, 0x08, 0x01}},
	{0xBe,	2,	{0x00, 0x4c}},
	{0x2c, 1, {0x00}},

	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.

	//only for mackup 	
	{0x11,	1,	{0x00}},
    {REGFLAG_DELAY, 150, {}},
	{0x29,	1,	{0x00}},
    {REGFLAG_DELAY, 30, {}},
	{0x53,	1,	{0x24}},
	{0x51,	1,	{0xff}},
	{REGFLAG_DELAY, 10, {}},

	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    // Sleep Mode On
	{0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_compare_id_setting[] = {
	// Display off sequence
	{0xF0,	5,	{0x55, 0xaa, 0x52,0x08,0x01}},
	{REGFLAG_DELAY, 10, {}},

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

	// enable tearing-free
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

	params->dsi.LANE_NUM				= LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;

	params->dsi.intermediat_buffer_num = 2;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count=480*3;

	params->dsi.vertical_sync_active    = 2;
	params->dsi.vertical_backporch	    = 50;
	params->dsi.vertical_frontporch 	= 20;
	params->dsi.vertical_active_line	= FRAME_HEIGHT;

	params->dsi.line_byte=2180;		
	params->dsi.horizontal_sync_active_byte=26;
	params->dsi.horizontal_backporch_byte=206;   ///146
	params->dsi.horizontal_frontporch_byte=206;   ///146
	params->dsi.rgb_byte=(480*3+6);

	params->dsi.horizontal_sync_active	= 2;
	params->dsi.horizontal_backporch	= 100;
	params->dsi.horizontal_frontporch	= 100;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.HS_TRAIL = 10;
	params->dsi.HS_ZERO = 8;
	params->dsi.HS_PRPR = 4;
	params->dsi.LPX = 12;
	params->dsi.TA_SACK = 1;
	params->dsi.TA_GET = 60;
	params->dsi.TA_SURE = 18;
	params->dsi.TA_GO = 12;
	params->dsi.CLK_TRAIL = 5;
	params->dsi.CLK_ZERO = 18;
	params->dsi.LPX_WAIT = 10;
	params->dsi.CONT_DET = 0;
	params->dsi.CLK_HS_PRPR = 4;

	// Bit rate calculation
	params->dsi.pll_div1=34;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
	params->dsi.pll_div2=1;			// div2=0~15: fout=fvo/(2*div2)

}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(20);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}
static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{
	lcm_init();
	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}


static void lcm_setbacklight(unsigned int level)
{
	unsigned int data_array[16];

	if(level > 255)
		level = 255;
	if(level >0 && level <30)
		level =30;

	data_array[0]= 0x00023902;
	data_array[1] =(0x51|(level<<8));
	dsi_set_cmdq(&data_array, 2, 1);
}

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_UBOOT
	unsigned char buffer[2];
	unsigned int array[16];

	push_table(lcm_compare_id_setting, sizeof(lcm_compare_id_setting) / sizeof(struct LCM_setting_table), 1);

	array[0] = 0x00023700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xbe,buffer, 2);

   // printk("[%s],  vcom is %x,%x-----------\n",__func__, buffer[0], buffer[1]);

    if(buffer[1] == 0x4c){
        return FALSE;
    }
    else{
        return TRUE;
    }
#endif
}

static unsigned int lcm_esd_recover(void)
{
    unsigned char para = 0;

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(50);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(5);
    return TRUE;
}

static unsigned int lcm_compare_id(void)
{
	unsigned int ret = 0;
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

#ifdef BUILD_UBOOT
	extern void DSI_clk_HS_mode();
#endif
#ifdef BUILD_UBOOT
	DSI_clk_HS_mode(1);
#endif
	MDELAY(10);
#ifdef BUILD_UBOOT
	DSI_clk_HS_mode(0);
#endif

	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(10);
	array[0] = 0x00033700;// read id return two byte
	dsi_set_cmdq(array, 1, 1);
	MDELAY(10);
	read_reg_v2(0xC5, buffer, 3);
		
#ifdef BUILD_UBOOT
	printf("\n\n\n\n[soso]35510 >> %s, id0 = 0x%08x,id1 = 0x%08x,id2 = 0x%08x\n", __func__, buffer[0],buffer[1],buffer[2]);
#endif

	if(0x55 == buffer[0] && 0x10 == buffer[1])   // nt35510
		ret = 1;	

	return ret;

}
// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER nt35510_smartiii_lcm_drv =
{
    .name			= "nt35510_smartiii_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
	.update         = lcm_update,
	//.set_backlight	= lcm_setbacklight,
	.esd_check   	= lcm_esd_check,
   	.esd_recover    = lcm_esd_recover,
	.compare_id     = lcm_compare_id,
#endif
};

