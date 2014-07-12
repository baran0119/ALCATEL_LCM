

#include <linux/string.h>
#ifdef BUILD_UBOOT
#include <asm/arch/mt6575_gpio.h>
#else
#include <mach/mt6575_gpio.h>
#endif
#include "lcm_drv.h"


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (800)
#define FRAME_HEIGHT (480)


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

    params->type   = LCM_TYPE_DPI;
    params->ctrl   = LCM_CTRL_SERIAL_DBI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    params->io_select_mode = 0;	

    /* RGB interface configurations */
    
    params->dpi.mipi_pll_clk_ref  = 0;      //the most important parameters: set pll clk to 66Mhz and dpi clk to 33Mhz
    params->dpi.mipi_pll_clk_div1 = 41;
    params->dpi.mipi_pll_clk_div2 = 8;
    params->dpi.dpi_clk_div       = 2;
    params->dpi.dpi_clk_duty      = 1;

    params->dpi.clk_pol           = LCM_POLARITY_FALLING;
    params->dpi.de_pol            = LCM_POLARITY_RISING;
    params->dpi.vsync_pol         = LCM_POLARITY_FALLING;
    params->dpi.hsync_pol         = LCM_POLARITY_FALLING;

    params->dpi.hsync_pulse_width = 30;
    params->dpi.hsync_back_porch  = 46;
    params->dpi.hsync_front_porch = 200;
    params->dpi.vsync_pulse_width = 3;
    params->dpi.vsync_back_porch  = 23;
    params->dpi.vsync_front_porch = 19;
    
    params->dpi.format            = LCM_DPI_FORMAT_RGB888;   // format is 24 bit
    params->dpi.rgb_order         = LCM_COLOR_ORDER_RGB;
    params->dpi.is_serial_output  = 0;

    params->dpi.intermediat_buffer_num = 2;

    params->dpi.io_driving_current = LCM_DRIVING_CURRENT_6575_8MA;//LCM_DRIVING_CURRENT_8MA | LCM_DRIVING_CURRENT_4MA | LCM_DRIVING_CURRENT_2MA;;
}


static void lcm_init(void)
{
	//lcm_util.set_gpio_mode(GPIO96, GPIO_MODE_00);    
    //lcm_util.set_gpio_dir(GPIO96, GPIO_DIR_OUT);
    //lcm_util.set_gpio_out(GPIO96, GPIO_OUT_ONE); // LCD_PWREN
    //MDELAY(10); 
    lcm_util.set_gpio_mode(GPIO14, GPIO_MODE_00);    
    lcm_util.set_gpio_dir(GPIO14, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO14, GPIO_OUT_ONE); // LCD_RST
    MDELAY(10);
}


static void lcm_suspend(void)
{
    lcm_util.set_gpio_mode(GPIO14, GPIO_MODE_00);    
    lcm_util.set_gpio_dir(GPIO14, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO14, GPIO_OUT_ZERO); // LCD_RST
    MDELAY(10);    
	//lcm_util.set_gpio_mode(GPIO96, GPIO_MODE_00);    
    //lcm_util.set_gpio_dir(GPIO96, GPIO_DIR_OUT);
    //lcm_util.set_gpio_out(GPIO96, GPIO_OUT_ZERO); // LCD_PWREN
    //MDELAY(10);
}


static void lcm_resume(void)
{
	//lcm_util.set_gpio_mode(GPIO96, GPIO_MODE_00);    
    //lcm_util.set_gpio_dir(GPIO96, GPIO_DIR_OUT);
    //lcm_util.set_gpio_out(GPIO96, GPIO_OUT_ONE); // LCD_PWREN
    //MDELAY(10); 
    lcm_util.set_gpio_mode(GPIO14, GPIO_MODE_00);    
    lcm_util.set_gpio_dir(GPIO14, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO14, GPIO_OUT_ONE); // LCD_RST
    MDELAY(10);
}


// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hb070d_lcm_drv = 
{
    .name			= "hb070d",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
};
