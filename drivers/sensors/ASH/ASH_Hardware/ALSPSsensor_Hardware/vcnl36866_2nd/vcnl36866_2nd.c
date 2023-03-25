/* 
 * Copyright (C) 2014 ASUSTek Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*******************************************************/
/* ALSPS Front RGB Sensor VCNL36866 Module */
/******************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input/ASH.h>
#include <linux/regulator/consumer.h>
#include "vcnl36866_2nd.h"

/**************************/
/* Debug and Log System */
/************************/
#define MODULE_NAME			"ASH_HW"
#define SENSOR_TYPE_NAME		"ALSPS_2nd"

#undef dbg
#ifdef ASH_HW_DEBUG
	#define dbg(fmt, args...) pr_debug(KERN_DEBUG "[%s][%s]"fmt,MODULE_NAME,SENSOR_TYPE_NAME,##args)
#else
	#define dbg(fmt, args...)
#endif
#define log(fmt, args...) pr_debug("[%s]"fmt,MODULE_NAME,##args)
#define err(fmt, args...) pr_debug("[%s][%s]"fmt,MODULE_NAME,SENSOR_TYPE_NAME,##args)

/*****************************************/
/*Global static variable and function*/
/****************************************/
static struct i2c_client	*g_i2c_client = NULL;

/*For Inititalization Function */
static int vcnl36866_proximity_hw_set_config(void);
static int vcnl36866_light_hw_set_config(void);

/*ALSPS Sensor Part*/
static int vcnl36866_ALSPS_hw_check_ID(void);
static int vcnl36866_ALSPS_hw_init(struct i2c_client* client);
static ssize_t vcnl36866_ALSPS_hw_show_allreg(struct device *dev, struct device_attribute *attr, char *buf);
static int vcnl36866_ALSPS_hw_get_interrupt(void);
static int vcnl36866_ALSPS_hw_set_register(uint8_t reg, int value);
static int vcnl36866_ALSPS_hw_get_register(uint8_t reg);

/*Proximity Part*/
static int vcnl36866_proximity_hw_set_cs_standby_config(uint8_t cs_standby_conf_reg);
static int vcnl36866_proximity_hw_set_cs_config(uint8_t cs_conf_reg);
static int vcnl36866_proximity_hw_set_ps_disable(void);
static int vcnl36866_proximity_hw_set_ps_start(uint8_t ps_start);
static int vcnl36866_proximity_hw_turn_onoff(bool bOn);
static int vcnl36866_proximity_hw_get_adc(void);
static int vcnl36866_proximity_hw_set_hi_threshold(int hi_threshold);
static int vcnl36866_proximity_hw_set_lo_threshold(int low_threshold);
static int vcnl36866_proximity_hw_set_force_mode(uint8_t force_mode);
static int vcnl36866_proximity_hw_set_led_current(uint8_t led_current_reg);
static int vcnl36866_proximity_hw_set_led_duty_ratio(uint8_t led_duty_ratio_reg);
static int vcnl36866_proximity_hw_set_persistence(uint8_t persistence);
static int vcnl36866_proximity_hw_set_integration(uint8_t integration);
static int vcnl36866_proximity_hw_set_ps_mps(uint8_t mps);
static int vcnl36866_proximity_hw_set_ps_high_gain_mode(uint8_t enable);
static int vcnl36866_proximity_hw_set_autoK(int autok);
static int vcnl36866_regulator_init(void);
static int vcnl36866_regulator_enable(void);
static int vcnl36866_regulator_disable(void);

/* Light Sensor Part */
static int vcnl36866_light_hw_turn_onoff(bool bOn);
static int vcnl36866_light_hw_set_hi_threshold(int hi_threshold);
static int vcnl36866_light_hw_get_adc(void);
static int vcnl36866_light_hw_set_lo_threshold(int low_threshold);
static int vcnl36866_light_hw_set_reserved(uint8_t cs_start);
static int vcnl36866_light_hw_set_persistence(uint8_t persistence);
static int vcnl36866_light_hw_set_integration(uint8_t integration);
static int vcnl36866_light_hw_set_ALS_HD(uint8_t value);

/********************************/
/*For Inititalization Function */
/*******************************/
static int vcnl36866_ALSPS_hw_check_ID(void)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	

	/* Check the Device ID */
	ret = i2c_read_reg_u16(g_i2c_client, ID_REG, data_buf);
	if((ret < 0) || (data_buf[0] != VCNL36866_ID)){
		err("%s ERROR(ID_REG : 0x%02x%02x). \n", __FUNCTION__, data_buf[1], data_buf[0]);
		return -ENOMEM;
	}
	log("%s Success(ID_REG : 0x%02x%02x). \n", __FUNCTION__, data_buf[1], data_buf[0]);
	return 0;
}

