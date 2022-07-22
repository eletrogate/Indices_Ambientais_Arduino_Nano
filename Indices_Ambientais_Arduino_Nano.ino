/******************************************************************************
    Índice de calor, Índice de Desconforto Térmico e Índice de Temperatura
                       e Umidade com Arduino NANO
                              Sketch Principal

                            Criado em 08 mar. 2022
                              por Michel Galvão

  Eletrogate - Loja de Arduino \\ Robótica \\ Automação \\ Apostilas \\ Kits
                            https://www.eletrogate.com/
******************************************************************************/

// Inclusão da(s) biblioteca(s)
#include <DHT.h> // Sensor de Temp./Umid. DHT11
#include <Wire.h> // Comunicação I2C
#include <LiquidCrystal_I2C.h> // Display LCD I2C 16x2
#include <EEPROM.h> // Mémoria EEPROM
#include <RTClib.h> // Real Time Clock DS3231

// Definição dos pinos
#define DHTPIN 2
#define JOY_Y_PIN A2
#define JOY_X_PIN A1
#define BUZZER_PIN 4

// Defnição do Tipo do DHT
#define DHTTYPE DHT11

// Protótipo das Funções
double calculaIndiceTemperaturaUmidade(double T, int R);
double calculaIndiceDesconfortoTermico(double T, int R);
double calculaIndiceCalor(double T, int R);
double converteCelsiusToFahrenheit(double valor);
double converteFahrenheitToCelsius(double valor);

// Instanciação dos objetos das classes
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;

// Variáveis Globais
unsigned long timeDHT; // Timer para leituras do sensor DHT
unsigned long timeLeituraJoy; // Timer para leituras do módulo Joystick
unsigned long timeEEPROM; // Timer para verificações/execuções da memória EEPROM

static int idMenu = 0; // identificador atual da tela do menu ciclíco
const int qtdMenus = 5; // quantidade máxima de telas do menu
double temp; // armazena temperatura
int umid; // armazena umidade

struct mediaTemperatura { // estrutura para manobras na EEPROM
  double media; // média de temperatura semanal
  int qtdDiasDaMedia; // quantidade de dias feitas as médias


  int horaUltimaAtualizacao; // última hora feita a atualização
  int minutoUltimaAtualizacao; // último minuto feita a atualização
  int segundoUltimaAtualizacao; // último segundo feita a atualização
  int diaUltimaAtualizacao; // último dia feita a atualização
  int mesUltimaAtualizacao; // último mês feita a atualização
  int anoUltimaAtualizacao; // último ano feita a atualização
  int primeiraVez; // especifica se é a primeira vez se o software foi executado
  //                    ou não
};

void setup() { // Configurações Iniciais
  // Inicializações necessárias
  Serial.begin(9600);
  dht.begin();
  lcd.init();

  lcd.backlight(); // ativa a luz de fundo do display LCD

  // Splash Screen
  lcd.setCursor(0, 0);
  lcd.print("Indices do Tempo");
  lcd.setCursor(0, 1);
  lcd.print(" eletrogate.com ");
  delay(1000);

  // Configuração do(s) pino(s)
  pinMode(BUZZER_PIN, OUTPUT);

  // Inicialização do RTC e testa se a conexão foi bem-sucedida
  if (!rtc.begin()) {
    lcd.setCursor(0, 0);
    lcd.print("Erro RTC:       ");
    lcd.setCursor(0, 1);
    lcd.print("nao encontrado   ");
    while (1); // loop infinito
  }

  // Verifica o sinalizador de parada do oscilador do registrador de status para
  //    ver se o DS3231 parou devido à perda de energia.
  if (rtc.lostPower()) {
    lcd.setCursor(0, 0);
    lcd.print("Erro RTC:       ");
    lcd.setCursor(0, 1);
    lcd.print("Acerte o Horario ");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Ajusta o horário para a
    //                                                  data/hora de compilação.

    // rtc.adjust(DateTime(2022, 3, 5, 16, 41, 0)); // Ajusta o horário para a
    //                                                  data/hora definido
    //                                                  nos parâmetros.
    delay(2000);
  }
  lcd.clear(); // limpa o display LCD
}

