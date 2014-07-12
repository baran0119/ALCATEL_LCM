

#include <linux/string.h>

#include "lcm_drv.h"


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (320)
#define FRAME_HEIGHT (480)
#define LCM_ID       (0x9481)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

static __inline unsigned int HIGH_BYTE(unsigned int val)
{
    return (val >> 8) & 0xFF;
}

static __inline unsigned int LOW_BYTE(unsigned int val)
{
    return (val & 0xFF);
}

static __inline void send_ctrl_cmd(unsigned int cmd)
{
    lcm_util.send_cmd(cmd);
}

static __inline void send_data_cmd(unsigned int data)
{
    lcm_util.send_data(data);
}

static __inline unsigned int read_data_cmd()
{
    return 0xFF&lcm_util.read_data();
}

static __inline void set_lcm_register(unsigned int regIndex,
                                      unsigned int regData)
{
    send_ctrl_cmd(regIndex);
    send_data_cmd(regData);
}


static void init_lcm_registers(void)
{
#if 0
	//waiting for internal operation,* Must be satisfied*

	//POWER SETTING
	send_ctrl_cmd(0xD0);
	send_data_cmd(0x07);
	send_data_cmd(0x41);
	send_data_cmd(0x1b);

	send_ctrl_cmd(0xD1);
	send_data_cmd(0x00);
	send_data_cmd(0x3f);//vcomh 33
	send_data_cmd(0x1b);//vdv

	send_ctrl_cmd(0xD2);
	send_data_cmd(0x01);
	send_data_cmd(0x11);

	//PANEL SETTING
	send_ctrl_cmd(0xC0);
	send_data_cmd(0x10);//10
	send_data_cmd(0x3B);
	send_data_cmd(0x00);
 	send_data_cmd(0x12);//12
	send_data_cmd(0x01);

//Default setting
//
	//Display_Timing_Setting for Normal Mode
	send_ctrl_cmd(0xC1);		//
	send_data_cmd(0x10);	//Line inversion,DIV1[1:0]
	send_data_cmd(0x13);	//RTN1[4:0]
	send_data_cmd(0x88);	//BP and FP

//DISPLAY MODE SETTING
	send_ctrl_cmd(0xC5);		//Frame rate and Inversion Control
	send_data_cmd(0x02);

//======RGB IF setting========
//RGB OR SYS INTERFACE
	send_ctrl_cmd(0xB4);
	send_data_cmd(0x00);//SYS
	//send_data_cmd(0x10);//RGB

//send_ctrl_cmd(0xC6);
//	send_data_cmd(0x00);//
//============================

//
//Gamma2.5 setting
	send_ctrl_cmd(0xC8);
	send_data_cmd(0x00);//KP1[2:0];KP0[2:0]
	send_data_cmd(0X46);//KP3[2:0];KP2[2:0]
	send_data_cmd(0X34);//KP5[2:0];KP4[2:0]
	send_data_cmd(0X10);//21 RP1[2:0];RP0[2:0]
	send_data_cmd(0X0f);//0a 01VRP0[3:0]
	send_data_cmd(0X02);//04 16VRP1[4:0]
	send_data_cmd(0X34);//KN1[2:0];KN0[2:0]
	send_data_cmd(0X13);//KN3[2:0];KN2[2:0]
	send_data_cmd(0X77);//KN5[2:0];KN4[2:0]
	send_data_cmd(0X01);//13 RN1[2:0];RN0[2:0]
	send_data_cmd(0X00);//02 0a 06 06 VRN0[3:0]
	send_data_cmd(0X1f);//0c 00 VRN1[4:0]

////Internal command
	//send_ctrl_cmd(0xE4);		//Internal LSI TEST Registers
	//send_data_cmd(0xA0);

	//send_ctrl_cmd(0xF0);		//Internal LSI TEST Registers
	//send_data_cmd(0x01);

	send_ctrl_cmd(0xF3);		//Internal LSI TEST Registers
	send_data_cmd(0x40);
	send_data_cmd(0x0A);

	send_ctrl_cmd(0xF6);		//Internal LSI TEST Registers
	send_data_cmd(0x80);

	send_ctrl_cmd(0xF7);		//Internal LSI TEST Registers
	send_data_cmd(0x80);

	send_ctrl_cmd(0x35);		//Set_address_mode
	send_data_cmd(0x00);	//1B

	send_ctrl_cmd(0x36);		//Set_address_mode
	send_data_cmd(0x0a);	//1B

	send_ctrl_cmd(0x3A);		//Set_pixel_format
	send_data_cmd(0x66);	//5-16bit,6-18bit

//Address
	send_ctrl_cmd(0x2A);		//Set_column_address
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x01);
	send_data_cmd(0x3F);

	send_ctrl_cmd(0x2B);		//Set_page_address
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x01);
	send_data_cmd(0xDF);

	send_ctrl_cmd(0x13);		//NORMAL DISPLAY
	MDELAY(100);

	send_ctrl_cmd(0x11);		//Sleep out
	MDELAY(200);

	send_ctrl_cmd(0x29);		//Display on

	send_ctrl_cmd(0x2C);	//Write_memory_start
