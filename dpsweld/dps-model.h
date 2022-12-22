#ifndef __DPS_MODEL_H__
#define __DPS_MODEL_H__

// if no other DPSxxxx model is specified, we will assume DPS5005

/*
 * K - angle factor
 * C - offset
 * for ADC
 * K = (I1-I2)/(ADC1-ADC2)
 * C = I1-K*ADC1
 * for DAC
 * K = (DAC1-DAC2)/(I1-I2)
 * C = DAC1-K*I1
 */

/* Exmaple for voltage ADC calibration
 * you can see real ADC value in CLI mode's stat:
 *                         ADC(U): 394
 * and you have to measure real voltage with reference voltmeter.
 * ADC value -> Voltage calculation
 * ADC  394 =  5001 mV
 * ADC  782 = 10030 mV
 * ADC 1393 = 18000 mV
 * K = (U1-U2)/(ADC1-ADC2) = (18000-5001)/(1393-394) = 76.8461538
 * C = U1-K*ADC1= 5001-K*394  = -125.732
 *
 * */

/* Example for voltage DAC calibration
 * DAC value -> Voltage
 * You can write DAC values to DHR12R1 register 0x40007408
 * over openocd console:
 * mww 0x40007408 77
 * and measure real voltage with reference voltmeter.
 *
 * DAC   77 =  1004 mV
 *      872 = 12005 mV
 *  K = (DAC1-DAC2)/(U1-U2) = (77-872)/(1004-12005) = .072266
 *  C = DAC1-K*U1 = 77-(K*1004) = 4.44477774
 *
 * */

/** Contribution by @cleverfox */
#ifndef CONFIG_DPS_MAX_CURRENT
#define CONFIG_DPS_MAX_CURRENT (5000)
#endif
#define CURRENT_DIGITS 1
#define CURRENT_DECIMALS 3
#define ADC_CHA_IOUT_GOLDEN_VALUE  (0x45)
#define A_ADC_K (float)1.713f
#define A_ADC_C (float)-118.51f
#define A_DAC_K (float)0.652f
#define A_DAC_C (float)288.611f
#define V_DAC_K (float)0.0768f
#define V_DAC_C (float)2.929f
#define V_ADC_K (float)13.022f
#define V_ADC_C (float)-113.543f

/** These are constant across all models currently but may require tuning for each model */
#ifndef VIN_ADC_K
 #define VIN_ADC_K (float)16.746f
#endif

#ifndef VIN_ADC_C
 #define VIN_ADC_C (float)64.112f
#endif

#define VIN_VOUT_RATIO (float)1.1f /** (Vin / VIN_VOUT_RATIO) = Max Vout */

#endif // __DPS_MODEL_H__
