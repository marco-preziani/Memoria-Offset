#include <Arduino.h>

#include <EEPROM.h>

#define analogPin A0 /* ESP8266 Analog Pin ADC0 = A0 */
#define outPWM 4 /* GPIO4 (D2) is connected to LED */
#define memorize 13 /* GPIO14 (D7)switch to memorize */
// #define memorize 14 /* GPIO14 (D5)switch to memorize */
#define triggerS 12 /* GPIO12 (D6) trigger to stop up-counter)*/
#define ledCalib 5 /* GPIO5 (D1)led */
// the current address in the EEPROM which byte
// we're going to write)
int addrhi = 11;
int addrlow = 10;
int adcValue = 0;  /* Variable to store Output of ADC */
int counterValue =0; /* è il contatore che incrementa l'uscita PWM */
word dacEPROM = 0;
int waitCalibr = 1; /* define value to block */
int tout_waitCal = 0;
int iToutCal = 0;
int iOff = 0;
int getCmd= 0;
int eepromVal;
byte valuehi;
byte valuelow;
int CalStatus =0;
int triggered = HIGH;
int startCounter =300;

//-------------------------------------------------------


void setup() {
  pinMode(ledCalib, OUTPUT);
  pinMode(outPWM, OUTPUT);    
  pinMode(memorize, INPUT);
  pinMode(triggerS, INPUT);
  analogWriteRange(1023);
  Serial.begin(115200);
  EEPROM.begin(512);
  tout_waitCal = 0;
  getCmd=0;
  waitCalibr=1;
  tout_waitCal=0;
  iOff=0;
  iToutCal =0;
  digitalWrite(ledCalib, HIGH);
  
}

void loop() {
//-------------------------------------------------------legge EEPROM e scrive PWM out
// read a byte from the address of the EEPROM

  valuehi = EEPROM.read(addrhi);
  valuelow = EEPROM.read(addrlow);
  dacEPROM = word(valuehi,valuelow);
  eepromVal=dacEPROM;
// show value on serial port
  Serial.print("EEPROM = ");
  Serial.print("\t");
  Serial.print(eepromVal*3.28);   // qui si corregge il fattore di scala tra EEPROM ed effettiva 
                                  // uscita dal filtro del PWM ********************************
                                  // (ai fini della visualizzazione)
  Serial.println();
// set the value on PWM
  analogWrite(outPWM, dacEPROM);
//-------------------------------------------------------legge il valore di setting per Ref
// read the Analog Input value
  adcValue = analogRead(analogPin); 
  
// Print the output in the Serial Monitor */
  Serial.print("ADC Value = ");
  Serial.println(adcValue*3.06);  // qui si corregge il guadagno dell'opamp che fa da buffer
                                  // al valore impostato dal trimmer ***********************
                                  // (ai soli fini della visualizzazione)
                                  
//-------------------------------------------------------Aspetta il comando per la calibrazione
// wait calibration activation
   counterValue = startCounter;
   if(waitCalibr==0)
   {
    Serial.print("captured ");
    Serial.println();

    delay(400);
  
// qui inizia la ricerca della reference
      for(iToutCal=startCounter ; iToutCal<1024; iToutCal++)
        {
        counterValue = counterValue+1;
// set the value on PWM
        analogWrite(outPWM, counterValue);
        Serial.println(counterValue*1);
        Serial.println();
        if(iToutCal==startCounter){
        delay(400);       // tempo per fare scaricare inizialmente il filtro del PWM
                          // così che triggerS possa salire
        }
        if(digitalRead(triggerS)==0)
          {
          break;
          }
          delay(50);   
        }
// write adcValue to EEPROM
        counterValue=counterValue-1;
        waitCalibr=1;
        EEPROM.write(addrhi, highByte(counterValue));
        EEPROM.write(addrlow, lowByte(counterValue));
        if (EEPROM.commit()) 
          {
          Serial.println("EEPROM successfully committed");
          digitalWrite(ledCalib, LOW);        
          } else 
          {
          Serial.println("ERROR! EEPROM commit failed");
          }        
        delay(3000);
        Serial.println("RESTARTED");
     waitCalibr=1;
     getCmd=0;
     iToutCal=0 ;
   }
   CalStatus=digitalRead(memorize);
//------------------------------------------------------- 
// valuta se il tasto è attivato 
     if(CalStatus==0)
        { 
        Serial.print("comON ");
        Serial.println();
        digitalWrite(ledCalib,LOW);  /* segnala la acquisizione del comando accendendo il led */
        iToutCal = iToutCal+1;
        if(iToutCal>10)
          {
          getCmd=1;           /* il comando è acquisito */
          iToutCal=10;
          }
        delay (250);
        } else  
        {
        if(iToutCal<10)
          {
          digitalWrite(ledCalib, HIGH);
          iToutCal=0;
          }
        }
// il tasto non è attivato ma era stato atttivato in precedenza?
     if(getCmd==1)
        {        
        Serial.print("CmdAcquired");
        Serial.println();
        digitalWrite(ledCalib, HIGH); /* spegne per segnalare che il comando è acquisito */
        if(CalStatus==1)
          { /*aspetta che si rilasci il comando se è stato aquisito */
          iOff=iOff+1;
          if(iOff>3)
            {
            waitCalibr=0; /* indica che siamo pronti per calibrare */
            iToutCal=0;
            iOff=0;
            }
          }
//     if(waitCalibr==1){digitalWrite(ledCalib, HIGH);} /* per fare pulsare il led */
          }
      delay(250);  
    }     /* fine dell'else */
// no il tato non è stato attivato ne ora ne in precedenza
