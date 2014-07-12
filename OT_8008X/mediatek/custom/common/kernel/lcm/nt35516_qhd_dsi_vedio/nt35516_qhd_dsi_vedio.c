
#ifdef BUILD_LK
#else
#include <linux/string.h>
#endif

#include "lcm_drv.h"
//yufeng
#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH  										(540)
#define FRAME_HEIGHT 										(960)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

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
#define read_reg											lcm_util.dsi_read_reg()

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
	params->dsi.LANE_NUM				= LCM_THREE_LANE;
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
	params->dsi.word_count=540*3;
	params->dsi.vertical_sync_active=3;
	params->dsi.vertical_backporch=12;
	params->dsi.vertical_frontporch=2;
	params->dsi.vertical_active_line=960;

	params->dsi.line_byte=2048;		// 2256 = 752*3
	params->dsi.horizontal_sync_active_byte=26;
	params->dsi.horizontal_backporch_byte=146;
	params->dsi.horizontal_frontporch_byte=146;
	params->dsi.rgb_byte=(540*3+6);

	params->dsi.horizontal_sync_active_word_count=20;
	params->dsi.horizontal_backporch_word_count=140;
	params->dsi.horizontal_frontporch_word_count=140;

	params->dsi.PLL_CLOCK = LCM_DSI_6589_PLL_CLOCK_221;//this value must be in MTK suggested table
										//if not config this para, must config other 7 or 3 paras to gen. PLL
	params->dsi.pll_div1=0x1;		// div1=0,1,2,3;div1_real=1,2,4,4
	params->dsi.pll_div2=0x1;		// div2=0,1,2,3;div1_real=1,2,4,4	
	params->dsi.fbk_div =0x11;	    // fref=26MHz, fvco=fref*(fbk_div+1)*fbk_sel_real/(div1_real*div2_real)	
	params->dsi.fbk_sel=0x1;		// fbk_sel=0,1,2,3;fbk_select_real=1,2,4,4
	params->dsi.rg_bir=0x5;
	params->dsi.rg_bic=0x2;
	params->dsi.rg_bp=0xC;
}

