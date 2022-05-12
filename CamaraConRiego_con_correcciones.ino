//////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////      CAMARA DE CULTIVO CONTROLADO       /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <DHT.h>    // importa la Librerias DHT
#include <DHT_U.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> // Debe descargar la Libreria que controla el I2C
LiquidCrystal_I2C lcd(0x27,16,2);

                                  //Pines de conexion SDA = A4; SCL = A5

#include <RTClib.h>    // incluye libreria para el manejo del modulo RTC

RTC_DS3231 rtc;     // crea objeto del tipo RTC_DS3231
                                  // Al igual que el LCD, se conectan en los pines SDA = A4; SCL = A5

///////////////////SIMBOLOS/////////////////

    byte sol[] = {
      B11111,
      B10001,
      B01010,
      B00100,
      B00100,
      B01010,
      B11111,
      B11111
    };
    byte GOTA[] = {
      B00100,
      B00100,
      B01100,
      B01110,
      B10111,
      B10111,
      B10011,
      B01110
    };
    
    byte Temp[] = {
      B01110,
      B01010,
      B01010,
      B01110,
      B01110,
      B11111,
      B11111,
      B01110
    };
    
    byte FLECHA[] = {
      B00000,
      B00100,
      B01100,
      B11111,
      B11111,
      B01100,
      B00100,
      B00000
    };
    
////////////////////////////////////SENSOR DHT22////////////////////////////////////////////


    int SENSOR = 5;     // pin DATA de DHT22 a pin digital 5
    int TEMPERATURA;
    int HUMEDAD;
    int vcc = 11; //Alimentación del sensor
//////////////////////////SENSOR ANALÓGICO DE HUMEDAD DE SUELO//////////////////////////////

int HUMEDADSUELO;
int SUELO;

////////////////////////VARIABLE DE ESTADO PARA SELECCION DE PARAMETROS/////////////////////

volatile int ESTADO = 0;

/////////////////////////////////PINES DEL ENCODER Y BOTÓN//////////////////////////////////

int A = 3;       //variable A a pin digital 3 (DT en modulo)
int B = 4;       //variable B a pin digital 4 (CLK en modulo)
int BOTON = 2;   //Variable BOTON a pin digital 2 (SW en modulo)

////////////////////////////////////PINES DE ACTUADORES/////////////////////////////////////

int HELADERA = 6;
int VENTILADOR = 7;
int RIEGO = 9;
int LUZ = 8;

/////////////////////////////VALORES DE INICIO PARA LOS PARAMETROS DE LOS ACTUADORES////////////////////

int heladera = 25;
int ventilador = 90;
int humedadsuelo = 20;
int hsluz = 13;

//////////////VARIABLES PARA ENCENDIDO Y APAGADO DE LA LUZ DE LA PANTALLA//////////////////

volatile boolean estadolcd = false; //guarda el estado de la luz
volatile unsigned long t = 0;
volatile long pantalla = 3000;

////////////////VARIABLES PARA EL ENCENDIDO Y APAGADO DE LA LUZ DE LA CAMARA///////////////

bool luz_inicio = true;  // variable de control para inicio de evento con valor true
bool luz_fin = true;
char texto[10];

/////////////////////////////VARIABLES DE POSICIÓN DEL ENCODER/////////////////////////////

int ANTERIOR;
volatile int POSICION = 500; // variable POSICION con valor inicial de 500 y definida
                                  // como global al ser usada en loop e ISR (encoder)
                                  
///////////////CREACIÓN DEL OBJETO "SENSOR" CON AYUDA DE LA LIBRERÍA DHT22/////////////////

DHT dht(SENSOR, DHT22);   

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////INICIALIZACIÓN DE PINES//////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
        
