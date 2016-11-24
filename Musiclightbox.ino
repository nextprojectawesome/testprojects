#include <FastLED.h>

//GREAT RESOURCES
//1)Lights
//https://www.baldengineer.com/msgeq7-simple-spectrum-analyzer.html
//2)
//SKOBA! http://nuewire.com/info-archive/msgeq7-by-j-skoba/
//The diagram is basically your wiring, and then you can learn how to interact with MSEQ17.
//3) 
//LIGHTS DOCUMENTATION: https://github.com/FastLED/FastLED/wiki/Pixel-reference
//4) Christmas Lights by Jeremey Bloom

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//AUDIO INTERFACE
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #define DATA_LPIN_STROBE 2//D2 Strobe
  #define DATA_LPIN_RESET   3//D3 reset
  #define AUDIO_LEFT 0 //A0 out

  #define DATA_RPIN_STROBE 4//D2 Strobe
  #define DATA_RPIN_RESET   5//D3 reset
  #define AUDIO_RIGHT 1 //A1 out
  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Buttons
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
    #define BUTTON_PIN 35
    #define SCHEME_BUTTON_PIN 24
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//LIGHT
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
    #define DATA_PIN_LEFT 50
    #define DATA_PIN_RIGHT 40
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//LED VARIABLES 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
    #define NUM_LEDS_LEFT 14
    #define NUM_LEDS_RIGHT 10
    #define NUM_PADS_LEFT 6
    #define NUM_PADS_RIGHT 4    

CRGB ledsL[NUM_LEDS_LEFT];
CRGB ledsR[NUM_LEDS_RIGHT];

int toleranceVariation=10; //originally 10
int sampleDelay=80;
int sampleDelaySnake=60;

//Remember pump 0 is at end of strand
int LLED[][3] = { {0,1,2},
                  {3,4,-1},
                  {5,6,-1},
                  {7,8,-1},
                  {9,10,-1},
                  {11,12,-1}};
int FLLed[] = {13}; //The LED COMPARTMENT LIGHT UP (Kind of like a tweeter, not really using it right now)

int RLED[][3] = { {0,1,2},
                  {3,4,-1},
                  {5,6,-1},
                  {7,8,-1}};

