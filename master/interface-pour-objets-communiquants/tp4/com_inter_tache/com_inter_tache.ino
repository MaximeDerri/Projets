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

#define MAX_WAIT_FOR_TIMER 4
unsigned int waitFor(int timer, unsigned long period){
  static unsigned long waitForTimer[MAX_WAIT_FOR_TIMER];  // il y a autant de timers que de tâches périodiques
  unsigned long newTime = micros() / period;              // numéro de la période modulo 2^32 
  int delta = newTime - waitForTimer[timer];              // delta entre la période courante et celle enregistrée
  if ( delta < 0 ) delta = 1 + newTime;                   // en cas de dépassement du nombre de périodes possibles sur 2^32 
  if ( delta ) waitForTimer[timer] = newTime;             // enregistrement du nouveau numéro de période
  return delta;
}

//--------- définition de la boite a lettre

enum {EMPTY, FULL}; //etat de la boite

struct mailbox_s {
  volatile int state; //etat
  volatile int value; //valeur
};

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

void loop_Led( struct Led_s * ctx, struct mailbox_s *com, struct mailbox_s *led_off) {
  if(com->state == FULL) { //modifier la période
    ctx->period = 100000 + (10000 * map(com->value, 0, 4095, 0, 100)); //ou mettre à 1000 * map(...) comme ça, si on a 100% alors on retrouve la periode initiale
    com->state = EMPTY;
  }

  if (!waitFor(ctx->timer, ctx->period)) return;          // sort s'il y a moins d'une période écoulée
  if(led_off->value)  {
    digitalWrite(ctx->pin,ctx->etat);                       // ecriture
    ctx->etat = 1 - ctx->etat;                              // changement d'état
  }
  else {
    digitalWrite(ctx->pin, 0);
    ctx->etat = 1 - ctx->etat;
  }
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
  int value;                                              // dernière valeur obtenue de la boite a lettre
};

//initialisation de la structure oled, le contexte de l'affichage sur l'ecran
void setup_Oled(struct Oled_s *ctx, int timer, unsigned long period, unsigned long counter, int value) {
  ctx->timer = timer;
  ctx->period = period;
  ctx->counter = counter;
  ctx->value = value;
  Wire.begin(4, 15);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("err display.begin");
    while(1); //erreur de l'init de l'écran, on ne peut rien afficher sur l'écran
  }
}

void loop_Oled(struct Oled_s *ctx, struct mailbox_s *com) {
  if(!waitFor(ctx->timer, ctx->period)) return; //au moins une periode non écoulée

  //sinon, on augmente le compteur et on l'affiche
  ctx->counter += 1;

  display.clearDisplay(); //nécessaire pour supprimer le contenu de la mémoire du buffer (sinon, a chaque écriture on augmente son contenu)

  //affichage pour le compteur
  display.setCursor(0,0); //positionner le curseur en haut à gauche
  display.setTextSize(2); //coefficient pour la taille du texte
  display.setTextColor(WHITE); //couleur, uniquement WHITE (macro) de dispo ?
  display.println("Count: "); //affichage du texte
  display.setTextSize(1);
  display.setCursor(23, 20); //changement de position
  display.print(ctx->counter, DEC); //affichage du compteur de seconde

  //photorésistance
  if(com->state == FULL) {
    ctx->value = map(com->value, 0, 4095, 0, 100); // 3.3v -> 12 bits -> 4095 ?  
    com->state = EMPTY;
  }
  
  if(ctx->value >= 0) {
    display.setCursor(0, SCREEN_HEIGHT / 2);
    display.println("---------------------"); //séparateur de '-' au milieu de l'écran
    
    display.setCursor(0, SCREEN_HEIGHT / 2 + 10);
    display.println("photores:"); //texte

    //affichage de la valeur et du pourcentage    
    display.setCursor(23, SCREEN_HEIGHT / 2 + 20);
    display.print(ctx->value, DEC);
    display.setCursor(100, SCREEN_HEIGHT / 2 + 20);
    display.println("%");
  }
  display.display(); // re-afficher l'écran, sinon il n'est pas mit à jour
}

// -------- declaration de la tache lum (photorésistance)

struct Lum_s {
  int timer;
  unsigned long period;
  int pin;
};

void setup_Lum(struct Lum_s *ctx, int timer, unsigned long period, byte pin) {
  ctx->timer = timer;
  ctx->period = period;
  ctx->pin = pin;
  pinMode(pin, INPUT);
}

void loop_Lum(struct Lum_s *ctx, struct mailbox_s *com, struct mailbox_s *com2) {
  if(!waitFor(ctx->timer, ctx->period)) return; //pas mon tour

  analogReadResolution(12); //passage sous 12 bits pour le 3.3v

  if(com->state == EMPTY) { //pas encore traité (oled)
    com->value = analogRead(ctx->pin);
    com->state = FULL;
  }
  if(com2->state == EMPTY) { //pas encore traité (led)
    com2->value = analogRead(ctx->pin);
    com2->state = FULL;
  }

}

//--------- Déclaration des tâches

struct Led_s Led1;
struct Mess_s Mess1;
struct Oled_s Oled1;
struct Lum_s Lum1;
struct mailbox_s mb = {.state = EMPTY}; //initialisation en dehors d'une fonction setup_mailbox()
struct mailbox_s mb2 = {.state = EMPTY}; //pour led

//interruption
struct mailbox_s isr = {.state = EMPTY, .value=1}; //1: allumer , 0: eteindre
volatile int is_char_detected = 0;

//ISR pour le clavier
void isr_keyboard() {
  Serial.println("isr");
  if(is_char_detected)
    isr.value = 0; //stopper la led    
}

void serialEvent() {
  while(Serial.available() && !is_char_detected) {
    if(Serial.read() == 's') {
      is_char_detected = 1;
      Serial.println("serialEvent 1");
      interrupts();
      break;
    }
  }
}

//--------- Setup et Loop

void setup() {
  Serial.begin(9600);                                     // initialisation du débit de la liaison série
  setup_Led(&Led1, 0, 100000, LED_BUILTIN);               // Led est exécutée toutes les 100ms 
  setup_Mess(&Mess1, 1, 1000000, "bonjour");              // Mess est exécutée toutes les secondes
  setup_Oled(&Oled1, 2, 1000000, 0, -1);                  //initialement, compteur à 0
  setup_Lum(&Lum1, 3, 500000, 36);                        //Lum exec toutes les 0.5 sec
  //d'après l'exemple du sujet de TP, on initialise la boite a lettre à sa déclaration
  
  //gestion des interruptions
  pinMode(0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(0), isr_keyboard, RISING);    
}

void loop() {
  loop_Lum(&Lum1, &mb, &mb2);
  loop_Oled(&Oled1, &mb);
  loop_Led(&Led1, &mb2, &isr);                                      
  loop_Mess(&Mess1);
  serialEvent(); //lire le port serie
}
