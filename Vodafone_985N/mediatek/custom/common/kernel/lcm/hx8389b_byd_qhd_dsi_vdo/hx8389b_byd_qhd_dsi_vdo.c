#ifndef BUILD_LK
    #include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
    #include <string.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH           	        (540)
#define FRAME_HEIGHT 			(960)

#define LCM_ID_HX8389B 			 0x89


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

static unsigned int lcm_esd_test = FALSE;      ///only for ESD test

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util ;

#define SET_RESET_PIN(v)        (lcm_util.set_reset_pin((v)))
#define UDELAY(n)               (lcm_util.udelay(n))
#define MDELAY(n)               (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)                     lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)							lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)							lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE						0

static LCM_setting_table_V3 lcm_initialization_setting[] = {
	
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
	//initial_code LCM has power IC
	{0x39,0xB9,3,{0xFF,0x83,0x89}},
	  	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 1, {}},

	{0x39,0xBA,7,{0x41,0x93,0x00,0x16,0xA4,0x10,0x18}},
		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 1, {}},

	{0x15,0xC6,1,{0x08}},
		//{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 1, {}},
	{0x15,0XCC,1,{0X02}},
		//{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 1, {}},

	{0x39,0xB1,19,{0X00,0X00,0X06,0XE8,0X59,0X10,0X11,0XB3,0XEF,0X24,
		       0X2C,0X3F,0X3F,0X43,0X01,0X58,0XF0,0X00,0XE6}},
		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 10, {}},

 	{0x39,0xB2,7,{0X00,0X00,0X78,0X0C,0X07,0X3F,0X80}},
		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 1, {}},

 	{0x39,0xB4,23,{0X80,0X08,0X00,0X32,0X10,0X04,0X32,0X10,0X00,0X32,
		       0X10,0X00,0X37,0X0A,0X40,0X08,0X37,0X0A,0X40,0X14,
		       0X46,0X50,0X0A}},		
 		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 10, {}}, 
	
 	{0x39,0xD5,56,{0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x60,0x00,
		       0x99,0x88,0xAA,0xBB,0x88,0x23,0x88,0x01,0x88,0x67,
 	 	       0x88,0x45,0x01,0x23,0x88,0x88,0x88,0x88,0x88,0x88,
		       0x99,0xBB,0xAA,0x88,0x54,0x88,0x76,0X88,0X10,0X88,
 	               0x32,0x32,0x10,0x88,0x88,0x88,0x88,0x88,0X00,0X04,
                       0X00,0X00,0X00,0X00,0X00,0X00}},		
 		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 10, {}},

	{0x39,0xCB,1,{0x07,0x07}},
		//{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 1, {}},

	{0x39,0XBB,4,{0X00,0X00,0XFF,0X80}},
		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 1, {}},
  
	{0x39,0XDE,3,{0X05,0X58,0X10}},  
		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 1, {}},

	{0x39,0XB6,4,{0X00,0X88,0X00,0X88}},
		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 1, {}},

	{0x39,0xE0,34,{0x01,0x08,0x08,0x1F,0x25,0x36,0x16,0x39,0x09,0x0E,
		       0X0E,0X10,0x13,0x11,0x11,0x10,0x1D,0x01,0x08,0x08,
		       0x1F,0x25,0X39,0X16,0x39,0x09,0x0E,0x0E,0x10,0x13,
		       0x11,0x11,0x10,0x1D}},
		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 10, {}},

	{0x05,0x11,0,{}},		
		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 150, {}},

	{0x05,0x29,0,{}},
		{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 10, {}},

};


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

        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
        #else
		params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
        #endif
	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
		params->dsi.vertical_sync_active				= 0x02;     // 2
		params->dsi.vertical_backporch					= 0x0E;	    // 15
		params->dsi.vertical_frontporch					= 0x09;     // 9
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 0x60;     //96 
		params->dsi.horizontal_backporch				= 0x60;     //96
		params->dsi.horizontal_frontporch				= 0x30;     //48
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		// Bit rate calculation

		params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4
		params->dsi.pll_div2=1;		// div2=0,1,2,3;div1_real=1,2,4,4	
		params->dsi.fbk_div =20;

				
}

static void lcm_init(void)
{


    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(20);                 	//Must over 6 ms,SPEC request
	
    dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
		

    
}



static void lcm_suspend(void)
{
	unsigned int data_array[16];
	SET_RESET_PIN(1);
   	SET_RESET_PIN(0);
   	MDELAY(20);
   	SET_RESET_PIN(1);
    	MDELAY(20); 
   
	data_array[0] = 0x00280500;         // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x00100500;         // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120); 

	
}


static void lcm_resume(void)
{
	lcm_init();

    #ifdef BUILD_LK
	  printf("[LK]------hx8389b----%s------\n",__func__);
    #else
	  printk("[KERNEL]------hx8389b----%s------\n",__func__);
    #endif	
}
         
#if (LCM_DSI_CMD_MODE)
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
#endif

static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[2];
	unsigned int array[16];  

    	SET_RESET_PIN(1);
   	SET_RESET_PIN(0);
   	MDELAY(20);
   	SET_RESET_PIN(1);
    	MDELAY(120);                     //Must over 6 ms

	array[0]=0x00043902;
	array[1]=0x8983FFB9;            // page enable
	dsi_set_cmdq(&array, 2, 1);
	MDELAY(20);
	
	/*array[0]=0x00083902;
	array[1]=0x009341BA;
	array[2]=0x1800A416;
	dsi_set_cmdq(&array, 3, 1);
	MDELAY(20);*/

	array[0] = 0x00023700;          // return byte number
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(20);

	read_reg_v2(0xF4, buffer, 2);
	id = buffer[0]; 
	
	#ifdef BUILD_LK
		printf("lcm_compare_id hx8389b uboot %s \n", __func__);
		printf("lcm_compare_id %s id = 0x%08x \n", __func__, id);
	#else
		printk("lcm_compare_id hx8389b uboot %s \n", __func__);
		printk("lcm_compare_id %s id = 0x%08x \n", __func__, id);
	#endif

	return (LCM_ID_HX8389B == id)?1:0;

}


LCM_DRIVER hx8389b_byd_qhd_dsi_vdo_lcm_drv = 
{
    .name			= "hx8389b_byd_dsi_vdo_RIO5",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
    };
