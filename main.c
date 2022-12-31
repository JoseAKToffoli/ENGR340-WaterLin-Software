/* ======================================== 
 *
 *          WATERLIN MAIN CODE
 * 
 *  This file contains the main operating
 * code for runnign the smart watering system
 * "WaterLin" developed for the F22:ENGR340 
 * final project.
 *
 * Jose A. K. Toffoli 
 * 2022-12-15
 *
 * ========================================
*/
#include "project.h"

/*              GLOBAL VARS INITIALIZATION                */
#define high 1u                     // High output state.
#define low  0u                     // Low output state.

int8 senCalibMode = 0;              // Boolean indicating if operation is in sensor calibration mode.

int16   voltageReadings[4];         // Empty array to store voltage readings.

                                    // Matrix array indicating fit parameters for each sensor.
float32 fitParam[4][2] = {{5.508,-4.328}, {8.168,-6.264}, {6.152,-4.684}, {5.947,-4.455}};

int8  selectOpt = 0;
int16 thetavLimList[4] = {5,5,5,5};  // Desired volumetric water content.
int16 wateringTime [4] = {1,1,1,1};  // In minutes.

int8  dryFlag[4] = {0, 0, 0, 0};     // Array of boolean variables indicating dryness of the soil. 

int32 waitForNextDay = 24*60*60*1000; // Time to wait for (i.e., 24h)
int32 timeForNextDay = 0;             // Time elapsed.

/*              FUNCTIONS INITIALIZATION                */
void initWaterlin (void);
void sampleBed    (void);
void waterBed     (void);
void getUserInput (void);

int16 getVoltageReading (uint8 selectChannel);

    
/*              MAIN CODE STARTS HERE                */
int main(void)
{
    CyGlobalIntEnable; 

    // Initite processor's components.
    initWaterlin();
    
    // SENSOR CALIBRATION MODE
    if (0){                      
        while (1){
            // Sample bed.
            sampleBed();
            
            // Display readings.
            LCD_PrintString("S1:     S2:     ");
            LCD_Position(1,0);
            LCD_PrintString("S3:     S4:     ");
            LCD_Position(0,3);
            LCD_PrintNumber(voltageReadings[0]);
            LCD_Position(0, 11);
            LCD_PrintNumber(voltageReadings[1]);
            LCD_Position(1, 3);
            LCD_PrintNumber(voltageReadings[2]);
            LCD_Position(1, 11);
            LCD_PrintNumber(voltageReadings[3]);
            CyDelay(200);
            LCD_ClearDisplay();
            LCD_Position(0,0);
        };
    }
    // OPERATION MODE
    else {      
        
        while (1){
            // Get user input.
            getUserInput();
            
            // If it is time to water and not on setting mode.
            if (selectOpt == 0 && timeForNextDay >= waitForNextDay){
                
                // Check the dryness level of the bed.
                sampleBed();
                
                // If no plants are dry, do not water the plants.
                int p;
                for (p = 0; p <=3; p++){
                    if (dryFlag[p] == 0){
                        continue;
                    };
                    
                    // Water the bed.
                    LCD_PrintString("    WATERING    ");
                    waterBed();
                    LCD_ClearDisplay();
                    LCD_Position(0,0);
                    break;
                };
                
                // Clean count for today.
                timeForNextDay -= waitForNextDay;
            };
            
            // Wait a milisecond.
            timeForNextDay += 1; 
            CyDelay(1);
        };
    };
    
};

/*              FUNCTIONS DEFINITION                 */
void initWaterlin (void){
    /*
        VOID FUNC initWaterlin.
        Carries out the steps required to initiate all devices used in waterlin.
    
        NO INPUT PARAM
    */
    
    // ADC.
    ADC_SEN_1_Start(); // ADC for sensors 1.
    ADC_SEN_1_StartConvert();
    
    // Analogue multiplexers.
    AMux_1_Start(); // Analog multiplexer 1.
   
    // VDAC.
    VDAC_Start();
    
    // LCD display.
    LCD_Start();
    LCD_Position(0,0);
    
    LCD_PrintString("STARTING");
    CyDelay(2000);
    LCD_ClearDisplay();
    LCD_Position(0,0);
};