void loop() { // Loop Infinito
  delay(1); // atraso mínimo para não travamento desconhecido

  // Timer será executado de 4 em 4 segundos ou quando o valor de
  //  timeDHT (Valor do Timer) for igual à zero
  if (millis() - timeDHT >= 4000 || timeDHT == 0) {
    temp = dht.readTemperature(true); // Lê temperatura em Fahrenheit
    umid = dht.readHumidity(); // Lê a umidade
    timeDHT = millis(); // Atualiza o valor do Timer com millis()
  }

  mediaTemperatura mT; // cria um "objeto" do tipo da estrutura mediaTemperatura

  // Timer será executado de 60 em 60 segundos ou quando o valor de
  //  timeEEPROM (Valor do Timer) for igual à zero
  if (millis() - timeEEPROM >= 60000 || timeEEPROM == 0) {

    EEPROM.get(0, mT); //Pega os dados da estrutura mT na posição 0
    Serial.println("get EEPROM executado");

    // Cria um objeto da classe DateTime para armazenar os dados de hora/tempo
    //    em que ocorreu o último registro de média semanal
    DateTime ultimaAtualizacao = DateTime(mT.anoUltimaAtualizacao,
                                          mT.mesUltimaAtualizacao,
                                          mT.diaUltimaAtualizacao,
                                          mT.horaUltimaAtualizacao,
                                          mT.minutoUltimaAtualizacao,
                                          mT.segundoUltimaAtualizacao);

    // Instruções que mostram na Serial a diferença de tempo entre
    //    agora e a última atualização
    String message = "Diferença entre tempo atual e do tempo ";
    message += "da última atualzação da média (HH:MM:SS): ";
    Serial.print(message);
    // a função unixtime() retorna os segundos desde a meia-noite de 01/01/1970,
    //    então, a diferença entre unixtime deste momento com unixtime da última
    //    atualização irá retornar a quantidade de segundos passados desde o
    //    último registro de média semanal.
    unsigned long seconds = (rtc.now().unixtime() - ultimaAtualizacao.unixtime());
    unsigned long s, m, h; // variáveis para armazenar o tempo passado no
    //                          formato HH:MM:SS.
    s = seconds % 60;
    seconds = (seconds - s) / 60;
    m = seconds % 60;
    seconds = (seconds - m) / 60;
    h = seconds;
    Serial.print(h);
    Serial.print(":");
    Serial.print(m);
    Serial.print(":");
    Serial.println(s);

    // Relatório na Serial que mostra os dados da média semanal
    Serial.print("soma das temperaturas da semana (°C): ");
    Serial.println(mT.media);
    Serial.print("qtdDiasDaMedia: ");
    Serial.println(mT.qtdDiasDaMedia);
    Serial.print("media (°C): ");
    Serial.print(mT.media);
    Serial.print(" / ");
    Serial.print(mT.qtdDiasDaMedia);
    Serial.print(" = ");
    Serial.println(mT.media / mT.qtdDiasDaMedia);
    Serial.println();

    // se a váriavel que verifica que o software iniciou a EEPROM pela
    //    primera vez indicar que não houve nicialização (valor diferente de 999)
    //    OU se a diferença entre unixtime deste momento com unixtime da última
    //    atualização convertido para horas for maior ou igual à 24h
    //    irá ser executado o bloco de código do if.
    if (mT.primeiraVez != 999 ||
        ((rtc.now().unixtime() - ultimaAtualizacao.unixtime()) / 3600) >= 24) {

      // Algoritmo da média
      if (mT.qtdDiasDaMedia < 7) {
        // se a quantidade de dias feitas as médias for menor que 7 (1 semana), ...

        // adiciona o valor da soma da média com o valor da temperatura atual
        //   convertido de Fahrenheit para Celsius
        mT.media += converteFahrenheitToCelsius(temp);
        mT.qtdDiasDaMedia++; // auto-incrementa a quantidade de dias feitas as médias
        Serial.print("soma total das temperaturas: ");
        Serial.println(mT.media);
        Serial.print("mT.qtdDiasDaMedia: ");
        Serial.println(mT.qtdDiasDaMedia);
        Serial.print("média: ");
        Serial.println(mT.media / mT.qtdDiasDaMedia);
      } else if (mT.qtdDiasDaMedia >= 7) {
        // se não, se a quantidade de dias feitas as médias for >= à 7, ...

        // atribui o valor da temperatura atual à variável da média
        mT.media = converteFahrenheitToCelsius(temp);
        // atribui o valor 1 à variável de quantidade de dias feitas as médias
        mT.qtdDiasDaMedia = 1;
      }

      //atribui os valores aos membros
      mT.horaUltimaAtualizacao = rtc.now().hour();
      mT.minutoUltimaAtualizacao = rtc.now().minute();
      mT.segundoUltimaAtualizacao = rtc.now().second();
      mT.diaUltimaAtualizacao = rtc.now().day();
      mT.mesUltimaAtualizacao = rtc.now().month();
      mT.anoUltimaAtualizacao = rtc.now().year();
      mT.primeiraVez = 999;

      EEPROM.put(0, mT); // atualiza os dados da EEPROM no endereço 0
      //                      com os dados de mT (a struct)

      Serial.println("put EEPROM executado");
    }
    timeEEPROM = millis(); // Atualiza o valor do Timer com millis()
  }

  switch (idMenu) { // fluxo de controle das telas do menu
    case 0: // Caso o identificador atual da tela do menu ciclíco seja 0, exibe a
      //          tela com dados atuais: temperatura e umidade atmosférica
      lcd.setCursor(0, 0);
      if (temp < 0) {
        lcd.print("T");
      } else {
        lcd.print("T:");
      }
      if (temp <= 10 && temp >= 0) {
        lcd.print("0");
      }
      lcd.print(converteFahrenheitToCelsius(temp)); // converte a temperatura
      //                                                atual de Fahrenheit em
      //                                                Celsius e a exibe no display.
      lcd.write(223); // símbolo grau (11011111) da tabela ascii
      // Tabela ASCII: https://www.google.com/search?q=tabela+ASCII+lcd+16x2&tbm=isch
      lcd.print("C");

      lcd.setCursor(11, 0);
      lcd.print("U:");
      if (umid <= 10 && umid >= 0) {
        lcd.print("0");
      }
      lcd.print(umid); // exibe a umidade no display.
      lcd.print("% ");
      lcd.setCursor(0, 1);
      lcd.print("< 1. Atuais    >");
      break;

    case 1: // Caso o identificador atual da tela do menu ciclíco seja 1, exibe a
      //          tela com o Índice de Temperatura e Umidade
      lcd.setCursor(4, 0);

      //calcula e exibe o Índice de Temperatura e Umidade em Celsius
      lcd.print(converteFahrenheitToCelsius(calculaIndiceTemperaturaUmidade(
                                              temp,
                                              umid)));
      lcd.print(" ");
      lcd.write(223);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("< 2. Indi. T/U >");
      break;

    case 2: // Caso o identificador atual da tela do menu ciclíco seja 2, exibe a
      //          tela com o Índice de Desconforto Térmico
      lcd.setCursor(4, 0);

      //calcula e exibe o Índice de Desconforto Térmico em Celsius
      lcd.print(converteFahrenheitToCelsius(calculaIndiceDesconfortoTermico(
                                              temp,
                                              umid)));
      lcd.print(" ");
      lcd.write(223);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("< 3. Indi. IDT >");
      break;

    case 3: // Caso o identificador atual da tela do menu ciclíco seja 3, exibe a
      //          tela com o Índice de Calor
      lcd.setCursor(4, 0);
      //calcula e exibe o Índice de Calor em Celsius
      lcd.print(converteFahrenheitToCelsius(calculaIndiceCalor(
                                              temp,
                                              umid)));
      lcd.print(" ");
      lcd.write(223);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("< 4. Indi. IC  >");
      break;

    case 4: // Caso o identificador atual da tela do menu ciclíco seja 4, exibe a
      //          tela com a média Semanal
      lcd.setCursor(4, 0);

      // exibe a média semanal
      lcd.print(mT.media / mT.qtdDiasDaMedia);
      lcd.print(" ");
      lcd.write(223);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("< 5. Media Sem.>");
      break;
  }

  // Timer será executado de 0,2 em 0,2 segundos
  if (millis() - timeLeituraJoy > 200) {

    // se a leitura analógica da porta conectada ao eixo X do JoyStick
    //    for maior que 900, ...
    if (analogRead(JOY_X_PIN) > 900) {// Para Direita
      // Efetua o Feedback do buzzer ao usuário
      digitalWrite(BUZZER_PIN, HIGH);
      delay(20);
      digitalWrite(BUZZER_PIN, LOW);
      // incrementa o identificador da tela do menu ciclíco
      idMenu++;
      if (idMenu > (qtdMenus - 1)) { // se idMenu for maior que a quantidade de
        //                                telas do menu (convertido para índice
        //                                começado no 0), ...
        idMenu = 0; // atribui o valor 0 ao identificador da tela do menu ciclíco
      }

      lcd.clear(); // limpa o display LCD
    }
    // se não, se leitura analógica da porta conectada ao eixo X do JoyStick
    //    for menor que 20, ...
    else if (analogRead(JOY_X_PIN) < 20)// Para Esquerda
    {
      // Efetua o Feedback do buzzer ao usuário
      digitalWrite(BUZZER_PIN, HIGH);
      delay(20);
      digitalWrite(BUZZER_PIN, LOW);
      // decrementa o identificador da tela do menu ciclíco
      idMenu--;

      if (idMenu < 0) { // se idMenu for menor que 0, ...
        idMenu = (qtdMenus - 1); // atribui o valor da quantidade de telas
        //                            do menu (convertido para índice começado
        //                            no 0) ao identificador da tela do menu ciclíco
      }

      lcd.clear(); // limpa o display LCD
    }

    timeLeituraJoy = millis();// Atualiza o valor do Timer com millis()
  }

}

