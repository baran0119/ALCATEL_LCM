
#ifdef BUILD_UBOOT
#include <asm/arch/mt6577_pwm.h>
#include <asm/arch/mt6577_gpio.h>
#include <asm/arch/mt65xx_leds.h>
#include <asm/io.h>

#else

#if defined(MT6575)
#include <mach/mt6575_gpio.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_clock_manager.h>
#include <mach/mt6575_pmic_feature_api.h>
#elif defined(MT6577)
#include <mach/mt6577_gpio.h>
#include <mach/mt6577_typedefs.h>
//#include <mach/mt6577_clock_manager.h>
//#include <mach/mt6577_pmic_feature_api.h>
#endif

#endif
#include "lcm_drv.h"

#ifdef BUILD_UBOOT
//
#else
#include <linux/kernel.h>//for printk
#endif


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(540)
#define FRAME_HEIGHT 										(960)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

#define LCM_ID_HX8389B 0x89

#define LCM_ID2_PIN		47
#define LCM_TDT			1
#define LCM_TRULY		0
#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif
static unsigned int lcm_esd_test = FALSE;      ///only for ESD test baoqiang add
static unsigned int first_inited = 0;
unsigned int lcm_select = LCM_TRULY; //for default select



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
static int lcm_judge_id_pin(void);
       

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


//first resource truly vendor
static struct LCM_setting_table lcm_first_initialization_setting[] = {

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
	 
	 #if 1
	{0xB9,	3,	{0xFF, 0x83, 0x89}},
	//{REGFLAG_DELAY, 5, {}}, //10ms



	{0xB1, 19, {0x00, 0x00, 0x04, 0xEB,//07 legen
				0x50, 0x10, 0x11, 0xB0, //B3 legen
				0xF0, 0x2f, 0x37, 0x1A, //0xF3, 0x2F, 0x37, 0x1A,
				0x1A, 0x43, 0x01, 0x58,//0x1A, 0x43, 0x01, 0x58,
				0xF1, 0x00, 0xE6}},//0xf2
	{REGFLAG_DELAY, 5, {}},

	//set power
	{0xB2,	7,	{0x00, 0x00, 0x78, 0x0C,			 
				 0x07, 0x00, 0xF0}},
	{REGFLAG_DELAY, 5, {}},

	//set cyc
	{0xB4, 	23,	{0x92, 0x08, 0x00, 0x32,
				0x10, 0x04, 0x32, 0x10,
				0x00, 0x32, 0x10, 0x00,
				0x37, 0x0A, 0x40, 0x08, 
				0x37, 0x0A, 0x40, 0x14, 
				0x46, 0x50, 0x0A}},//0x46,0x53,0x0d //0x46,0x50,0x0A
				
	//{REGFLAG_DELAY, 5, {}},



	//set gip
	{0xD5,	56, {0x00, 0x00, 0x00, 0x00,
				 0x01, 0x00, 0x00, 0x00,
				 0x60, 0x00, 0x88, 0x88,
				 0x88, 0x88, 0x88, 0x23,
				 0x88, 0x01, 0x88, 0x67,
				 0x88, 0x45, 0x01, 0x23,
				 0x88, 0x88, 0x88, 0x88,
				 0x88, 0x88, 0x88, 0x88,
				 0x88, 0x88, 0x54, 0x88,
				 0x76, 0x88, 0x10, 0x88,
				 0x32, 0x32, 0x10, 0x88,
				 0x88, 0x88, 0x88, 0x88,
				 0x00, 0x00, 0x00, 0x00,
				 0x00, 0x00, 0x00, 0x00,
				 }},
	//{REGFLAG_DELAY, 5, {}},
      //Set VCOM
	{0xCB,  2,      {0x07, 0x07}},
	//{REGFLAG_DELAY, 5, {}},

	//Set OTP
	{0xBB,  4,     {0x00, 0x00, 0xFF, 0x80}},
	//{REGFLAG_DELAY, 5, {}},
	
	//SET VCOM
	{0xB6,  4,   {0x00, 0xa0, 0x00, 0xa0}},
	//{REGFLAG_DELAY, 5, {}},      

	
	{0xCC,	1,	{0x02}}, 
	//{REGFLAG_DELAY, 5, {}}, //10ms
	{0xC6,	1,	{0x08}}, //add---baoqiang

	// ENABLE FMARK
	//{0x44,	2,	{((FRAME_HEIGHT/2)>>8), ((FRAME_HEIGHT/2)&0xFF)}},
	{0x35,	1,	{0x00}},
  	{0x3A,	1,	{0x77}},       
   	//baoqiang add	
  	{0xDE,	3,	{0x05,0x58,0x10}},//0x58
	//{REGFLAG_DELAY, 5, {}},
  	//baoqiang add end   