void  waterBed (void){
    /*
        VOID FUNC waterBed.
        Carries out the steps required to water the bed.
    
        NO INPUT PARAM
    */
    
    // Open Valves.
    if (dryFlag[0]){ // First Valve.
        SOLVAL_1_Write(high);
    };
    if (dryFlag[1]){ // Second Valve.
        SOLVAL_2_Write(high);
    };
    if (dryFlag[2]){ // Third Valve.
        SOLVAL_3_Write(high);
    };
    if (dryFlag[3]){ // Fouth Valve.
        SOLVAL_4_Write(high);
    }; 
    
    
    // Find greatest time in watering time.
    int16 maxTime = 0;
    int8 k;
    for (k = 0; k <= 3; k += 1){
        if (wateringTime[k]>maxTime && dryFlag[k]){
            maxTime = wateringTime[k];
        };
    };
    
    // Close Valves.
    int16 time; // min
    for (time = 0; time <= maxTime; time += 1){
        if (dryFlag[0] && wateringTime[0] <= time){ // First Valve.
            SOLVAL_1_Write(low);
            dryFlag[0] = 0;
        };
        if (dryFlag[1] && wateringTime[1] <= time){ // Second Valve.
            SOLVAL_2_Write(low);
            dryFlag[1] = 0;
        };
        if (dryFlag[2] && wateringTime[2] <= time){ // Third Valve.
            SOLVAL_3_Write(low);
            dryFlag[2] = 0;
        };
        if (dryFlag[3] && wateringTime[3] <= time){ // Fouth Valve.
            SOLVAL_4_Write(low);
            dryFlag[3] = 0;
        }; 
        
        CyDelay(1000*60); //Wait a minute.
        timeForNextDay += 1000*60;
    }; 
};

void sampleBed (void){
    /*
        VOID FUNC sampleBed.
        Carries out the steps required to check if the soil in bed is dry.
    
        NO INPUT PARAM
    */
    
    int8  i;
    
    // Iterate through analog inputs.  
    for (i = 0; i <= 3; i++){  
        
        // Get voltage readings.
        voltageReadings[i] = getVoltageReading(i);    
        
        // Voltage to humidity content conversion.
        int16 thetavTemp = fitParam[i][0]*100000/voltageReadings[i] + fitParam[i][1]*100;
        
        // Update dryness flag.
        dryFlag[i] = (thetavTemp < thetavLimList[i]);
         
        /*
        // Debbuging
        LCD_ClearDisplay();
        LCD_Position(0,0);
        LCD_PrintNumber(voltageReadings[i]);
        LCD_Position(1,0);
        LCD_PrintNumber(thetavTemp < 0);
        LCD_Position(0,7);
        LCD_PrintNumber(thetavLimList[i]);
        LCD_Position(1,7);
        LCD_PrintNumber(dryFlag[i]);
        CyDelay(2000);
        LCD_ClearDisplay();
        // Debugging
        */
    };
};