#else
	//waiting for internal operation,* Must be satisfied*      //gamma 2.2

	//POWER SETTING
	send_ctrl_cmd(0xD0);
	send_data_cmd(0x07);
	send_data_cmd(0x41);
	send_data_cmd(0x1b);

	send_ctrl_cmd(0xD1);
	send_data_cmd(0x00);
	send_data_cmd(0x3f);//vcomh 33
	send_data_cmd(0x1b);//vdv

	send_ctrl_cmd(0xD2);
	send_data_cmd(0x01);
	send_data_cmd(0x11);

	//PANEL SETTING
	send_ctrl_cmd(0xC0);
	send_data_cmd(0x10);//10
	send_data_cmd(0x3B);
	send_data_cmd(0x00);
 	send_data_cmd(0x12);//12
	send_data_cmd(0x01);

//Default setting
//
	//Display_Timing_Setting for Normal Mode
	send_ctrl_cmd(0xC1);		//
	send_data_cmd(0x10);	//Line inversion,DIV1[1:0]
	send_data_cmd(0x13);	//RTN1[4:0]
	send_data_cmd(0x88);	//BP and FP

//DISPLAY MODE SETTING
	send_ctrl_cmd(0xC5);		//Frame rate and Inversion Control
	send_data_cmd(0x02);

//======RGB IF setting========
//RGB OR SYS INTERFACE
	send_ctrl_cmd(0xB4);
	send_data_cmd(0x00);//SYS
	//send_data_cmd(0x10);//RGB

//send_ctrl_cmd(0xC6);
//	send_data_cmd(0x00);//
//============================

//
//Gamma2.5 setting
	send_ctrl_cmd(0xC8);
	send_data_cmd(0x10);//KP1[2:0];KP0[2:0]
	send_data_cmd(0X17);//KP3[2:0];KP2[2:0]
	send_data_cmd(0X30);//KP5[2:0];KP4[2:0]
	send_data_cmd(0X02);//21 RP1[2:0];RP0[2:0]
	send_data_cmd(0X0f);//0a 01VRP0[3:0]
	send_data_cmd(0X00);//04 16VRP1[4:0]
	send_data_cmd(0X75);//KN1[2:0];KN0[2:0]
	send_data_cmd(0X06);//KN3[2:0];KN2[2:0]
	send_data_cmd(0X76);//KN5[2:0];KN4[2:0]
	send_data_cmd(0X20);//13 RN1[2:0];RN0[2:0]
	send_data_cmd(0X00);//02 0a 06 06 VRN0[3:0]
	send_data_cmd(0X0f);//0c 00 VRN1[4:0]

////Internal command
	//send_ctrl_cmd(0xE4);		//Internal LSI TEST Registers
	//send_data_cmd(0xA0);

	//send_ctrl_cmd(0xF0);		//Internal LSI TEST Registers
	//send_data_cmd(0x01);

	send_ctrl_cmd(0xF3);		//Internal LSI TEST Registers
	send_data_cmd(0x40);
	send_data_cmd(0x0A);

	send_ctrl_cmd(0xF6);		//Internal LSI TEST Registers
	send_data_cmd(0x80);

	send_ctrl_cmd(0xF7);		//Internal LSI TEST Registers
	send_data_cmd(0x80);

	send_ctrl_cmd(0x35);		//Set_address_mode
	send_data_cmd(0x00);	//1B

	send_ctrl_cmd(0x36);		//Set_address_mode
	send_data_cmd(0x0a);	//1B

	send_ctrl_cmd(0x3A);		//Set_pixel_format
	send_data_cmd(0x66);	//5-16bit,6-18bit

