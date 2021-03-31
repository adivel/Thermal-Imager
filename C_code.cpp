#include <Arduino.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Wire.h>
#include "MLX90641_API.h"
#include "MLX9064X_I2C_Driver.h"

#define TFT_CS   15 
#define TFT_DC    2
#define TFT_MOSI 13     
#define TFT_CLK  14
#define TFT_RST  26
#define TFT_MISO 12
#define TFT_LED  27

#if defined(ARDUINO_ARCH_AVR)
    #define debug  Serial
#elif defined(ARDUINO_ARCH_SAMD) ||  defined(ARDUINO_ARCH_SAM)
    #define debug  Serial
#else
    #define debug  Serial
#endif

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);


//static float MLX90641To[768];
const byte MLX90641_address = 0x33; //Default 7-bit unshifted address of the MLX90640
#define TA_SHIFT 8 //Default shift for MLX90640 in open air

uint16_t eeMLX90641[832];
float MLX90641To[192];
uint16_t MLX90641Frame[242];
paramsMLX90641 MLX90641;
int errorno = 0;


int xPos, yPos;                                // Scan position
int R_colour, G_colour, B_colour;              // RGB-color values
int i, j;                                      // counter variable
float T_max, T_min;                            // maximam or minimam measured Temperature
float T_center;                                // Temperatur in the centre of the screen

void setup() {
    Wire.begin();
    Wire.setClock(3000000); //Increased I2C clock speed to 3Mhz instead of 400kHz
    debug.begin(115200); //Fast debug as possible
    debug.println("MLX90641 IR ARRAY -Deepsea");
    while (!debug); //Wait for user to open terminal

    if (isConnected() == false){
        debug.println("MLX90641 not detected at default I2C address. Please check wiring. Freezing.");
        while (1);
    }

    debug.println("MLX90641 online!");
    
    //Get device parameters - We only have to do this once
    int status;
    status = MLX90641_DumpEE(MLX90641_address, eeMLX90641);
    
    //errorno = status;//MLX90641_CheckEEPROMValid(eeMLX90641);//eeMLX90641[10] & 0x0040;//
    
    if (status != 0) {
        debug.println("Failed to load system parameters");
       while(1);
    }

    status = MLX90641_ExtractParameters(eeMLX90641, &MLX90641);
    //errorno = status;
    if (status != 0) {
        debug.println("Parameter extraction failed");
        while(1);
    }

    //Once params are extracted, we can release eeMLX90641 array

    //MLX90641_SetRefreshRate(MLX90641_address, 0x02); //Set rate to 2Hz
    MLX90641_SetRefreshRate(MLX90641_address, 0x03); //Set rate to 4Hz
    //MLX90641_SetRefreshRate(MLX90641_address, 0x07); //Set rate to 64Hz    


    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);

    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
    tft.fillRect(0, 0, 319, 20, tft.color565(0,128, 128));//top line
    tft.setCursor(80, 3);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE, tft.color565(0,128, 128));
    tft.print("Thermal Imager");    

    tft.fillRect(0, 220, 319, 20, tft.color565(0,128, 128));//bottom line

    tft.drawLine(250, 210 - 0, 258, 210 - 0, tft.color565(255, 255, 255));
    tft.drawLine(250, 210 - 30, 258, 210 - 30, tft.color565(255, 255, 255));
    tft.drawLine(250, 210 - 60, 258, 210 - 60, tft.color565(255, 255, 255));
    tft.drawLine(250, 210 - 90, 258, 210 - 90, tft.color565(255, 255, 255));
    tft.drawLine(250, 210 - 120, 258, 210 - 120, tft.color565(255, 255, 255));
    tft.drawLine(250, 210 - 150, 258, 210 - 150, tft.color565(255, 255, 255));
    tft.drawLine(250, 210 - 180, 258, 210 - 180, tft.color565(255, 255, 255));

    tft.setCursor(20, 205);
    tft.setTextColor(ILI9341_WHITE, tft.color565(0, 0, 0));
    tft.print("Point T = ");    


    // drawing the colour-scale
    // ========================
 
    for (i = 0; i < 181; i++)
       {
        //value = random(180);
        
        getColour(i);
        tft.drawLine(240, 210 - i, 250, 210 - i, tft.color565(R_colour, G_colour, B_colour));
       } 
}

