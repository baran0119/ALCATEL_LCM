#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

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
#define FRAME_WIDTH  											(720)
#define FRAME_HEIGHT 											(1280)

#define REGFLAG_DELAY             							0xFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

#define SET_RESET_PIN(v)    									(lcm_util.set_reset_pin((v)))
#define SET_GPIO_OUT(n, v)	        							(lcm_util.set_gpio_out((n), (v)))
#define UDELAY(n) 												(lcm_util.udelay(n))
#define MDELAY(n) 												(lcm_util.mdelay(n))

#define LCM_ID                                                                                 (0x01)

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)			lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)											lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)						lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

static LCM_UTIL_FUNCS   										lcm_util = {0};

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
struct LCM_setting_table {
unsigned char cmd;
unsigned char count;
unsigned char para_list[64];
};

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

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

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));
	params->type   = LCM_TYPE_DSI;
	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->dsi.mode   					= SYNC_PULSE_VDO_MODE;
	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;//LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;
	// Video mode setting
	params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count=720*3;

	params->dsi.vertical_sync_active	= 2;
	params->dsi.vertical_backporch		= 8;
	params->dsi.vertical_frontporch		= 6;
	params->dsi.vertical_active_line	= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active	= 60;
	params->dsi.horizontal_backporch	= 60;
	params->dsi.horizontal_frontporch	= 107;
	params->dsi.horizontal_active_pixel	= FRAME_WIDTH;

	// Bit rate calculation
	params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
	params->dsi.pll_div2=1;		// div2=0,1,2,3;div1_real=1,2,4,4
	params->dsi.fbk_div =16;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)
}

