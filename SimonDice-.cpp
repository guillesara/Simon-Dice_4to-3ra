#include <LiquidCrystal_I2C.h>

// Definición de pines para la pantalla LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Definición de pines para LEDs, pads (botones), botones de menú y un zumbador (buzzer)
int Leds[] = {2, 3, 4, 5};
int Pads[] = {6, 7, 8, 9};
int MenuButtons[] = {10, 11, 12};
int Buzzer = 13;
int BuzzerTones[] = {3200, 3300, 3400, 3500};

// Variables del juego
int Sequence[255];
int Level = 0;
int RemainPads = 0;
int Fails = 0;
unsigned long Time = 0;

// Datos del menú
char* CurrentPanel = "HOME";
char* Items[3];
int CurrentItem = 0;

// Estructura para almacenar elementos del menú
struct MenuItem {
    char* name;
    char* options[4][3];
};

// Datos del menú con opciones y configuraciones
MenuItem MenuData[] = {
    {"HOME", 
        {
            {"Normal", "SELECTED", "NORMAL_DIFICULTY"}, 
            {"Speedrun", "NOT SELECTED", "SPEEDRUN_DIFICULTY"}
        }
    },
    {"NORMAL_DIFICULTY", 
        {
            {"Facil", "SELECTED", "NORMAL_FACIL"}, 
            {"Normal", "NOT SELECTED", "NORMAL_NORMAL"}, 
            {"Dificil", "NOT SELECTED", "NORMAL_DIFICIL"}
        }
    },
    {"SPEEDRUN_DIFICULTY", 
        {
            {"1 Minutos", "SELECTED", "SPEEDRUN_1MIN"}, 
            {"30 Segundos", "NOT SELECTED", "SPEEDRUN_30SEC"}, 
            {"15 Segundos", "NOT SELECTED", "SPEEDRUN_15SEC"}
        }
    }
};

// Configuración inicial
void setup()
{
    Serial.begin(9600);

    // Inicialización de la pantalla LCD y activación del retroiluminado
    lcd.init();
    lcd.backlight();
    
    // Configuración de pines
    pinMode(Buzzer, OUTPUT); 
    for (int i = 0; i < (sizeof(Leds) / sizeof(int)); i++)
    {
        pinMode(Leds[i], OUTPUT); 
    }
    for (int i = 0; i < (sizeof(Pads) / sizeof(int)); i++)
    {
        pinMode(Pads[i], INPUT); 
    }
    for (int i = 0; i < (sizeof(MenuButtons) / sizeof(int)); i++)
    {
        pinMode(MenuButtons[i], INPUT); 
    }

    // Generar la secuencia inicial y mostrar la pantalla de inicio
    GenerateSequence();
    DrawScreen("HOME");
  
    // Prende y apaga los leds
    for (int i = 0; i < (sizeof(Leds)/sizeof(Leds[0])); i++)
    {
      digitalWrite(Leds[i], HIGH);
    }
    delay(1000);
    for (int i = 0; i < (sizeof(Leds)/sizeof(Leds[0])); i++)
    {
      digitalWrite(Leds[i], LOW);
    }

    Serial.println("Started");
}

// Bucle principal
void loop()
{
    // Controles del menú
    if (!digitalRead(MenuButtons[0]))
    {
        ChangeOption("UP");
        delay(200);
    }
    else if (!digitalRead(MenuButtons[1]))
    {
        ChangeOption("DOWN");
        delay(200);
    }
    else if (!digitalRead(MenuButtons[2]))
    {
        char* Option = ChangeOption("ENTER");
        Serial.print("optn: ");
        Serial.println(Option);

        // Iniciar el juego según la opción seleccionada
        if ((strstr(Option, "NORMAL_") != NULL && Option != "NORMAL_DIFICULTY"))
        {
            Serial.println("norml");
            GameNormalMode(Option);
        }
        else if ((strstr(Option, "SPEEDRUN_") != NULL && Option != "SPEEDRUN_DIFICULTY"))
        {
            Serial.println("speud");
            GameSpeedrunMode(Option);
        }

        delay(200);
    }
}

// Generar una nueva secuencia aleatoria para el juego
void GenerateSequence()
{
    Level = 0;
    randomSeed(analogRead(A1));
    for (int i = 0; i < (sizeof(Sequence) / sizeof(Sequence[0])); i++)
    {
        Sequence[i] = random(4);
    }
}