void loop() {

//    long startTime = millis();
    
    for (byte x = 0 ; x < 2 ; x++) {
        int status = MLX90641_GetFrameData(MLX90641_address, MLX90641Frame);

        float vdd = MLX90641_GetVdd(MLX90641Frame, &MLX90641);
        float Ta = MLX90641_GetTa(MLX90641Frame, &MLX90641);

        float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
        float emissivity = 0.95;

        MLX90641_CalculateTo(MLX90641Frame, &MLX90641, emissivity, tr, MLX90641To);
    }

 for (byte x = 0 ; x < 2 ; x++) {
        int status = MLX90641_GetFrameData(MLX90641_address, MLX90641Frame);

        float vdd = MLX90641_GetVdd(MLX90641Frame, &MLX90641);
        float Ta = MLX90641_GetTa(MLX90641Frame, &MLX90641);

        float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
        float emissivity = 0.95;

        MLX90641_CalculateTo(MLX90641Frame, &MLX90641, emissivity, tr, MLX90641To);
    }

 

    T_min = MLX90641To[0];
    T_max = MLX90641To[0];

    for (i = 1; i < 768; i++)
       {
        if((MLX90641To[i] > -41) && (MLX90641To[i] < 301))
           {
            if(MLX90641To[i] < T_min)
               {
                T_min = MLX90641To[i];
               }

            if(MLX90641To[i] > T_max)
               {
                T_max = MLX90641To[i];
               }
           }
        else if(i > 0)   // temperature out of range
           {
            MLX90641To[i] = MLX90641To[i-1];
           }
        else
           {
            MLX90641To[i] = MLX90641To[i+1];
           }
       }


    // determine T_center
    // ==================

    T_center = MLX90641To[11* 16 + 15];// originally it is 32 for MLX90640    
  
    // drawing the picture
    // ===================

    for (i = 0 ; i < 12 ; i++)// originally 24 for MLX90640
       {
        for (j = 0; j < 16; j++)// originally 32 for MLX90640 
           {
            MLX90641To[i*16 + j] = 180.0 * (MLX90641To[i*16 + j] - T_min) / (T_max - T_min);
                       
            getColour(MLX90641To[i*16 + j]);
            
            tft.fillRect(217 - j * 14, 35 + i * 14, 14,14, tft.color565(R_colour, G_colour, B_colour));//  7,7 is the original increased to 14
           }
       }
    
    tft.drawLine(217 - 15*7 + 3.5 - 5, 11*7 + 35 + 3.5, 217 - 15*7 + 3.5 + 5, 11*7 + 35 + 3.5, tft.color565(255, 255, 255));
    tft.drawLine(217 - 15*7 + 3.5, 11*7 + 35 + 3.5 - 5, 217 - 15*7 + 3.5, 11*7 + 35 + 3.5 + 5,  tft.color565(255, 255, 255));
 
    tft.fillRect(260, 25, 37, 10, tft.color565(0, 0, 0));
    tft.fillRect(260, 205, 37, 10, tft.color565(0, 0, 0));    
    //tft.fillRect(115, 220, 37, 10, tft.color565(0, 0, 0));    

    tft.setTextColor(ILI9341_WHITE, tft.color565(0, 0, 0));
    tft.setCursor(265, 25);
    tft.print(T_max, 1);
    tft.setCursor(265, 205);
    tft.print(T_min, 1);
    tft.setCursor(130, 205);
    tft.print(T_center, 1);

    tft.setCursor(300, 25);// temp sign
    tft.print("C");
    tft.setCursor(300, 205);//temp sign
    tft.print("C");
    tft.setCursor(165, 205);// temp sign
    tft.print("C");

    
    delay(20);
}    


// ===============================
// ===== determine the colour ====
// ===============================

void getColour(int j)
   {
    if (j >= 0 && j < 30)
       {
        R_colour = 0;
        G_colour = 0;
        B_colour = 20 + (120.0/30.0) * j;
       }
    
    if (j >= 30 && j < 60)
       {
        R_colour = (120.0 / 30) * (j - 30.0);
        G_colour = 0;
        B_colour = 140 - (60.0/30.0) * (j - 30.0);
       }

    if (j >= 60 && j < 90)
       {
        R_colour = 120 + (135.0/30.0) * (j - 60.0);
        G_colour = 0;
        B_colour = 80 - (70.0/30.0) * (j - 60.0);
       }

    if (j >= 90 && j < 120)
       {
        R_colour = 255;
        G_colour = 0 + (60.0/30.0) * (j - 90.0);
        B_colour = 10 - (10.0/30.0) * (j - 90.0);
       }

    if (j >= 120 && j < 150)
       {
        R_colour = 255;
        G_colour = 60 + (175.0/30.0) * (j - 120.0);
        B_colour = 0;
       }

    if (j >= 150 && j <= 180)
       {
        R_colour = 255;
        G_colour = 235 + (20.0/30.0) * (j - 150.0);
        B_colour = 0 + 255.0/30.0 * (j - 150.0);
       }

   }
   
//Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected(){
   
    Wire.beginTransmission((uint8_t)MLX90641_address);
  
    if (Wire.endTransmission() != 0){
       return (false); //Sensor did not ACK
    
    return (true);
   }   
}