void getUserInput (void){
    /*
        VOID FUNC getUserInput
        Carries out the steps to update the thetav limits and watering times
        based on user inputs.
        
        Moving the potentiometer based on the number of button click counts.
        0   -- no chnage
        1-4 -- Valve moisture 
        5-8 -- Valve watering time
        
        NO INPUT PARAM
    */
    // LOCAL VARS
    int16 adcCounts;
    int16 adcVoltageRatio = 0;
    
    int8  selectButVal;
    
    // Read pin out from the debouncer.
    selectButVal = selButPinOut_Read();
      
    // Count the number of clicks up to 8 then restart.
    if (selectButVal == 0){
            
        selectOpt += 1;
            
        if (selectOpt == 9){
           selectOpt = 0; 
        }; 
    };
        
    // If device recieving an user input get the ratio of voltage reading to the max voltage drop across potentiometer.
    if (selectOpt != 0){
        AMux_1_Select(4);
        ADC_SEN_1_IsEndConversion(ADC_SEN_1_WAIT_FOR_RESULT);
        adcCounts = ADC_SEN_1_GetResult16();
        adcVoltageRatio = (ADC_SEN_1_CountsTo_mVolts(adcCounts))/49.99;
    };
    
    // Set desired values based on the setting used.
    switch(selectOpt){
        case 0:     // Operational mode.
            break;
        case 1:     // Desired volumetric water content for row 1.
            thetavLimList[0] = adcVoltageRatio;
            break;
        case 2:     // Desired volumetric water content for row 2.      
            thetavLimList[1] = adcVoltageRatio;
            break;
        case 3:     // Desired volumetric water content for row 3.
            thetavLimList[2] = adcVoltageRatio;
            break;
        case 4:     // Desired volumetric water content for row 4.
            thetavLimList[3] = adcVoltageRatio;
            break;
        case 5:     // Desired watering time for row 1.
            wateringTime[0] = (adcVoltageRatio*250)/100;
            break;
        case 6:     // Desired watering time for row 2.
            wateringTime[1] = (adcVoltageRatio*250)/100;
            break;
        case 7:     // Desired watering time for row 3.  
            wateringTime[2] = (adcVoltageRatio*250)/100;
            break;
        case 8:     // Desired watering time for row 4.
            wateringTime[3] = (adcVoltageRatio*250)/100;
            break;   
    };
    
    // Display desired voltage ratio if they are being altered.
    if (1 <= selectOpt && selectOpt <= 4){  
        LCD_PrintString("R1:   % R2:   % ");
        LCD_Position(1,0);
        LCD_PrintString("R3:   % R4:   % ");
        
        LCD_Position(0, 3);
        LCD_PrintNumber(thetavLimList[0]);
        LCD_Position(0, 11);
        LCD_PrintNumber(thetavLimList[1]);
        LCD_Position(1, 3);
        LCD_PrintNumber(thetavLimList[2]);
        LCD_Position(1, 11);
        LCD_PrintNumber(thetavLimList[3]);
        LCD_Position(0, 0);
        
    // Display desired watering time if they are being altered.
    } else if (4 <= selectOpt){
        LCD_PrintString("T1:     T2:     ");
        LCD_Position(1,0);
        LCD_PrintString("T3:     T4:     ");
        
        LCD_Position(0, 3);
        LCD_PrintNumber(wateringTime[0]);
        LCD_Position(0, 11);
        LCD_PrintNumber(wateringTime[1]);
        LCD_Position(1, 3);
        LCD_PrintNumber(wateringTime[2]);
        LCD_Position(1, 11);
        LCD_PrintNumber(wateringTime[3]);
        LCD_Position(0, 0);
    
    // Display device name.
    } else {
        LCD_PrintString("    WaterLin    ");
        LCD_Position(0, 0);
    };
    
    CyDelay(200);
    LCD_ClearDisplay();
    LCD_Position(0,0);        
};

int16 getVoltageReading (uint8 selectChannel) {
    /*
        INT16 FUNC getVoltageReading.
        Gets the voltage reading from a specific sensor 
    
        INPUT
        @param selectChannel: Analog multiplexer channel (0, 1, 2, or 3).
        @type  selectChannel: uint8.
    
        OUTPUT
        @param adcVoltage: ADC voltage reading.
        @type  selectADC: uint16.
    */
    
    int16 adcVoltage;
    int16 adcCounts;
    
    // Select the multiplexer channel.
    AMux_1_Select(selectChannel);
    
    // Wait for conversion to be done.
    ADC_SEN_1_IsEndConversion(ADC_SEN_1_WAIT_FOR_RESULT);
    
    // Get counts from ADC.
    adcCounts = ADC_SEN_1_GetResult16();
    
    // Convert counts to voltage.
    adcVoltage = ADC_SEN_1_CountsTo_mVolts(adcCounts);
    
    return adcVoltage;
};
