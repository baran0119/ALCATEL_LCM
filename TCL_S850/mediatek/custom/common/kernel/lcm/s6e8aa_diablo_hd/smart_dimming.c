/**************************************************************************/
/*                                                            			 */
/*                            PRESENTATION                          	 */
/*              Copyright (c) 2012 JRD Communications, Inc.        		 */
/***************************************************************************/
/*                                                                        */
/*    This material is company confidential, cannot be reproduced in any  */
/*    form without the written permission of JRD Communications, Inc.      */
/*                                                                         */
/*---------------------------------------------------------------------------*/
/*   Author :    XIE Wei        wei.xie@tcl.com                             */
/*---------------------------------------------------------------------------*/
/*    Comments :    s6e8aa0-AMS465GS45 AMOLED Smart Dimmy   [ Dibalo HD project ]  */
/*    File      : mediatek/custom/common/kernel/lcm/s6e8aa_diablo_hd/smart_dimming.c*/
/*    Labels   :                                                             */
/*=========================================================*/
/* Modifications on Features list / Changes Request / Problems Report    */                                                                                                           
/* date    | author           | Key                      | comment           */
/*---------|------------------|--------------------------|-------------------*/
/*2013/01/05 |
/*---------|------------------|--------------------------|-------------------*/
/*---------|------------------|--------------------------|-------------------*/
/*========================================================*/
#if defined(BUILD_LK) || defined(BUILD_UBOOT)
#else
#include "smart_dimming.h"

extern bool lcm_log_on;
s16 adjust_mtp[CI_MAX][IV_MAX];


