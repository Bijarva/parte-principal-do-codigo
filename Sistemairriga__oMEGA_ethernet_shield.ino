#include "stationDefines.h"                            //
#include <SPI.h>                                       //biblioteca SPI, usada para permitir que nosso programa se comunique pela ethernet através do barramento serial.
#include <Ethernet.h>                                  //biblioteca necessária para ser usado o ethernet shield.
#include <DHT.h>                                       //biblioteca necessária para ter aceso aos dados do sensor DHT.
#include <Wire.h>                                      //
#include <SimpleTimer.h>                               //biblioteca de temporizadores.
SimpleTimer timer;                                     
int BOMBA = 6;                                         //Define a saida da BOMBA no pino 6.
int PROTECAOBOMBA = 7;                                 ////Define a saida da PROTEÇÃOBOMBA no pino 7.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   //Endereço Físico da Placa Ethernet
byte ip[] = { 192, 168, 0, 125 };                      //IP da Lan onde queremos que seja exibida a pagina HTML ( deve ser configurado conforme rede local ).
byte gateway[] = { 192, 168, 0, 1 };                   //acesso à internet via roteador
byte subnet[] = { 255, 255, 255, 0 };                  //máscara de sub-rede, normalmente esse é o padrão usado.
EthernetServer server(80);                             //Porta do Servidor, dependendo do provedor deverá ser alterada para ter acesso externo.  
String readString;


/* Parte sensor DHT*/
#define DHTPIN 3                                       //Qual o pino digital o sensor DHT está conectado
#define DHTTYPE DHT11                                  //Define o tipo de sensor DHT vai ser usado, nesse cado o DHT11.
DHT dht(DHTPIN, DHTTYPE);                              //Inicializa o sensor DHT.                      

/* Parte sensor DS18B20 */
#include <OneWire.h>                                   //Importa a biblioteca OneWire. 
#include <DallasTemperature.h>                         //Importa a biblioteca DallasTemperature.
OneWire ds(2);                                         //Determina entrada no pino 2.

                                                       //Endereço do sensor usado no projeto = { 0x28, 0xFF, 0xA5, 0x89, 0xA2, 0x16, 0x04, 0x7D };

void setup() {                                         //Open serial communications and wait for port to open:
 
  Serial.begin(9600);                                  //Define a taxa de dados em bits por segundo (baud) (serial).
     while (!Serial) {
    ; 
  }
  pinMode(BOMBA, OUTPUT);                              //Configura o Pino da BOMBA como saida.
  pinMode(PROTECAOBOMBA, OUTPUT);                      //Configura o Pino da PROTECAOBOMBA como saida.
  pinMode(BOMBA_ON_BUTTON, INPUT_PULLUP);              //Habilita os resistores internos de pullup.
  pinMode(PROTECAOBOMBA_ON_BUTTON, INPUT_PULLUP);      //Habilita os resistores internos de pullup.
  pinMode(soilMoisterVcc, OUTPUT);                     //Configura o Pino do sensor como saida.              
  
 /* Parte inicio da conexao Ethernet */ 
  Ethernet.begin(mac, ip, gateway, subnet);            //Inicia a conexão Ethernet. 
  server.begin();                                      //Inicia a conexãoe do Servidor.
  Serial.print("Controle Automatico e Manual de Irrigacao Para Jardinagem ");       // Imprime o Nome do projeto.
  Serial.println();                                    //Imprime na próxima linha.
  Serial.print("IP do servidor é ");                   //Imprime a mensagem  "IP do servidor é"       
  Serial.println(Ethernet.localIP());                  //Imprime o endereço IP do servidor na serial. 
  delay(250);    
  dht.begin();                                         //Inicia o sensor DHT.
 
  digitalWrite(BOMBA_PIN, HIGH);                        //Inverte a lógica normalmente HIGH.
  digitalWrite(PROTECAOBOMBA_PIN, HIGH);                //Inverte a lógica normalmente HIGH.
  digitalWrite (soilMoisterVcc, LOW);                   //Inverte a lógica normalmente LOW.
  //waitButtonPress (SHOW_SET_UP);                      // ainda estou estudando essa parte
  //startTimers();                                      // ainda estou estudando essa parte
}