// Modo de juego normal
void GameNormalMode(char* Mode)
{
    bool MainLoop = true;
    int FailsLimit = 0;

    // Configuración de la dificultad
    if (Mode == "NORMAL_FACIL")
    {
        FailsLimit = 3;
        Fails = 0;
    }

    DrawScreen("NORMAL_GAME");

    delay(1500);

    while (MainLoop)
    {
        RemainPads = Level;
        DrawScreen("UPDATE");

        // Mostrar la secuencia
        Serial.print("Sequencia ");
        for (int i = 0; i < Level; i++)
        {
            tone(Buzzer, BuzzerTones[Sequence[i]], 200);
            // Visualización de la secuencia en los LEDs
            digitalWrite(Leds[Sequence[i]], HIGH);
            delay(500);
            digitalWrite(Leds[Sequence[i]], LOW);
            delay(500);
        }
        Serial.println("");

        // Leer la secuencia del usuario
        for (int i = 0; i < Level; i++)
        {
            bool loop = true;
            Time = millis() + 2000; //+ (Level * 500)

            while (loop)
            {
                // Verificar si el tiempo de respuesta ha expirado
                if (Mode == "NORMAL_DIFICIL")
                {
                    Serial.println(millis());
                    Serial.println(Time);

                    if (millis() > Time) {
                        MainLoop = false;
                        return GameFail();
                    }
                }
                
                // Comprobar si se ha presionado el botón correspondiente
                if (!digitalRead(Pads[Sequence[i]]))
                {
                    // Indicar visual y auditivamente la respuesta correcta
                    digitalWrite(Leds[Sequence[i]], HIGH);
                    Serial.println(BuzzerTones[Sequence[i]]);
                    tone(Buzzer, BuzzerTones[Sequence[i]], 200);
                    delay(200);
                    digitalWrite(Leds[Sequence[i]], LOW);

                    RemainPads = (Level - 1) - i;
                    DrawScreen("UPDATE");
                    loop = false;

                    delay(500);
                }
                else
                { 
                    // Comprobar si se ha presionado un botón incorrecto
                    for (int i2 = 0; i2 < (sizeof(Pads) / sizeof(Pads[0])); i2++)
                    {
                        if (!digitalRead(Pads[i2]) && i2 != Sequence[i])
                        {
                            // Manejo de fallos
                            if (Fails >= FailsLimit)
                            {
                                MainLoop = false;
                                return GameFail();
                            }
                            else
                            {
                                //tone(Buzzer, 100, 500);
                                for (int i = 0; i < (sizeof(Leds) / sizeof(Leds[0])); i++)
                                {
                                    digitalWrite(Leds[i], HIGH);
                                }
                                delay(500);
                                for (int i = 0; i < (sizeof(Leds) / sizeof(Leds[0])); i++)
                                {
                                    digitalWrite(Leds[i], LOW);
                                }
                                Fails++;

                                delay(500);
                            }
                        }
                    }
                }
            }
        }

        // Incrementar el nivel y actualizar la pantalla
        Level++;
        DrawScreen("UPDATE");
    }
}

// Modo de juego de velocidad
void GameSpeedrunMode(char* Mode)
{
    bool MainLoop = true;
    unsigned long refreshTime = 0;

    // Configuración del tiempo límite
    if (Mode == "SPEEDRUN_1MIN")
    {
        Time = millis() + 60000;
    }
    else if (Mode == "SPEEDRUN_30SEC")
    {
        Time = millis() + 30000;
    }
    else if (Mode == "SPEEDRUN_15SEC")
    {
        Serial.print("titi: ");
        Time = millis() + 15000;
    }

    DrawScreen("SPEEDRUN_GAME");

    while (MainLoop)
    {
        // Verificar si se ha agotado el tiempo
        if (millis() > Time) {
            MainLoop = false;
            return GameFail();
        }

        // Mostrar la secuencia
        digitalWrite(Leds[Sequence[Level]], HIGH);

        // Leer la secuencia del usuario
        if (!digitalRead(Pads[Sequence[Level]]))
        {
            digitalWrite(Leds[Sequence[Level]], LOW);
            DrawScreen("UPDATE");
            Level++;

            delay(50);
        }

        // Actualizar la pantalla cada segundo
        if (millis() > refreshTime)
        {
            refreshTime = millis() + 1000;
            DrawScreen("UPDATE");
        }
    }
}

// Manejo de fallos en el juego
void GameFail()
{
    for (int i = 0; i < 3; i++)
    {
        // Indicar visual y auditivamente el fallo
        tone(Buzzer, 1000, 200);
        for (int i = 0; i < (sizeof(Leds) / sizeof(Leds[0])); i++)
        {
            digitalWrite(Leds[i], HIGH);
        }
        delay(500);
        for (int i = 0; i < (sizeof(Leds) / sizeof(Leds[0])); i++)
        {
            digitalWrite(Leds[i], LOW);
        }
        delay(500);
    }

    // Generar una nueva secuencia y mostrar la pantalla de inicio
    GenerateSequence();
    DrawScreen("HOME");
}

