#ifdef BUILD_LK
#include <platform/disp_drv_platform.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/disp_drv_platform.h>
#else
    #include <linux/string.h>
    #include <linux/delay.h>
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(540)
#define FRAME_HEIGHT 										(960)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									1

#define LCD_LDO2V8_GPIO_PIN            (12)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define SET_GPIO_OUT(n, v)	        (lcm_util.set_gpio_out((n), (v)))

#define UDELAY(n) (lcm_util.udelay(n))
//#define MDELAY(n) (lcm_util.mdelay(n))

#define MDELAY(n)							\
do{										\
	((n)<=20)?mdelay(n):msleep(n);			\
}while(0)


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()


static struct LCM_setting_table {
    unsigned cmd;
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
	{0x11, 1, {0x00}},

	{0xF0,	5,	{0x55, 0xaa, 0x52,0x08,0x00}},
	{REGFLAG_DELAY, 1, {}},

	{0xB1,	1,	{0xec}},
	{REGFLAG_DELAY, 1, {}},

	{0xBc,	3,	{0x02, 0x02, 0x02}},
	{REGFLAG_DELAY, 1, {}},

	{0xF0,	5,	{0x55, 0xaa, 0x52,0x08,0x01}},
	{REGFLAG_DELAY, 1, {}},

	{0xBE,	1, {0x51}},
	{REGFLAG_DELAY, 1, {}},

	{0xD0,	4,	{0x0f, 0x0f, 0x10,0x10}},
	{REGFLAG_DELAY, 1, {}},

	{0xBc,	3,	{0x00, 0x6c, 0x00}},
	{REGFLAG_DELAY, 1, {}},

	{0xBd,	3,	{0x00, 0x6c, 0x00}},
	{REGFLAG_DELAY, 1, {}},

	{0xD1,	16, {0x00, 0x37, 0x00, 0x4f,
				 0x00, 0x72, 0x00, 0x89,
				 0x00, 0xa3, 0x00, 0xc9,
				 0x00, 0xe4, 0x01, 0x14}},
	{REGFLAG_DELAY, 1, {}},

	{0xD2,	16, {0x01, 0x3d, 0x01, 0x7d,
				 0x01, 0xaf, 0x01, 0xfd,
				 0x02, 0x3c, 0x02, 0x3d,
				 0x02, 0x77, 0x02, 0xb3}},
	{REGFLAG_DELAY, 1, {}},

	{0xD3,	16, {0x02, 0xdb, 0x03, 0x0f,
				 0x03, 0x25, 0x03, 0x5c,
				 0x03, 0x77, 0x03, 0x94,
				 0x03, 0x9f, 0x03, 0xac}},
	{REGFLAG_DELAY, 1, {}},

	{0xD4,	4, {0x03, 0xba, 0x03, 0xc1}},
	{REGFLAG_DELAY, 1, {}},

	{0xD5,	16, {0x00, 0x37, 0x00, 0x4f,
				 0x00, 0x72, 0x00, 0x89,
				 0x00, 0xa3, 0x00, 0xc9,
				 0x00, 0xe4, 0x01, 0x14}},
	{REGFLAG_DELAY, 1, {}},

	{0xD6,	16, {0x01, 0x3d, 0x01, 0x7d,
				 0x01, 0xaf, 0x01, 0xfd,
				 0x02, 0x3c, 0x02, 0x3d,
				 0x02, 0x77, 0x02, 0xb3}},
	{REGFLAG_DELAY, 1, {}},

	{0xD7,	16, {0x02, 0xdb, 0x03, 0x0f,
				 0x03, 0x25, 0x03, 0x5c,
				 0x03, 0x77, 0x03, 0x94,
				 0x03, 0x9f, 0x03, 0xac}},
	{REGFLAG_DELAY, 1, {}},

	{0xD8,	4, {0x03, 0xba, 0x03, 0xc1}},
	{REGFLAG_DELAY, 1, {}},

	{0xD9,	16, {0x00, 0x37, 0x00, 0x4f,
				 0x00, 0x72, 0x00, 0x89,
				 0x00, 0xa3, 0x00, 0xc9,
				 0x00, 0xe4, 0x01, 0x14}},
	{REGFLAG_DELAY, 1, {}},

	{0xDD,	16, {0x01, 0x3d, 0x01, 0x7d,
				 0x01, 0xaf, 0x01, 0xfd,
				 0x02, 0x3c, 0x02, 0x3d,
				 0x02, 0x77, 0x02, 0xb3}},
	{REGFLAG_DELAY, 1, {}},

	{0xDE,	16, {0x02, 0xdb, 0x03, 0x0f,
				 0x03, 0x25, 0x03, 0x5c,
				 0x03, 0x77, 0x03, 0x94,
				 0x03, 0x9f, 0x03, 0xac}},
	{REGFLAG_DELAY, 1, {}},

	{0xDF,	4, {0x03, 0xba, 0x03, 0xc1}},
	{REGFLAG_DELAY, 1, {}},
//

	{0xE0,	16, {0x00, 0x37, 0x00, 0x4f,
				 0x00, 0x72, 0x00, 0x89,
				 0x00, 0xa3, 0x00, 0xc9,
				 0x00, 0xe4, 0x01, 0x14}},
	{REGFLAG_DELAY, 1, {}},

	{0xE1,	16, {0x01, 0x3d, 0x01, 0x7d,
				 0x01, 0xaf, 0x01, 0xfd,
				 0x02, 0x3c, 0x02, 0x3d,
				 0x02, 0x77, 0x02, 0xb3}},
	{REGFLAG_DELAY, 1, {}},

	{0xE2,	16, {0x02, 0xdb, 0x03, 0x0f,
				 0x03, 0x25, 0x03, 0x5c,
				 0x03, 0x77, 0x03, 0x94,
				 0x03, 0x9f, 0x03, 0xac}},
	{REGFLAG_DELAY, 1, {}},

