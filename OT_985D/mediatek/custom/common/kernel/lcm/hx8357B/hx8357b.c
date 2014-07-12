

#include <linux/string.h>

#include "lcm_drv.h"


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (320)
#define FRAME_HEIGHT (480)
#define LCM_ID       (0x90)


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

static __inline void send_ctrl_cmd(unsigned int cmd)
{
	lcm_util.send_cmd(cmd);
}

static __inline void send_data_cmd(unsigned int data)
{
	lcm_util.send_data(data);
}

static __inline unsigned int read_data_cmd(void)
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
	send_ctrl_cmd(0xB9); //EXTC
	send_data_cmd(0xFF); //EXTC
	send_data_cmd(0x83); //EXTC
	send_data_cmd(0x57); //EXTC

	MDELAY(15);

	send_ctrl_cmd(0xB6); //
	send_data_cmd(0x0c); //6f VCOMDC//56   0c

	send_ctrl_cmd(0x11); // SLPOUT
	MDELAY(250);
	//send_ctrl_cmd(0x35); // TE ON

         send_ctrl_cmd(0x36); // Set Panel
	send_data_cmd(0x00); //

	send_ctrl_cmd(0x3A);
	send_data_cmd(0x66); //262K

	send_ctrl_cmd(0xB0);
	send_data_cmd(0x68); //80Hz
	//send_data_cmd(0x01);//osc enable

	send_ctrl_cmd(0xcc);
	send_data_cmd(0x0b); // 09

	send_ctrl_cmd(0xB1); //
	send_data_cmd(0x00); //
	send_data_cmd(0x14); //BT
	send_data_cmd(0x1c); //1c VSPR
	send_data_cmd(0x1c); //1c VSNR
	send_data_cmd(0x83); //83 AP  83
	send_data_cmd(0x48); //FS

   	send_ctrl_cmd(0xC0); //STBA
	send_data_cmd(0x50); //OPON
	send_data_cmd(0x50); //OPON
	send_data_cmd(0x01); //
	send_data_cmd(0x3C); //
	send_data_cmd(0x1e); //
	send_data_cmd(0x08); //GEN

	send_ctrl_cmd(0xB4); //
	send_data_cmd(0x02); //NW
	send_data_cmd(0x40); //RTN
	send_data_cmd(0x00); //DIV
	send_data_cmd(0x2A); //DUM
	send_data_cmd(0x2A); //DUM
	send_data_cmd(0x0d); //0D GDON
	send_data_cmd(0x78); //GDOFF

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

	send_ctrl_cmd(0xE0); //
	send_data_cmd(0x00); //02 1//
	send_data_cmd(0x02); //06 2//
	send_data_cmd(0x05); //3
	send_data_cmd(0x18); //4
	send_data_cmd(0x21); //5
	send_data_cmd(0x35); //6
	send_data_cmd(0x41); //7
	send_data_cmd(0x4A); //8
	send_data_cmd(0x4E); //9
	send_data_cmd(0x47); //10
	send_data_cmd(0x40); //11
	send_data_cmd(0x3A); //12
	send_data_cmd(0x30); //13
	send_data_cmd(0x2E); //14
	send_data_cmd(0x2C); //15
	send_data_cmd(0x07); //16
	send_data_cmd(0x00); //02 17 v1
	send_data_cmd(0x02); //06 18
	send_data_cmd(0x05); //19
	send_data_cmd(0x18); //20
	send_data_cmd(0x21); //21
	send_data_cmd(0x35); //22
	send_data_cmd(0x41); //23
	send_data_cmd(0x4A); //24
	send_data_cmd(0x4E); //25
	send_data_cmd(0x47); //26
	send_data_cmd(0x40); //27
	send_data_cmd(0x3A); //28
	send_data_cmd(0x30); //29
	send_data_cmd(0x2E); //30
	send_data_cmd(0x2C); //28 31
	send_data_cmd(0x07); //03 32
	send_data_cmd(0x44); //33
	send_data_cmd(0x00); //34

	send_ctrl_cmd(0x21);
	send_ctrl_cmd(0x29); // Display On

	MDELAY(25);

	send_ctrl_cmd(0x2C);
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
	params->dbi.data_format.padding     = LCM_DBI_PADDING_ON_MSB;
	params->dbi.data_format.format      = LCM_DBI_FORMAT_RGB666;
	params->dbi.data_format.width       = LCM_DBI_DATA_WIDTH_18BITS;
	params->dbi.cpu_write_bits          = LCM_DBI_CPU_WRITE_16_BITS;
	params->dbi.io_driving_current      = LCM_DRIVING_CURRENT_8MA;
	 params->dbi.parallel.write_setup    = 4;
          params->dbi.parallel.write_hold     = 4;
          params->dbi.parallel.write_wait     = 12;
          params->dbi.parallel.read_setup     = 4;
          params->dbi.parallel.read_latency   = 40;
          params->dbi.parallel.wait_period    = 1;//0

	// enable tearing-free
         params->dbi.te_mode                 = LCM_DBI_TE_MODE_VSYNC_ONLY;
         params->dbi.te_edge_polarity        = LCM_POLARITY_FALLING;
}


