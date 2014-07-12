#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#else

#include <linux/string.h>

#ifdef BUILD_UBOOT
#include <asm/arch/mt6577_gpio.h>
#else
#include <mach/mt6577_gpio.h>
#endif
#endif

#include "lcm_drv.h"
#if !defined(BUILD_LK) && !defined(BUILD_UBOOT)
#include <linux/module.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(800)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0x00   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
    #define   TRUE     1
#endif
 
#ifndef FALSE
    #define   FALSE    0
#endif

#define	GPIO_LCD_ID0_PIN	47
#define	GPIO_LCD_ID1_PIN	50
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

static unsigned int lcd_id_pin = 0;
#if !defined(BUILD_LK) && !defined(BUILD_UBOOT)
module_param_named(lcd_id_pin, lcd_id_pin, uint, S_IRUGO);
#endif
static unsigned int first_inited = 0;

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

//static kal_bool IsFirstBoot = KAL_TRUE;

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[120];
};



static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 0, {0x00}},
    {REGFLAG_DELAY, 10, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
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

		params->dsi.vertical_sync_active				= 5;
		params->dsi.vertical_backporch					= 5;//50
		params->dsi.vertical_frontporch					= 5;//20
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 2;
		params->dsi.horizontal_backporch				= 100;
		params->dsi.horizontal_frontporch				= 200;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;


		// Bit rate calculation
		params->dsi.pll_div1=30;//32		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
		params->dsi.pll_div2=1; 		// div2=0~15: fout=fvo/(2*div2)

		/* ESD or noise interference recovery For video mode LCM only. */ // Send TE packet to LCM in a period of n frames and check the response. 
		params->dsi.lcm_int_te_monitor = FALSE; 
		params->dsi.lcm_int_te_period = 1; // Unit : frames 
 
		// Need longer FP for more opportunity to do int. TE monitor applicably. 
		if(params->dsi.lcm_int_te_monitor) 
			params->dsi.vertical_frontporch *= 2; 
 
		// Monitor external TE (or named VSYNC) from LCM once per 2 sec. (LCM VSYNC must be wired to baseband TE pin.) 
		params->dsi.lcm_ext_te_monitor = TRUE;
		// Non-continuous clock 
		params->dsi.noncont_clock = TRUE; 
		params->dsi.noncont_clock_period = 2; // Unit : frames		
}