static struct LCM_setting_table lcm_initialization_ret[] = {
	/*Note :
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

	{0xB9,	3,	{0xFF, 0x83, 0x94}},
	{0xC7,	4,	{0x00, 0x10, 0x00, 0x10}},
	{0xBC,	1,	{0x07}},
	{0xBA,	1,	{0x13}},
	{0xB1,	15,	{0x01, 0x00, 0x07, 0x84, 0x01, 0x0E, 
				 0x0E, 0x32, 0x38, 0x29, 0x29, 0x50, 
				 0x02, 0x00, 0x00}},
	{0xB2,	6,	{0x00, 0xC8, 0x09, 0x05, 0x00, 0x71}},
	{0xCC,	1,	{0x09}},

	{0x00,	0,	{0x00}},

	{0xD5,	52,	{0x00,0x00,0x00,0x00,0x0A,0x00,0x01,
				 0x00,0x00,0x00,0x33,0x00,0x23,0x45,
				 0x67,0x01,0x01,0x23,0x88,0x88,0x88,
				 0x88,0x88,0x88,0x88,0x99,0x99,0x99,
				 0x88,0x88,0x99,0x88,0x54,0x32,0x10,
				 0x76,0x32,0x10,0x88,0x88,0x88,0x88,
				 0x88,0x88,0x88,0x99,0x99,0x99,0x88,
				 0x88,0x88,0x99}},
	
	{0xB4,	22,	{0x80, 0x08, 0x32, 0x10, 0x00, 0x32,
				 0x15, 0x08, 0x32, 0x12, 0x20, 0x33, 
				 0x05, 0x4C, 0x05, 0x37, 0x05, 0x3F, 
				 0x1E, 0x5F, 0x5F, 0x06}},

	{0xB6,	1,	{0x00}},

	{0xE0,	34,	{0x01, 0x0b, 0x10, 0x25, 0x35, 0x3F, 
				 0x15, 0x36, 0x04, 0x09, 0x0E, 0x10,
				 0x13, 0x10, 0x14, 0x16, 0x1B, 0x01, 
				 0x0b, 0x10, 0x25, 0x35, 0x3F, 0x15, 
				 0x36, 0x04, 0x09, 0x0E, 0x10, 0x13, 
				 0x10, 0x14, 0x16, 0x1B}},

	{0xBF,	3,	{0x06,0x00,0x10}},

	{0x11,	1,	{0x00}},
	{REGFLAG_DELAY, 200, {}},
	{0x29,	1,	{0x00}},
	{REGFLAG_DELAY, 50, {}},


	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_init(void)
{
	unsigned int data_array[16]; 

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);	
	MDELAY(50);

	data_array[0]=0x00043902;
	data_array[1]=0x9483FFB9;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x13BA1502;
	dsi_set_cmdq(&data_array,1,1);

	data_array[0]=0x00113902;
	data_array[1]=0x040001B1;
	data_array[2]=0x11110187;
	data_array[3]=0x3F3F372F;
	data_array[4]=0xE6010247;
	data_array[5]=0x000000E2;//ADD for support new tdt 
	dsi_set_cmdq(&data_array,6,1);

	data_array[0]=0x00073902;
	data_array[1]=0x08C800B2;
	data_array[2]=0x00220004;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00173902;
	data_array[1]=0x320680B4;
	data_array[2]=0x15320310;
	data_array[3]=0x08103208;
	data_array[4]=0x05430433;
	data_array[5]=0x06430437;
	data_array[6]=0x00066161;
	dsi_set_cmdq(&data_array,7,1);
	
	data_array[0]=0x00053902;
	data_array[1]=0x100006BF;
	data_array[2]=0x00000004;
	dsi_set_cmdq(&data_array,3,1);

	data_array[0]=0x00033902;
	data_array[1]=0x00170CC0;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00000BB6;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00213902;
	data_array[1]=0x000000D5;
	data_array[2]=0x01000A00;
	data_array[3]=0x0000CC00;
	data_array[4]=0x88888800;
	data_array[5]=0x88888888;
	data_array[6]=0x01888888;
	data_array[7]=0x01234567;
	data_array[8]=0x88888823;
	data_array[9]=0x00000088;
	dsi_set_cmdq(&data_array,10,1);

	data_array[0]=0x00023902;
	data_array[1]=0x000009CC;
	dsi_set_cmdq(&data_array,2,1);

	data_array[0]=0x00053902;
	data_array[1]=0x001000C7;
	data_array[2]=0x00000010;
	dsi_set_cmdq(&data_array,3,1);	

	data_array[0]=0x002B3902;
	data_array[1]=0x060400E0;
	data_array[2]=0x133F332B;
	data_array[3]=0x0D0E0A34;
	data_array[4]=0x13111311;
	data_array[5]=0x04001710;
	data_array[6]=0x3F332B06;
	data_array[7]=0x0e0a3413;
	data_array[8]=0x1113110D;
	data_array[9]=0x0B171013;
	data_array[10]=0x0B110717;
	data_array[11]=0x00110717;
	dsi_set_cmdq(&data_array,12,1);
	MDELAY(5);

	data_array[0]=0x00110500;
	dsi_set_cmdq(&data_array,1,1);
	MDELAY(200);
	data_array[0]=0x00290500;
	dsi_set_cmdq(&data_array,1,1);
	MDELAY(120);
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0]=0x00280500; 
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);

	data_array[0] = 0x00100500; 
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	SET_RESET_PIN(0);
}

static void lcm_resume(void)
{
    	unsigned int data_array[16];

	lcm_init();

	//data_array[0]=0x00110500;
	//dsi_set_cmdq(&data_array,1,1);
	//MDELAY(120);
	//data_array[0]=0x00290500;
	//dsi_set_cmdq(&data_array,1,1);
	//MDELAY(20);
}

static int dummy_delay = 0;
static unsigned int lcm_esd_check(void)
{  
    #ifndef BUILD_LK
    unsigned int  data_array[16];
    unsigned char buffer_0a;
    unsigned char buffer_0b;
    unsigned char buffer_0c;
    unsigned char buffer_0d;
    unsigned int retval = 0;
    
    dummy_delay ++;
    
    if (dummy_delay >=10000)
        dummy_delay = 0;

    if(dummy_delay %2 == 0)
    {    
        //printk("%s return 1\n",__FUNCTION__);

	    data_array[0] = 0x00013700;
	    dsi_set_cmdq(data_array, 1, 1);
	    read_reg_v2(0x0A,&buffer_0a, 1);

	    data_array[0] = 0x00013700;
	    dsi_set_cmdq(data_array, 1, 1);
	    read_reg_v2(0x0B,&buffer_0b, 1);

	    data_array[0] = 0x00013700;
	    dsi_set_cmdq(data_array, 1, 1);
	    read_reg_v2(0x0C,&buffer_0c, 1);

	    data_array[0] = 0x00013700;
	    dsi_set_cmdq(data_array, 1, 1);
	    read_reg_v2(0x0D,&buffer_0d, 1);

#if defined(BUILD_LK)
            printf("BUILD_LK lcm_esd_check lcm 0x0A is %x-----------------\n", buffer_0a);
	    printf("lcm_esd_check lcm 0x0B is %x-----------------\n", buffer_0b);
	    printf("lcm_esd_check lcm 0x0C is %x-----------------\n", buffer_0c);
	    printf("lcm_esd_check lcm 0x0D is %x-----------------\n", buffer_0d);
#else
            printk("lcm_esd_check lcm 0x0A is %x-----------------\n", buffer_0a);
	    printk("lcm_esd_check lcm 0x0B is %x-----------------\n", buffer_0b);
	    printk("lcm_esd_check lcm 0x0C is %x-----------------\n", buffer_0c);
	    printk("lcm_esd_check lcm 0x0D is %x-----------------\n", buffer_0d);
#endif       
	    if ((buffer_0a==0x1C)&&(buffer_0b==0x00)&&(buffer_0c==0x70)&&(buffer_0d==0x00)){
		    //printk("diablox_lcd lcm_esd_check done\n");
		    retval = 0;
	    }else{
		    //printk("diablox_lcd lcm_esd_check return true\n");
		    retval = 1;
	    }
    }

	return retval;
    #endif
}

static unsigned int lcm_esd_recover(void)
{
    //printk("%s \n",__FUNCTION__);
    
    //lcm_resume();
    lcm_init();

    return 1;
}

#if 1
static unsigned int lcm_compare_id(void)
{
	unsigned int data_array[16];
	unsigned int id=0;
        unsigned char buff[3];
        unsigned char id_04;
        
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);	
	MDELAY(50);
        
 	data_array[0]=0x00043902;
	data_array[1]=0x9483FFB9;
	dsi_set_cmdq(&data_array,2,1);
	MDELAY(10);

	data_array[0]=0x13BA1502;
	dsi_set_cmdq(&data_array,1,1);
	MDELAY(10);
    
	data_array[0] = 0x00033700;
	dsi_set_cmdq(&data_array, 1, 1);
    	MDELAY(10); 
        
	read_reg_v2(0x04,buff, 3);
        id_04 = buff[0];
        
	mt_set_gpio_mode(142,0);  // gpio mode   high
	mt_set_gpio_pull_enable(142,0);
	mt_set_gpio_dir(142,0);  //input
	id = mt_get_gpio_in(142);//should be 0

#if defined(BUILD_LK)
        printf(" BUILD_LK m_compare_id id_04 =  %x,buff = %x,%x.%x-----------------\n", id_04,buff[0],buff[1],buff[2]);
        printf("BUILD_LK cm_compare_id id =  %x-----------------\n", id);
#elif defined(BUILD_UBOOT)
        printf(" BUILD_UBOOT m_compare_id id_04 =  %x,buff = %x,%x.%x-----------------\n", id_04,buff[0],buff[1],buff[2]);
        printf("BUILD_UBOOT cm_compare_id id =  %x-----------------\n", id);
#else
        printk(" lcm_compare_id id_04 =  %x,buff = %x,%x.%x-----------------\n", id_04,buff[0],buff[1],buff[2]);
        printk("lcm_compare_id id =  %x-----------------\n", id);
#endif
         
        if((id_04 & 0xff) == 0x1C) 
        {
            return 0;
        }
	else if(id == LCM_ID)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
#endif
//for new patch ,modify none
LCM_DRIVER hx8394a_vdo_diabloalpha_second_lcm_drv =
{
	.name			= "hx8394a_vdo_diabloalpha_tdt_second",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id   = lcm_compare_id,
	.esd_check		= lcm_esd_check,
	.esd_recover	= lcm_esd_recover,
};