static int vcnl36866_proximity_hw_set_config(void)
{
	int ret = 0;
	
	ret = vcnl36866_proximity_hw_set_cs_standby_config(VCNL36866_CS_STANDBY);
	if(ret < 0)		
		return ret;
	
	ret = vcnl36866_proximity_hw_set_cs_config(VCNL36866_CS_START);
	if(ret < 0)		
		return ret;
	
	/*must disable p-sensor interrupt befrore IST create*//*disable PS func*/
	ret = vcnl36866_proximity_hw_set_ps_disable();
	if(ret < 0)
		return ret;
		
	ret = vcnl36866_proximity_hw_set_ps_start(VCNL36866_PS_START);
		if (ret < 0)		
			return ret;
	
	ret = vcnl36866_proximity_hw_set_force_mode(VCNL36866_PS_START2);
		if (ret < 0)
			return ret;

	ret = vcnl36866_proximity_hw_set_led_duty_ratio(VCNL36866_PS_PERIOD_10);
	if(ret < 0)		
		return ret;

	ret = vcnl36866_proximity_hw_set_persistence(VCNL36866_PS_PERS_1);
	if(ret < 0)		
		return ret;

#if defined LONG_LENGTH_LOGN_IT_NON_PERF || defined LONG_LENGTH_LOGN_IT_PERF
	ret = vcnl36866_proximity_hw_set_integration(VCNL36866_PS_IT_4T);
	if(ret < 0)		
		return ret;
	ret = vcnl36866_proximity_hw_set_led_current(VCNL36866_VCSEL_I_17mA);
	if(ret < 0)		
		return ret;
#else
	ret = vcnl36866_proximity_hw_set_integration(VCNL36866_PS_IT_2T);
	if(ret < 0)		
		return ret;
	ret = vcnl36866_proximity_hw_set_led_current(VCNL36866_VCSEL_I_14mA);
	if(ret < 0)		
		return ret;
#endif
		
	ret = vcnl36866_proximity_hw_set_ps_mps(VCNL36866_PS_MPS_2);
	if(ret < 0)		
		return ret;
	
	ret = vcnl36866_proximity_hw_set_ps_high_gain_mode(VCNL36866_PS_HG_ENABLE);
	if(ret < 0)		
		return ret;

	return 0;	
}

static int vcnl36866_light_hw_set_config(void)
{
	int ret = 0;	
	
	ret = vcnl36866_light_hw_set_reserved(VCNL36866_CS_START2);
	if(ret < 0)
		return ret;
	
	ret = vcnl36866_light_hw_set_persistence(VCNL36866_CS_PERS_1);
	if(ret < 0)		
		return ret;

	ret = vcnl36866_light_hw_set_integration(VCNL36866_CS_IT_50MS);
	if(ret < 0)		
		return ret;
		
	ret = vcnl36866_light_hw_set_ALS_HD(VCNL36866_CS_HD_ENABLE);
	if(ret < 0)		
		return ret;
	
	return 0;	
}

/*********************************/
/*ALSPS Sensor Part*/
/*******************************/
static int vcnl36866_ALSPS_hw_init(struct i2c_client* client)
{
	int ret = 0;

	g_i2c_client = client;
	
	/*Init regulator setting */
	ret = vcnl36866_regulator_init();
	if(ret < 0){
		return ret;
	}
	vcnl36866_regulator_enable();
	
	/* Check the Device ID 
	 * Do Not return when check ID
	 */
	ret = vcnl36866_ALSPS_hw_check_ID();

	/*Set Proximity config */
	ret = vcnl36866_proximity_hw_set_config();
	if(ret < 0){		
		return ret;
	}

	 /*Set Light Sensor config*/
	ret = vcnl36866_light_hw_set_config();
	if(ret < 0){		
		return ret;
	}
	
	return 0;
}

static ssize_t vcnl36866_ALSPS_hw_show_allreg(struct device *dev, struct device_attribute *attr, char *buf){
	int ret = 0;
	uint8_t buffer[2] = {0};
	int reg = 0;
	
	for(reg = 0; reg < ARRAY_SIZE(vcnl36866_regs); reg++){
		ret = i2c_read_reg_u16(g_i2c_client, vcnl36866_regs[reg].reg, buffer);
		if(ret < 0){
			err("%s: show all Register ERROR. (REG:0x%x)\n", __FUNCTION__, vcnl36866_regs[reg].reg);
			return ret;
		}
		log("Show All Register (0x%x) = 0x%02x%02x\n", vcnl36866_regs[reg].reg, buffer[1], buffer[0]);
	}

	return 0;
}

static int vcnl36866_ALSPS_hw_set_register(uint8_t reg, int value)
{	
	int ret = 0;	
	uint8_t buf[2] = {0};

	buf[0] = value%256;
	buf[1] = value/256;
	
	ret = i2c_write_reg_u16(g_i2c_client, reg, buf);
	if(ret < 0){
		err("Set Register Value ERROR. (REG:0x%x)\n", reg);
		return ret;
	}
	log("Set Register Value (0x%X) = 0x%02x%02x\n", reg, buf[1], buf[0]);
	
	return 0;
}