static void init_lcm_registers_id10(void)
{
	unsigned int data_array[64];

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
	data_array[1]=0x004400BE;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D1;
	data_array[2]=0x00270003;//00270013
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02D4019B;
	data_array[7]=0x025A0223;
	data_array[8]=0x028E025C;
	data_array[9]=0x03E502C7;
	data_array[10]=0x03140305;
	data_array[11]=0x033B0331;
	data_array[12]=0x03640354;
	data_array[13]=0x03B90374;
	data_array[14]=0x000000FF;//000000E8
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D2;
	data_array[2]=0x00270003;//00270013
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02D4019B;
	data_array[7]=0x025A0223;
	data_array[8]=0x028E025C;
	data_array[9]=0x03E502C7;
	data_array[10]=0x03140305;
	data_array[11]=0x033B0331;
	data_array[12]=0x03640354;
	data_array[13]=0x03B90374;
	data_array[14]=0x000000FF;//000000E8
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D3;
	data_array[2]=0x00270003;//00270013
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02D4019B;
	data_array[7]=0x025A0223;
	data_array[8]=0x028E025C;
	data_array[9]=0x03E502C7;
	data_array[10]=0x03140305;
	data_array[11]=0x033B0331;
	data_array[12]=0x03640354;
	data_array[13]=0x03B90374;
	data_array[14]=0x000000FF;//000000E8
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D4;
	data_array[2]=0x00270003;//00270013
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02D4019B;
	data_array[7]=0x025A0223;
	data_array[8]=0x028E025C;
	data_array[9]=0x03E502C7;
	data_array[10]=0x03140305;
	data_array[11]=0x033B0331;
	data_array[12]=0x03640354;
	data_array[13]=0x03B90374;
	data_array[14]=0x000000FF;//000000E8
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D5;
	data_array[2]=0x00270003;//00270013
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02D4019B;
	data_array[7]=0x025A0223;
	data_array[8]=0x028E025C;
	data_array[9]=0x03E502C7;
	data_array[10]=0x03140305;
	data_array[11]=0x033B0331;
	data_array[12]=0x03640354;
	data_array[13]=0x03B90374;
	data_array[14]=0x000000FF;//000000E8
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D6;
	data_array[2]=0x00270003;//00270013
	data_array[3]=0x006A0046;
	data_array[4]=0x01D500A4;
	data_array[5]=0x0153011E;
	data_array[6]=0x02D4019B;
	data_array[7]=0x025A0223;
	data_array[8]=0x028E025C;
	data_array[9]=0x03E502C7;
	data_array[10]=0x03140305;
	data_array[11]=0x033B0331;
	data_array[12]=0x03640354;
	data_array[13]=0x03B90374;
	data_array[14]=0x000000FF;//000000E8
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

/* add lcd initialization for TIANMA LCD */
static void init_lcm_registers_id01(void)
{
	unsigned int data_array[64];

//******* for TIANMA TFT-RFQ-201301160002 YGT 1 ************//

//*************Enable CMD2 Page1  *******************//
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(data_array, 3, 1);

//************* AVDD: manual  *******************//
	data_array[0]=0x00043902;	//AVDD=5.5V
	data_array[1]=0x0A0A0AB0;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;	//AVDD chan sheng pin lv
	data_array[1]=0x444444B6;
	dsi_set_cmdq(data_array, 2, 1);

//************************************************//
	data_array[0]=0x00353902;
	data_array[1]=0x002300D1;
	data_array[2]=0x003C0039;	 //V1		  1	  30
	data_array[3]=0x00710053;	  //V5		  5	  52
	data_array[4]=0x01D100A8;	//V11		  11	   B0
	data_array[5]=0x013A010E;//V23	 	23		 10
	data_array[6]=0x01A5017A;	//V47 	 47		  77
	data_array[7]=0x021B02EA;	 //V95 	 95		  E0
	data_array[8]=0x0244021B;	 //V128 	 10		  128
	data_array[9]=0x0284026C;	 //V192	  192
	data_array[10]=0x02AB029C;	 //V224 	  224
	data_array[11]=0x02CA02BE;	 //V240  	b8	  240
	data_array[12]=0x02DF02D8;	//V248	 248
	data_array[13]=0x03F602EB;	//V252 	 252
	data_array[14]=0x000000A0;	//V255		60
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x002300D2;
	data_array[2]=0x003C0039;	 //V1		  1	  30
	data_array[3]=0x00710053;	  //V5		  5	  52
	data_array[4]=0x01D100A8;	//V11		  11	   B0
	data_array[5]=0x013A010E;//V23	 	23		 10
	data_array[6]=0x01A5017A;	//V47 	 47		  77
	data_array[7]=0x021B02EA;	 //V95 	 95		  E0
	data_array[8]=0x0244021B;	 //V128 	 10		  128
	data_array[9]=0x0284026C;	 //V192	  192
	data_array[10]=0x02AB029C;	 //V224 	  224
	data_array[11]=0x02CA02BE;	 //V240  	b8	  240
	data_array[12]=0x02DF02D8;	//V248	 248
	data_array[13]=0x03F602EB;	//V252 	 252
	data_array[14]=0x000000A0;	//V255		60
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x002300D3;
	data_array[2]=0x003C0039;	 //V1		  1	  30
	data_array[3]=0x00710053;	  //V5		  5	  52
	data_array[4]=0x01D100A8;	//V11		  11	   B0
	data_array[5]=0x013A010E;//V23	 	23		 10
	data_array[6]=0x01A5017A;	//V47 	 47		  77
	data_array[7]=0x021B02EA;	 //V95 	 95		  E0
	data_array[8]=0x0244021B;	 //V128 	 10		  128
	data_array[9]=0x0284026C;	 //V192	  192
	data_array[10]=0x02AB029C;	 //V224 	  224
	data_array[11]=0x02CA02BE;	 //V240  	b8	  240
	data_array[12]=0x02DF02D8;	//V248	 248
	data_array[13]=0x03F602EB;	//V252 	 252
	data_array[14]=0x000000A0;	//V255		60
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x002300D4;
	data_array[2]=0x003C0039;	 //V1		  1	  30
	data_array[3]=0x00710053;	  //V5		  5	  52
	data_array[4]=0x01D100A8;	//V11		  11	   B0
	data_array[5]=0x013A010E;//V23	 	23		 10
	data_array[6]=0x01A5017A;	//V47 	 47		  77
	data_array[7]=0x021B02EA;	 //V95 	 95		  E0
	data_array[8]=0x0244021B;	 //V128 	 10		  128
	data_array[9]=0x0284026C;	 //V192	  192
	data_array[10]=0x02AB029C;	 //V224 	  224
	data_array[11]=0x02CA02BE;	 //V240  	b8	  240
	data_array[12]=0x02DF02D8;	//V248	 248
	data_array[13]=0x03F602EB;	//V252 	 252
	data_array[14]=0x000000A0;	//V255		60
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x002300D5;
	data_array[2]=0x003C0039;	 //V1		  1	  30
	data_array[3]=0x00710053;	  //V5		  5	  52
	data_array[4]=0x01D100A8;	//V11		  11	   B0
	data_array[5]=0x013A010E;//V23	 	23		 10
	data_array[6]=0x01A5017A;	//V47 	 47		  77
	data_array[7]=0x021B02EA;	 //V95 	 95		  E0
	data_array[8]=0x0244021B;	 //V128 	 10		  128
	data_array[9]=0x0284026C;	 //V192	  192
	data_array[10]=0x02AB029C;	 //V224 	  224
	data_array[11]=0x02CA02BE;	 //V240  	b8	  240
	data_array[12]=0x02DF02D8;	//V248	 248
	data_array[13]=0x03F602EB;	//V252 	 252
	data_array[14]=0x000000A0;	//V255		60
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x002300D6;
	data_array[2]=0x003C0039;	 //V1		  1	  30
	data_array[3]=0x00710053;	  //V5		  5	  52
	data_array[4]=0x01D100A8;	//V11		  11	   B0
	data_array[5]=0x013A010E;//V23	 	23		 10
	data_array[6]=0x01A5017A;	//V47 	 47		  77
	data_array[7]=0x021B02EA;	 //V95 	 95		  E0
	data_array[8]=0x0244021B;	 //V128 	 10		  128
	data_array[9]=0x0284026C;	 //V192	  192
	data_array[10]=0x02AB029C;	 //V224 	  224
	data_array[11]=0x02CA02BE;	 //V240  	b8	  240
	data_array[12]=0x02DF02D8;	//V248	 248
	data_array[13]=0x03F602EB;	//V252 	 252
	data_array[14]=0x000000A0;	//V255		60
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00043902;	//AVEE=-5.5v
	data_array[1]=0x0A0A0AB1;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;	//AVEE chan sheng pin lv
	data_array[1]=0x444444B7;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;	//VGL=-10v
	data_array[1]=0x080808B5;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;	//VGL chan sheng pin lv
	data_array[1]=0x141414BA;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;
	data_array[1]=0x00A000BC;	//0x88=4.7v   a0=5.0v
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;
	data_array[1]=0x00A000BD;	//0x88=4.7v   a0=5.0v
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00033902;
	data_array[1]=0x008000BE;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;	// control for display and application  VCL
	data_array[1]=0x000000B2;	//Generic read/write/Video mode enable/disable VCL=-2.5V
	dsi_set_cmdq(data_array, 2, 1);//00 forward Scan; 06 backward Scan

	data_array[0]=0x00043902;	// control for display and application VCL chan sheng pin lv
	data_array[1]=0x242424B8;	//Generic read/write/Video mode enable/disable VCL=-2.5V
	dsi_set_cmdq(data_array, 2, 1);//00 forward Scan; 06 backward Scan

	data_array[0]=0x00043902;//VGH
	data_array[1]=0x080808B3;//0X08=15V
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;// control for display and application   VGH chan sheng pin lv
	data_array[1]=0x242424B9;
	dsi_set_cmdq(data_array, 2, 1);

// ********************  EABLE CMD2 PAGE 0 **************//

	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000008;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x10B41500;//vivid color
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x50B51500;//480*800
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x05B61500;//source output data hold time
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x00043902;// flicker inversiom
	data_array[1]=0x000002BC;//00 coloum 	02  2dot
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;// display option control
	data_array[1]=0x0000FCB1;//BGR  RGB,top-down,s1-s1440
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00063902;//Display Timing
	data_array[1]=0x500200C9;
	data_array[2]=0x00005050;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x00033902;//Set Gate signal control
	data_array[1]=0x000000B7;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00053902;//Set Source signal control
	data_array[1]=0x070701B8;
	data_array[2]=0x00000007;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00103902;
	data_array[1] = 0x004063F7;
	data_array[2] = 0xC4010000;
	data_array[3] = 0x640200A2;
	data_array[4] = 0xD0004854;//0x54,0x48,0x00,0xD0
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0]=0x00351500;//Enable TE signal output
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0]=0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(150);

}