	// SET GAMMA	2.2   
	{0xE0,	34,	{0x05, 0x10, 0x1C, 0x2D,
				 0x2D, 0x3E, 0x3D, 0x51,
				 0x07, 0x11, 0x12, 0x15,
				 0x17, 0x16, 0x16, 0x12,
				 0x1A,
				 0x05, 0x10, 0x1C, 0x2D,
				 0x2D, 0x3E, 0x3D, 0x51,
				 0x07, 0x11, 0x12, 0x15,
				 0x17, 0x16, 0x16, 0x12,
				 0x1A}},
	//{REGFLAG_DELAY, 5, {}},
	
	  //baoqiang add for esd protection
 
	 {0xBA, 18, {0x41, 0x83, 0x00, 0x16,
				 0xa4, 0x00, 0x18, 0xff,
				 0x0f, 0x21, 0x03, 0x21,
				 0x23, 0x25, 0x20, 0x02,
				 0x35, 0x40}},
	 //{REGFLAG_DELAY, 5, {}},
 
	  //baoqiang add end


	// Sleep Out
	{0x11, 0, {0x00}},
   	{REGFLAG_DELAY, 120, {}},

    	// Display ON
	{0x29, 0, {0x00}},
	//{REGFLAG_DELAY, 10, {}},



	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	    #endif
};


//second resource tdt vendor
static struct LCM_setting_table lcm_second_initialization_setting[] = {

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
	
	{0xB9,	3,	{0xFF, 0x83, 0x89}},
	//{REGFLAG_DELAY, 5, {}}, //10ms


	//baoqiang add for esd protection
 
	 {0xBA, 7,  {0x41, 0x93, 0x00, 0x16,
				 0xa4, 0x10, 0x18}},
	 //{REGFLAG_DELAY, 5, {}},
 
	  //baoqiang add end


	{0xC6,	1,	{0x08}},
	//{REGFLAG_DELAY, 5, {}}, //10ms

	//Set power
	{0xB1, 19, {0x00, 0x00, 0x04, 0xEB,//07 legen
				0x98, 0x10, 0x11, 0x90, //B3 legen
				0xF0, 0x2f, 0x37, 0x26, //0xF3, 0x2F, 0x37, 0x1A,
				0x26, 0x42, 0x01, 0x2A,//0x1A, 0x43, 0x01, 0x58,
				0xFC, 0x00, 0xE6}},//0xf1
	{REGFLAG_DELAY, 5, {}},

	//set display related register
	{0xB2,	7,	{0x00, 0x00, 0x78, 0x0C,			 
				 0x07, 0x3F, 0x80}},
	{REGFLAG_DELAY, 5, {}},

	//set cyc
	{0xB4, 	23,	{0x82, 0x08, 0x00, 0x32,
				0x10, 0x04, 0x32, 0x10,
				0x00, 0x32, 0x10, 0x00,
				0x37, 0x0A, 0x40, 0x08, 
				0x37, 0x0A, 0x40, 0x14, 
				0x46, 0x50, 0x0A}},//0x46,0x53,0x0d //0x46,0x50,0x0A
				
	//{REGFLAG_DELAY, 5, {}},



	//set gip
	{0xD5,	56, {0x00, 0x00, 0x00, 0x00,
				 0x01, 0x00, 0x00, 0x00,
				 0x60, 0x00, 0x88, 0x88,
				 0x88, 0x88, 0x88, 0x23,
				 0x88, 0x01, 0x88, 0x67,
				 0x88, 0x45, 0x01, 0x23,
				 0x88, 0x88, 0x88, 0x88,
				 0x88, 0x88, 0x88, 0x88,
				 0x88, 0x88, 0x54, 0x88,
				 0x76, 0x88, 0x10, 0x88,
				 0x32, 0x32, 0x10, 0x88,
				 0x88, 0x88, 0x88, 0x88,
				 0x00, 0x00, 0x00, 0x00,
				 0x00, 0x00, 0x00, 0x00,
				 }},
	//{REGFLAG_DELAY, 5, {}},

	// SET GAMMA	2.2   
	{0xE0,	34,	{0x00, 0x0A, 0x0B, 0x15,
				 0x15, 0x3F, 0x27, 0x38,
				 0x03, 0x0C, 0x0F, 0x16,
				 0x18, 0x16, 0x16, 0x10,
				 0x17,
				 0x00, 0x0A, 0x0B, 0x15,
				 0x15, 0x3F, 0x27, 0x38,
				 0x03, 0x0C, 0x0F, 0x16,
				 0x18, 0x16, 0x16, 0x10,
				 0x17}},
	//{REGFLAG_DELAY, 5, {}},
	

	//legen add at 20121115
	// gamma2.2

		
	{0xDE,	2,	{0x05,0x58}},//0x58	
	
	{0xCC,	1,	{0x02}}, 
	//{REGFLAG_DELAY, 5, {}}, //10ms

	{0xE6,	1,	{0x01}},
	{0xE4,	1,	{0x03}},
	

	//SET VCOM
	{0xB6,  4,   {0x00, 0xA0, 0x00, 0xA0}},//0xac
	//{REGFLAG_DELAY, 5, {}}, 	