	{0xE3,	4, {0x03, 0xba, 0x03, 0xc1}},
	{REGFLAG_DELAY, 1, {}},

	{0xE4,	16, {0x00, 0x37, 0x00, 0x4f,
				 0x00, 0x72, 0x00, 0x89,
				 0x00, 0xa3, 0x00, 0xc9,
				 0x00, 0xe4, 0x01, 0x14}},
	{REGFLAG_DELAY, 1, {}},

	{0xE5,	16, {0x01, 0x3d, 0x01, 0x7d,
				 0x01, 0xaf, 0x01, 0xfd,
				 0x02, 0x3c, 0x02, 0x3d,
				 0x02, 0x77, 0x02, 0xb3}},
	{REGFLAG_DELAY, 1, {}},

	{0xE6,	16, {0x02, 0xdb, 0x03, 0x0f,
				 0x03, 0x25, 0x03, 0x5c,
				 0x03, 0x77, 0x03, 0x94,
				 0x03, 0x9f, 0x03, 0xac}},
	{REGFLAG_DELAY, 1, {}},

	{0xE7,	4, {0x03, 0xba, 0x03, 0xc1}},
	{REGFLAG_DELAY, 1, {}},

	{0xE8,	16, {0x00, 0x37, 0x00, 0x4f,
				 0x00, 0x72, 0x00, 0x89,
				 0x00, 0xa3, 0x00, 0xc9,
				 0x00, 0xe4, 0x01, 0x14}},
	{REGFLAG_DELAY, 1, {}},

	{0xE9,	16, {0x01, 0x3d, 0x01, 0x7d,
				 0x01, 0xaf, 0x01, 0xfd,
				 0x02, 0x3c, 0x02, 0x3d,
				 0x02, 0x77, 0x02, 0xb3}},
	{REGFLAG_DELAY, 1, {}},

	{0xEA,	16, {0x02, 0xdb, 0x03, 0x0f,
				 0x03, 0x25, 0x03, 0x5c,
				 0x03, 0x77, 0x03, 0x94,
				 0x03, 0x9f, 0x03, 0xac}},
	{REGFLAG_DELAY, 1, {}},

	{0xEB,	4, {0x03, 0xba, 0x03, 0xc1}},
	{REGFLAG_DELAY, 1, {}},

	{0xB3,	3,	{0x10, 0x10, 0x10}},
	{REGFLAG_DELAY, 1, {}},

	{0xB4,	3,	{0x0a, 0x0a, 0x0a}},
	{REGFLAG_DELAY, 1, {}},

	{0xB2,	3,	{0x11, 0x11, 0x11}},

	{0x35, 1, {0x00}},

    	{REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 10, {}},

	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.

	{0x53,	1,	{0x24}},

	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};



static struct LCM_setting_table lcm_set_window[] = {
	{0x2A,	4,	{0x00, 0x00, (FRAME_WIDTH>>8), (FRAME_WIDTH&0xFF)}},
	{0x2B,	4,	{0x00, 0x00, (FRAME_HEIGHT>>8), (FRAME_HEIGHT&0xFF)}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 1, {0x00}},
    	{REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
    	//{REGFLAG_DELAY, 40, {}},

    // Sleep Mode On
	{0x10, 1, {0x00}},
    	{REGFLAG_DELAY, 120, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},
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

		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
		params->dsi.packet_size=256;

		// Video mode setting
		params->dsi.intermediat_buffer_num = 2;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.word_count=480*3;

		params->dsi.vertical_sync_active=3;
		params->dsi.vertical_backporch=12;
		params->dsi.vertical_frontporch=2;
		params->dsi.vertical_active_line=800;

		params->dsi.line_byte=2048;		// 2256 = 752*3
		params->dsi.horizontal_sync_active_byte=26;
		params->dsi.horizontal_backporch_byte=146;
		params->dsi.horizontal_frontporch_byte=146;
		params->dsi.rgb_byte=(480*3+6);

		params->dsi.horizontal_sync_active_word_count=20;
		params->dsi.horizontal_backporch_word_count=140;
		params->dsi.horizontal_frontporch_word_count=140;

		// Bit rate calculation
		params->dsi.pll_div1=38;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
		params->dsi.pll_div2=1;			// div2=0~15: fout=fvo/(2*div2)

}

static void lcm_init(void)
{
    SET_GPIO_OUT(LCD_LDO2V8_GPIO_PIN , 1);
    MDELAY(200);

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(10);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	SET_GPIO_OUT(LCD_LDO2V8_GPIO_PIN , 0);
}


static void lcm_resume(void)
{
    SET_GPIO_OUT(LCD_LDO2V8_GPIO_PIN , 1);
    MDELAY(10);

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(10);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
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

	dsi_set_cmdq(&data_array, 7, 0);

}


static void lcm_setbacklight(unsigned int level)
{

	//for LGE backlight IC mapping table
	if(level > 255)
			level = 255;

	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_setpwm(unsigned int divider)
{
	// TBD
}


static unsigned int lcm_getpwm(unsigned int divider)
{
	// ref freq = 15MHz, B0h setting 0x80, so 80.6% * freq is pwm_clk;
	// pwm_clk / 255 / 2(lcm_setpwm() 6th params) = pwm_duration = 23706
	unsigned int pwm_clk = 23706 / (1<<divider);
	return pwm_clk;
}
LCM_DRIVER nt35516_lcm_drv =
{
    .name			= "nt35516",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
        .set_backlight	= lcm_setbacklight,
		//.set_pwm        = lcm_setpwm,
		//.get_pwm        = lcm_getpwm,
        .update         = lcm_update
#endif
    };