/* add lcd initialization for NT35512_TD-TNWV4003-35-A1 */
static void init_lcm_registers_id00(void)
{
	unsigned int data_array[64];

//******* for NT35512_TD-TNWV4003-35-A1 ************//

	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(data_array, 3, 1);

	//#VGMP/VGSP 4.5V/0V
	data_array[0]=0x00043902;
	data_array[1]=0x1A8C00BC;
	dsi_set_cmdq(data_array, 2, 1);

	//#VGMN/VGSN -4.5V/0V
	data_array[0]=0x00043902;
	data_array[1]=0x1A8C00BD;
	dsi_set_cmdq(data_array, 2, 1);

	//#VCOM, 0x64
	data_array[0]=0x00033902;
	data_array[1]=0x004800BE;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D1;
	data_array[2]=0x00080001;
	data_array[3]=0x00290016;

	data_array[4]=0x00950054;
	data_array[5]=0x014301D6;
	data_array[6]=0x02F6019E;
	data_array[7]=0x0287024F;

	data_array[8]=0x02BD0288;
	data_array[9]=0x030203EA;
	data_array[10]=0x03320323;
	data_array[11]=0x0353034A;

	data_array[12]=0x037C036B;
	data_array[13]=0x03C90395;
	data_array[14]=0x000000FF;
	dsi_set_cmdq(data_array, 15, 1);


	data_array[0]=0x00353902;
	data_array[1]=0x000000D2;
	data_array[2]=0x00080001;
	data_array[3]=0x00290016;

	data_array[4]=0x00950054;
	data_array[5]=0x014301D6;
	data_array[6]=0x02F6019E;
	data_array[7]=0x0287024F;

	data_array[8]=0x02BD0288;
	data_array[9]=0x030203EA;
	data_array[10]=0x03320323;
	data_array[11]=0x0353034A;

	data_array[12]=0x037C036B;
	data_array[13]=0x03C90395;
	data_array[14]=0x000000FF;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D3;
	data_array[2]=0x00080001;
	data_array[3]=0x00290016;

	data_array[4]=0x00950054;
	data_array[5]=0x014301D6;
	data_array[6]=0x02F6019E;
	data_array[7]=0x0287024F;

	data_array[8]=0x02BD0288;
	data_array[9]=0x030203EA;
	data_array[10]=0x03320323;
	data_array[11]=0x0353034A;

	data_array[12]=0x037C036B;
	data_array[13]=0x03C90395;
	data_array[14]=0x000000FF;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D4;
	data_array[2]=0x00080001;
	data_array[3]=0x00290016;

	data_array[4]=0x00950054;
	data_array[5]=0x014301D6;
	data_array[6]=0x02F6019E;
	data_array[7]=0x0287024F;

	data_array[8]=0x02BD0288;
	data_array[9]=0x030203EA;
	data_array[10]=0x03320323;
	data_array[11]=0x0353034A;

	data_array[12]=0x037C036B;
	data_array[13]=0x03C90395;
	data_array[14]=0x000000FF;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D5;
	data_array[2]=0x00080001;
	data_array[3]=0x00290016;

	data_array[4]=0x00950054;
	data_array[5]=0x014301D6;
	data_array[6]=0x02F6019E;
	data_array[7]=0x0287024F;

	data_array[8]=0x02BD0288;
	data_array[9]=0x030203EA;
	data_array[10]=0x03320323;
	data_array[11]=0x0353034A;

	data_array[12]=0x037C036B;
	data_array[13]=0x03C90395;
	data_array[14]=0x000000FF;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0]=0x00353902;
	data_array[1]=0x000000D6;
	data_array[2]=0x00080001;
	data_array[3]=0x00290016;

	data_array[4]=0x00950054;
	data_array[5]=0x014301D6;
	data_array[6]=0x02F6019E;
	data_array[7]=0x0287024F;

	data_array[8]=0x02BD0288;
	data_array[9]=0x030203EA;
	data_array[10]=0x03320323;
	data_array[11]=0x0353034A;

	data_array[12]=0x037C036B;
	data_array[13]=0x03C90395;
	data_array[14]=0x000000FF;
	dsi_set_cmdq(data_array, 15, 1);

	//************* AVDD: manual  *******************//
	//#AVDD Set AVDD 5.2V
	data_array[0]=0x00043902;
	data_array[1]=0x0D0D0DB0;
	dsi_set_cmdq(data_array, 2, 1);

	//#AVEE  -5.2V 
	data_array[0]=0x00043902;//AVEE voltage, Set AVEE -6V
	data_array[1]=0x0D0D0DB1;
	dsi_set_cmdq(data_array, 2, 1);

	//#AVDD ratio
	data_array[0]=0x00043902;
	data_array[1]=0x444444B6;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;//AVEE: manual, -6V
	data_array[1]=0x353535B7;
	dsi_set_cmdq(data_array, 2, 1);

	//#VCL ratio
	//data_array[0]=0x00043902;
	//data_array[1]=0x262626B8;
	//dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;//VGH: Clamp Enable
	data_array[1]=0x373737B9;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00043902;//VGL(LVGL)
	data_array[1]=0x161616BA;
	dsi_set_cmdq(data_array, 2, 1);

	// ********************  EABLE CMD2 PAGE 0 **************//

	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000008;
	dsi_set_cmdq(data_array, 3, 1);

	//#Display control
	data_array[0]=0xFCB11500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x10B41500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x04B61500;//#Source hold time
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x00033902;//Set Gate EQ
	data_array[1]=0x000000B7;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00053902;//Set Source EQ
	data_array[1]=0x070701B8;
	data_array[2]=0x00000007;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x00043902;//Inversion: Column inversion (NVT)
	data_array[1]=0x040404BC;
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

	data_array[0]=0x773A1500;//16.7M color
	dsi_set_cmdq(data_array, 1, 1);

	//data_array[0]=0x00211500;//Start GRAM write
	//dsi_set_cmdq(data_array, 1, 1);

	//Page 0 2
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000008;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x02C71500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x11C91500;//#Timing control 4H w/ 4-delay
	dsi_set_cmdq(data_array, 1, 1);



	data_array[0]=0x000D3902;
	data_array[1]=0xE4E401CA;
	data_array[2]=0xE4E4E4E4;
	data_array[3]=0x000808E4;
	data_array[4]=0x00000000;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0]=0x00033902;//#Display control
	data_array[1]=0x0000FCB1;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x11111500;
	dsi_set_cmdq(data_array, 1, 1);
	//MDELAY(150);

	data_array[0]=0x29291500;
	dsi_set_cmdq(data_array, 1, 1);
	//MDELAY(300);

	//*************Enable CMD2 Page1  *******************//
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(data_array, 3, 1);


	data_array[0]=0x00053902;//Set Source EQ
	data_array[1]=0xA555AAFF;
	data_array[2]=0x00000080;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x00103902;//Set Source EQ
	data_array[1]=0x004063F7;
	data_array[2]=0xC4010000;
	data_array[3]=0x640200A2;
	data_array[4]=0xD0004854;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0]=0x00053902;//Set Source EQ
	data_array[1]=0xA555AAFF;
	data_array[2]=0x00000000;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000008;
	dsi_set_cmdq(data_array, 3, 1);
}
/* add lcd initialization end */

