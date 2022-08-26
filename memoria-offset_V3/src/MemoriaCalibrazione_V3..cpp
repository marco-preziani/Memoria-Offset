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
int buttonStatus =0;
int triggered = HIGH;
int startCounter =300;

//-------------------------------------------------------
int readEEP()
    {
      // read a byte from the address of the EEPROM
      valuehi = EEPROM.read(addrhi);
      valuelow = EEPROM.read(addrlow);
      dacEPROM = word(valuehi,valuelow);
      // show value on serial port
      Serial.print("EEPROM[V] = ");
      Serial.print("\t");
      Serial.print(dacEPROM*3.28);   // qui si corregge il fattore di scala tra EEPROM ed effettiva 
                                      // uscita dal filtro del PWM ********************************
                                      // (ai fini della visualizzazione)
      Serial.println();
      return dacEPROM;
    }
//------------------------------------------------------- valuta getCmd
 int buttActivated_tout()
    {
    //
    // valuta se il tasto è attivato per più di 2s --> getCmd=1 e fa pulsare il led
    //
     if(buttonStatus==0)
        { 
          // se il tasto è stato premuto
          Serial.print("comON ");
          Serial.println();
          iToutCal = iToutCal+1;
          if(iToutCal>10)
            {
              getCmd=1;           /* il comando è acquisito */
              iToutCal=10;
              Serial.print("CmdAcquired");
              Serial.println();
            }
        delay (200);                        // per tenere spento il led per un pò
        } 
        else  
        {
        if(iToutCal<10)
            {        
             getCmd=0;
             iToutCal=0;
            }
        }
      return getCmd;
    }
//------------------------------------------------------- valuta waitCalibr
int checkButReleased()
  {
    int retval=0;
        if(buttonStatus==1)
          { /* il tasto è stato rilasciato ed il comando è stato aquisito */
            iOff=iOff+1;
            if(iOff>3)
              {
                waitCalibr=0;                   /* indica che siamo pronti per calibrare */
                Serial.print("CmdReleased");
                Serial.println();
                retval=1;
              }
          }
          else
          {
              iOff = 0;
              retval= 0;
          }
  return retval;
  }
//------------------------------------------------------- effettua la ricerca del valore di calibrazione
void checkCalibActive()
  {
    // wait calibration activation
   counterValue = startCounter;
 
   if(waitCalibr == 0)
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
     getCmd=0;              // ripristina i valori iniziali delle variabili
     iToutCal=0 ;           // necessarie per eseguire correttamente la sequenza
     iOff=0;                // di calibrazione
   }
  }



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
  iToutCal =0;
  digitalWrite(ledCalib, HIGH);
  
}
  
void loop() 
{
//-------------------------------------------------------legge EEPROM e scrive PWM out
// read a byte from the address of the EEPROM
  eepromVal = readEEP();
  
// set the value on PWM
  analogWrite(outPWM, eepromVal);
  
//-------------------------------------------------------legge il valore di setting per Ref
// read the Analog Input value
  adcValue = analogRead(analogPin); 
  
// Print the output in the Serial Monitor */
  Serial.print("ADC Value[V] = ");
  Serial.println(adcValue*3.06);  // qui si corregge il guadagno dell'opamp che fa da buffer
                                  // al valore impostato dal trimmer ***********************
                                  // (ai soli fini della visualizzazione)
                                  
//-------------------------------------------------------Aspetta la pressione del tasto
// rileva se il tasto è stato attivat. Dopo un attesa fa pulsare il led, e poi
// valuta se è stato rilasciato. In tal caso --> waitCalibr=0 e spegnerà il led in CC
//
     buttonStatus=digitalRead(memorize);
//
// AA - segnala l'eventuale pressione del tasto accendendo il led (== 0 se premuto)
     if(buttonStatus == 0){ digitalWrite(ledCalib,LOW);}else{digitalWrite(ledCalib,HIGH);}
//
     if(buttActivated_tout() == 1)
        {  
// il comando è acquisito    
// il tasto è rimasto premuto per un certo tempo, quindi spegne e, con AA, fa pulsare il led
          digitalWrite(ledCalib, HIGH); 
        
          if(checkButReleased() == 1)
          {
// CC - il tasto è stato rilasciato e spegne il led
              digitalWrite(ledCalib, HIGH);
              getCmd=0;               // ripristina i valori iniziali
              iToutCal=0;             // per la sequenza di interpretazione del tasto
              iOff = 0;
  //-------------------------------------------------------Aspetta il comando per la calibrazione
              checkCalibActive();
          } 
        }   
   delay(250);   
} 
 
// no il tasto non è stato attivato ne ora ne in precedenza