void loop() {                                           //Comando de inicio do programa; Inicia o programa após a chave.

float h = dht.readHumidity();                           //Lê a umidade do sensor.               
float t = dht.readTemperature();                        //Lê a temperatura do sensor em Celsius.               
float u = soilMoister;                                  //Lê a umidade do sensor de umidade do solo.
getSoilMoisterData();                                   //Obtem os dados do sensor de umidade do solo.

/* Parte do programa do DS18B20 */
byte i;
byte present = 0;
byte type_s;
byte data[12];
byte addr[8];
float celsius, fahrenheit;
if ( !ds.search(addr)) {
    Serial.println("No more addresses.");               //Se não tiver mais nenhum sensor imprime.
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }

  Serial.print("ROM =");                               //Imprime a "ROM"
  for( i = 0; i < 8; i++) {
    Serial.write(' ');                                 //Imprime a ROM do sensor DS18B20 em HEX.
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();

  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");                      //Imprime o chip usado no projeto que no caso o DS18B20.
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device."); //Se não for da familia DS imprime.
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44);                                               // inicia a conversão.

  delay(1000);                                                  
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; 
    if (data[7] == 0x10) {
      
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;  
    else if (cfg == 0x20) raw = raw & ~3; 
    else if (cfg == 0x40) raw = raw & ~1; 
    
  }
  celsius = (float)raw / 16.0; 
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  
  
                                                                                    // Cria a conexão do cliente
  EthernetClient client = server.available();
  if (client) {
     boolean currentLineIsBlank = true;
    while (client.connected()) {   
      if (client.available()) {
        char c = client.read();
     
                                                                                    //Ler a Solicitação HTTP.
        if (readString.length() < 100) {
          //store characters to string
          readString += c;
          
         }

                                                                                    //Condição se a solicitação HTTP tiver terminado.
         if (c == '\n') {          
           Serial.println(readString);                                              //Imprime o monitor serial para depuração.
     
           client.println("HTTP/1.1 200 OK");                                       //Envia uma nova página.
           client.println("Content-Type: text/html");                               //Imprime conteúdo texto html.
           client.println();     
           client.println("<HTML>");
           client.println("<HEAD>");
           client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");                                                          //Habilita o conteudo a ser executado em tela inteira.
           client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");                                   //Coloca o conteúdo preto translúcido.
           client.println("<style type=\"text/css\">body{ margin:60px 0px; padding:0px; text-align:center; }</style>");                           //Define a margem do texto 60 pixel por 0 Pixel alinhamento central.
           client.println("<style type=\"text/css\">body h1 { text-align: center; font-family:Arial, Helvetica, sans-serif; }</style>");          //determina  o corpo h1 como central e a fonte.  
           client.println("<style type=\"text/css\">body h2 { text-align: center; font-family:Arial; }</style>");                                 //determina  o corpo h2 como central e a fonte.        
           client.println("<style type=\"text/css\">body a { text-decoration:none; width:75px; height:50px; border-color:black; border-top:2px solid; border-bottom:2px solid; border-right:2px solid; border-left:2px solid; border-radius:10px 10px 10px; -o-border-radius:10px 10px 10px; -webkit-border-radius:10px 10px 10px; font-family: Arial, Helvetica, sans-serif; -moz-border-radius:10px 10px 10px; background-color:#527fbd; padding:8px; text-align:center; }</style>"); 
           client.println("<TITLE>Controle Automatico e Manual de Irrigacao Para Jardinagem </TITLE>");                                           
           //determina  o corpo a como central e a fonte e pixel, corresponde aos botões que foram usados.
           client.println("</HEAD>");
           client.println("<BODY>");
           client.println("<H1>Controle Automatico e Manual de Irrigacao Para Jardinagem </H1>");                                                //Imprime o titulo do projeto com os tamanhos pré definidos.
           client.println("<hr />");                                                                                                             //Imprime uma linha entre o Titulo e o proximo texto.
           client.println("<H2>Controle do Sistema</H2>");                                                                                       //Imprime o titulo no tamanho h2
           client.println("<br />");                                                                                                             //Cria um espaço para o próximo texto.
           client.println("<a href=\"/?button1on\"\">Ligar BOMBA</a>");                                                                          //Cria o botão Ligar BOMBA com stilo de texto corpo "a".
           client.println("<a href=\"/?button1off\"\">Desligar BOMBA</a><br />");                                                                //Cria o botão Desligar BOMBA com stilo de texto corpo "a".
           client.println("<br />");                                                                                                             //Cria um espaço para o próximo texto. 
           client.println("<br />");                                                                                                             //Cria um espaço para o próximo texto.
           client.println("<a href=\"/?button2on\"\">Ligar PROTECAO BOMBA</a>");                                                                 //Cria o botão Ligar PROTECAO BOMBA com stilo de texto corpo "a".
           client.println("<a href=\"/?button2off\"\">Desligar PROTECAO BOMBA</a><br />");                                                       //Cria o botão Desligar PROTECAO BOMBA com stilo de texto corpo "a".
           client.println("<br />");                                                                                                             //Cria um espaço para o próximo texto.
           client.println("</BODY>");
           client.println("</HTML>");                      
           client.print("<H2>Umidade do Ar <H2>");                                                                                              //Escreve o texto "Umidade do Ar" no formato H2.
           client.println("<H2>");
           client.println("<p />");
           client.println("<H1>");
           client.print(h);                                                                                                                    //Imprime a "umidade" conforme declarado. 
           client.print(" %\t");                                                                                                               //Imprime "%/" conforme  [ISO-8859-1].           
           client.println("</H1>");                                                                                                            
           client.println("<p />");  
           client.println("<H2>");
           client.print("Temperatura do Ar ");                                                                                                 //Imprime "Temperatura do Ar" no formato H2.
           client.println("</H2>");
           client.println("<H1>");
           client.print(t);                                                                                                                    //Imprime a "temperatura do ar" em Celcius conforme declarado.
           client.println(" &#176;");                                                                                                          //Imprime "°" conforme  [ISO-8859-1].
           client.println("C");                                                                                                                //Imprime "C" no formato H2.
           client.println("<H2>");
           client.print("Temperatura da Bomba ");                                                                                              //Imprime texto "Temperatura da Bomba" no formato H2.
           client.println("</H2>");
           client.println("<H1>");
           client.println(celsius);                                                                                                            //Imprime a "temperatura" em Celcius conforme declarado.
           client.println(" &#176;");                                                                                                          //Imprime "°" conforme  [ISO-8859-1].
           client.println("C");                                                                                                                //Imprime "C" no formato H2.
           client.println("<H2>");
           client.print("Umidade do solo ");                                                                                                   //Imprime "Temperatura do Ar" no formato H2.
           client.println("</H2>");
           client.println("<H1>");
           client.print(u);                                                                                                                    //Imprime a "umidade" conforme declarado.                                                                                                             
           client.print(" %\t");                                                                                                               //Imprime "%/" conforme  [ISO-8859-1].                                                                                                      
           client.println("</H1>");                
           client.println();          
           client.println("</html>");
          break;
        }
        if (c == '\n') {                                                                        //Começa uma nova linha.
          
          currentLineIsBlank = true;
        }
        else if (c != '\r') {                                                                  //Obter um caracter na proxima linha.
          
          currentLineIsBlank = false;
        }
      }
    }
    
    delay(1);                                                                                  //Atraso para receber os dados.
    
    client.stop();                                                                             //Fecha a conexão.
    Serial.println("client disonnected");
  }
                                                                                              //Ao pressionar o botão no navegador controla as saídas uma solicitação é enviada.
           if (readString.indexOf("?button1on") >0){
               digitalWrite(BOMBA, LOW);                                                      //Escreve no pino da bomba LOW.                                             
           }
           if (readString.indexOf("?button1off") >0){
               digitalWrite(BOMBA, HIGH);                                                      //Escreve no pino da bomba HIGH.
           }
           if (readString.indexOf("?button2on") >0){
                digitalWrite(PROTECAOBOMBA, LOW);                                              //Escreve no pino da PROTECAOBOMBA LOW. 
                 
                
           }
           if (readString.indexOf("?button2off") >0){
                digitalWrite(PROTECAOBOMBA, HIGH);                                            //Escreve no pino da PROTECAOBOMBA HIGH. 
                
           }
           
            readString="";                                                                     //Limpa o String para a próxima linha.
           
        }

  }


void autoControlPlantation(void)
{ 
  if (soilMoister < SECO_SOLO) 
  {
    turnPumpOn();
  }

  if (airTemp < FRIO_TEMP) 
  {
    turnLampOn();
  }
}

/***************************************************
* Turn Pump On for a certain amount of time
****************************************************/
void turnPumpOn()
{
  pumpStatus = 1;
  aplyCmd();
  delay (TIME_PUMP_ON*1000);
  pumpStatus = 0;
  aplyCmd();
}

/***************************************************
* Turn Lamp On for a certain amount of time 
****************************************************/
void turnLampOn()
{
  lampStatus = 1;
  aplyCmd();
  delay (TIME_LAMP_ON*1000);
  lampStatus = 0;
  aplyCmd();
}


  