extern void esd_recovery_pause(unsigned char en);
/* be compatible with TD-TNWV4003-35-A1 and TD-TNWV4003-35 */
static void init_lcm_registers(void)
{
	unsigned int lcd_id0_pin = 0;
	unsigned int lcd_id1_pin = 0;
#if !defined(BUILD_UBOOT) && !defined(BUILD_LK)
	unsigned char buffer[3];
	unsigned int array[16];
#endif

	/* because lcm_compare_id() may not be called,so do check id pin here! */
	if (!first_inited)
	{
		/* get lcd id pin to distinguish TD-TNWV4003-35 and TD-TNWV4003-35-A1*/
		mt_set_gpio_mode(GPIO_LCD_ID0_PIN, GPIO_MODE_GPIO);
		mt_set_gpio_dir(GPIO_LCD_ID0_PIN, GPIO_DIR_IN);
		mt_set_gpio_pull_enable(GPIO_LCD_ID0_PIN, GPIO_PULL_DISABLE);
		lcd_id0_pin = mt_get_gpio_in(GPIO_LCD_ID0_PIN);

		mt_set_gpio_mode(GPIO_LCD_ID1_PIN, GPIO_MODE_GPIO);
		mt_set_gpio_dir(GPIO_LCD_ID1_PIN, GPIO_DIR_IN);
		mt_set_gpio_pull_enable(GPIO_LCD_ID1_PIN, GPIO_PULL_DISABLE);
		lcd_id1_pin = mt_get_gpio_in(GPIO_LCD_ID1_PIN);

		lcd_id_pin = (lcd_id1_pin << 1) | lcd_id0_pin;
		first_inited = 1;

/* add by zhiping.liu for disable esd recover when no connect lcd @start */
#if !defined(BUILD_UBOOT) && !defined(BUILD_LK)
		buffer[1] = 0x00;
		array[0] = 0x00033700;// read id return two byte,version and id
		dsi_set_cmdq(array, 1, 1);
		read_reg_v2(0x04, buffer, 3);
		printk("[LCD Driver ID]id0 = 0x%02x,id1 = 0x%02x,id2 = 0x%02x",buffer[0],buffer[1],buffer[2]);
		/* we only need check id */
		if (buffer[1] != 0x80)
		{/* if id is error, pause esd recover thread */
			esd_recovery_pause(TRUE);
		}
#endif
/* add by zhiping.liu for disable esd recover when no connect lcd @end */
	}

	if (lcd_id_pin == 0){//id = 00b
		init_lcm_registers_id00();
	}
	else if (lcd_id_pin == 0x01) {//id = 01b
		init_lcm_registers_id01();
	}
	else {//id = 10b
		init_lcm_registers_id10();
	}
}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(120);//Must > 120ms

    init_lcm_registers();
}