static int vcnl36866_ALSPS_hw_get_register(uint8_t reg)
{
	int ret = 0;	
	uint8_t buf[2] = {0};
	int value;
	
	ret = i2c_read_reg_u16(g_i2c_client, reg, buf);
	if(ret < 0){
		err("ALSPS Get Register Value ERROR. (REG:0x%X)\n", reg);
		return ret;
	}
	log("Get Register Value (0x%x) = 0x%02x%02x\n", reg, buf[1], buf[0]);

	value = (buf[1] << 8) + buf[0];
	
	return value;
}

static int vcnl36866_ALSPS_hw_close_power(void)
{
	//vcnl36866_regulator_disable_1v8();
	vcnl36866_regulator_disable();
	return 0;
}

static int vcnl36866_ALSPS_hw_get_interrupt(void)
{
	uint8_t buf[2] = {0};
	bool check_flag = false;
	int alsps_int = 0;

	/* Read INT_FLAG will clean the interrupt */
	i2c_read_reg_u16(g_i2c_client, INT_FLAG, buf);

	/*Proximity Sensor work */
	if (buf[1]&INT_FLAG_PS_IF_AWAY || buf[1]&INT_FLAG_PS_IF_CLOSE){
		check_flag =true;
		if(buf[1]&INT_FLAG_PS_IF_AWAY){
			alsps_int |= ALSPS_INT_PS_AWAY;	
		}else if(buf[1]&INT_FLAG_PS_IF_CLOSE){		
			alsps_int |= ALSPS_INT_PS_CLOSE;
		}else{
			err("Can NOT recognize the Proximity INT_FLAG (0x%02x%02x)\n", buf[1], buf[0]);
			return -1;
		}
	}

	/* Light Sensor work */
	if(buf[1]&INT_FLAG_CS_IF_L || buf[1]&INT_FLAG_CS_IF_H){	
		check_flag =true;
		alsps_int |= ALSPS_INT_ALS;
	} 

	/* Interrupt Error */
	if(check_flag == false){		
		//err("Can NOT recognize the INT_FLAG (0x%02x%02x)\n", buf[1], buf[0]);
		return -1;
	}

	return alsps_int;
}

/******************/
/*Proximity Part*/
/******************/
static struct regulator *reg;
static int enable_3v_count = 0;
static int vcnl36866_regulator_init(void)
{
	int ret = 0;

	reg = regulator_get(&g_i2c_client->dev,"vcc_psensor");
    if (IS_ERR_OR_NULL(reg)) {
        ret = PTR_ERR(reg);
        err("Failed to get regulator vcc_psensor %d\n", ret);
        return ret;
    }
    
    ret = regulator_set_voltage(reg, 3300000, 3300000);
    if (ret) {
        err("Failed to set voltage for vcc_psensor reg %d\n", ret);
        return -1;
    }
    
    log("vcc_psensor regulator setting init");
    return ret;
}
static int vcnl36866_regulator_enable(void)
{
    int ret = 0, idx = 0;

    if(IS_ERR_OR_NULL(reg)){
        ret = PTR_ERR(reg);
        err("Failed to get regulator vcc_psensor %d\n", ret);
        return ret;
    }
	
    ret = regulator_set_load(reg, 10000);
    if(ret < 0){
        err("Failed to set load for vcc_psensor reg %d\n", ret);
        return ret;
    }
    
    ret = regulator_enable(reg);
    if(ret){
        err("Failed to enable vcc_psensor reg %d\n", ret);
        return -1;
    }
    
    for(idx=0; idx<10; idx++){
        if(regulator_is_enabled(reg) > 0){
            dbg("vcc_psensor regulator is enabled(idx=%d)", idx);
            break;
        }
    }
    if(idx >= 10){
        err("vcc_psensor regulator is enabled fail(retry count >= %d)", idx);
        return -1;
    }
    enable_3v_count++;
    log("Update vcc_psensor to NPM_mode");
    return ret;
}

