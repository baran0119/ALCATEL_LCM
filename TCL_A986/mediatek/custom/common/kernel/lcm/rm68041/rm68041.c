

#include <linux/string.h>

#include "lcm_drv.h"


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (320)
#define FRAME_HEIGHT (480)

#define LCM_ID       (0x9481)
#define LCM_ID_MASK       (0xFF)
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
    return lcm_util.read_data();
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
	send_ctrl_cmd(0x11);
	MDELAY(120);

	send_ctrl_cmd(0xD0);//Power_Setting (D0h)
	send_data_cmd(0x07);
	send_data_cmd(0x42); //41
	send_data_cmd(0x1C); //1C

	send_ctrl_cmd(0xD1);//VCOM Control (D1h)
	send_data_cmd(0x00);
	send_data_cmd(0x1A);//
	send_data_cmd(0x1A);

	send_ctrl_cmd(0xD2);//Power_Setting for Normal Mode
	send_data_cmd(0x01);
	send_data_cmd(0x11);

	send_ctrl_cmd(0xC0);//Panel Driving Setting (C0h)
	send_data_cmd(0x10);//10
	send_data_cmd(0x3B);
	send_data_cmd(0x00);
	send_data_cmd(0x12);
	send_data_cmd(0x01);

	send_ctrl_cmd(0xC1);
	send_data_cmd(0x10);
	send_data_cmd(0x12);
	send_data_cmd(0x22);

	send_ctrl_cmd(0xC5);
	send_data_cmd(0x02);//03

	send_ctrl_cmd(0xC8);
	send_data_cmd(0x03);
	send_data_cmd(0x24);
	send_data_cmd(0x43);
	send_data_cmd(0x07);
	send_data_cmd(0x08);
	send_data_cmd(0x00);
	send_data_cmd(0x43);
	send_data_cmd(0x35);
	send_data_cmd(0x47);
	send_data_cmd(0x70);
	send_data_cmd(0x00);
	send_data_cmd(0x00);

	send_ctrl_cmd(0xF8);
	send_data_cmd(0x01);

	send_ctrl_cmd(0xFE);
	send_data_cmd(0x00);
	send_data_cmd(0x02);
	send_data_cmd(0x14);

	send_ctrl_cmd(0x36);
	send_data_cmd(0x0A);

	send_ctrl_cmd(0x3A);
	send_data_cmd(0x66);

	MDELAY(120);
	send_ctrl_cmd(0x29);

	send_ctrl_cmd(0x2c);

#else
	send_ctrl_cmd(0x11);
	MDELAY(120);

	send_ctrl_cmd(0xD0);//Power_Setting (D0h)
	send_data_cmd(0x07);
	send_data_cmd(0x41); //41
	send_data_cmd(0x1C); //1C

	send_ctrl_cmd(0xD1);//VCOM Control (D1h)
	send_data_cmd(0x00);
	send_data_cmd(0x14);//VCM 0x0a
	send_data_cmd(0x0b);//VDV

	send_ctrl_cmd(0xD2);//Power_Setting for Normal Mode
	send_data_cmd(0x01);
	send_data_cmd(0x11);

	send_ctrl_cmd(0xC0);//Panel Driving Setting (C0h)
	send_data_cmd(0x10);//10
	send_data_cmd(0x3B);
	send_data_cmd(0x00);
	send_data_cmd(0x12);
	send_data_cmd(0x01);

	send_ctrl_cmd(0xC5);
	send_data_cmd(0x01);//03

	send_ctrl_cmd(0xC8);
	send_data_cmd(0x00);
	send_data_cmd(0x15);
	send_data_cmd(0x10);
	send_data_cmd(0x20);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x76);
	send_data_cmd(0x26);
	send_data_cmd(0x77);
	send_data_cmd(0x20);
	send_data_cmd(0x00);
	send_data_cmd(0x00);

	send_ctrl_cmd(0xF8);
	send_data_cmd(0x01);

	send_ctrl_cmd(0xFE);
	send_data_cmd(0x00);
	send_data_cmd(0x02);
	send_data_cmd(0x14);

	send_ctrl_cmd(0x36);
	send_data_cmd(0x0A);

	send_ctrl_cmd(0x3A);
	send_data_cmd(0x66);

	MDELAY(120);
	send_ctrl_cmd(0x29);

	send_ctrl_cmd(0x2c);
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
    params->dbi.clock_freq              = LCM_DBI_CLOCK_FREQ_52M;//52
    params->dbi.data_width              = LCM_DBI_DATA_WIDTH_18BITS;
    params->dbi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dbi.data_format.trans_seq   = LCM_DBI_TRANS_SEQ_MSB_FIRST;
    params->dbi.data_format.padding     = LCM_DBI_PADDING_ON_LSB;
    params->dbi.data_format.format      = LCM_DBI_FORMAT_RGB666;
    params->dbi.data_format.width       = LCM_DBI_DATA_WIDTH_18BITS;
    params->dbi.cpu_write_bits          = LCM_DBI_CPU_WRITE_16_BITS;
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
	//MDELAY(50);
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

	unsigned int id=0;

	send_ctrl_cmd(0xBF);

	read_data_cmd();	//dummy code:0
	read_data_cmd();  //02
	read_data_cmd();  //04

	unsigned int param1 = 0;
	param1 =read_data_cmd(); //manufacturer id   94

	unsigned int param2 =0;
	param2 = read_data_cmd(); //module/driver  id  81

	read_data_cmd();  //ff

         id = ((param1 & LCM_ID_MASK)<<8)|(param2);

         return (LCM_ID == id)?1:0;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER rm68041_lcm_drv =
{
    	.name			= "rm68041",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.update         = lcm_update,
	.compare_id     = lcm_compare_id
};