static void lcm_suspend(void)
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(20);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(150);//Must > 120ms

	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
/*
#ifdef BUILD_LK
#else
	mt_set_gpio_mode(GPIO68, 0);
	mt_set_gpio_dir(GPIO68, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO68, GPIO_OUT_ZERO);
#endif
*/
}


static void lcm_resume(void)
{
	//lcm_compare_id();
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(10);//Must > 10ms
	SET_RESET_PIN(1);
	MDELAY(20);

	init_lcm_registers();

	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}
static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[3];
	unsigned int array[16];
	
	SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);

	array[0] = 0x00033700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0x04, buffer, 3);
	id = buffer[1]; //we only need ID
#if defined(BUILD_UBOOT)
	/*The Default Value should be 0x00,0x80,0x00*/
	//printf("\n\n\n\n[soso]%s, id0 = 0x%08x,id1 = 0x%08x,id2 = 0x%08x\n", __func__, buffer[0],buffer[1],buffer[2]);
#endif
    //return (id == 0x80)?1:0;
    return 1;
}

static unsigned int lcm_esd_recover()
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(150);//make reset time as 150ms for ESD recover
    SET_RESET_PIN(1);
    MDELAY(120);//Must > 120ms

    init_lcm_registers();

    return TRUE;
}

LCM_DRIVER nt35512_dsi_vdo_lcm_drv = 
{
    .name			= "nt35512_dsi_vdo_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.esd_recover	= lcm_esd_recover,
	.compare_id    = lcm_compare_id,
};