static int vcnl36866_regulator_disable(void)
{
	int ret = 0;

    if(IS_ERR_OR_NULL(reg)){
        ret = PTR_ERR(reg);
        err("Failed to get regulator vcc_psensor %d\n", ret);
        return ret;
    }
	
    ret = regulator_set_load(reg, 0);
    if(ret < 0){
        err("Failed to set load for vcc_psensor reg %d\n", ret);
        return ret;
    }
    do{
	    ret = regulator_disable(reg);
	    if(ret){
	        err("Failed to enable vincentr reg %d\n", ret);
	        return -1;
	    }
		enable_3v_count--;
	}while(enable_3v_count > 0);
    
    log("Update vcc_psensor to LPM_mode");
    return ret;
}
static int vcnl36866_proximity_hw_turn_onoff(bool bOn)
{
	//static int PS_START = 0;
	int ret = 0;
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	if(bOn == 1){
		ret = vcnl36866_regulator_enable();
		if(ret < 0)
			return ret;
		/*Set Proximity config */
		ret = vcnl36866_proximity_hw_set_config();
		if(ret < 0){		
			return ret;
		}
	} else {
		//ret = vcnl36866_regulator_disable();
		if(ret < 0)
			return ret;
	}

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02x%02x)\n", power_state_data_buf[1], power_state_data_buf[0]);
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));
	
	if(bOn == 1){  /* power on */		
		power_state_data_buf[0] &= VCNL36866_PS_SD_MASK;
		
		ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
		if(ret < 0){
			err("Proximity power on ERROR (PS_CONF1)\n");
			//vcnl36866_regulator_disable();
			return ret;
		}else{
			log("Proximity power on (PS_CONF1 : 0x%x -> 0x%x)\n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	} else	{	/* power off */		
		power_state_data_buf[0] |= VCNL36866_PS_SD;
		
		ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
		if(ret < 0){
			err("Proximity power off ERROR (PS_CONF1)\n");
			vcnl36866_regulator_enable();
			return ret;
		}else{
			log("Proximity power off (PS_CONF1 : 0x%x -> 0x%x)\n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	}
	
	return 0;
}

static int vcnl36866_proximity_hw_interrupt_onoff(bool bOn)
{
	int ret = 0;
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02x%02x) \n", power_state_data_buf[1], power_state_data_buf[0]);
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));
	
	if(bOn == 1){ /* Enable INT */
		power_state_data_buf[0] &= VCNL36866_PS_INT_MASK;
		power_state_data_buf[0] |= VCNL36866_PS_INT_IN_AND_OUT;
		
		ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
		if(ret < 0){
			err("Proximity Enable INT ERROR (PS_CONF1)\n");
			return ret;
		}else{
			log("Proximity Enable INT (PS_CONF1 : 0x%x -> 0x%x)\n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	}else{ /* Disable INT */		
      	power_state_data_buf[0] &= VCNL36866_PS_INT_MASK;
		
		ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
		if(ret < 0){
			err("Proximity Disable INT ERROR (PS_CONF1)\n");
			return ret;
		} else {
			log("Proximity Disable INT (PS_CONF1 : 0x%x -> 0x%x)\n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	}
	
	return 0;
}

static int vcnl36866_proximity_hw_get_adc(void)
{
	int ret = 0;
	int adc = 0;
	uint8_t adc_buf[2] = {0, 0};	

	ret = i2c_read_reg_u16(g_i2c_client, PS_DATA, adc_buf);
	if(ret < 0){
		err("Proximity get adc ERROR. (PS_DATA)\n");
		return ret;
	}
	adc = (adc_buf[1] << 8) + adc_buf[0];
	dbg("Proximity get adc : 0x%02x%02x\n", adc_buf[1], adc_buf[0]); 
	
	return adc;
}

static int vcnl36866_proximity_hw_set_hi_threshold(int hi_threshold)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	
	/*Set Proximity High Threshold*/
	data_buf[0] = hi_threshold%256;
	data_buf[1] = hi_threshold/256;
	
	ret = i2c_write_reg_u16(g_i2c_client, PS_THDH, data_buf);
	if(ret < 0){
		err("Proximity write High Threshold ERROR. (PS_THDH : 0x%02x%02x)\n", data_buf[1], data_buf[0]);
	    	return ret;
	}else{
	    log("Proximity write High Threshold (PS_THDH : 0x%02x%02x)\n", data_buf[1], data_buf[0]);
	}		

	return 0;
}

static int vcnl36866_proximity_hw_set_lo_threshold(int low_threshold)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	

	/*Set Proximity Low Threshold*/	
	data_buf[0] = low_threshold%256;
	data_buf[1] = low_threshold/256;
	
	ret = i2c_write_reg_u16(g_i2c_client, PS_THDL, data_buf);
	if(ret < 0){
		err("Proximity write Low Threshold ERROR. (PS_THDL : 0x%02x%02x)\n", data_buf[1], data_buf[0]);
	    	return ret;
	}else{
	    log("Proximity write Low Threshold (PS_THDL : 0x%02x%02x)\n", data_buf[1], data_buf[0]);
	}

	return 0;
}

static int vcnl36866_proximity_hw_set_cs_standby_config(uint8_t cs_standby_conf_reg)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	uint8_t data_origin[2] = {0, 0};	

	/* set cs config */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF1, data_buf);
	if(ret < 0){
		err("Proximity read CS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read CS_CONF1 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	data_origin[0] = data_buf[0];
	data_origin[1] = data_buf[1];

	data_buf[0] &= VCNL36866_CS_STANDBY_MASK;
	data_buf[0] |= cs_standby_conf_reg;
		
	ret = i2c_write_reg_u16(g_i2c_client, CS_CONF1, data_buf);
	if(ret < 0){
		err("Proximity set CS standby (CS_CONF1) ERROR\n");
		return ret;
	}else{
		log("Proximity set CS standby bit (CS_CONF1 : 0x%x -> 0x%x)\n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

static int vcnl36866_proximity_hw_set_cs_config(uint8_t cs_conf_reg)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	uint8_t data_origin[2] = {0, 0};	

	/* set cs config */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF1, data_buf);
	if(ret < 0){
		err("Proximity read CS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read CS_CONF1 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	data_origin[0] = data_buf[0];
	data_origin[1] = data_buf[1];

	cs_conf_reg <<= VCNL36866_CS_START_SHIFT;
	data_buf[0] &= VCNL36866_CS_START_MASK;
	data_buf[0] |= cs_conf_reg;
		
	ret = i2c_write_reg_u16(g_i2c_client, CS_CONF1, data_buf);
	if(ret < 0){
		err("Proximity set CS start (CS_CONF1) ERROR\n");
		return ret;
	}else{
		log("Proximity set CS start bit (CS_CONF1 : 0x%x -> 0x%x)\n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

static int vcnl36866_proximity_hw_set_force_mode(uint8_t force_mode)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Persistence */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF3, data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF3 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF3 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	force_mode <<= VCNL36866_PS_START2_SHIFT;
	data_buf[0] &= VCNL36866_PS_START2_MASK;
	data_buf[0] |= force_mode;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF3, data_buf);
	if(ret < 0){
		err("Proximity set Persistence (PS_CONF3) ERROR\n");
		return ret;
	}else{
		log("Proximity set Persistence (PS_CONF3 : 0x%x -> 0x%x)\n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

static int vcnl36866_proximity_hw_set_led_current(uint8_t led_current_reg)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	uint8_t data_origin[2] = {0, 0};	

	/* set LED config */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF4, data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF4 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF4 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	data_origin[0] = data_buf[0];
	data_origin[1] = data_buf[1];

	led_current_reg <<= VCNL36866_VCSEL_I_SHIFT;
	data_buf[1] &= VCNL36866_VCSEL_I_MASK;
	data_buf[1] |= led_current_reg;
		
	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF4, data_buf);
	if(ret < 0){
		err("Proximity set LED Current (PS_CONF4) ERROR\n");
		return ret;
	}else{
		log("Proximity set LED Current (PS_CONF4 : 0x%x -> 0x%x)\n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int vcnl36866_proximity_hw_set_led_duty_ratio(uint8_t led_duty_ratio_reg)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity LED Duty Ratio */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	led_duty_ratio_reg <<= VCNL36866_PS_DR_SHIFT;
	data_buf[0] &= VCNL36866_PS_DR_MASK;
	data_buf[0] |= led_duty_ratio_reg;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if(ret < 0){
		err("Proximity set LED Duty Ratio (PS_CONF1) ERROR\n");
		return ret;
	}else{
		log("Proximity set LED Duty Ratio (PS_CONF1 : 0x%x -> 0x%x)\n", 
			data_origin[0], data_buf[0]);
	}

	return 0;	
}

static int vcnl36866_proximity_hw_set_persistence(uint8_t persistence)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Persistence */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	persistence <<= VCNL36866_PS_PERS_SHIFT;
	data_buf[0] &= VCNL36866_PS_PERS_MASK;
	data_buf[0] |= persistence;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if(ret < 0){
		err("Proximity set Persistence (PS_CONF1) ERROR\n");
		return ret;
	}else{
		log("Proximity set Persistence (PS_CONF1 : 0x%x -> 0x%x)\n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

static int vcnl36866_proximity_hw_set_integration(uint8_t integration)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Integration */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF2 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF2 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	integration <<= VCNL36866_PS_IT_SHIFT;
	data_buf[1] &= VCNL36866_PS_IT_MASK;
	data_buf[1] |= integration;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if(ret < 0){
		err("Proximity set Integration (PS_CONF2) ERROR\n");
		return ret;
	}else{
		log("Proximity set Integration (PS_CONF2 : 0x%x -> 0x%x)\n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int vcnl36866_proximity_hw_set_ps_mps(uint8_t mps)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Integration */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF2 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF2 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	mps <<= VCNL36866_PS_MPS_SHIFT;
	data_buf[1] &= VCNL36866_PS_MPS_MASK;
	data_buf[1] |= mps;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if(ret < 0){
		err("Proximity set PS Multi-Pulse setting (PS_CONF2) ERROR\n");
		return ret;
	}else{
		log("Proximity set PS Multi-Pulse setting (PS_CONF2 : 0x%x -> 0x%x)\n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int vcnl36866_proximity_hw_set_ps_high_gain_mode(uint8_t enable)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Integration */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF2 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF2 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	enable <<= VCNL36866_PS_HG_ENABLE_SHIFT;
	data_buf[1] &= VCNL36866_PS_HG_ENABLE_MASK;
	data_buf[1] |= enable;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if(ret < 0){
		err("Proximity set PS high gain mode (PS_CONF2) ERROR\n");
		return ret;
	}else{
		log("Proximity set PS high gain mode (PS_CONF2 : 0x%x -> 0x%x)\n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int vcnl36866_proximity_hw_set_ps_disable(void)
{
	int ret = 0;	
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02x%02x)\n", power_state_data_buf[1], power_state_data_buf[0]);
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));
		
	power_state_data_buf[0] |= VCNL36866_PS_SD;
	power_state_data_buf[0] &= VCNL36866_PS_INT_MASK; 
	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
	if(ret < 0){
		err("Disable Proximity interrupt befrore IST create ERROR (PS_CONF1)\n");
		return ret;
	}else{
		log("Disable Proximity interrupt befrore IST create (PS_CONF1: 0x%x -> 0x%x)\n", 
			power_state_data_origin[0], power_state_data_buf[0]);
	}
	return 0;
}

static int vcnl36866_proximity_hw_set_ps_start(uint8_t ps_start)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Integration */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if(ret < 0){
		err("Proximity read PS_CONF2 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF2 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	ps_start <<= VCNL36866_PS_START_SHIFT;
	data_buf[1] &= VCNL36866_PS_START_MASK;
	data_buf[1] |= ps_start;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if(ret < 0){
		err("Proximity set PS start (PS_CONF2) ERROR\n");
		return ret;
	}else{
		log("Proximity set PS start (PS_CONF2 : 0x%x -> 0x%x)\n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int vcnl36866_proximity_hw_set_autoK(int autok)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	int hi_threshold, low_threshold;

	/*Get High threshold value and adjust*/
	ret = i2c_read_reg_u16(g_i2c_client, PS_THDH, data_buf);
	if(ret < 0){
		err("Proximity get High Threshold ERROR. (PS_THDH)\n");
		return ret;
	}
	hi_threshold = (data_buf[1] << 8) + data_buf[0];
	dbg("Proximity get High Threshold : 0x%02x%02x\n", data_buf[1], data_buf[0]); 	
	vcnl36866_proximity_hw_set_hi_threshold(hi_threshold + autok);

	/*Get Low threshold value and adjust*/
	ret = i2c_read_reg_u16(g_i2c_client, PS_THDL, data_buf);
	if(ret < 0){
		err("Proximity get Low Threshold ERROR. (PS_THDL)\n");
		return ret;
	}
	low_threshold = (data_buf[1] << 8) + data_buf[0];
	dbg("Proximity get Low Threshold : 0x%02x%02x\n", data_buf[1], data_buf[0]); 
	vcnl36866_proximity_hw_set_lo_threshold(low_threshold + autok);

	return 0;
}

/***********************/
/* Light Sensor Part */
/***********************/
static int vcnl36866_light_hw_turn_onoff(bool bOn)
{
	int ret = 0;	
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF1, power_state_data_buf);	
	if(ret < 0){
		err("Light Sensor read CS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF1 (0x%02x%02x)\n", power_state_data_buf[1], power_state_data_buf[0]);
	
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));

	if(bOn == 1){ /* power on */			
		power_state_data_buf[0] &= VCNL36866_CS_SD_MASK;

		ret = i2c_write_reg_u16(g_i2c_client, CS_CONF1, power_state_data_buf);
		if(ret < 0){
			err("Light Sensor power on ERROR (CS_CONF1)\n");
			return ret;
		} else {
			log("Light Sensor power on (CS_CONF1 : 0x%x -> 0x%x)\n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}		
	}else{ /* power off */	
		power_state_data_buf[0] |= VCNL36866_CS_SD;
		
		ret = i2c_write_reg_u16(g_i2c_client, CS_CONF1, power_state_data_buf);
		if(ret < 0){
			err("Light Sensor power off ERROR (CS_CONF1) \n");
			return ret;
		}else{
			log("Light Sensor power off (CS_CONF1 : 0x%x -> 0x%x)\n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	}	

	return 0;
}

static int vcnl36866_light_hw_interrupt_onoff(bool bOn)
{
	int ret = 0;	
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF2, power_state_data_buf);	
	if(ret < 0){
		err("Light Sensor read CS_CONF2 ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF2 (0x%02x%02x)\n", power_state_data_buf[1], power_state_data_buf[0]);
	
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));

	if(bOn == 1){ /* Enable INT */
		power_state_data_buf[1] |= VCNL36866_CS_INT_EN;

		ret = i2c_write_reg_u16(g_i2c_client, CS_CONF2, power_state_data_buf);
		if(ret < 0){
			err("Light Sensor Enable INT ERROR (CS_CONF2)\n");
			return ret;
		}else{
			log("Light Sensor Enable INT (CS_CONF2 : 0x%x -> 0x%x)\n", 
				power_state_data_origin[1], power_state_data_buf[1]);
		}		
	}else{ /* Disable INT */	
      	power_state_data_buf[1] &= VCNL36866_CS_INT_MASK;
		
		ret = i2c_write_reg_u16(g_i2c_client, CS_CONF2, power_state_data_buf);
		if(ret < 0){
			err("Light Sensor Disable INT ERROR (CS_CONF2)\n");
			return ret;
		}else{
			log("Light Sensor Disable INT (CS_CONF2 : 0x%x -> 0x%x)\n", 
				power_state_data_origin[1], power_state_data_buf[1]);
		}
	}	

	return 0;
}

static int vcnl36866_light_hw_get_adc(void)
{
        int ret = 0;
        int adc = 0;
        uint8_t adc_buf[2] = {0, 0};

        ret = i2c_read_reg_u16(g_i2c_client, ALS_DATA, adc_buf);
        if(ret < 0){
                err("Light Sensor Get adc ERROR (ALS_DATA)\n");
                return ret;
        }
        adc = (adc_buf[1] << 8) + adc_buf[0];
        dbg("Light Sensor Get adc : 0x%02x%02x\n", adc_buf[1], adc_buf[0]);

        return adc;
}


static int vcnl36866_light_hw_set_hi_threshold(int hi_threshold)
{
	int ret = 0;	
	uint8_t data_buf[2] = {0, 0};	

	/*Set Light Sensor High Threshold*/	
	data_buf[0] = hi_threshold%256;
	data_buf[1] = hi_threshold/256;
	
	ret = i2c_write_reg_u16(g_i2c_client, CS_THDH, data_buf);
	if(ret < 0){
		err("[i2c] Light Sensor write High Threshold ERROR. (CS_THDH : 0x%02x%02x)\n", data_buf[1], data_buf[0]);
	    	return ret;
	}else{
	    dbg("[i2c] Light Sensor write High Threshold (CS_THDH : 0x%02x%02x)\n", data_buf[1], data_buf[0]);
	}		

	return 0;
}

static int vcnl36866_light_hw_set_lo_threshold(int low_threshold)
{
	int ret = 0;	
	uint8_t data_buf[2] = {0, 0};	

	/*Set Light Sensor Low Threshold*/		
	data_buf[0] = low_threshold%256;
	data_buf[1] = low_threshold/256;
	
	ret = i2c_write_reg_u16(g_i2c_client, CS_THDL, data_buf);
	if(ret < 0){
		err("[i2c] Light Sensor write Low Threshold ERROR. (CS_THDL : 0x%02x%02x)\n", data_buf[1], data_buf[0]);
	    	return ret;
	} else {
	    dbg("[i2c] Light Sensor write Low Threshold (CS_THDL : 0x%02x%02x)\n", data_buf[1], data_buf[0]);
	}

	return 0;
}

static int vcnl36866_light_hw_set_reserved(uint8_t cs_start)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Light Sensor Reserved bit */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF2, data_buf);
	if(ret < 0){
		err("Light Sensor read CS_CONF2 ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF2 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	cs_start    <<= VCNL36866_CS_START2_SHIFT;
	data_buf[1] &= VCNL36866_CS_START2_MASK;
	data_buf[1] |= cs_start;

	ret = i2c_write_reg_u16(g_i2c_client, CS_CONF2, data_buf);
	if(ret < 0){
		err("Light Sensor set Persistence (CS_CONF2) ERROR\n");
		return ret;
	}else{
		log("Light Sensor set Persistence (CS_CONF2 : 0x%x -> 0x%x)\n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int vcnl36866_light_hw_set_persistence(uint8_t persistence)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Light Sensor Persistence */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF2, data_buf);
	if(ret < 0){
		err("Light Sensor read CS_CONF2 ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF2 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	persistence <<= VCNL36866_CS_PERS_SHIFT;
	data_buf[1] &= VCNL36866_CS_PERS_MASK;
	data_buf[1] |= persistence;

	ret = i2c_write_reg_u16(g_i2c_client, CS_CONF2, data_buf);
	if(ret < 0){
		err("Light Sensor set Persistence (CS_CONF2) ERROR\n");
		return ret;
	}else{
		log("Light Sensor set Persistence (CS_CONF2 : 0x%x -> 0x%x)\n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int vcnl36866_light_hw_set_ALS_HD(uint8_t value)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Light Sensor Integration */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF2, data_buf);
	if(ret < 0){
		err("Light Sensor read CS_CONF1 ERROR\n");
		return ret;
	}
	log("Light Sensor read CS_CONF2 (0x%02x%02x),%x, %x\n", data_buf[1], data_buf[0], value, value << 5);
	memcpy(data_origin, data_buf, sizeof(data_buf));
	
	value = (value << 6);
	data_buf[1] = data_buf[1] | value;
	
	ret = i2c_write_reg_u16(g_i2c_client, CS_CONF2, data_buf);
	if(ret < 0){
		err("Light Sensor set ALS_HD (CS_CONF2) ERROR\n");
		return ret;
	}else{
		log("Light Sensor set ALS_HD (CS_CONF2 : 0x%x -> 0x%x)\n", 
			data_origin[1], data_buf[1]);
	}

	return 0;	
}


static int vcnl36866_light_hw_set_integration(uint8_t integration)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Light Sensor Integration */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF1, data_buf);
	if(ret < 0){
		err("Light Sensor read CS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF1 (0x%02x%02x)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	integration <<= VCNL36866_CS_IT_SHIFT;
	data_buf[0] &= VCNL36866_CS_IT_MASK;
	data_buf[0] |= integration;

	ret = i2c_write_reg_u16(g_i2c_client, CS_CONF1, data_buf);
	if(ret < 0){
		err("Light Sensor set Integration (CS_CONF1) ERROR\n");
		return ret;
	}else{
		log("Light Sensor set Integration (CS_CONF1 : 0x%x -> 0x%x)\n", 
			data_origin[0], data_buf[0]);
	}

	return 0;	
}

static uint8_t vcnl36866_light_hw_get_integration(void)
{
	int ret = 0;	
	uint8_t data_buf[2] = {0, 0};	

	/* set Light Sensor Integration */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF1, data_buf);
	if(ret < 0){
		err("Light Sensor read CS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF1 (0x%02x%02x)\n", data_buf[1], data_buf[0]);

	data_buf[0] &= VCNL36866_CS_IT_NOT_MASK;
	data_buf[0] >>= VCNL36866_CS_IT_SHIFT;

	return data_buf[0];	
}

static struct psensor_hw psensor_hw_vcnl36866 = {
	.proximity_low_threshold_default = VCNL36866_PROXIMITY_THDL_DEFAULT,
	.proximity_hi_threshold_default = VCNL36866_PROXIMITY_THDH_DEFAULT,
	.proximity_crosstalk_default = VCNL36866_PROXIMITY_INF_DEFAULT,
	.proximity_autok_min = VCNL36866_PROXIMITY_AUTOK_MIN,
	.proximity_autok_max = VCNL36866_PROXIMITY_AUTOK_MAX,
	
	.proximity_hw_turn_onoff = vcnl36866_proximity_hw_turn_onoff,
	.proximity_hw_interrupt_onoff = vcnl36866_proximity_hw_interrupt_onoff,
	.proximity_hw_get_adc = vcnl36866_proximity_hw_get_adc,
	.proximity_hw_set_hi_threshold = vcnl36866_proximity_hw_set_hi_threshold,
	.proximity_hw_set_lo_threshold = vcnl36866_proximity_hw_set_lo_threshold,
	.proximity_hw_set_autoK = vcnl36866_proximity_hw_set_autoK,
};

static struct lsensor_hw lsensor_hw_vcnl36866 = {
	.light_max_threshold = VCNL36866_LIGHT_MAX_THRESHOLD,
	.light_calibration_default = VCNL36866_LIGHT_CALIBRATION_DEFAULT,
		
	.light_hw_turn_onoff = vcnl36866_light_hw_turn_onoff,
	.light_hw_interrupt_onoff = vcnl36866_light_hw_interrupt_onoff,
	.light_hw_get_adc = vcnl36866_light_hw_get_adc,
	.light_hw_set_hi_threshold = vcnl36866_light_hw_set_hi_threshold,
	.light_hw_set_lo_threshold = vcnl36866_light_hw_set_lo_threshold,
	.light_hw_set_integration = vcnl36866_light_hw_set_integration,
	.light_hw_get_integration = vcnl36866_light_hw_get_integration,
};

static struct ALSPS_hw ALSPS_hw_vcnl36866 = {	
	.vendor = "Capella",
	.module_number = "vcnl36866",

	.ALSPS_hw_check_ID = vcnl36866_ALSPS_hw_check_ID,
	.ALSPS_hw_init = vcnl36866_ALSPS_hw_init,
	.ALSPS_hw_get_interrupt = vcnl36866_ALSPS_hw_get_interrupt,
	.ALSPS_hw_show_allreg = vcnl36866_ALSPS_hw_show_allreg,
	.ALSPS_hw_set_register = vcnl36866_ALSPS_hw_set_register,
	.ALSPS_hw_get_register = vcnl36866_ALSPS_hw_get_register,
	.ALSPS_hw_close_power = vcnl36866_ALSPS_hw_close_power,

	.mpsensor_hw = &psensor_hw_vcnl36866,
	.mlsensor_hw = &lsensor_hw_vcnl36866,
};

ALSPS_hw* ALSPS_hw_vcnl36866_getHardware_2nd(void)
{
	ALSPS_hw* ALSPS_hw_client = NULL;
	ALSPS_hw_client = &ALSPS_hw_vcnl36866;
	return ALSPS_hw_client;
}
