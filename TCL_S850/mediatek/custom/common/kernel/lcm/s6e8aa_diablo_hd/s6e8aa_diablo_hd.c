//#define  LCD_DEBUG    
#define SMART_DIMMY

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <platform/mt_pwm.h>
	#include <platform/mt_pmic.h>
	#ifdef LCD_DEBUG
		#define LCM_DEBUG(format, ...)   printf("uboot ssd2825" format "\n", ## __VA_ARGS__)
	#else
		#define LCM_DEBUG(format, ...)
	#endif

#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt6577_gpio.h>
	#include <asm/arch/mt6577_pwm.h>
	#include <asm/arch/mt6577_pmic6329.h>
	#ifdef LCD_DEBUG
		#define LCM_DEBUG(format, ...)   printf("uboot ssd2825" format "\n", ## __VA_ARGS__)
	#else
		#define LCM_DEBUG(format, ...)
	#endif
#else
	#include <mach/mt_gpio.h>
	#include <mach/mt_pwm.h>
	#include <mach/upmu_hw.h>
	#include <mach/pmic_mt6329_sw.h>
	#include <mach/pmic_mt6329_hw.h>
	#include <mach/pmic_mt6329_sw_bank1.h>
	#include <mach/pmic_mt6329_hw_bank1.h>
	//#include <linux/timer.h>
	//#include <linux/module.h>
	//#include <linux/init.h>	
	#include <linux/delay.h>
	#include <linux/errno.h>
	#include <linux/err.h>
	#include "smart_dimming.h"	

	#ifdef LCD_DEBUG
		#define LCM_DEBUG(format, ...)   printk("kernel ssd2825" format "\n", ## __VA_ARGS__)
	#else
		#define LCM_DEBUG(format, ...)
	#endif
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LSCE_GPIO_PIN                 (GPIO47)
#define LSCK_GPIO_PIN                 (GPIO51)
#define LSDIO_GPIO_PIN                (GPIO52)

#define SSD2825_SHUT_GPIO_PIN         (GPIO12)
#define SSD2825_MIPI_CLK_GPIO_PIN     (GPIO68)
#define LCD_POWER_GPIO_PIN            (GPIO86)

#define FRAME_WIDTH                   (720)
#define FRAME_HEIGHT                  (1280)

#define SSD2825_ID                    (0x2825)

#define GAMMABACKLIGHT_NUM             25

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
#define UDELAY(n)	                (lcm_util.udelay(n))
#define MDELAY(n)	                (lcm_util.mdelay(n))
#else
#define UDELAY(n)	                udelay(n)
#define MDELAY(n)	                msleep(n)
#endif

#define SET_RESET_PIN(v)	        (lcm_util.set_reset_pin((v)))
#define SET_GPIO_OUT(n, v)	        (lcm_util.set_gpio_out((n), (v)))


#define SET_LSCE_LOW                 SET_GPIO_OUT(LSCE_GPIO_PIN, 0)
#define SET_LSCE_HIGH                SET_GPIO_OUT(LSCE_GPIO_PIN, 1)
#define SET_LSCK_LOW   				 SET_GPIO_OUT(LSCK_GPIO_PIN, 0)
#define SET_LSCK_HIGH  				 SET_GPIO_OUT(LSCK_GPIO_PIN, 1)
#define SET_LSDA_LOW                 SET_GPIO_OUT(LSDIO_GPIO_PIN, 0)
#define SET_LSDA_HIGH                SET_GPIO_OUT(LSDIO_GPIO_PIN, 1)
#define GET_HX_SDI                   mt_get_gpio_in(LSDIO_GPIO_PIN)

#define HX_WR_COM                   (0x70)
#define HX_WR_REGISTER              (0x72)
#define HX_RD_REGISTER              (0x73)

#define ARRAY_OF(x)      			((int)(sizeof(x)/sizeof(x[0])))


static LCM_UTIL_FUNCS lcm_util = {0};
static int acl_enable = 0;//defaule value close the ACL function
static int g_last_Backlight_level =  0;
#ifdef BUILD_UBOOT
static int g_resume_flag = 0;
#endif
//static bool  first_init_lcm = true;
static bool esd_check_recover = false;
static bool jrd_esd_check = false;


#if defined(SMART_DIMMY)
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
#else
struct lcd_info {
	unsigned char			**gamma_table;
	struct str_smart_dim	smart;
};

extern s16 adjust_mtp[CI_MAX][IV_MAX];
unsigned char calc_gamma_300cd[25];
unsigned char mtp_data_dump[CI_MAX][IV_MAX];
bool lcm_log_on = false;