static void lcm_init(void)
{
	unsigned int data_array[16];    
	SET_RESET_PIN(1);
	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(50);

	data_array[0]=0x00063902;
	data_array[1]=0x2555aaff;
	data_array[2]=0x00000101;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00363902;
	data_array[1]=0x4A0000F2;
	data_array[2]=0x0000A80A;
	data_array[3]=0x00000000;
	data_array[4]=0x00000000;
	data_array[5]=0x000B0000;
	data_array[6]=0x00000000;
	data_array[7]=0x00000000;
	data_array[8]=0x51014000;
	data_array[9]=0x01000100;
	dsi_set_cmdq(&data_array,10,1);

	data_array[0]=0x00083902;
	data_array[1]=0x070302F3;
	data_array[2]=0x0DD18845;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000008;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00043902;
	data_array[1]=0x0000CCB1;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x01B61500;
	dsi_set_cmdq(&data_array,1,1);

	data_array[0]=0x00023902;
	data_array[1]=0x007272B7;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00053902;
	data_array[1]=0x010101B8;
	data_array[2]=0x00000001;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x53BB1500;
	dsi_set_cmdq(&data_array,1,1);

	data_array[0]=0x00043902;
	data_array[1]=0x000000BC;
	dsi_set_cmdq(&data_array,2,1);

	/*data_array[0]=0x00063902;
	data_array[1]=0x109301BD;
	data_array[2]=0x00000120;
	//data_array[1]=0x084101BD;
	//data_array[2]=0x00000140;
	dsi_set_cmdq(&data_array,3,1);*/

	data_array[0]=0x00073902;
	data_array[1]=0x0D0661C9;
	data_array[2]=0x00001717;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00043902;
	data_array[1]=0x0C0C0CB0;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x0C0C0CB1;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x020202B2;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x101010B3;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x060606B4;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x545454B6;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x242424B7;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x303030B8;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x343434B9;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x242424BA;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x009800BC;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00043902;
	data_array[1]=0x009800BD;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x57BE1500;
	dsi_set_cmdq(&data_array,1,1);

	data_array[0]=0x00C21500;
	dsi_set_cmdq(&data_array,1,1);

	data_array[0]=0x00053902;
	data_array[1]=0x100F0FD0;
	data_array[2]=0x00000010;
	dsi_set_cmdq(&data_array,3,1);

//#Gamma Setting
	data_array[0]=0x00173902;
	data_array[1]=0x002300D1;
	data_array[2]=0x00310024;
	data_array[3]=0x00720052;
	data_array[4]=0x01DE00AE;
	data_array[5]=0x00000024;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00173902;
	data_array[1]=0x015401D2;
	data_array[2]=0x02C8019F;
	data_array[3]=0x02350208;
	data_array[4]=0x025E0236;
	data_array[5]=0x00000083;
	dsi_set_cmdq(&data_array,6,1);
	
	data_array[0]=0x00173902;
	data_array[1]=0x029802D3;//yufeng
	data_array[2]=0x02C002AF;
	data_array[3]=0x02D802D1;
	data_array[4]=0x03F802F3;
	data_array[5]=0x00000000;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00053902;
	data_array[1]=0x031C03D4;
	data_array[2]=0x00000052;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00173902;
	data_array[1]=0x002300D5;
	data_array[2]=0x00310024;
	data_array[3]=0x00720052;
	data_array[4]=0x01DE00AE;
	data_array[5]=0x00000024;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00173902;
	data_array[1]=0x015401D6;
	data_array[2]=0x02C8019F;
	data_array[3]=0x02350208;
	data_array[4]=0x025E0236;
	data_array[5]=0x00000083;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00173902;
	data_array[1]=0x029802D7;
	data_array[2]=0x02C002AF;
	data_array[3]=0x02D802D1;
	data_array[4]=0x03F802F3;
	data_array[5]=0x00000000;
	dsi_set_cmdq(&data_array,6,1);
	
 	data_array[0]=0x00053902;
	data_array[1]=0x031C03D8;
	data_array[2]=0x00000052;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00173902;
	data_array[1]=0x002300D9;
	data_array[2]=0x00310024;
	data_array[3]=0x00720052;
	data_array[4]=0x01DE00AE;
	data_array[5]=0x00000024;
	dsi_set_cmdq(&data_array,6,1);

    data_array[0]=0x00173902;
	data_array[1]=0x015401DD;
	data_array[2]=0x02C8019F;
	data_array[3]=0x02350208;
	data_array[4]=0x025E0236;
	data_array[5]=0x00000083;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00173902;
	data_array[1]=0x029802DE;
	data_array[2]=0x02C002AF;
	data_array[3]=0x02D802D1;
	data_array[4]=0x03F802F3;
	data_array[5]=0x00000000;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00053902;
	data_array[1]=0x031C03DF;
	data_array[2]=0x00000052;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00173902;
	data_array[1]=0x002300E0;
	data_array[2]=0x00310024;
	data_array[3]=0x00720052;
	data_array[4]=0x01DE00AE;
	data_array[5]=0x00000024;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00173902;
	data_array[1]=0x015401E1;
	data_array[2]=0x02C8019F;
	data_array[3]=0x02350208;
	data_array[4]=0x025E0236;
	data_array[5]=0x00000083;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00173902;
	data_array[1]=0x029802E2;
	data_array[2]=0x02C002AF;
	data_array[3]=0x02D802D1;
	data_array[4]=0x03F802F3;
	data_array[5]=0x00000000;
	dsi_set_cmdq(&data_array,6,1);

    data_array[0]=0x00053902;
	data_array[1]=0x031C03E3;
	data_array[2]=0x00000052;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00173902;
	data_array[1]=0x002300E4;
	data_array[2]=0x00310024;
	data_array[3]=0x00720052;
	data_array[4]=0x01DE00AE;
	data_array[5]=0x00000024;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00173902;
	data_array[1]=0x015401E5;
	data_array[2]=0x02C8019F;
	data_array[3]=0x02350208;
	data_array[4]=0x025E0236;
	data_array[5]=0x00000083;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00173902;
	data_array[1]=0x029802E6;
	data_array[2]=0x02C002AF;
	data_array[3]=0x02D802D1;
	data_array[4]=0x03F802F3;
	data_array[5]=0x00000000;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00053902;
	data_array[1]=0x031C03E7;
	data_array[2]=0x00000052;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00173902;
	data_array[1]=0x002300E8;
	data_array[2]=0x00310024;
	data_array[3]=0x00720052;
	data_array[4]=0x01DE00AE;
	data_array[5]=0x00000024;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00173902;
	data_array[1]=0x015401E9;
	data_array[2]=0x02C8019F;
	data_array[3]=0x02350208;
	data_array[4]=0x025E0236;
	data_array[5]=0x00000083;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00173902;
	data_array[1]=0x029802EA;
	data_array[2]=0x02C002AF;
	data_array[3]=0x02D802D1;
	data_array[4]=0x03F802F3;
	data_array[5]=0x00000000;
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00053902;
	data_array[1]=0x031C03EB;
	data_array[2]=0x00000052;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x773A1500;
	dsi_set_cmdq(&data_array,1,1);

    data_array[0]=0x00361500;
	dsi_set_cmdq(&data_array,1,1);

	data_array[0]=0x00351500;
	dsi_set_cmdq(&data_array,1,1);

	data_array[0]=0x00291500;
	dsi_set_cmdq(&data_array,1,1);
	MDELAY(40);

	data_array[0]=0x00110500;
	dsi_set_cmdq(&data_array,1,1);
	MDELAY(150);	
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
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];
	data_array[0]=0x00280500;
	dsi_set_cmdq(&data_array,1,1);
	MDELAY(20);
	data_array[0]=0x00100500;
	dsi_set_cmdq(&data_array,1,1);
	MDELAY(150);
}

static void lcm_resume(void)
{
	/*unsigned int data_array[16];
	data_array[0]=0x00110500;
	dsi_set_cmdq(&data_array,1,1);
	MDELAY(150);
	data_array[0]=0x00290500;
	dsi_set_cmdq(&data_array,1,1);
	MDELAY(20);*/

	lcm_init();
}

static unsigned int lcm_compare_id(void)
{
	return 1;
}

LCM_DRIVER nt35516_qhd_rav4_lcm_drv = 
{
	.name			= "nt35516_TDT_rav4",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
	.compare_id     = lcm_compare_id,
	.update         = lcm_update,
#endif
};