static void lcm_init(void)
{
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(5);
         SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(100);
	init_lcm_registers();

    //Set TE register
	//send_ctrl_cmd(0x35);
	//send_data_cmd(0x00);

    //send_ctrl_cmd(0X0044);  // Set TE signal delay scanline
    //send_data_cmd(0X0000);  // Set as 0-th scanline
    //send_data_cmd(0X0000);
	//sw_clear_panel(0);
}


static void lcm_suspend(void)
{
	send_ctrl_cmd(0x10);
	MDELAY(120);
}


static void lcm_resume(void)
{
	send_ctrl_cmd(0x11);
	MDELAY(120);
}

static void lcm_update(unsigned int x, unsigned int y,
		unsigned int width, unsigned int height)
{
	unsigned short x0, y0, x1, y1;
	unsigned short h_X_start,l_X_start,h_X_end,l_X_end,h_Y_start,l_Y_start,h_Y_end,l_Y_end;

	x0 = (unsigned short)x;
	y0 = (unsigned short)y;
	x1 = (unsigned short)x+width-1;
	y1 = (unsigned short)y+height-1;

	h_X_start=((x0&0xFF00)>>8);
	l_X_start=(x0&0x00FF);
	h_X_end=((x1&0xFF00)>>8);
	l_X_end=(x1&0x00FF);

	h_Y_start=((y0&0xFF00)>>8);
	l_Y_start=(y0&0x00FF);
	h_Y_end=((y1&0xFF00)>>8);
	l_Y_end=(y1&0x00FF);

	send_ctrl_cmd(0x2A);
	send_data_cmd(h_X_start);
	send_data_cmd(l_X_start);
	send_data_cmd(h_X_end);
	send_data_cmd(l_X_end);

	send_ctrl_cmd(0x2B);
	send_data_cmd(h_Y_start);
	send_data_cmd(l_Y_start);
	send_data_cmd(h_Y_end);
	send_data_cmd(l_Y_end);

	send_ctrl_cmd(0x29);
	MDELAY(20);
	send_ctrl_cmd(0x2C);
}

void lcm_setbacklight(unsigned int level)
{
	if(level > 255) level = 255;
	send_ctrl_cmd(0x51);
	send_data_cmd(level);
}

static unsigned int lcm_compare_id(void)
{
         send_ctrl_cmd(0xB9);  // SET password
	send_data_cmd(0xFF);
	send_data_cmd(0x83);
	send_data_cmd(0x57);
         send_ctrl_cmd(0xd0);
	read_data_cmd();
    	return (LCM_ID == read_data_cmd())?1:0;
}

LCM_DRIVER hx8357b_lcm_drv =
{
          .name			= "hx8357b",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.update         = lcm_update,
	.set_backlight	= lcm_setbacklight,
	.compare_id     = lcm_compare_id,
};
