#ifndef BUILD_LK
#include <linux/string.h>
#endif
#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#elif (defined BUILD_UBOOT)
#include <asm/arch/mt6577_gpio.h>
#include <asm/arch/mt6577_pmic6329.h>
#else
#include <mach/mt_gpio.h>
#endif

#include "lcm_drv.h"


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (1024)
#define FRAME_HEIGHT (768)


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
    unsigned char temp1 = (unsigned char)((cmd >> 8) & 0xFF);
    unsigned char temp2 = (unsigned char)(cmd & 0xFF);

    lcm_util.send_data(0x2000 | temp1);
    lcm_util.send_data(0x0000 | temp2);
}

static __inline void send_data_cmd(unsigned int data)
{
    lcm_util.send_data(0x0004 | data);
}

static __inline void set_lcm_register(unsigned int regIndex,
                                      unsigned int regData)
{
    send_ctrl_cmd(regIndex);
    send_data_cmd(regData);
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

    params->type   = LCM_TYPE_DPI;
    params->ctrl   = LCM_CTRL_SERIAL_DBI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    params->io_select_mode = 0;	

    /* RGB interface configurations */
    
    params->dpi.mipi_pll_clk_ref  = 0;      //the most important parameters: set pll clk to 66Mhz and dpi clk to 33Mhz
    params->dpi.mipi_pll_clk_div1 = 29;
    params->dpi.mipi_pll_clk_div2 = 3;
    params->dpi.dpi_clk_div       = 2;
    params->dpi.dpi_clk_duty      = 1;

    params->dpi.clk_pol           = LCM_POLARITY_FALLING;
    params->dpi.de_pol            = LCM_POLARITY_RISING;
    params->dpi.vsync_pol         = LCM_POLARITY_FALLING;
    params->dpi.hsync_pol         = LCM_POLARITY_FALLING;

    params->dpi.hsync_pulse_width = 140;
    params->dpi.hsync_back_porch  = 120;
    params->dpi.hsync_front_porch = 60;
    params->dpi.vsync_pulse_width = 16;
    params->dpi.vsync_back_porch  = 12;
    params->dpi.vsync_front_porch = 10;
    
    params->dpi.format            = LCM_DPI_FORMAT_RGB888;   // format is 24 bit
    params->dpi.rgb_order         = LCM_COLOR_ORDER_RGB;
    params->dpi.is_serial_output  = 0;

    params->dpi.intermediat_buffer_num = 2;

    params->dpi.io_driving_current = LCM_DRIVING_CURRENT_6575_8MA;
}


static void lcm_init(void)
{
#ifdef BUILD_LK
#if defined(MT6577)
    kal_uint32 ret=0;
    ret=pmic_config_interface(BANK0_DIGLDO_CON22, 0x7, BANK_0_RG_VCAM_AF_VOSEL_MASK, BANK_0_RG_VCAM_AF_VOSEL_SHIFT);
	ret=pmic_config_interface(BANK0_DIGLDO_CON30, 0x7, BANK_0_RG_VGP2_VOSEL_MASK, BANK_0_RG_VGP2_VOSEL_SHIFT);
	
	ret=pmic_config_interface(BANK0_DIGLDO_CON22, 0x1, BANK_0_RG_VCAM_AF_EN_MASK, BANK_0_RG_VCAM_AF_EN_SHIFT);
	ret=pmic_config_interface(BANK0_DIGLDO_CON30, 0x1, BANK_0_RG_VGP2_EN_MASK, BANK_0_RG_VGP2_EN_SHIFT);
	printf("PMIC VGP2/VCAM_AF -->3.3V \n");
#endif
	MDELAY(5);
    mt_set_gpio_mode(GPIO14, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO14, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO14, GPIO_OUT_ONE); // LCM_STBY
    MDELAY(5);
    mt_set_gpio_mode(GPIO52, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO52, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO52, GPIO_OUT_ONE); // LCM_VLED_EN 
    MDELAY(50);
    mt_set_gpio_mode(GPIO49, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO49, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO49, GPIO_OUT_ONE); // LCM_RST
    MDELAY(5);
    mt_set_gpio_mode(GPIO84, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO84, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO84, GPIO_OUT_ONE); // LVDS_SHUTDOWN_N   
    MDELAY(120);
    //lcm_util.set_gpio_mode(GPIO47, GPIO_MODE_00);    
    //lcm_util.set_gpio_dir(GPIO47, GPIO_DIR_OUT);
    //lcm_util.set_gpio_out(GPIO47, GPIO_OUT_ONE); // LCM_BL_ENABLE  
#elif (defined BUILD_UBOOT)
    // do nothing in uboot
#else

#endif 		

     
}