	// ENABLE FMARK
	//{0x44,	2,	{((FRAME_HEIGHT/2)>>8), ((FRAME_HEIGHT/2)&0xFF)}},
	{0x3A,	1,	{0x77}},
	{0x35,	1,	{0x00}},
  	       
   	
	// Sleep Out
	{0x11, 0, {0x00}},
        {REGFLAG_DELAY, 120, {}},//150	

     	// Display ON
	{0x29, 0, {0x00}},
	//{REGFLAG_DELAY, 10, {}},

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
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 0, {0x00}},
	//{REGFLAG_DELAY, 10, {}},
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_sleep_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},

    // Sleep Mode On
	{0x10, 0, {0x00}},

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
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED; //LCM_DBI_TE_MODE_VSYNC_ONLY;//baoqiang//LCM_DBI_TE_MODE_DISABLED;//soso
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
		params->dsi.lcm_ext_te_monitor = TRUE; //legen 0922 
		params->dsi.noncont_clock = FALSE;
		params->dsi.noncont_clock_period = 1;
		params->dsi.lcm_int_te_monitor = FALSE;
		params->dsi.lcm_int_te_period = 2;
		

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

		params->dsi.vertical_sync_active				= 6;//5
		params->dsi.vertical_backporch					= 10;//5
		params->dsi.vertical_frontporch					= 9;//5
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 20;
		params->dsi.horizontal_backporch				= 46;//46
		params->dsi.horizontal_frontporch				= 21;//21
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		// Bit rate calculation
		params->dsi.pll_div1=34;		//0x37// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
		params->dsi.pll_div2=1; 		// div2=0~15: fout=fvo/(2*div2)

}

static unsigned int lcm_compare_id(void);

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(10);//Must over 6 ms,SPEC request
    
	//lcm_compare_id();
	if(!first_inited)
	{
		lcm_select=lcm_judge_id_pin();
		first_inited=1;
	}
	
	#if defined(BUILD_UBOOT)
		printf("[qbq]---1:tdt,0:truly------%s---lcm_select=%d-----\n",__func__,lcm_select);
	#else
		printk("[qbq]---1:tdt,0:truly-----%s----lcm_select=%d------\n",__func__,lcm_select);//legen
	#endif
	if(lcm_select==LCM_TDT)
		push_table(lcm_second_initialization_setting, sizeof(lcm_second_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	else
		push_table(lcm_first_initialization_setting, sizeof(lcm_first_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	SET_RESET_PIN(0);	
	MDELAY(1);	
	SET_RESET_PIN(1);
MDELAY(1);	

	push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);

}


static void lcm_resume(void)
{
	lcm_init();
	//push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
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


static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[2];
	unsigned int array[16];  

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(10);//Must over 6 ms

	array[0]=0x00043902;
	array[1]=0x8983FFB9;// page enable
	dsi_set_cmdq(&array, 2, 1);
	MDELAY(10);

	array[0] = 0x00023700;// return byte number
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xF4, buffer, 2);
	id = buffer[0]; 
	
#if defined(BUILD_UBOOT)
	printf("%s, id = 0x%08x\n", __func__, id);
#endif

#if BUILD_UBOOT
	
	printf("[soso]%s, v1.00 id = 0x%08x\n", __func__, id);
	
#else
	
	printk("%s, v1.00id = 0x%08x\n", __func__, id);
	
#endif

	return (LCM_ID_HX8389B == id)?1:0;

}

static int lcm_judge_id_pin(void)
{
	unsigned int value=0;
	unsigned int cal_num=0;
	int i;
	mt_set_gpio_mode(LCM_ID2_PIN, GPIO_MODE_GPIO);
	mt_set_gpio_dir(LCM_ID2_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(LCM_ID2_PIN, GPIO_PULL_DISABLE);
	for(i=0;i<5;i++)
	{
		value=mt_get_gpio_in(LCM_ID2_PIN);
		if(value)
			cal_num++;
	}

	if(cal_num >= 3)
	{
		#ifndef BUILD_UBOOT)
		printk("---cal_num=%d-------select tdt-----\n",cal_num);
		#endif
		return LCM_TDT;
	}
	else
	{
		#ifndef BUILD_UBOOT)
		printk("---cal_num=%d-------select truly-----\n",cal_num);
		#endif
		return LCM_TRULY;
	}
}
static unsigned int lcm_esd_recover(void)
{	

	lcm_resume();
	
	MDELAY(10);	

	return TRUE;
}
//baoqiang add end


LCM_DRIVER hx8389b_dsi_vdo_lcm_drv = 
{
    .name			= "hx8389b_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
//	.esd_check		= lcm_esd_check,
	.esd_recover 	= lcm_esd_recover,
//	.set_backlight	= lcm_setbacklight,
//	.compare_id     = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
//	.set_backlight	= lcm_setbacklight,
 //   .update         = lcm_update,
#endif
};

