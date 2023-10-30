// --------------------------------------------------------------------------------------------------------------------
// Multi-tâches cooperatives : solution basique mais efficace :-)
// --------------------------------------------------------------------------------------------------------------------

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET    16 //Pin reset          - Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --------------------------------------------------------------------------------------------------------------------
// unsigned int waitFor(timer, period) 
// Timer pour taches périodiques 
// configuration :
//  - MAX_WAIT_FOR_TIMER : nombre maximum de timers utilisés
// arguments :
//  - timer  : numéro de timer entre 0 et MAX_WAIT_FOR_TIMER-1
//  - period : période souhaitée
// retour :
//  - nombre de périodes écoulées depuis le dernier appel
// --------------------------------------------------------------------------------------------------------------------

#define MAX_WAIT_FOR_TIMER 3
unsigned int waitFor(int timer, unsigned long period){
  static unsigned long waitForTimer[MAX_WAIT_FOR_TIMER];  // il y a autant de timers que de tâches périodiques
  unsigned long newTime = micros() / period;              // numéro de la période modulo 2^32 
  int delta = newTime - waitForTimer[timer];              // delta entre la période courante et celle enregistrée
  if ( delta < 0 ) delta = 1 + newTime;                   // en cas de dépassement du nombre de périodes possibles sur 2^32 
  if ( delta ) waitForTimer[timer] = newTime;             // enregistrement du nouveau numéro de période
  return delta;
}

//--------- définition de la tache Led

struct Led_s {
  int timer;                                              // numéro du timer pour cette tâche utilisé par WaitFor
  unsigned long period;                                   // periode de clignotement
  int pin;                                                // numéro de la broche sur laquelle est la LED
  int etat;                                               // etat interne de la led
}; 

void setup_Led( struct Led_s * ctx, int timer, unsigned long period, byte pin) {
  ctx->timer = timer;
  ctx->period = period;
  ctx->pin = pin;
  ctx->etat = 0;
  pinMode(pin,OUTPUT);
  digitalWrite(pin, ctx->etat);
}

void loop_Led( struct Led_s * ctx) {
  if (!waitFor(ctx->timer, ctx->period)) return;          // sort s'il y a moins d'une période écoulée
  digitalWrite(ctx->pin,ctx->etat);                       // ecriture
  ctx->etat = 1 - ctx->etat;                              // changement d'état
}

//--------- definition de la tache Mess

struct Mess_s {
  int timer;                                              // numéro de timer utilisé par WaitFor
  unsigned long period;                                             // periode d'affichage
  char mess[20];
} ; 

void setup_Mess( struct Mess_s * ctx, int timer, unsigned long period, const char * mess) {
  ctx->timer = timer;
  ctx->period = period;
  strcpy(ctx->mess, mess);
  Serial.begin(9600);                                     // initialisation du débit de la liaison série
}

void loop_Mess(struct Mess_s *ctx) {
  if (!(waitFor(ctx->timer,ctx->period))) return;         // sort s'il y a moins d'une période écoulée
  Serial.println(ctx->mess);                              // affichage du message
}

//------------- definition de la tache Oled
struct Oled_s {
  int timer;                                              // numero de timer pour l'affichage sur l'écran oled
  unsigned long period;                                   // periode, 1 sec par 1 sec
  unsigned long counter;                                  // compteur de secondes
};

//initialisation de la structure oled, le contexte de l'affichage sur l'ecran
void setup_Oled(struct Oled_s *ctx, int timer, unsigned long period, unsigned long counter) {
  ctx->timer = timer;
  ctx->period = period;
  ctx->counter = counter;
  Wire.begin(4, 15);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("err display.begin");
    while(1); //erreur de l'init de l'écran, on ne peut rien afficher sur l'écran
  }
}

void loop_Oled(struct Oled_s *ctx) {
  if(!(waitFor(ctx->timer, ctx->period))) return; //au moins une periode non écoulée

  //sinon, on augmente le compteur et on l'affiche
  ctx->counter += 1;
  display.clearDisplay(); //nécessaire pour supprimer le contenu de la mémoire du buffer (sinon, a chaque écriture on augmente son contenu)
  display.setCursor(0,0); //positionner le curseur en haut à gauche
  display.setTextSize(3); //coefficient pour la taille du texte
  display.setTextColor(WHITE); //couleur, uniquement WHITE (macro) de dispo ?
  display.println("Count: "); //affichage du texte
  display.setTextSize(1);
  display.setCursor(23, 45); //changement de position
  display.print(ctx->counter, DEC); //affichage du compteur de seconde
  display.display(); // re-afficher l'écran, sinon il n'est pas mit à jour
}


//--------- Déclaration des tâches

struct Led_s Led1;
struct Mess_s Mess1;
struct Oled_s Oled1;

//--------- Setup et Loop

void setup() {
  setup_Led(&Led1, 0, 100000, LED_BUILTIN);               // Led est exécutée toutes les 100ms 
  setup_Mess(&Mess1, 1, 1000000, "bonjour");              // Mess est exécutée toutes les secondes
  setup_Oled(&Oled1, 2, 1000000, 0); //initialement, compteur à 0
}

void loop() {
  loop_Oled(&Oled1);
  loop_Led(&Led1);                                        
  loop_Mess(&Mess1); 
}