#define LCM_SMART_DIMMY(fmt, arg...)									\
	do {																\
		if (lcm_log_on) printk(" " fmt, ##arg);		\
	}while(0)


static s16 int9_to_int16(s16 v)
{
	return (s16)(v << 7) >> 7;
}

u32 calc_v1_volt(s16 gamma, int rgb_index, u32 (*adjust_volt)[AD_IVMAX])
{
	u32 ret = 0;
	u32 v0;
	int temp;

	v0 = adjust_volt[rgb_index][AD_IV0] * 10;
	ret = v0-v0*(gamma+5)/600;
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;

	return ret;
}

u32 calc_v15_volt(s16 gamma, int rgb_index, u32 (*adjust_volt)[AD_IVMAX])
{
	int ret = 0;
	u32 v1, v35;
	u32 ratio = 0;
	int temp;

	v1 = adjust_volt[rgb_index][AD_IV1] * 10;
	v35 = adjust_volt[rgb_index][AD_IV35] * 10;
	ret = v1-(v1-v35)*(gamma+20)/320;
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;

	return ret;
}

u32 calc_v35_volt(s16 gamma, int rgb_index, u32 (*adjust_volt)[AD_IVMAX])
{
	int ret = 0;
	u32 v1, v59;
	u32 ratio = 0;
	int temp;

	v1 = adjust_volt[rgb_index][AD_IV1] * 10;
	v59 = adjust_volt[rgb_index][AD_IV59] * 10;
	ret = v1-(v1-v59)*(gamma+65)/320;
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;

	return ret;
}

u32 calc_v50_v59_volt(s16 gamma, int rgb_index, u32 (*adjust_volt)[AD_IVMAX])
{
	int ret = 0;
	u32 v1, v87;
	u32 ratio = 0;
	int temp;	

	v1 = adjust_volt[rgb_index][AD_IV1] * 10;
	v87 = adjust_volt[rgb_index][AD_IV87] * 10;
	ret = v1-(v1-v87)*(gamma+65)/320;
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;

	return ret;
}

u32 calc_v87_volt(s16 gamma, int rgb_index, u32 (*adjust_volt)[AD_IVMAX])
{
	int ret = 0;
	u32 v1, v171;
	u32 ratio = 0;
	int temp;

	v1 = adjust_volt[rgb_index][AD_IV1] * 10;
	v171 = adjust_volt[rgb_index][AD_IV171] * 10;
	ret = v1-(v1-v171)*(gamma+65)/320;
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;	

	return ret;
}

u32 calc_v171_volt(s16 gamma, int rgb_index, u32 (*adjust_volt)[AD_IVMAX])
{
	int ret = 0;
	u32 v1, v255;
	u32 ratio = 0;
	int temp;	

	v1 = adjust_volt[rgb_index][AD_IV1] * 10;
	v255 = adjust_volt[rgb_index][AD_IV255] * 10;
	ret = v1-(v1-v255)*(gamma+65)/320;
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;	
	
	return ret;
}

u32 calc_v255_volt(s16 gamma, int rgb_index, u32 (*adjust_volt)[AD_IVMAX])
{
	u32 ret = 0;
	int temp;
	u32 v0;

	v0 = adjust_volt[rgb_index][AD_IV0] * 10;
	ret = v0-v0*(gamma+100)/600;
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;

	return ret;
}

u8 calc_voltage_table(struct str_smart_dim *smart, const u8 *mtp)
{
	int c, i, j;
	int offset1 = 0;
	int offset = 0;
	u32 v1, v2;
	u32 ratio;
	s16 t1, t2;
	u8 range_index;
	u8 table_index = 0;

	u32(*calc_volt[IV_MAX])(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX]) = {
		calc_v1_volt,
		calc_v15_volt,
		calc_v35_volt,
		calc_v50_v59_volt,
		calc_v87_volt,
		calc_v171_volt,
		calc_v255_volt,
	};
	u8 calc_seq[6] = {IV_1, IV_171, IV_87, IV_59, IV_35, IV_15};
	u8 ad_seq[6] = {AD_IV1, AD_IV171, AD_IV87, AD_IV59, AD_IV35, AD_IV15};

	memset(adjust_mtp, 0, sizeof(adjust_mtp));

	for (c = CI_RED; c < CI_MAX; c++) {
		/* for V0 All RGB Voltage Value is Reference Voltage 4.6v */
		smart->adjust_volt[c][AD_IV0] = VREG_OUT_1000;
		
		offset = IV_255*CI_MAX+c*2;
		offset1 = IV_255*(c+1)+(c*2);
		t1 = int9_to_int16(mtp[offset1]<<8|mtp[offset1+1]);//get register 7 ande 8 value 
		t2 = int9_to_int16(smart->default_gamma[offset]<<8|
			smart->default_gamma[offset+1]) + t1;
		smart->mtp[c][IV_255] = t1;
		adjust_mtp[c][IV_255] = t1;//MTP D3 
		smart->adjust_volt[c][AD_IV255] = calc_volt[IV_255](t2, c, smart->adjust_volt);
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_1; i < IV_255; i++) {
			t1 = (s8)mtp[(calc_seq[i])+(c*8)];
			t2 = smart->default_gamma[CI_MAX*calc_seq[i]+c] + t1;
			smart->mtp[c][calc_seq[i]] = t1;
			adjust_mtp[c][calc_seq[i]] = t1;
			smart->adjust_volt[c][ad_seq[i]] = calc_volt[calc_seq[i]](t2, c, smart->adjust_volt);
		}
	}

	/*calculate voltage table*/
	for (i = 0; i < AD_IVMAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			smart->ve[table_index].v[c] = smart->adjust_volt[c][i];

		range_index = 0;
		for (j = table_index+1; j < table_index+range_table_count[i]; j++) {
			for (c = CI_RED; c < CI_MAX; c++) {
				if (smart->t_info[i].offset_table != NULL)
					ratio = smart->t_info[i].offset_table[range_index] * smart->t_info[i].rv;
				else
					ratio = (range_table_count[i]-(range_index+1)) * smart->t_info[i].rv;

				v1 = smart->adjust_volt[c][i+1] << 15;
				v2 = (smart->adjust_volt[c][i] - smart->adjust_volt[c][i+1])*ratio;
				smart->ve[j].v[c] = ((v1+v2) >> 15);
			}
			range_index++;
		}
		table_index = j;
	}

	LCM_SMART_DIMMY(KERN_INFO "++++++++++ MTP VALUE ++++++++++++\n");
	for (i = IV_1; i < IV_MAX; i++) {
		LCM_SMART_DIMMY("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			LCM_SMART_DIMMY("  %c : 0x%08x(%04d)", color_name[c], smart->mtp[c][i], smart->mtp[c][i]);
		LCM_SMART_DIMMY("\n");
	}
	LCM_SMART_DIMMY(KERN_INFO "\n\n++++++++ ADJUST VOLTAGE ++++++++++\n");
	for (i = AD_IV0; i < AD_IVMAX; i++) {
		LCM_SMART_DIMMY("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			LCM_SMART_DIMMY("  %c : %04dV", color_name[c], smart->adjust_volt[c][i]);
		LCM_SMART_DIMMY("\n");
	}

	LCM_SMART_DIMMY(KERN_INFO "\n\n+++++++GAMMA VOLTAGE TABLE ++++++++++\n");
	for (i = 0; i < 256; i++) {
		LCM_SMART_DIMMY("Gray Level : %03d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			LCM_SMART_DIMMY("  %c : %04dV", color_name[c], smart->ve[i].v[c]);
		LCM_SMART_DIMMY("\n");
	}
	return 0;
}

int init_gamma300cd_table(struct str_smart_dim *smart)
{
	int i;

	for (i = 0; i < IV_TABLE_MAX; i++) {
		smart->t_info[i].offset_table = offset_table[i];
		smart->t_info[i].rv = table_radio[i];
	}
	smart->flooktbl = flookup_table;
	smart->g300_gra_tbl = gamma_300_gra_table;
	smart->g22_tbl = gamma_22_table;
	smart->default_gamma = gamma_300cd;
	
	return 0;
}

u32 lookup_vtbl_idx(struct str_smart_dim *smart, u32 gamma)
{
	u32 lookup_index;
	u16 table_count, table_index;
	u32 gap, i;
	u32 minimum = smart->g300_gra_tbl[255];
	u32 candidate = 0;
	u32 offset = 0;

	/*LCM_SMART_DIMMY("Input Gamma Value : %d\n", gamma); */
	lookup_index = (gamma/VALUE_DIM_1000)+1;
	if (lookup_index > MAX_GRADATION) {
		LCM_SMART_DIMMY(KERN_ERR "ERROR Wrong input value  LOOKUP INDEX : %d\n", lookup_index);
		return 0;
	}
	/*LCM_SMART_DIMMY("lookup index : %d\n",lookup_index);*/

	if (smart->flooktbl[lookup_index].count) {
		if (smart->flooktbl[lookup_index-1].count) {
			table_index = smart->flooktbl[lookup_index-1].entry;
			table_count = smart->flooktbl[lookup_index].count + smart->flooktbl[lookup_index-1].count;
		} else {
			table_index = smart->flooktbl[lookup_index].entry;
			table_count = smart->flooktbl[lookup_index].count;
		}
	} else {
		offset += 1;
		while (!(smart->flooktbl[lookup_index+offset].count || smart->flooktbl[lookup_index-offset].count))
			offset++;

		if (smart->flooktbl[lookup_index-offset].count)
			table_index = smart->flooktbl[lookup_index-offset].entry;
		else
			table_index = smart->flooktbl[lookup_index+offset].entry;
		table_count = smart->flooktbl[lookup_index+offset].count + smart->flooktbl[lookup_index-offset].count;
	}

	/*To find the nearest value*/
	for (i = 0; i < table_count; i++) {
		if (gamma > smart->g300_gra_tbl[table_index])
			gap = gamma - smart->g300_gra_tbl[table_index];
		else
			gap = smart->g300_gra_tbl[table_index] - gamma;

		if (gap == 0) {
			candidate = table_index;
			break;
		}

		if (gap < minimum) {
			minimum = gap;
			candidate = table_index;
		}
		table_index++;
	}
	
	return candidate;
}

u32 calc_v1_reg(int ci, u32 (*dv)[IV_MAX])
{
	u32 ret;
	u32 v1;

	v1 = dv[ci][IV_1];
	ret = (VREG_OUT_1000-v1)/VREG_OUT_1000*600-5;

	return ret;
}

u32 calc_v15_reg(int ci, u32 (*dv)[IV_MAX])
{
	u32 t1, t2;
	u32 v1, v15, v35;
	u32 ret;
	int temp;

	v1 = dv[ci][IV_1];
	v15 = dv[ci][IV_15];
	v35 = dv[ci][IV_35];
	ret = (v1-v15)*320*10/(v1-v35);
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;
	ret = ret - 20;

	return ret;
}

u32 calc_v35_reg(int ci, u32 (*dv)[IV_MAX])
{
	u32 t1, t2;
	u32 v1, v35, v57;
	u32 ret;
	int temp;

	v1 = dv[ci][IV_1];
	v35 = dv[ci][IV_35];
	v57 = dv[ci][IV_59];
	ret = (v1-v35)*320*10/(v1-v57);
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;
	ret = ret - 65;

	return ret;
}

u32 calc_v50_reg(int ci, u32 (*dv)[IV_MAX])
{
	u32 t1, t2;
	u32 v1, v57, v87;
	u32 ret;
	int temp;

	v1 = dv[ci][IV_1];
	v57 = dv[ci][IV_59];
	v87 = dv[ci][IV_87];
	ret = (v1-v57)*320*10/(v1-v87);
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;
	ret = ret - 65;

	return ret;
}

u32 calc_v87_reg(int ci, u32 (*dv)[IV_MAX])
{
	u32 t1, t2;
	u32 v1, v87, v171;
	u32 ret;
	int temp;

	v1 = dv[ci][IV_1];
	v87 = dv[ci][IV_87];
	v171 = dv[ci][IV_171];
	ret = (v1-v87)*320*10/(v1-v171);
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;
	ret = ret - 65;

	return ret;
}

u32 calc_v171_reg(int ci, u32 (*dv)[IV_MAX])
{
	u32 t1, t2;
	u32 v1, v171, v255;
	u32 ret;
	int temp;

	v1 = dv[ci][IV_1];
	v171 = dv[ci][IV_171];
	v255 = dv[ci][IV_255];
	ret = (v1-v171)*320*10/(v1-v255);
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;
	ret = ret - 65;

	return ret;
}

u32 calc_v255_reg(int ci, u32 (*dv)[IV_MAX])
{
	u32 ret;
	u32 v255;
	int temp;

	v255 = dv[ci][IV_255];
	ret = (VREG_OUT_1000-v255)*6000/VREG_OUT_1000;
	temp = ret/10;
	ret = ((ret%10)>5)?	(temp+1):temp;
	ret = ret - 100;

	return ret;
}

u32 calc_gamma_table(struct str_smart_dim *smart, u32 gv, u8 result[])
{
	u32 i, c;
	u32 temp;
	u32 lidx;
	u32 dv[CI_MAX][IV_MAX];
	s16 gamma[CI_MAX][IV_MAX];
	u16 offset;
	u32(*calc_reg[IV_MAX])(int c, u32 dv[CI_MAX][IV_MAX]) = {
		calc_v1_reg,//no call
		calc_v15_reg,
		calc_v35_reg,
		calc_v50_reg,
		calc_v87_reg,
		calc_v171_reg,
		calc_v255_reg,
	};

	memset(gamma, 0, sizeof(gamma));

	for (c = CI_RED; c < CI_MAX; c++)
		dv[c][0] = smart->adjust_volt[c][AD_IV1];

	/*found lookup voltage  dv[]*/
	for (i = IV_15; i < IV_MAX; i++) {
		temp = smart->g22_tbl[i] * gv;
		lidx = lookup_vtbl_idx(smart, temp);
		for (c = CI_RED; c < CI_MAX; c++)
			dv[c][i] = smart->ve[lidx].v[c];
	}

	/* for IV1 does not calculate value just use default gamma value (IV1) */
	for (c = CI_RED; c < CI_MAX; c++)
		gamma[c][IV_1] = smart->default_gamma[c];

	/*calculate gamma*/
	for (i = IV_15; i < IV_MAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			gamma[c][i] = (s16)calc_reg[i](c, dv) - smart->mtp[c][i];
	}

	/*get gamma result*/
	for (c = CI_RED; c < CI_MAX; c++) {
		offset = IV_255*CI_MAX+c*2;
		result[offset+1] = gamma[c][IV_255];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_1; i < IV_255; i++)
			result[(CI_MAX*i)+c] = gamma[c][i];
	}

	LCM_SMART_DIMMY(KERN_INFO "\n*********** FOUND VOLTAGE ***** (%d) *************\n", gv+1);
	for (i = IV_1; i < IV_MAX; i++) {
		LCM_SMART_DIMMY("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			LCM_SMART_DIMMY("%c : %04dV  ", color_name[c], dv[c][i]);
		LCM_SMART_DIMMY("\n");
	}

	return 0;
}
#endif