void setup(){
  pinMode(vcc,OUTPUT); // Declaro la alimentación del sensor dth22 como salida
  pinMode(A, INPUT);    // A como entrada
  pinMode(B, INPUT);    // B como entrada
  pinMode(BOTON, INPUT); // BOTON como entrada
  pinMode(HELADERA, OUTPUT); //HELADERA como Salida
  pinMode(VENTILADOR, OUTPUT); //HELADERA como Salida
  pinMode(LUZ, OUTPUT);
  pinMode(RIEGO, OUTPUT); //Riego como salida
  digitalWrite (vcc, HIGH);// Le doy corriente al sensor dht22.
  digitalWrite (RIEGO, HIGH);
  digitalWrite (HELADERA, LOW);
  digitalWrite (VENTILADOR, LOW);
  digitalWrite (LUZ, LOW);

  /////////////////INICIALIZACIÓN DEL SENSOR DE TEMPERATURA Y HUMEDAD ////////////////////
   
  dht.begin();      // inicializacion de sensor

/////////////////////INICIALIZACIÓN DE MONITOR SEREAL////////////////////////////////////

  Serial.begin(9600);
 
///////////////////PROTOCOLO DE INTERRUPCIÓNES PARA SELECCIONAR PARAMETROS////////////////
  
  attachInterrupt(digitalPinToInterrupt(2), estado, LOW);// interrupcion sobre pin BOTON con
  attachInterrupt(digitalPinToInterrupt(3), encoder, LOW);// interrupcion sobre pin A con

//////////////////////////////INICIALIZACIÓN DE RELOG RTC////////////////////////////////

   if (! rtc.begin()) {        // si falla la inicializacion del modulo
 Serial.println("Modulo RTC no encontrado !");  // muestra mensaje de error
 while (1);         // bucle infinito que detiene ejecucion del programa
 }

///////////////////////ACTIVAR PARA ACTUALIZAR HORA DEL RELOJ RTC////////////////////////

   //Le doy fecha y hora al reloj rtc, es necesario hacerlo una sola vez
  //rtc.adjust(DateTime(__DATE__, __TIME__ ));
  
/////////////////INICIALIZACIÓN DE PANTALLA LCD Y MENSAJES DE BIENVENIDA/////////////////
  
  lcd.init(); //Inicializamos el LCD
  lcd.backlight(); //Activamos la luz de fondo
  lcd.clear(); //Limpiamos lo que haya en pantalla
  lcd.createChar (0,GOTA);
  lcd.createChar (1,Temp);
  lcd.createChar (2,FLECHA);
  lcd.createChar(3,sol);
  lcd.home();
  lcd.setCursor(0,0);//Posicion: columna cero fila cero
  lcd.print("    CULTIVO");
  lcd.setCursor(0,1);
  lcd.print("   CONTROLADO");
  delay(3000);
  lcd.clear(); //Limpiamos lo que haya en pantalla
  lcd.setCursor(0,0);
  lcd.print("      INTA/");
  lcd.setCursor(0,1);
  lcd.print("     CONICET");
  delay(3000);
  lcd.clear();
  lcd.noBacklight();

}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////RUTINA DE TRABAJO LOOP//////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////