/**
  Calcula o Índice de Calor, retornando o valor em Fahrenheit.

  @param T : Valor da temperatura na escala Fahrenheit
  @param R : Valor da Umidade Relativa em %
*/
double calculaIndiceCalor(double T, int R)
{
  double HI;

  HI = 0.5 * (T + 61.0 + ((T - 68.0) * 1.2) + (R * 0.094));

  if (HI >= 80.0)
  {
    double c1 = -42.379;
    double c2 = 2.04901523;
    double c3 = 10.14333127;
    double c4 = -0.22475541;
    double c5 = -0.00683783;
    double c6 = -0.05481717;
    double c7 = 0.00122874;
    double c8 = 0.00085282;
    double c9 = -0.00000199;

    HI = c1 +
         c2 * (T) +
         c3 * R +
         c4 * T * R +
         c5 * pow(T, 2) +
         c6 * pow(R, 2) +
         c7 * pow(T, 2) * R +
         c8 * T * pow(R, 2) +
         c9 * pow(T, 2) * pow(R, 2);

    if (R < 13 && (T >= 80.0 && T <= 112.0))
    {
      double ajuste = ((13.0 - R) / 4.0) *
                      sqrt((17.0 - abs(T - 95.0)) / 17.0);

      HI = HI - ajuste;
    }
    else if (R > 85.0 && (T >= 80.0 && T <= 87.0))
    {
      double ajuste = ((R - 85.0) / 10) *
                      ((87.0 - T) / 5);

      HI = HI + ajuste;
    }
  }

  return HI;
}