//Address
	send_ctrl_cmd(0x2A);		//Set_column_address
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x01);
	send_data_cmd(0x3F);

	send_ctrl_cmd(0x2B);		//Set_page_address
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x01);
	send_data_cmd(0xDF);

	send_ctrl_cmd(0x13);		//NORMAL DISPLAY
	MDELAY(100);

	send_ctrl_cmd(0x11);		//Sleep out
	MDELAY(200);

	send_ctrl_cmd(0x29);		//Display on

	send_ctrl_cmd(0x2C);	//Write_memory_start
#endif

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

    params->type   = LCM_TYPE_DBI;
    params->ctrl   = LCM_CTRL_PARALLEL_DBI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    params->io_select_mode = 3;

    params->dbi.port                    = 0;
    params->dbi.clock_freq              = LCM_DBI_CLOCK_FREQ_52M;
    params->dbi.data_width              = LCM_DBI_DATA_WIDTH_18BITS;
    params->dbi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dbi.data_format.trans_seq   = LCM_DBI_TRANS_SEQ_MSB_FIRST;
    params->dbi.data_format.padding     = LCM_DBI_PADDING_ON_LSB;
    params->dbi.data_format.format      = LCM_DBI_FORMAT_RGB666;
    params->dbi.data_format.width       = LCM_DBI_DATA_WIDTH_18BITS;
    params->dbi.cpu_write_bits          = LCM_DBI_CPU_WRITE_32_BITS;
    params->dbi.io_driving_current      = LCM_DRIVING_CURRENT_8MA;
    params->dbi.te_mode= 1;//libin changed 1
    params->dbi.te_edge_polarity= 1;//  1
    /////////////////ILI9481
///////////changed by max
    params->dbi.parallel.write_setup    = 4;
    params->dbi.parallel.write_hold     = 4;
    params->dbi.parallel.write_wait     = 12;
    params->dbi.parallel.read_setup     = 4;
    params->dbi.parallel.read_latency   = 40;
    params->dbi.parallel.wait_period    = 1;//0
}


static void lcm_init(void)
{
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);
         SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(100);
         init_lcm_registers();
}


static void lcm_suspend(void)
{
	send_ctrl_cmd(0x28);
	MDELAY(50);
	send_ctrl_cmd(0x10);
	MDELAY(120);
}

static void lcm_resume(void)
{
	send_ctrl_cmd(0x11);
	MDELAY(150);
	send_ctrl_cmd(0x29);
	MDELAY(50);
	send_ctrl_cmd(0x2c);
}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
    	unsigned int x0 = x;
    	unsigned int y0 = y;
    	unsigned int x1 = x0 + width - 1;
    	unsigned int y1 = y0 + height - 1;

	send_ctrl_cmd(0x2A);
	send_data_cmd(HIGH_BYTE(x0));
	send_data_cmd(LOW_BYTE(x0));
	send_data_cmd(HIGH_BYTE(x1));
	send_data_cmd(LOW_BYTE(x1));

	send_ctrl_cmd(0x2B);
	send_data_cmd(HIGH_BYTE(y0));
	send_data_cmd(LOW_BYTE(y0));
	send_data_cmd(HIGH_BYTE(y1));
	send_data_cmd(LOW_BYTE(y1));

	// Write To GRAM
	send_ctrl_cmd(0x2C);
}

static unsigned int lcm_compare_id(void)
{
	unsigned int i;
         send_ctrl_cmd(0xBF);  // SET password
	read_data_cmd();  //dummy read
	read_data_cmd();	//02
	read_data_cmd();	//04
	i = read_data_cmd(); //94
	i = ( i << 8 )  | read_data_cmd();	//81
	read_data_cmd(); //ff

    	return (LCM_ID == i)?1:0;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER ili9481_lcm_drv =
{
    	.name			= "ili9481",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.update         = lcm_update,
	.compare_id     = lcm_compare_id,
};