void loop(){
////////////////////////////////LECTURA DEL RELOJ RTC//////////////////////////////////

DateTime fecha = rtc.now();

/////////////////////////////////REINICIO SENSOR/////////////////////////////////////
if(fecha.second()==5){
  
  digitalWrite (vcc,LOW);
  delay(1000);
  digitalWrite(vcc,HIGH);
  }

/////////////////////////ACTUALIZAR VALORES DE POSICIÓN DEL ENCODER/////////////////////
     
ANTERIOR = POSICION;

//////////////////////////////SENSADO DE TEMPERATURA////////////////////////////////////

if (fecha.second()== 1){
  
    TEMPERATURA = dht.readTemperature();  // obtencion de valor de temperatura 
    HUMEDADSUELO = map(SUELO, 700, 200, 0, 99); //Creo la variable HUMEDADSUELO transformando los valores leidos
    SUELO = analogRead(A3);// Leo el sensor de HUMEDAD DE SUELO. 
     
     }
                
/////////////////////////////SENSADO DE HUMEDAD ATMOSFÉRICA////////////////////////////
    
   HUMEDAD = dht.readHumidity();

///////////////////////////////// PANTALLA LCD////////////////////////////////////////

        /////Para mostrarlo la temperatura leída por el sensor
    
     lcd.setCursor(0,0);//Posición: columna 0 fila 1, para que escriba en la segunda linea
     lcd.write(1);
     sprintf(texto,"%02d",TEMPERATURA);
     lcd.print(texto);
     lcd.print("C ");
     
 
                /////Temperatura seleccionada
    
     lcd.setCursor(0,1);
     lcd.write(1);
     sprintf(texto,"%02d",heladera);
     lcd.print(texto);
     lcd.print("C");  
     

             //Humedad Leída por el sensor

     lcd.setCursor(5,0);//Posicion: columna cero fila cero  
     lcd.print("H");
     sprintf(texto,"%02d",HUMEDAD);
     lcd.print(texto);
     lcd.print("% ");
     
             //Humedad Seleccionada para ventilación
     
     lcd.setCursor(5,1);
     lcd.print("H");
     sprintf(texto,"%02d",ventilador);
     lcd.print(texto);
     lcd.print("%");

 
                    //Reloj
     lcd.setCursor(9,0);  
     lcd.write(3);             
     lcd.setCursor(10,0);
     lcd.print(fecha.hour());
     
        //Cantidad de Horas de Luz seleccionadas
     
     lcd.setCursor(9,1);  
     lcd.write(3);
     lcd.setCursor(10,1);
     sprintf(texto,"%02d",hsluz);
     lcd.print(texto);

              //Humedad de suelo Leída por el sensor
              
     lcd.setCursor(12,0); 
     lcd.write(0);
     lcd.setCursor(13,0);
     sprintf(texto,"%02d",HUMEDADSUELO);
     lcd.print(texto);
       
    //Humedad de suelo seleccionada para regar
     
     lcd.setCursor(12,1); 
     lcd.write(0);
     lcd.setCursor(13,1);
     sprintf(texto,"%02d",humedadsuelo);
     lcd.print(texto);
  
//////////////////////////MENU DE SELECCIÓN DE VARIABLES///////////////////////////

        switch(ESTADO){
          case 0:
          if ((POSICION > ANTERIOR)){
          estadolcd = true;
         }
         
        if(estadolcd == true){
          lcd.backlight();
          unsigned long t_reciente = millis();
        
          if ((t_reciente - t) >= pantalla) {
          lcd.noBacklight();
           estadolcd = false;
           t = t_reciente;
          }
         }
          break;
          
          case 4:
             if (POSICION > ANTERIOR){
              heladera++;
             } if(POSICION < ANTERIOR) {
              heladera--;
             }
             
            if (heladera > 65){
              heladera = 0;
              lcd.clear();
          }
           if (heladera<0){
              heladera = 65;
              lcd.clear();
           } 
            lcd.setCursor(1,1);
            lcd.print(heladera);
            lcd.setCursor(4,1);  
            lcd.write(2);
           break;
           
         case 2:
          Serial.println(POSICION);
          lcd.backlight();
           if (POSICION > ANTERIOR){
              ventilador++;
             } if(POSICION < ANTERIOR) {
              ventilador--;
             }
          if (ventilador > 99){
              ventilador = 0;
              lcd.clear();
          }
          if (ventilador < 0){
             ventilador = 99;
             lcd.clear();
             }
          lcd.setCursor(6,1);
          lcd.print(ventilador);
          lcd.setCursor(9,1);
          lcd.write(2);
          
          break;
          
          case 3:
          Serial.println(POSICION);
             if (POSICION > ANTERIOR){
              hsluz++;
             } if(POSICION < ANTERIOR) {
              hsluz--;
             }
            if (hsluz > 23){
              hsluz = 0;
              lcd.clear();
          }
          if (hsluz < 0){
             hsluz = 23;
             lcd.clear();
          }
          lcd.setCursor(10,1);
          sprintf(texto, "%02d", hsluz);
          lcd.print(texto);
          //lcd.print(hsluz);
          lcd.setCursor(12,1);
          lcd.write(2);
          break;
            
         case 1:
          Serial.println(POSICION);
           if (POSICION > ANTERIOR){
              humedadsuelo++;
             } if(POSICION < ANTERIOR) {
              humedadsuelo--;
          } 
            if (humedadsuelo > 99){
              humedadsuelo = 0;
              lcd.clear();
          }
          if (humedadsuelo < 0){
             humedadsuelo = 99;
             lcd.clear();
          }
          lcd.setCursor(15,1);
          lcd.write(2);
          lcd.setCursor(13,1);
          lcd.print(humedadsuelo);
          //lcd.print(minutosluz);
          break;
            
          case 5:
            lcd.clear();
            lcd.setCursor(5,0);
            lcd.print("LISTO!");
            delay(2000);
            lcd.noBacklight();
            ESTADO=0;
              
          break;
          
        } 
        
 ////////////////////////////CONTROL DE TEMPERATURA//////////////////////////////


 if (TEMPERATURA > heladera){
   digitalWrite (HELADERA, HIGH);
 } else { digitalWrite (HELADERA, LOW);
  
 }   
 
 ///////////////////////////CONTROL DE LA HUMEDAD ATMOSFÉRICA//////////////////
       
     if (HUMEDAD > ventilador) { digitalWrite (VENTILADOR, HIGH);
    } else { digitalWrite (VENTILADOR, LOW);
  }

/////////////////////////CONTROL DE HUMIFICADOR DEL AIRE///////////////////////

//int HUMIDIFICADOR;

//HUMIDIFICADOR = (ventilador - 30);

//if (HUMEDAD < HUMIDIFICADOR){
  //digitalWrite(RIEGO, HIGH);
//} else {digitalWrite (RIEGO,LOW);
//}

//////////////////////CONDICIONAL ENCENDIDO DE LA LUZ//////////////////////////

if (fecha.hour()> 6){
  if(luz_inicio == true){
      digitalWrite (LUZ, HIGH);
      luz_inicio = false;
  }
}

int HORASLUZ;
HORASLUZ = (hsluz + 5);
 
if (fecha.hour()> HORASLUZ){
  if (luz_fin == true){
    digitalWrite (LUZ, LOW);
    luz_fin = false;
  }
  
}

  if (fecha.hour() == 2 && fecha.minute() == 0 ){   // si hora = 2 y minutos = 0 restablece valores de
    luz_inicio = true;       // variables de control en verdadero
    luz_fin = true;
 
 }

//////////////////////////CONDICIONAL PARA RIEGO///////////////////////////////
 
//if (HUMEDADSUELO < humedadsuelo){
  //        digitalWrite (RIEGO, HIGH);
    //      } else {digitalWrite (RIEGO, LOW);
      //  }

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////FUNCION DE INTERRUPCIÓN PARA CAMBIO DE ESTADO//////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
   
void estado(){
       static unsigned long ultInterrupcion = 0;  // variable static con ultimo valor de
              // tiempo de interrupcion
    unsigned long tiempInterrupcion = millis();  // variable almacena valor de func. millis
  
    if (tiempInterrupcion - ultInterrupcion > 10) {  // rutina antirebote desestima
                // pulsos menores a 10 mseg.
      if (digitalRead(BOTON) == LOW)     // si B es HIGH, sentido horario
      { 
        ESTADO++ ;        // incrementa POSICION en 1
        if (ESTADO == 8){
          ESTADO = 0;
        }
      }
  
       // establece limite inferior de 0 y
              // superior de 8 para POSICION
      ultInterrupcion = tiempInterrupcion;  // guarda valor actualizado del tiempo
    }           // de la interrupcion en variable static
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////FUNCION DE INTERRUPCIÓN PARA CAMBIO DE POSICIÓN DE ENCONDER//////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
  
void encoder()  {
  
    static unsigned long ultimaInterrupcion = 0;  // variable static con ultimo valor de
              // tiempo de interrupcion
    unsigned long tiempoInterrupcion = millis();  // variable almacena valor de func. millis
  
    if (tiempoInterrupcion - ultimaInterrupcion > 10) {  // rutina antirebote desestima
                // pulsos menores a 5 mseg.
      if (digitalRead(B) == LOW)     // si B es HIGH, sentido horario
      {
        POSICION++ ;        // incrementa POSICION en 1
      }
      else {          // si B es LOW, senti anti horario
        POSICION-- ;        // decrementa POSICION en 1
      }
  
      POSICION = min(1000, max(0, POSICION));  // establece limite inferior de 0 y
              // superior de 100 para POSICION
      ultimaInterrupcion = tiempoInterrupcion;  // guarda valor actualizado del tiempo
    }           // de la interrupcion en variable static
}