/**
  Calcula o Índice de Desconforto Térmico, retornando o valor em Fahrenheit.

  @param T : Valor da temperatura na escala Fahrenheit
  @param R : Valor da Umidade Relativa em %
*/
double calculaIndiceDesconfortoTermico(double T, int R)
{
  double IDT;
  T = (T - 32) * 5 / 9; // converte T de fahrenheit para celsius

  IDT = T - (0.55 - 0.0055 * R) * (T - 14.5);
  IDT = (IDT * 9 / 5) + 32; // converte IDT de celsius para fahrenheit

  return IDT;
}

/**
  Calcula o Índice de Temperatura e Umidade, retornando o valor em Fahrenheit.

  @param T : Valor da temperatura na escala Fahrenheit
  @param R : Valor da Umidade Relativa em %
*/
double calculaIndiceTemperaturaUmidade(double T, int R)
{
  double THI;
  T = (T - 32) * 5 / 9; // converte T de fahrenheit para celsius

  THI = 0.8 * T + ((R * T) / 500);
  THI = (THI * 9 / 5) + 32; // converte THI de celsius para fahrenheit

  return THI;
}

double converteCelsiusToFahrenheit(double valor) {
  return (valor * 9 / 5) + 32;
}

double converteFahrenheitToCelsius(double valor) {
  return (valor - 32) * 5 / 9;
}