static void lcm_suspend(void)
{
    //SET_RESET_PIN(0);  //CH7035B reset control

    //lcm_util.set_gpio_mode(GPIO60, GPIO_MODE_00);    
    //lcm_util.set_gpio_dir(GPIO60, GPIO_DIR_OUT);
    //lcm_util.set_gpio_out(GPIO60, GPIO_OUT_ZERO); // HDMI_POWER_EN

    //lcm_util.set_gpio_mode(GPIO47, GPIO_MODE_00);    
    //lcm_util.set_gpio_dir(GPIO47, GPIO_DIR_OUT);
    //lcm_util.set_gpio_out(GPIO47, GPIO_OUT_ZERO); // LCM_BL_ENABLE
    //MDELAY(10);
    //lcm_util.set_gpio_mode(GPIO48, GPIO_MODE_00);    
    //lcm_util.set_gpio_dir(GPIO48, GPIO_DIR_OUT);
    //lcm_util.set_gpio_out(GPIO48, GPIO_OUT_ZERO); // LCM_BL_EN
    MDELAY(10);
    mt_set_gpio_mode(GPIO84, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO84, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO84, GPIO_OUT_ZERO); // LVDS_SHUTDOWN_N      
    MDELAY(10);
    mt_set_gpio_mode(GPIO14, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO14, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO14, GPIO_OUT_ZERO); // LCM_STBY     
    MDELAY(10);
    mt_set_gpio_mode(GPIO49, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO49, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO49, GPIO_OUT_ZERO); // LCM_RST    
    MDELAY(10);
    mt_set_gpio_mode(GPIO52, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO52, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO52, GPIO_OUT_ZERO); // LCM_VLED_EN
    MDELAY(60); // avoid LCD resume transint
}


static void lcm_resume(void)
{
    //SET_RESET_PIN(1);  //CH7035B reset control

    //lcm_util.set_gpio_mode(GPIO60, GPIO_MODE_00);    
    //lcm_util.set_gpio_dir(GPIO60, GPIO_DIR_OUT);
    //lcm_util.set_gpio_out(GPIO60, GPIO_OUT_ONE); // HDMI_POWER_EN

    //lcm_util.set_gpio_mode(GPIO48, GPIO_MODE_00);    
    //lcm_util.set_gpio_dir(GPIO48, GPIO_DIR_OUT);
    //lcm_util.set_gpio_out(GPIO48, GPIO_OUT_ONE); // LCM_BL_EN
    MDELAY(5);
    mt_set_gpio_mode(GPIO14, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO14, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO14, GPIO_OUT_ONE); // LCM_STBY
    MDELAY(5);
    mt_set_gpio_mode(GPIO52, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO52, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO52, GPIO_OUT_ONE); // LCM_VLED_EN 
    MDELAY(50);
    mt_set_gpio_mode(GPIO49, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO49, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO49, GPIO_OUT_ONE); // LCM_RST
    MDELAY(5);
    mt_set_gpio_mode(GPIO84, GPIO_MODE_00);    
    mt_set_gpio_dir(GPIO84, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO84, GPIO_OUT_ONE); // LVDS_SHUTDOWN_N   
    MDELAY(120);
    //lcm_util.set_gpio_mode(GPIO47, GPIO_MODE_00);    
    //lcm_util.set_gpio_dir(GPIO47, GPIO_DIR_OUT);
    //lcm_util.set_gpio_out(GPIO47, GPIO_OUT_ONE); // LCM_BL_ENABLE
}

LCM_DRIVER hb080_lcm_drv = 
{
    .name		= "HB080",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
};