#define LCM_SMART_DIMMY(fmt, arg...)									\
	do {																\
		if (lcm_log_on) printk("[Smart Dimmimg]" fmt, ##arg);		\
	}while(0)

int S6e8aa0_Smart_Dimmy(void);

#endif
#endif

static void setGammaBacklight(unsigned int level);
static void lcm_setbacklight(unsigned int level);

void system_power_on(void)
{
	pmic_config_interface( 	(kal_uint8)(BANK0_LDO_VGP2+LDO_CON1_OFFSET),
							(kal_uint8)(0x06),
							(kal_uint8)(BANK_0_RG_LDO_VOSEL_MASK),
							(kal_uint8)(BANK_0_RG_LDO_VOSEL_SHIFT)
						  );

	pmic_config_interface( 	(kal_uint8)(BANK0_LDO_VGP2+LDO_CON1_OFFSET),
							(kal_uint8)(0x01),
							(kal_uint8)(BANK_0_LDO_EN_MASK),
							(kal_uint8)(BANK_0_LDO_EN_SHIFT)
						  );
}

#ifdef BUILD_LK
int lcd_set_pwm(int pwm_num)
{
	struct pwm_spec_config pwm_setting;

	pwm_setting.pwm_no = pwm_num;
	pwm_setting.mode = PWM_MODE_FIFO; //new mode fifo and periodical mode
	pwm_setting.clk_div = CLK_DIV1;
	pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK;

	pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.HDURATION = 1;
	pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.LDURATION = 1;
	pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
	pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
	pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
	pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.GDURATION = 0;
	pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.SEND_DATA0 = 0xAAAAAAAA;
	pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xAAAAAAAA ;
	pwm_set_spec_config(&pwm_setting);

	return 0;
}
#else
int lcd_set_pwm(int pwm_num)
{
	struct pwm_spec_config pwm_setting;

	pwm_setting.pwm_no = pwm_num;
	pwm_setting.mode = PWM_MODE_FIFO; //new mode fifo and periodical mode
	pwm_setting.clk_div = CLK_DIV1;
	pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK;

	pwm_setting.PWM_MODE_FIFO_REGS.HDURATION = 1;
	pwm_setting.PWM_MODE_FIFO_REGS.LDURATION = 1;
	pwm_setting.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
	pwm_setting.PWM_MODE_FIFO_REGS.GDURATION = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

    pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA0 = 0xAAAAAAAA;
	pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xAAAAAAAA;
	pwm_set_spec_config(&pwm_setting);

	return 0;
}
#endif

struct __gamma_backlight {
    unsigned int backlight_level;                 //backlight level
    unsigned char gammaValue[GAMMABACKLIGHT_NUM]; //gamma value for backlight
};

static struct __gamma_backlight gamma_table[] = {
/*{0,  {0x01,0x52,0x24,0x5D,0x00,0x00,0x00,0x74,0x78,0x5F,0xA8,0xB6,0xAE,0x8A,0xA5,0x93,0xB8,0xBF,0xB3,0x00,0x58,0x00,0x39,0x00,0x62}},
{10, {0x01,0x52,0x24,0x5D,0x00,0x00,0x00,0x74,0x78,0x5F,0xA8,0xB6,0xAE,0x8A,0xA5,0x93,0xB8,0xBF,0xB3,0x00,0x58,0x00,0x39,0x00,0x62}},
{20, {0x01,0x52,0x24,0x5D,0x00,0x00,0x00,0x74,0x78,0x5F,0xA8,0xB6,0xAE,0x8A,0xA5,0x93,0xB8,0xBF,0xB3,0x00,0x58,0x00,0x39,0x00,0x62}},
{30, {0x01,0x52,0x24,0x5D,0x00,0x00,0x00,0x74,0x78,0x5F,0xA8,0xB6,0xAE,0x8A,0xA5,0x93,0xB8,0xBF,0xB3,0x00,0x58,0x00,0x39,0x00,0x62}},
{40, {0x01,0x52,0x24,0x5D,0x00,0x00,0x08,0x7E,0x84,0x6D,0xAD,0xBE,0xB7,0x8C,0xA3,0x91,0xB7,0xC0,0xB3,0x00,0x61,0x00,0x41,0x00,0x6B}},
{50, {0x01,0x52,0x24,0x5D,0x22,0x09,0x1C,0x83,0x8D,0x77,0xAF,0xC4,0xBC,0x8F,0xA3,0x91,0xB7,0xBC,0xB1,0x00,0x68,0x00,0x49,0x00,0x73}},
{60, {0x01,0x52,0x24,0x5D,0x37,0x23,0x28,0x87,0x95,0x83,0xB2,0xC5,0xBD,0x90,0xA0,0x8E,0xB8,0xBE,0xB7,0x00,0x6E,0x00,0x50,0x00,0x78}},
{70, {0x01,0x52,0x24,0x5D,0x47,0x38,0x34,0x8B,0x9B,0x8C,0xB3,0xC7,0xBD,0x93,0x9F,0x8E,0xB6,0xBD,0xB4,0x00,0x74,0x00,0x57,0x00,0x80}},
{80, {0x01,0x52,0x24,0x5D,0x51,0x45,0x3B,0x8E,0x9F,0x94,0xB5,0xC8,0xBC,0x91,0x9D,0x8E,0xB8,0xBD,0xB2,0x00,0x79,0x00,0x5C,0x00,0x86}},
{90, {0x01,0x52,0x24,0x5D,0x58,0x4E,0x40,0x90,0xA4,0x97,0xB6,0xC8,0xBC,0x90,0x9D,0x8F,0xB9,0xBB,0xB1,0x00,0x7D,0x00,0x61,0x00,0x8A}},
*/
{0,  {0x01,0x52,0x24,0x5D,0x00,0x00,0x00,0x74,0x78,0x5F,0xA8,0xB6,0xAE,0x8A,0xA5,0x93,0xB8,0xBF,0xB3,0x00,0x58,0x00,0x39,0x00,0x62}},
{30, {0x01,0x52,0x24,0x5D,0x00,0x00,0x00,0x74,0x78,0x5F,0xA8,0xB6,0xAE,0x8A,0xA5,0x93,0xB8,0xBF,0xB3,0x00,0x58,0x00,0x39,0x00,0x62}},
{100,{0x01,0x52,0x24,0x5D,0x5F,0x58,0x47,0x92,0xA8,0x9C,0xB7,0xC8,0xBC,0x91,0x9B,0x8D,0xB7,0xBC,0xB2,0x00,0x83,0x00,0x66,0x00,0x90}},
{110,{0x01,0x52,0x24,0x5D,0x65,0x5F,0x4B,0x94,0xA9,0x9F,0xB6,0xC6,0xBB,0x91,0x9C,0x8C,0xB7,0xBB,0xB3,0x00,0x87,0x00,0x6A,0x00,0x94}},
{120,{0x01,0x52,0x24,0x5D,0x68,0x66,0x50,0x95,0xAC,0xA2,0xB8,0xC7,0xB9,0x93,0x9B,0x8D,0xB5,0xBA,0xB2,0x00,0x88,0x00,0x6F,0x00,0x99}},
{130,{0x01,0x52,0x24,0x5D,0x6B,0x69,0x54,0x97,0xAE,0xA3,0xB7,0xC7,0xBB,0x92,0x9A,0x8B,0xB8,0xB9,0xB1,0x00,0x8D,0x00,0x73,0x00,0x9D}},
{140,{0x01,0x52,0x24,0x5D,0x6E,0x6F,0x57,0x97,0xB1,0xA4,0xB8,0xC5,0xBA,0x92,0x9A,0x8C,0xB6,0xB9,0xB1,0x00,0x92,0x00,0x77,0x00,0xA1}},
{150,{0x01,0x52,0x24,0x5D,0x71,0x73,0x5B,0x98,0xB1,0xA6,0xBA,0xC4,0xB7,0x90,0x9B,0x8D,0xB6,0xB8,0xB0,0x00,0x96,0x00,0x7A,0x00,0xA5}},
{160,{0x01,0x52,0x24,0x5D,0x74,0x77,0x5F,0x98,0xB3,0xA6,0xBA,0xC4,0xB9,0x92,0x9B,0x8B,0xB4,0xB6,0xB2,0x00,0x99,0x00,0x7E,0x00,0xA7}},
{170,{0x01,0x52,0x24,0x5D,0x76,0x7A,0x63,0x9B,0xB4,0xA7,0xB7,0xC3,0xB7,0x93,0x9A,0x8D,0xB4,0xB6,0xB0,0x00,0x9D,0x00,0x82,0x00,0xAC}},
{180,{0x01,0x52,0x24,0x5D,0x77,0x7E,0x66,0x9B,0xB5,0xA6,0xB9,0xC3,0xB8,0x92,0x99,0x8C,0xB3,0xB7,0xAE,0x00,0xA0,0x00,0x85,0x00,0xB0}},
{190,{0x01,0x52,0x24,0x5D,0x7A,0x81,0x69,0x9C,0xB5,0xA6,0xB9,0xC3,0xB9,0x92,0x99,0x8B,0xB3,0xB6,0xAF,0x00,0xA3,0x00,0x89,0x00,0xB3}},
{200,{0x01,0x52,0x24,0x5D,0x7B,0x83,0x6B,0x9C,0xB5,0xA7,0xBA,0xC3,0xB9,0x91,0x98,0x8B,0xB3,0xB6,0xAD,0x00,0xA6,0x00,0x8C,0x00,0xB7}},
{210,{0x01,0x52,0x24,0x5D,0x7D,0x87,0x6E,0x9D,0xB6,0xA7,0xB7,0xC2,0xB9,0x93,0x99,0x8B,0xB1,0xB5,0xAD,0x00,0xAA,0x00,0x8F,0x00,0xBA}},
{220,{0x01,0x52,0x24,0x5D,0x80,0x84,0x70,0x9E,0xB6,0xA8,0xB9,0xC2,0xB7,0x92,0x98,0x8B,0xB0,0xB4,0xAD,0x00,0xAC,0x00,0x92,0x00,0xBD}},
{230,{0x01,0x52,0x24,0x5D,0x7E,0x8A,0x72,0x9E,0xB6,0xA8,0xBA,0xC3,0xB8,0x90,0x97,0x8A,0xB1,0xB5,0xAD,0x00,0xAF,0x00,0x95,0x00,0xC0}},
{240,{0x01,0x52,0x24,0x5D,0x81,0x8D,0x75,0x9D,0xB5,0xA8,0xBA,0xCC,0xB7,0x92,0x97,0x8A,0xAE,0xB4,0xAD,0x00,0xB3,0x00,0x98,0x00,0xC3}},
{250,{0x01,0x52,0x24,0x5D,0x82,0x8A,0x75,0x9E,0xB6,0xA7,0xBA,0xC2,0xB9,0x92,0x97,0x8A,0xAE,0xB3,0xAE,0x00,0xB5,0x00,0x9A,0x00,0xC4}},
{260,{0x01,0x52,0x24,0x5D,0x84,0x91,0x79,0x9F,0xB5,0xA9,0xBA,0xC2,0xB6,0x91,0x97,0x8A,0xAF,0xB2,0xAB,0x00,0xB7,0x00,0x9E,0x00,0xC9}},
{270,{0x01,0x52,0x24,0x5D,0x84,0x92,0x7A,0x9F,0xB6,0xA8,0xB8,0xC2,0xB8,0x8F,0x97,0x89,0xB0,0xB2,0xAB,0x00,0xBA,0x00,0xA1,0x00,0xCC}},
{280,{0x01,0x52,0x24,0x5D,0x85,0x94,0x7C,0xA0,0xB6,0xA7,0xB9,0xC1,0xB5,0x91,0x97,0x8A,0xAE,0xB1,0xAB,0x00,0xBC,0x00,0xA3,0x00,0xCF}},
{290,{0x01,0x52,0x24,0x5D,0x86,0x97,0x7E,0xA1,0xB5,0xA7,0xBA,0xC3,0xB7,0x90,0x96,0x8A,0xAE,0xB0,0xAB,0x00,0xBF,0x00,0xA7,0x00,0xD1}},
{300,{0x01,0x52,0x24,0x5D,0xBA,0xCD,0xB3,0xAD,0xC0,0xB1,0xBF,0xC7,0xBC,0x90,0x97,0x8A,0xAA,0xAE,0xA5,0x00,0xC2,0x00,0xA8,0x00,0xD7}},
};
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
static __inline void spi_send_data(unsigned int data)
{
	unsigned int i;

	SET_LSCE_HIGH;
	SET_LSCK_HIGH;
	SET_LSDA_HIGH;
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
	UDELAY(1);
#else
	ndelay(100);
#endif
	SET_LSCE_LOW;

#if defined(BUILD_LK) || defined(BUILD_UBOOT)
	UDELAY(1);
#else
	ndelay(100);
#endif
	for (i = 0; i < 24; ++i){
		SET_LSCK_LOW;
		if (data & (1 << 23)){
			SET_LSDA_HIGH;
		} else {
			SET_LSDA_LOW;
		}
		data <<= 1;
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
		UDELAY(1);
#else
		ndelay(50);
#endif
		SET_LSCK_HIGH;
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
		UDELAY(1);
#else
		ndelay(50);
#endif
	}
	SET_LSCE_HIGH;
}

static __inline void Write_com(unsigned int cmd)
{
    unsigned int out = ((HX_WR_COM<<16) | (cmd & 0xFFFF));
    spi_send_data(out);
}

static __inline void Write_register(unsigned int data)
{
    unsigned int out = ((HX_WR_REGISTER<<16) |(data & 0xFFFF));
    spi_send_data(out);
}

static __inline unsigned short Read_register(void)
{
	unsigned char i,j,front_data;
	unsigned short value = 0;

	front_data=HX_RD_REGISTER;

	SET_LSCE_HIGH;
	SET_LSCK_HIGH;
	SET_LSDA_HIGH;
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
	UDELAY(1);
#else
	ndelay(100);
#endif
	SET_LSCE_LOW;
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
	UDELAY(1);
#else
	ndelay(200);
#endif
	for(i=0; i<8; i++){ // 8  Data
		SET_LSCK_LOW;
		if ( front_data& 0x80)
			SET_LSDA_HIGH;
		else
			SET_LSDA_LOW;
		front_data<<= 1;
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
		UDELAY(1);
#else
		ndelay(50);
#endif
		SET_LSCK_HIGH;
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
		UDELAY(1);
#else
		ndelay(50);
#endif
	}
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
	UDELAY(1);
#else
	ndelay(200);
#endif

	lcm_util.set_gpio_dir(LSDIO_GPIO_PIN, GPIO_DIR_IN);
	lcm_util.set_gpio_pull_enable(LSDIO_GPIO_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(LSDIO_GPIO_PIN, GPIO_PULL_UP);

	for(j=0; j<16; j++){ // 16 Data
		SET_LSCK_LOW;
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
		UDELAY(1);
#else
		ndelay(50);
#endif
		SET_LSCK_HIGH;
		value<<= 1;
		value |= GET_HX_SDI;
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
		UDELAY(1);
#else
		ndelay(50);
#endif
	}
	SET_LSCE_HIGH;
	lcm_util.set_gpio_dir(LSDIO_GPIO_PIN, GPIO_DIR_OUT);
	lcm_util.set_gpio_pull_enable(LSDIO_GPIO_PIN, GPIO_PULL_DISABLE);
	SET_GPIO_OUT(LSDIO_GPIO_PIN , 1);

	return value;
}

unsigned int ssd2825_read_s6e8aa_reg(unsigned int reg_s6e, unsigned int num, unsigned int *data)
{
	int i;
	int temp=0;
	int num_c6 = 0;

	Write_com(0x00B7);
	Write_register(0x03CB);
	
	//Write_com(0x00C0);//cancel operation
	//Write_register(0x0001);
	Write_com(0x00C1);//max return
	Write_register(num);
	
	Write_com(0x00BC);
	Write_register(0x0001);
	Write_com(0x00BF);
	Write_register(reg_s6e);//read register address
	//MDELAY(1);//don't delete

	Write_com(0x00C6);
	temp=Read_register();
	while((!(temp&0x1))&&(num_c6!=200))//read ready
	{
		Write_com(0x00C6);
		temp=Read_register();
		num_c6++;
//#if defined(BUILD_LK) || defined(BUILD_UBOOT)
//#else
//		printk("LCD esd check Read REG[C6]=0x%04x\n\n", temp);
//#endif
	}
	if (num_c6==200){//only for read failed
		*data = 0;
		return -1;
	}
	Write_com(0x00FF);//read the data returned by the MIPI slave
	for(i=0; i<(num+1)/2; i++){
		*data=Read_register();
		//MDELAY(1);//don't delete for debug
		LCM_DEBUG("Read amoled_reg[%02X]--data[%d]=0x%04x ######\n", reg_s6e, i, *data);
		data++;
	}
	return 1;
}

static void init_lcm_registers(void)
{
	LCM_DEBUG("[LCM************]: init_lcm_registers. \n");

	Write_com(0x00B1);
	Write_register(0x0102);
	Write_com(0x00B2);
	Write_register(0x0308);//0x040E 0x020E
	Write_com(0x00B3);
	Write_register(0x0D4B);
	Write_com(0x00B4);
	Write_register(0x02D0);
	Write_com(0x00B5);
	Write_register(0x0500);
	Write_com(0x00B6);
	Write_register(0x008B);

	Write_com(0x00DE);
	Write_register(0x0003);
	Write_com(0x00D6);
	Write_register(0x0004);
	Write_com(0x00B9);
	Write_register(0x0000);

	Write_com(0x00BA);
	Write_register(0x801E);//480Mbps

	Write_com(0x00BB);
	Write_register(0x0009);
	Write_com(0x00B9);
	Write_register(0x0001);
	Write_com(0x00B8);
	Write_register(0x0000);

	//F0
	Write_com(0x00B7);
	Write_register(0x034B);
	Write_com(0x00B8);
	Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0003);
	Write_com(0x00BF);
	Write_register(0x5AF0);
	Write_register(0x005A);
	//F1
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0003);
	Write_com(0x00BF);
	Write_register(0x5AF1);
	Write_register(0x005A);

	//0x11
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0001);
	Write_com(0x00BF);
	Write_register(0x0011);
	MDELAY(25);//change the delay from 5ms to 25ms for low battery power on

	//F8
	//Write_com(0x00B7);//v0.4 data sheet
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0027);
	Write_com(0x00BF);
	Write_register(0x19F8);
	Write_register(0x0035);
	Write_register(0x0000);
	Write_register(0x0093);
	Write_register(0x7D3C);
	Write_register(0x2708);
	Write_register(0x3F7D);
	Write_register(0x0810);
	Write_register(0x2000);
	Write_register(0x0804);
	Write_register(0x086E);
	Write_register(0x0000);
	Write_register(0x0802);
	Write_register(0x2308);
	Write_register(0xC023);
	Write_register(0x01C1);
	Write_register(0xC181);
	Write_register(0xC100);
	Write_register(0xF6F6);
	Write_register(0x00C1);	
	//F8
	/*Write_com(0x00B7);//for to-top
	Write_register(0x034B);
	Write_com(0x00B8);
	Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0027);
	Write_com(0x00BF);
	Write_register(0x1DF8);
	Write_register(0x0035);
	Write_register(0x0000);
	Write_register(0x0093);
	Write_register(0x7D3C);
	Write_register(0x2708);
	Write_register(0x3F7D);
	Write_register(0x0000);
	Write_register(0x2000);
	Write_register(0x0804);
	Write_register(0x0869);
	Write_register(0x0000);
	Write_register(0x0702);
	Write_register(0x2107);
	Write_register(0xC021);
	Write_register(0x01C1);
	Write_register(0xC181);
	Write_register(0xC100);
	Write_register(0xF6F6);
	Write_register(0x00C1);*/
	
	//F2
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0004);
	Write_com(0x00BF);
	Write_register(0x80F2);
	Write_register(0x0D03);

	//set gamma 230cd
	setGammaBacklight(15);
	
	//F7
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0002);
	Write_com(0x00BF);
	Write_register(0x03F7);

	//F6
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0003);
	Write_com(0x00BF);
	Write_register(0x00F6);
	Write_register(0x0002);

	//B6
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x000A);
	Write_com(0x00BF);
	Write_register(0x0CB6);
	Write_register(0x0302);
	Write_register(0xBF32);
	Write_register(0x4444);
	Write_register(0x00C0);

	//D9
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x000F);
	Write_com(0x00BF);
	Write_register(0x14D9);
	Write_register(0x0C40);
	Write_register(0xCECB);
	Write_register(0xC46E);
	Write_register(0x4007);
	Write_register(0xD541);
	Write_register(0x6000);
	Write_register(0x0019);
	
	//F4
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0008);
	Write_com(0x00BF);
	Write_register(0xCFF4);
	Write_register(0x120A);
	Write_register(0x1E10);
	Write_register(0x0233);

	//B1  setDynamicElvss
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0003);
	Write_com(0x00BF);
	Write_register(0x04B1);
	Write_register(0x0095);
	MDELAY(20);//ACL abnormity power on with USB in charge mode 

	//C1 ACL 
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x001D);
	Write_com(0x00BF);
	Write_register(0x47C1);
	Write_register(0x1353);
	Write_register(0x0053);
	Write_register(0x0200);
	Write_register(0x00CF);
	Write_register(0x0400);
	Write_register(0x00FF);
	Write_register(0x0000);	
	Write_register(0x0000);	
	Write_register(0x0000);	
	Write_register(0x0700);	
	Write_register(0x170F);	
	Write_register(0x2920);	
	Write_register(0x3C33);	
	Write_register(0x0046);	

	//C0
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0002);
	Write_com(0x00BF);
	if (acl_enable==1){
		Write_register(0x01C0);//ACL on
	}else{
		Write_register(0x00C0);//ACL off
	}
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void config_gpio(void)
{
	lcm_util.set_gpio_mode(LSCE_GPIO_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_mode(LSCK_GPIO_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_mode(LSDIO_GPIO_PIN, GPIO_MODE_00);

    lcm_util.set_gpio_dir(LSCE_GPIO_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_dir(LSCK_GPIO_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_dir(LSDIO_GPIO_PIN, GPIO_DIR_OUT);

	lcm_util.set_gpio_pull_enable(LSCE_GPIO_PIN, GPIO_PULL_DISABLE);
	lcm_util.set_gpio_pull_enable(LSCK_GPIO_PIN, GPIO_PULL_DISABLE);
	lcm_util.set_gpio_pull_enable(LSDIO_GPIO_PIN, GPIO_PULL_DISABLE);

	//set pwm output clk
	lcm_util.set_gpio_mode(SSD2825_MIPI_CLK_GPIO_PIN, GPIO_MODE_01);
	lcm_util.set_gpio_dir(SSD2825_MIPI_CLK_GPIO_PIN, GPIO_DIR_OUT);
	lcm_util.set_gpio_pull_enable(SSD2825_MIPI_CLK_GPIO_PIN, GPIO_PULL_DISABLE);
	lcd_set_pwm(PWM3);
	MDELAY(10);

	//set ssd2825 shut pin high
	lcm_util.set_gpio_mode(SSD2825_SHUT_GPIO_PIN, GPIO_MODE_00);
	lcm_util.set_gpio_dir(SSD2825_SHUT_GPIO_PIN, GPIO_DIR_OUT);
	lcm_util.set_gpio_pull_enable(SSD2825_SHUT_GPIO_PIN, GPIO_PULL_DISABLE);
	SET_GPIO_OUT(SSD2825_SHUT_GPIO_PIN , 1);
	MDELAY(1);

	//set s6e8aa poweron 2.2V output
	lcm_util.set_gpio_mode(LCD_POWER_GPIO_PIN, GPIO_MODE_00);
	lcm_util.set_gpio_dir(LCD_POWER_GPIO_PIN, GPIO_DIR_OUT);
	lcm_util.set_gpio_pull_enable(LCD_POWER_GPIO_PIN, GPIO_PULL_DISABLE);
	SET_GPIO_OUT(LCD_POWER_GPIO_PIN , 1);
	MDELAY(50);

//#ifdef BUILD_UBOOT
	//if (first_init_lcm){
	//	first_init_lcm = false;
	system_power_on();
	MDELAY(200);
	//}
//#endif
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
	params->ctrl   = LCM_CTRL_GPIO;
	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	params->dpi.mipi_pll_clk_ref  = 0;
	params->dpi.mipi_pll_clk_div1 = 46;
	params->dpi.mipi_pll_clk_div2 = 5;
	params->dpi.dpi_clk_div       = 2;
	params->dpi.dpi_clk_duty      = 1;

	params->dpi.clk_pol           = LCM_POLARITY_FALLING;
	params->dpi.de_pol            = LCM_POLARITY_RISING;
	params->dpi.vsync_pol         = LCM_POLARITY_FALLING;
	params->dpi.hsync_pol         = LCM_POLARITY_FALLING;

	params->dpi.hsync_pulse_width = 2;
	params->dpi.hsync_back_porch  = 6;
	params->dpi.hsync_front_porch = 75;
	params->dpi.vsync_pulse_width = 1;
	params->dpi.vsync_back_porch  = 2;
	params->dpi.vsync_front_porch = 13;

	params->dpi.format            = LCM_DPI_FORMAT_RGB888;
	params->dpi.rgb_order         = LCM_COLOR_ORDER_BGR;
	params->dpi.is_serial_output  = 0;

	params->dpi.intermediat_buffer_num = 2;
	params->dpi.io_driving_current = LCM_DRIVING_CURRENT_8MA;

	params->dpi.i2x_en   = 0;
	params->dpi.i2x_edge = 1;
}

static void lcm_init(void)
{
	 //unsigned short id;

	 config_gpio();
	 SET_RESET_PIN(0);
	 MDELAY(25);
	 SET_RESET_PIN(1);
	 MDELAY(10);
	 init_lcm_registers();

	 /*Write_com(0x00b0);
	 id=Read_register();
	 LCM_DEBUG("lcm_compare_id id is: %x\n", id);*/
}

static void lcm_suspend(void)
{
#ifdef BUILD_UBOOT
		g_resume_flag=1;
#endif

	Write_com(0x00B7);
	Write_register(0x034B);
	Write_com(0x00B8);
	Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0001);
	Write_com(0x00BF);
	Write_register(0x0010);
	MDELAY(120);

	//display off
	/*Write_com(0x00B7);
	Write_register(0x034B);
	Write_com(0x00B8);
	Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x0001);
	Write_com(0x00BF);
	Write_register(0x0028);*/

	Write_com(0x00B7);
	Write_register(0x0300);

	Write_com(0x00B7);
	Write_register(0x0304);
	Write_com(0x00B9);
	Write_register(0x0000);
	MDELAY(10);

/*#if defined(BUILD_LK)
#elif defined(BUILD_UBOOT)
#else
	mt_pwm_power_off(PWM3);
#endif*/
	LCM_DEBUG("lcm_suspend\n");
}

static void lcm_resume(void)
{
#ifdef BUILD_UBOOT
	if(g_resume_flag==0){
		g_resume_flag=1;
		return;
	}
#endif
	//config_gpio();

	SET_RESET_PIN(0);
	MDELAY(25);
	SET_RESET_PIN(1);
	MDELAY(10);
	init_lcm_registers();

	LCM_DEBUG("lcm_resume\n");
}

static unsigned int lcm_compare_id(void)
{
	unsigned short id;
	Write_com(0x00b0);
	id=Read_register();
    LCM_DEBUG("lcm_compare_id id is: %x\n", id);

	return (SSD2825_ID == id) ? 1 : 0;
}

static void setGammaBacklight(unsigned int index)
{
#ifdef LCD_DEBUG
	unsigned int i = 0;
#endif
	LCM_DEBUG("setGammaBacklight  index=%d ARRAY_OF(gamma_table)=%d\n", index,ARRAY_OF(gamma_table));
    //FA
	//Write_com(0x00B7);
	//Write_register(0x034B);
	//Write_com(0x00B8);
	//Write_register(0x0000);
	Write_com(0x00BC);
	Write_register(0x001A);
	Write_com(0x00BF);
	Write_register((gamma_table[index].gammaValue[0]<<8)|0xFA);
	Write_register((gamma_table[index].gammaValue[2]<<8)|(gamma_table[index].gammaValue[1]));
	Write_register((gamma_table[index].gammaValue[4]<<8)|(gamma_table[index].gammaValue[3]));
	Write_register((gamma_table[index].gammaValue[6]<<8)|(gamma_table[index].gammaValue[5]));
	Write_register((gamma_table[index].gammaValue[8]<<8)|(gamma_table[index].gammaValue[7]));
	Write_register((gamma_table[index].gammaValue[10]<<8)|(gamma_table[index].gammaValue[9]));
	Write_register((gamma_table[index].gammaValue[12]<<8)|(gamma_table[index].gammaValue[11]));
	Write_register((gamma_table[index].gammaValue[14]<<8)|(gamma_table[index].gammaValue[13]));
	Write_register((gamma_table[index].gammaValue[16]<<8)|(gamma_table[index].gammaValue[15]));
	Write_register((gamma_table[index].gammaValue[18]<<8)|(gamma_table[index].gammaValue[17]));
	Write_register((gamma_table[index].gammaValue[20]<<8)|(gamma_table[index].gammaValue[19]));
	Write_register((gamma_table[index].gammaValue[22]<<8)|(gamma_table[index].gammaValue[21]));
	Write_register((gamma_table[index].gammaValue[24]<<8)|(gamma_table[index].gammaValue[23]));

#ifdef LCD_DEBUG
	for (i=0; i<25; i++){
		LCM_DEBUG("gamma_table[%d] is %x\n", i, gamma_table[index].gammaValue[i]);
	}
#endif	
}

static void lcm_setbacklight(unsigned int level)   //back_light setting
{
	int index;

	index = level*(ARRAY_OF(gamma_table)-1)/255;
	if(g_last_Backlight_level == index){
		return;
	}
	if(level == 0){
		jrd_esd_check = false;    //disable esd check

		//display off
		Write_com(0x00B7);
		Write_register(0x034B);
		Write_com(0x00B8);
		Write_register(0x0000);
		Write_com(0x00BC);
		Write_register(0x0001);
		Write_com(0x00BF);
		Write_register(0x0028);
	}else{
		Write_com(0x00B7);
		Write_register(0x034B);
		Write_com(0x00B8);
		Write_register(0x0000);		
		setGammaBacklight(index);
			
		Write_com(0x00BC);
		Write_register(0x0002);
		Write_com(0x00BF);
		Write_register(0x03F7);
		if(!g_last_Backlight_level){//for wake-up amoled
		    MDELAY(30);	//for lcm flash

	        //0x29  display on
		    Write_com(0x00B7);
		    Write_register(0x034B);
		    Write_com(0x00B8);
		    Write_register(0x0000);
		    Write_com(0x00BC);
		    Write_register(0x0001);
		    Write_com(0x00BF);
		    Write_register(0x0029);
			
			jrd_esd_check = true;
    	}
    }
	g_last_Backlight_level=index;
    LCM_DEBUG("lcd-backlight  level=%d, index=%d\n", level,index);
}

static unsigned int lcm_esd_check(void)
{
	unsigned int buffer_0E;
	unsigned int dsi_err_05;
	unsigned int re;

	//LCM_DEBUG("lcm_esd_check -- begin\n");	

	if (jrd_esd_check){
		re=ssd2825_read_s6e8aa_reg(0x0E, 1, &buffer_0E);
		if (re<0){
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
#else
			printk("Fialed to read amoled_reg[0E] for LCD esd check\n");
#endif	
		}
		re=ssd2825_read_s6e8aa_reg(0x05, 1, &dsi_err_05);	
		if (re<0){
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
#else
			printk("Fialed to read amoled_reg[05] for LCD esd check\n");
#endif	
		}
		
		if (((buffer_0E&0x01)==0x00)&&(dsi_err_05==0x00)){
			LCM_DEBUG("lcd esd check return false\n");
			return false;
		}else{
			jrd_esd_check = false;
			esd_check_recover= true;
			return true;//esd recover
		}
	}
}

static unsigned int lcm_esd_recover(void)
{
	LCM_DEBUG("lcm_esd_recover -- begin\n");
	if (esd_check_recover){
		//display off
		Write_com(0x00B7);
		Write_register(0x034B);
		Write_com(0x00B8);
		Write_register(0x0000);
		Write_com(0x00BC);
		Write_register(0x0001);
		Write_com(0x00BF);
		Write_register(0x0028);
		MDELAY(20);

		Write_com(0x00B7);
		Write_register(0x034B);
		Write_com(0x00B8);
		Write_register(0x0000);
		Write_com(0x00BC);
		Write_register(0x0001);
		Write_com(0x00BF);
		Write_register(0x0010);
		MDELAY(10);
		
		Write_com(0x00B7);
		Write_register(0x0300);
		MDELAY(150);
		
		config_gpio();
		SET_RESET_PIN(0);
		MDELAY(25);
		SET_RESET_PIN(1);
		MDELAY(10);
		
		init_lcm_registers();
		MDELAY(150);
		lcm_setbacklight(180);
		
		//0x29	display on
		Write_com(0x00B7);
		Write_register(0x034B);
		Write_com(0x00B8);
		Write_register(0x0000);
		Write_com(0x00BC);
		Write_register(0x0001);
		Write_com(0x00BF);
		Write_register(0x0029);

		jrd_esd_check = true;
		esd_check_recover = false;
	}
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER s6e8aa_diablo_hd_lcm_drv =
{
	.name			= "s6e8aa_diablo_hd",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.set_backlight  = lcm_setbacklight,
	.esd_check		= lcm_esd_check,
	.esd_recover    = lcm_esd_recover,		
};

/*jrd add for LCD ACL function,PR371012*/
int set_acl_function(unsigned int enable){
	if(enable == 1) {
		acl_enable = 1;
		Write_com(0x00B7);
		Write_register(0x034B);
		Write_com(0x00B8);
		Write_register(0x0000);
		Write_com(0x00BC);
		Write_register(0x0002);
		Write_com(0x00BF);
		Write_register(0x01C0);//ACL on	
	}else {
		acl_enable= 0;
		Write_com(0x00B7);
		Write_register(0x034B);
		Write_com(0x00B8);
		Write_register(0x0000);
		Write_com(0x00BC);
		Write_register(0x0002);
		Write_com(0x00BF);
		Write_register(0x00C0);//ACL off
	}
}

int get_acl_status(void) {
	return acl_enable;
}
/*end*/


/**************** s6e8aa0 smart dimmy *******************
***************** s6e8aa0 smart dimmy *******************/
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
#else
const unsigned int backlight_table[GAMMA_MAX] = {
	100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 
	200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 
	300,
};

int dump_lcd_gamma_value(int data)
{
	unsigned int i,j;

	/*only dump all gamma value*/
	if (data==1){
		printk("\n\n************Smart Dimmy MTP Value***********\n");
		for (i = IV_1; i < IV_MAX; i++) {
			for (j = CI_RED; j < CI_MAX; j++)
				printk(" %c : 0x%08x(%04d)", color_name[j], mtp_data_dump[j][i], mtp_data_dump[j][i]);
			printk("\n");
		}
		
		printk("\n\n************Smart Dimmy Adjust MTP Value ************\n");
		for (i = IV_1; i < IV_MAX; i++) {
			for (j = CI_RED; j < CI_MAX; j++)
				printk(" %c : 0x%08x(%04d)", color_name[j], adjust_mtp[j][i], adjust_mtp[j][i]);
			printk("\n");
		}
		
		printk("\n\n************Smart Dimmy Dump All Gamma ************\n");		
		for (i = 0; i < 23; i++) {
			printk("xiewei Smart Dimmy:: gamma_%d; ", gamma_table[i].backlight_level);
			for (j = 0; j < 25; j++){
				printk(" 0x%02x,", gamma_table[i].gammaValue[j]);
			}
			printk("\n");
		}
		/*dump Calculate gamma_300cd */
		printk("Smart Dimmy Calculate gamma_300cd; ");
		for (j = 0; j < 25; j++){
			printk(" 0x%02x,", calc_gamma_300cd[j]);
		}
	}else{ /*re-calculate gamma value and printk out all value*/
		lcm_log_on = true;
		S6e8aa0_Smart_Dimmy();
		lcm_log_on = false;
		
		printk("\n\n************Smart Dimmy Dump All Gamma ************\n");		
		for (i = 0; i < 23; i++) {
			printk("xiewei Smart Dimmy:: gamma_%d; ", gamma_table[i].backlight_level);
			for (j = 0; j < 25; j++){
				printk(" 0x%02x,", gamma_table[i].gammaValue[j]);
			}
			printk("\n");
		}
		/*dump Calculate gamma_300cd */
		printk("Smart Dimmy Calculate gamma_300cd; ");
		for (j = 0; j < 25; j++){
			printk(" 0x%02x,", calc_gamma_300cd[j]);
		}
	}
}

static int init_gamma_table(struct lcd_info *lcd)
{
	int i, ret = 0, j;

	lcd->gamma_table = kzalloc(GAMMA_MAX * sizeof(u8 *), GFP_KERNEL);
	if (IS_ERR_OR_NULL(lcd->gamma_table)) {
		pr_err("failed to allocate gamma table\n");
		ret = -ENOMEM;
		goto err_alloc_gamma_table;
	}

	for (i = 0; i < GAMMA_MAX; i++) {
		lcd->gamma_table[i] = kzalloc(GAMMA_PARAM_SIZE * sizeof(u8), GFP_KERNEL);
		if (IS_ERR_OR_NULL(lcd->gamma_table[i])) {
			pr_err("failed to allocate gamma\n");
			ret = -ENOMEM;
			goto err_alloc_gamma;
		}
		lcd->gamma_table[i][0] = 0x01;
		calc_gamma_table(&lcd->smart, backlight_table[i]-1, lcd->gamma_table[i]+1);
	}

	/*Store gamma value*/
	for (i = 0; i < GAMMA_MAX-1; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++){
			gamma_table[i+2].gammaValue[j] = lcd->gamma_table[i][j];
		}
	}
	/*Store 300cd gamma value through calculate*/
	for (j = 0; j < GAMMA_PARAM_SIZE; j++){
		calc_gamma_300cd[j] = lcd->gamma_table[GAMMA_MAX-1][j];
	}

	/*free all memory*/
	i = GAMMA_MAX;
	while (i > 0) {
		kfree(lcd->gamma_table[i-1]);
		i--;
		lcd->gamma_table[i-1] = NULL;
	}
	kfree(lcd->gamma_table);
	lcd->gamma_table = NULL;
	
	return 0;

err_alloc_gamma:
	while (i > 0) {
		kfree(lcd->gamma_table[i-1]);
		i--;
		lcd->gamma_table[i-1] = NULL;
	}
	kfree(lcd->gamma_table);
	lcd->gamma_table = NULL;
err_alloc_gamma_table:
	return ret;
}

#if defined(SMART_DIMMY)
int S6e8aa0_Smart_Dimmy(void)
{
	unsigned int i,j,z;
	int ret = 0;
	unsigned int *mtp_D3_temp;
	unsigned char *mtp_data;
	struct lcd_info *lcd;
	
	printk("************s6ea880 smart dimmy ------- begin***********\n");

	lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (IS_ERR_OR_NULL(lcd)) {
		pr_err("failed to allocate for lcd struct\n");
		ret = -ENOMEM;
		goto err_alloc_lcd;
	}
	mtp_data = (unsigned char *)kzalloc(sizeof(unsigned char)*24, GFP_KERNEL);
	if (IS_ERR_OR_NULL(mtp_data)) {
		pr_err("failed to allocate for mtp_data\n");
		ret = -ENOMEM;
		goto err_alloc_mtp_data;
	}
	mtp_D3_temp = (unsigned int *)kzalloc(sizeof(unsigned int)*12, GFP_KERNEL);
	if (IS_ERR_OR_NULL(mtp_D3_temp)) {
		pr_err("failed to allocate for mtp_D3_temp\n");
		ret = -ENOMEM;
		goto err_alloc_mtp_D3_temp;
	}	

	/*read MTP D3h*/
	LCM_SMART_DIMMY("smart dimmy read MTP 0xD3 ------------begin\n");
	ret=ssd2825_read_s6e8aa_reg(0xD3, 24, mtp_D3_temp);
	if (ret<0){
		pr_err("Fialed to read amoled_reg[D3] for AMOLED smart dimmy\n");
		ret = -ENOENT;
		goto err_read_D3;
	}
	for (i=0; i<12; i++){
		mtp_data[j] = mtp_D3_temp[i]&0xFF;
		mtp_data[j+1] = (mtp_D3_temp[i]&0xFF00)>>8;
		LCM_SMART_DIMMY("Read amoled_regD3[%d]=0x%02x \n", j, mtp_data[j]);
		LCM_SMART_DIMMY("Read amoled_regD3[%d]=0x%02x \n", j+1, mtp_data[j+1]);
		j = j+2;
	}
	LCM_SMART_DIMMY("smart dimmy read MTP 0xD3 -------------end\n");

	/*only for dump mtp value*/
	for (i=0; i<7; i++){
		for (j=0; j<3; j++){
			mtp_data_dump[j][i] = mtp_data[z++];
		}
	}
	
	/*get default gamma 300cd value*/
	init_gamma300cd_table(&lcd->smart);
	for (i=0; i<24; i++){
		LCM_SMART_DIMMY("Gamma 300cd--data[%d]=0x%02x  [%d]\n", i, lcd->smart.default_gamma[i], lcd->smart.default_gamma[i]);
	}	
	
	/*calculate voltage table*/
	LCM_SMART_DIMMY("smart dimmy calc_voltage_table ----------begin\n");
	calc_voltage_table(&lcd->smart, mtp_data);
	LCM_SMART_DIMMY("smart dimmy calc_voltage_table ------------end\n");			

	/*calculate  FAh register data*/
	LCM_SMART_DIMMY("smart dimmy init_gamma_table ----------begin\n");
	init_gamma_table(lcd);
	LCM_SMART_DIMMY("smart dimmy init_gamma_table ------------end\n");

	printk("************s6ea880 smart dimmy ---------- end***********\n");

	/*free all memory*/
	kfree(mtp_D3_temp);
	kfree(mtp_data);
	kfree(lcd);	
	mtp_D3_temp = NULL;
	mtp_data = NULL;
	lcd = NULL;
	return 0;

err_read_D3:
	kfree(mtp_D3_temp);
	mtp_D3_temp = NULL;
err_alloc_mtp_D3_temp:
	kfree(mtp_data);
	mtp_data = NULL;
err_alloc_mtp_data:
	kfree(lcd);
	lcd = NULL;
err_alloc_lcd:
	return ret;	
}
#else
int S6e8aa0_Smart_Dimmy(void)
{
	return 0;
}
#endif

EXPORT_SYMBOL(S6e8aa0_Smart_Dimmy);
#endif