// Dibujar la pantalla en la pantalla LCD
void DrawScreen(char* Panel)
{
    // Actualizar el panel actual si no es una actualización
    if (Panel != "UPDATE") 
    {
        CurrentPanel = Panel; 
    }

    lcd.clear();

    // Mostrar la pantalla según el panel actual
    if (CurrentPanel == "NORMAL_GAME")
    {
        lcd.setCursor(0, 0);
        lcd.print("Pads rest: ");
        lcd.print(RemainPads);
        
        lcd.setCursor(0, 1);
        lcd.print("Puntos: ");
        lcd.print(Level);

        return;
    }
    else if (CurrentPanel == "SPEEDRUN_GAME")
    {
        lcd.setCursor(0, 0);
        lcd.print("Tiempo rest: ");
        lcd.print((Time - millis()) / 1000);
        
        lcd.setCursor(0, 1);
        lcd.print("Puntos: ");
        lcd.print(Level);

        return;
    }

    // Mostrar las opciones del menú
    for (int i = 0; i < (sizeof(MenuData) / sizeof(MenuData[0])); i++)
    {
        if (MenuData[i].name == CurrentPanel)
        {
            int Count = 0;
            int Count2 = 0;
            int SelectIndex = 0;

            // Encontrar el índice de la opción seleccionada
            for (int i2 = 0; i2 < GetOptionSize(i); i2++)
            {
                if (MenuData[i].options[i2][1] == "SELECTED")
                {
                    SelectIndex = i2;
                    break;
                }
            }

            // Ajustar el contador para manejar la visualización
            for (int i2 = 0; i2 < GetOptionSize(i); i2++) 
            { 
                if (i2 >= 2 && SelectIndex >= 2) Count++; 
            } 

            // Mostrar las opciones en la pantalla
            for (int i2 = 0 + Count2; i2 < GetOptionSize(i); i2++)
            {
                if (Count2 > 2) break;
                Count2++;

                if (MenuData[i].options[i2][1] == "SELECTED")
                {
                    lcd.setCursor(15, i2 - Count);
                    lcd.print("<");
                }
                else
                {
                    lcd.setCursor(15, i2 - Count);
                    lcd.print(" ");
                }

                lcd.setCursor(0, i2 - Count);
                lcd.print(MenuData[i].options[i2][0]);
            }

            break;
        }
    } 
}

// Cambiar la opción del menú
char* ChangeOption(char* Mode)
{
    for (int i = 0; i < (sizeof(MenuData) / sizeof(MenuData[0])); i++)
    {
        Serial.println(CurrentPanel);
        if (MenuData[i].name == CurrentPanel)
        {
            int NextItem = -1;

            // Encontrar la siguiente opción según la dirección indicada
            for (int i2 = 0; i2 < GetOptionSize(i); i2++)
            {
                
                if (MenuData[i].options[i2][1] == "SELECTED")
                {
                    if (Mode == "UP" && (i2 - 1) >= 0)
                    {
                        NextItem = i2 - 1;
                        MenuData[i].options[i2][1] = "NOT SELECTED";
                    }
                    else if (Mode == "DOWN" && (i2 + 1) < GetOptionSize(i))
                    {
                        NextItem = i2 + 1;
                        MenuData[i].options[i2][1] = "NOT SELECTED";
                    }
                    else if (Mode == "ENTER")
                    {
                        char* Option = MenuData[i].options[i2][2];

                        // Cambiar a la pantalla correspondiente al seleccionar una opción
                        if ((strstr(Option, "NORMAL_") != NULL && Option == "NORMAL_DIFICULTY") || (strstr(Option, "SPEEDRUN_") != NULL && Option == "SPEEDRUN_DIFICULTY"))
                        {
                            DrawScreen(Option);
                        }

                        return Option;
                    }
                }
            } 

            // Actualizar la opción seleccionada
            if (NextItem != -1)
            {
                
                MenuData[i].options[NextItem][1] = "SELECTED";
            }

            break;
        }
    } 

    // Actualizar la pantalla después de cambiar la opción
    DrawScreen("UPDATE");

    return "NONE";
}

// Obtener el tamaño de las opciones en un menú
int GetOptionSize(int MenuIndex)
{
    int Size = 0;

    while (MenuData[MenuIndex].options[Size][0] != NULL && Size < (sizeof(MenuData[MenuIndex].options)/sizeof(MenuData[MenuIndex].options[0]))) {
        Size++;
    }

    return Size;
}