int FRLed[] = {9}; //The LED COMPARTMENT LIGHT UP(Kind of like a tweeter, not really using it right now)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//COLOR MEMORY
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int LEFT_CHANNEL[7] = {-1,-1,-1,-1,-1,-1,-1}; //ALL 7 FREQUENCIES for CHANNEL SAMPLED
int LEFT_PAD_BUFFER[NUM_PADS_LEFT]= {-1,-1,-1,-1,-1,-1}; //MEMORY OF PREVIOUS STATE (SAMPLE VALUE SAVED)
int RIGHT_CHANNEL[7] ={-1,-1,-1,-1,-1,-1,-1}; //ALL 7 FREQUENCIES for CHANNEL SAMPLED
int RIGHT_PAD_BUFFER[NUM_PADS_RIGHT]= {-1,-1,-1,-1}; //MEMORY OF PREVIOUS STATE (SAMPLE VALUE SAVED)
int defaultColor=0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//BUTTON CONTROLLERS
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int display_scheme=0;
int display_num_schemes=4;
int schemeLock=0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//CODE
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup() {
  // put your setup code here, to run once:
  FastLED.addLeds<NEOPIXEL, DATA_PIN_LEFT>(ledsL, NUM_LEDS_LEFT);
  FastLED.addLeds<NEOPIXEL, DATA_PIN_RIGHT>(ledsR, NUM_LEDS_RIGHT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(AUDIO_LEFT, INPUT);
  pinMode(AUDIO_RIGHT, INPUT);
  pinMode(DATA_LPIN_RESET, OUTPUT);
  pinMode(DATA_LPIN_STROBE, OUTPUT);
  pinMode(DATA_RPIN_RESET, OUTPUT);
  pinMode(DATA_RPIN_STROBE, OUTPUT);
}
void lightPad(String strPad, int padNum, int rCol,int gCol,int bCol)
{
  if(strPad=="LEFT")
  {
    //LEFT
        for(int x=0; x<3; x++)
        {
         if(LLED[padNum][x] != -1)
         {
          ledsL[LLED[padNum][x]].setRGB(rCol,gCol,bCol);
        } 
      }
  }

  //RIGHT
  if(strPad=="RIGHT")
  {
    for(int x=0; x<3; x++)
    {
         if(RLED[padNum][x] != -1)
         {
          ledsR[RLED[padNum][x]].setRGB(rCol,gCol,bCol);
        } 
    }
  }
}
void lightPad(String strPad, int padNum, int spectrum)
{

  if(strPad=="LEFT")
  {
    //LEFT
        for(int x=0; x<3; x++)
        {
         if(LLED[padNum][x] != -1)
         {
          ledsL[LLED[padNum][x]].setHSV(spectrum,255,255);
        } 
      }
  }

  //RIGHT
  if(strPad=="RIGHT")
  {
    for(int x=0; x<3; x++)
    {
         if(RLED[padNum][x] != -1)
         {
          ledsR[RLED[padNum][x]].setHSV(spectrum,255,255);
        } 
    }
  }
}

void lightCenter(int rCol, int gCol, int bCol)
{
    ledsL[FLLed[0]].r=rCol;
    ledsL[FLLed[0]].g=gCol;
    ledsL[FLLed[0]].b=bCol;

    ledsR[FRLed[0]].r=rCol;
    ledsR[FRLed[0]].g=gCol;
    ledsR[FRLed[0]].b=bCol;
}
void clearBlinkers()//generic color function meant to be inline before .show() cmd
{
  ledsL[FLLed[0]]=CRGB::Black; 
  ledsR[FRLed[0]]=CRGB::Black;  
}
void sampleSound()
{
    digitalWrite(DATA_LPIN_RESET, HIGH);          // reset the MSGEQ7's counter
    digitalWrite(DATA_RPIN_RESET, HIGH);          // reset the MSGEQ7's counter
    delay(5);
    digitalWrite(DATA_LPIN_RESET, LOW);
    digitalWrite(DATA_RPIN_RESET, LOW);

    for (int x = 0; x < 7; x++){
        digitalWrite(DATA_LPIN_STROBE, LOW);      // output each DC value for each freq band
        digitalWrite(DATA_RPIN_STROBE, LOW);      // output each DC value for each freq band
        delayMicroseconds(35); // to allow the output to settle
        //READ AND STORE VALUES
        int lspectrumRead = analogRead(AUDIO_LEFT);
        int rspectrumRead = analogRead(AUDIO_RIGHT);

        int newLVal= map(lspectrumRead, 0, 1024, 0, 255);  // scale analogRead's value to Write's 255 max
        if ((LEFT_CHANNEL[x]-newLVal >0 && LEFT_CHANNEL[x]-newLVal > toleranceVariation) || (newLVal-LEFT_CHANNEL[x] >0 && newLVal-LEFT_CHANNEL[x] > toleranceVariation))
        {
          LEFT_CHANNEL[x] = newLVal; 
        }
        int newRVal= map(rspectrumRead, 0, 1024, 0, 255);  // scale analogRead's value to Write's 255 max
        if ((RIGHT_CHANNEL[x]-newRVal >0 && RIGHT_CHANNEL[x]-newRVal > toleranceVariation) || (newRVal-RIGHT_CHANNEL[x] >0 && newRVal-RIGHT_CHANNEL[x] > toleranceVariation))
        {
          RIGHT_CHANNEL[x] = newRVal; 
        }         
        //NEXT FREQUENCY
        digitalWrite(DATA_LPIN_STROBE, HIGH);
        digitalWrite(DATA_RPIN_STROBE, HIGH);
    }
}
void lightBoard()
{
  for(int x=0;x<NUM_LEDS_LEFT;x++)
  {  
    ledsL[x]=CRGB::White;  
  }
  for(int x=0;x<NUM_LEDS_RIGHT;x++)
  {  
    ledsR[x]=CRGB::White ; 
  }
}
void displaySound()
{
  //NICE PURPLE: 144,0,165
  if(display_scheme>=display_num_schemes)
  {
      display_scheme=0; 
   }

    if(display_scheme==0) //Static nice purple
    {
      if(defaultColor!=1)
      {
        defaultColor=1;
      
         //Clear blinkers
         ledsL[NUM_LEDS_LEFT].setHSV(0,0,0);
         ledsR[NUM_LEDS_RIGHT].setHSV(0,0,0);
        //Light pads
        for(int x=0; x<NUM_PADS_LEFT;x++)
        {
          lightPad("LEFT", x,144,0,165);
        }
        for(int x=0; x<NUM_PADS_RIGHT;x++)
        {
          lightPad("RIGHT", x,144,0,165);
        }  
      }
   
    }
     
    else if(display_scheme==1) //each pad based on frequency
    {
      defaultColor=0;
       //Clear blinkers
       ledsL[NUM_LEDS_LEFT].setHSV(0,0,0);
       ledsR[NUM_LEDS_RIGHT].setHSV(0,0,0);

       //Display light based on sample
        for(int x=0; x<NUM_PADS_LEFT;x++) //X will always be less than the number of frequencies (6 pads, 7 frequencies)
        {
          lightPad("LEFT", x,LEFT_CHANNEL[x]);
        }
        for(int x=0; x<NUM_PADS_RIGHT;x++) //X will always be less than the number of frequencies (4 pads, 7 frequencies)
        {
          lightPad("RIGHT", x,RIGHT_CHANNEL[x]);
        }

    }
    
    else if(display_scheme==2) //Flow frontleft to frontright(follow U shape using max left/right as basis)
    {
       //Clear blinkers
       ledsL[NUM_LEDS_LEFT].setHSV(0,0,0);
       ledsR[NUM_LEDS_RIGHT].setHSV(0,0,0);
        int tmpBuffer = LEFT_PAD_BUFFER[0];
        //Light LEFT pads 
        for(int x=0; x<=NUM_PADS_LEFT-2;x++) 
        { 
            LEFT_PAD_BUFFER[x]=LEFT_PAD_BUFFER[x+1];
            lightPad("LEFT", x,LEFT_PAD_BUFFER[x]);
        }
        //set new value
        int maxVal=0;
        int calcVal1=map(LEFT_CHANNEL[0]+LEFT_CHANNEL[1]+LEFT_CHANNEL[2]+LEFT_CHANNEL[3]+LEFT_CHANNEL[4]+LEFT_CHANNEL[5]+LEFT_CHANNEL[6],0,1785,0,255);
        int calcVal2=map(RIGHT_CHANNEL[0]+RIGHT_CHANNEL[1]+RIGHT_CHANNEL[2]+RIGHT_CHANNEL[3]+RIGHT_CHANNEL[4]+RIGHT_CHANNEL[5]+RIGHT_CHANNEL[6],0,1785,0,255);
        
        maxVal=max(calcVal1,calcVal2);
        LEFT_PAD_BUFFER[NUM_PADS_LEFT-1]=maxVal;
        lightPad("LEFT", NUM_PADS_LEFT-1,LEFT_PAD_BUFFER[NUM_PADS_LEFT-1]);

         //Light RIGHT pads using continuance of left
        for(int x=NUM_PADS_RIGHT-1; x>0;x--) 
        { 
          if(RIGHT_PAD_BUFFER[x-1]>=0)  //if color exists in previous buffer, copy it and display it
          {
            RIGHT_PAD_BUFFER[x]=RIGHT_PAD_BUFFER[x-1];
            lightPad("RIGHT", x,RIGHT_PAD_BUFFER[x]);
          }
        }
        //set new value
        RIGHT_PAD_BUFFER[0]=tmpBuffer;
        lightPad("RIGHT", 0,RIGHT_PAD_BUFFER[0]);
         //delay(sampleDelaySnake);
    }
    else if(display_scheme==3) //same color selection as front left to front right, except light whole board
    {
       //Clear blinkers
       ledsL[NUM_LEDS_LEFT].setHSV(0,0,0);
       ledsR[NUM_LEDS_RIGHT].setHSV(0,0,0);

        //set new value
        int maxVal=0;
        int calcVal1=map(LEFT_CHANNEL[0]+LEFT_CHANNEL[1]+LEFT_CHANNEL[2]+LEFT_CHANNEL[3]+LEFT_CHANNEL[4]+LEFT_CHANNEL[5]+LEFT_CHANNEL[6],0,1785,0,255);
        int calcVal2=map(RIGHT_CHANNEL[0]+RIGHT_CHANNEL[1]+RIGHT_CHANNEL[2]+RIGHT_CHANNEL[3]+RIGHT_CHANNEL[4]+RIGHT_CHANNEL[5]+RIGHT_CHANNEL[6],0,1785,0,255);       
        maxVal=max(calcVal1,calcVal2);

        //Light LEFT pads 
        for(int x=0; x<NUM_PADS_LEFT;x++) 
        { 
            LEFT_PAD_BUFFER[x]=maxVal;
            lightPad("LEFT", x,maxVal);
        }
         //Light RIGHT pads using continuance of left
        for(int x=0; x<NUM_PADS_RIGHT; x++) 
        { 
            RIGHT_PAD_BUFFER[x]=maxVal;
            lightPad("RIGHT", x,maxVal);
        }
         //delay(sampleDelaySnake);
    }
   /*else if(display_scheme==4) //Pair pads and use max of each frequencies, grouping the last 4 into pairs of 2
    {
       //Clear blinkers
       ledsL[NUM_LEDS_LEFT].setHSV(0,0,0);
       ledsR[NUM_LEDS_RIGHT].setHSV(0,0,0);

       int maxVal=0;
       int calcVal1=0;
       int calcVal2=0;
        
        int  colPos=0;
        for(int i=0;i<5;i++) //bad coding, I know I have 10 pads, that I'm mapping to 5 paids from 7 frequencies
        {
          if(i<=2)//first four frequencies are direct maps
          {
            calcVal1=LEFT_CHANNEL[i];
            calcVal2=RIGHT_CHANNEL[i];
            maxVal=max(calcVal1,calcVal2);
            lightPad("LEFT", colPos,maxVal);
            lightPad("LEFT", colPos+1,maxVal);
            colPos=colPos+2;
          }
          else if(i==3)//first four frequencies are direct maps
          {
           calcVal1=map(LEFT_CHANNEL[3]+LEFT_CHANNEL[4],0,510,0,255);
           calcVal2=map(RIGHT_CHANNEL[3]+RIGHT_CHANNEL[4],0,510,0,255);
           maxVal=max(calcVal1,calcVal1);
            lightPad("RIGHT", 0,maxVal);
            lightPad("RIGHT", 1,maxVal);
          }
          else if(i==4)//first four frequencies are direct maps
          {
             calcVal1=map(LEFT_CHANNEL[5]+LEFT_CHANNEL[6],0,1785,0,255);
             calcVal2=map(RIGHT_CHANNEL[5]+RIGHT_CHANNEL[6],0,1785,0,255);
             maxVal=max(calcVal1,calcVal1);
            lightPad("LEFT", 2,maxVal);
            lightPad("LEFT", 3,maxVal);
          }
          
        }

       // delay(sampleDelay);
    }
    else if(display_scheme==4) //Flow colors from back (two seperate Ls based on left/right channels)
    {
       //Clear blinkers
       ledsL[NUM_LEDS_LEFT].setHSV(0,0,0);
       ledsR[NUM_LEDS_RIGHT].setHSV(0,0,0);

        //Light LEFT pads 
        for(int x=NUM_PADS_LEFT-1; x>0;x--) 
        { 
          if(LEFT_PAD_BUFFER[x-1]>=0)  //if color exists in previous buffer, copy it and display it
          {
            LEFT_PAD_BUFFER[x]=LEFT_PAD_BUFFER[x-1];
            lightPad("LEFT", x,LEFT_PAD_BUFFER[x]);
          }
        }
        //set new value
        LEFT_PAD_BUFFER[0]=LEFT_CHANNEL[0];
        lightPad("LEFT", 0,LEFT_PAD_BUFFER[0]);

        //Light RIGHT pads
        for(int x=NUM_PADS_RIGHT-1; x>0;x--) 
        { 
          if(RIGHT_PAD_BUFFER[x-1]>=0)  //if color exists in previous buffer, copy it and display it
          {
            RIGHT_PAD_BUFFER[x]=RIGHT_PAD_BUFFER[x-1];
            lightPad("RIGHT", x,RIGHT_PAD_BUFFER[x]);
          }
        }
        //set new value
        RIGHT_PAD_BUFFER[0]=RIGHT_CHANNEL[0];
        lightPad("RIGHT", 0,RIGHT_PAD_BUFFER[0]);
        
        //delay(sampleDelaySnake);
    }
    else if(display_scheme==5) //Flow colors from front (two seperate Ls based on left/right channels)
    {
       //Clear blinkers
       ledsL[NUM_LEDS_LEFT].setHSV(0,0,0);
       ledsR[NUM_LEDS_RIGHT].setHSV(0,0,0);

        //Light LEFT pads 
        for(int x=0; x<=NUM_PADS_LEFT-2;x++) 
        { 

          if(LEFT_PAD_BUFFER[x+1]>=0)  //if color exists in previous buffer, copy it and display it
          {
            LEFT_PAD_BUFFER[x]=LEFT_PAD_BUFFER[x+1];
            lightPad("LEFT", x,LEFT_PAD_BUFFER[x]);
          }
        }
        //set new value
        LEFT_PAD_BUFFER[NUM_PADS_LEFT-1]=LEFT_CHANNEL[0];
        lightPad("LEFT", NUM_PADS_LEFT-1,LEFT_PAD_BUFFER[NUM_PADS_LEFT-1]);

        //Light RIGHT pads 
        for(int x=0; x<=NUM_PADS_RIGHT-2;x++) 
        { 

          if(RIGHT_PAD_BUFFER[x+1]>=0)  //if color exists in previous buffer, copy it and display it
          {
            RIGHT_PAD_BUFFER[x]=RIGHT_PAD_BUFFER[x+1];
            lightPad("RIGHT", x,RIGHT_PAD_BUFFER[x]);
          }
        }
        //set new value
        RIGHT_PAD_BUFFER[NUM_PADS_RIGHT-1]=RIGHT_CHANNEL[0];
        lightPad("RIGHT", NUM_PADS_RIGHT-1,RIGHT_PAD_BUFFER[NUM_PADS_RIGHT-1]);
        
        //delay(sampleDelaySnake);
    }
      */ 

    
}
void loop() {
  int buttonState = digitalRead(BUTTON_PIN);
  int schemebuttonState = digitalRead(SCHEME_BUTTON_PIN);
  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (buttonState == HIGH) 
  { 
      defaultColor=0;
      lightBoard();
      FastLED.show();
      delay(500);//half second hold white
  }
  else
  {
    clearBlinkers();
    if (schemebuttonState == HIGH && schemeLock==0) //force user to release before letting next sceme to be selected
    { 
        schemeLock=1;
        display_scheme++;
    }
    else if(schemebuttonState == LOW)//button down - clear lock
    {
      schemeLock=0;
    }
    
    if(display_scheme==0) //static color, no need to sample
    {
      if(defaultColor==0)
      {
        displaySound();
         FastLED.show();
         delay(250); //quarter second to latch and check again
      }
    }
    else // sample and display
    {
      sampleSound();
      displaySound();
       FastLED.show();
       if(display_scheme==1)
       {
            delay(sampleDelay);
        }
       else if(display_scheme>1)
       {
          delay(sampleDelaySnake);
       }
    }
   
  }
   
}
