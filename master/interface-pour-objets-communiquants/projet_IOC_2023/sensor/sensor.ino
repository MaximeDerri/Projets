#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include <WiFi.h>
//Adafruit MQTT library 2.5.0
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

//a remplire ici, car on suppose ne pas lire de fichier depuis l'esp
#define WIFI_SSID "X" //wifi
#define WIFI_PASSWD "X" //wifi

#define MQTT_USERNAME "ioc23" //mqtt
#define MQTT_DEVICE_NAME "esp32_1"
#define MQTT_PASSWD "helloworld" //mqtt
#define MQTT_ADDR "192.168.1.19" //mqtt
#define MQTT_PORT 1883 //mqtt

//wifi et mqtt declatation
WiFiClient wifi_client;
Adafruit_MQTT_Client mqtt_client(&wifi_client, MQTT_ADDR, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWD);

//le nom de cet esp32 est "esp32_1"
Adafruit_MQTT_Subscribe sub_setup = Adafruit_MQTT_Subscribe(&mqtt_client, "/esp32/esp32_1/setup", MQTT_QOS_0);
Adafruit_MQTT_Subscribe sub_screen = Adafruit_MQTT_Subscribe(&mqtt_client, "/esp32/esp32_1/screen", MQTT_QOS_0);
Adafruit_MQTT_Subscribe sub_led = Adafruit_MQTT_Subscribe(&mqtt_client, "/esp32/esp32_1/led", MQTT_QOS_0);

Adafruit_MQTT_Publish pub_init = Adafruit_MQTT_Publish(&mqtt_client, "/bdd/init", MQTT_QOS_0);
Adafruit_MQTT_Publish pub_save = Adafruit_MQTT_Publish(&mqtt_client, "/bdd/save", MQTT_QOS_0);
Adafruit_MQTT_publish pub_cnct = Adafruit_MQTT_Publish(&mqtt_client, "/server/cnct", MQTT_QOS_0);

bool is_init = false; //tant qu'on a pas ete init, on attend une reponse pour les valeurs

#define OLED_RESET    16 //Pin reset

//SSD1306 I2C declaration
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

#define MAX_WAIT_FOR_TIMER 5
unsigned int waitFor(int timer, unsigned long period){
  static unsigned long waitForTimer[MAX_WAIT_FOR_TIMER];  // il y a autant de timers que de tâches périodiques
  unsigned long newTime = micros() / period;              // numéro de la période modulo 2^32 
  int delta = newTime - waitForTimer[timer];              // delta entre la période courante et celle enregistrée
  if ( delta < 0 ) delta = 1 + newTime;                   // en cas de dépassement du nombre de périodes possibles sur 2^32 
  if ( delta ) waitForTimer[timer] = newTime;             // enregistrement du nouveau numéro de période
  return delta;
}

//---------------------------------------

enum {EMPTY, FULL}; //etat de la lecture pour ctl_box

struct ctl_box { //stockage
  volatile int value;
  volatile int state;  
};

struct ctl_box_msg { //stockage
  String str;
  volatile int state;
};

struct led_s { //led
  unsigned long period;
  int timer;
  int pin;
  int state; //etat de la led
};

struct photores_s { //photoresistance
  unsigned long period;
  int timer;
  int pin;
  int value; //valeur en %
};

struct button_s { //bouton poussoir
  unsigned period;
  int timer;
  int pin;
  int count; //compteur d'appuis
};

struct oled_s { //ecran
  unsigned int period;
  int timer;
  String str; //chaine a afficher sur l'ecran
};

struct mqtt_s {
  unsigned int period;
  int timer;
};

//---------------------------------------

struct led_s led; //led
struct photores_s photores; //photoresistance
struct button_s button; //BP
struct oled_s oled; //ecran
struct mqtt_s save; //sauvegarde des resultats

struct ctl_box led_box; //comm inter tache
struct ctl_box_msg msg_buff; //msg buffer

//---------------------------------------

void setup_led(struct led_s *led, int timer, unsigned long period, byte pin) {
  led->period = period;
  led->timer = timer;
  led->pin = pin;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, led->state);
}

void loop_led(struct led_s *led, struct ctl_box *cb) {
  if (!waitFor(led->timer, led->period))
    return; //pas encore le moment

  if (cb->state == FULL) { //demande de changement d'etat de la led
    led->state = cb->value % 2; //par securitee
    cb->state = EMPTY;
    digitalWrite(led->pin, led->state); //changer l'etat de la led
  }
}

//---------------------------------------

void setup_photores(struct photores_s *pres, int timer, unsigned long period, byte pin) {
  pres->period = period;
  pres->timer = timer;
  pres->pin = pin;
  pinMode(pin, INPUT);
}

void loop_photores(struct photores_s *pres) {
  int tmp;

  if (!waitFor(pres->timer, pres->period))
    return; //pas encore le moment

  analogReadResolution(12); //changer pour 12 bits, pour l'esp32
  tmp = analogRead(pres->pin);
  pres->value = (100 - map(tmp, 0, 4095, 0, 100)); //changer l'echelle ~ %
}

//---------------------------------------

void setup_button(struct button_s *but, int timer, unsigned long period, byte pin) {
  but->period = period;
  but->timer = timer;
  but->pin = pin;
  pinMode(pin, INPUT_PULLUP);
}

//loop_button est une ISR

//---------------------------------------

void setup_oled(struct oled_s *oled, int timer, unsigned long period, struct ctl_box_msg *msg) {
  oled->period = period;
  oled->timer = timer;

  oled->str = msg->str;

  Wire.begin(4, 15);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { //initialiser l'affichage sur l'ecran
    Serial.println("error diplay.begin in setup_oled()");
    while (1); //erreur, on loop...
  }

  display.clearDisplay(); //nettoyage
}

void loop_oled(struct oled_s *oled, struct ctl_box_msg *msg) {
  if (!waitFor(oled->timer, oled->period))
    return; //pas encore le moment

  //changer le texte sur l'ecran SI il y a du nouveau
  if (msg->state == FULL) {
    oled->str = msg->str;
    msg->state = EMPTY;
    display.clearDisplay();
    display.setCursor(5,5);
    display.setTextSize(1);
    display.setTextColor(WHITE); 
    display.print(msg->str);
    display.display();
  }

}

//---------------------------------------

//decouper la chaine recu du script python pour initialiser les structures de l'esp.
//type de chaine attendue:
//String("led:1#pres:99#counter:32#screen:HelloWorld!");
void parse_init(String *init, int *state, int *counter, int *value) {
  String tmp;
  int idx = 0, idx2, idx_in;

  while (idx2 > 0) {
    idx2 = init->indexOf('#', idx); //retourne -1 si char pas trouve
    tmp = init->substring(idx, idx2);
    
    idx_in = tmp.indexOf(':',0);

    if (idx_in < 0)
      goto loop_init;

    if (tmp.substring(0,idx_in) == "led") {
      *state = tmp.substring(idx_in+1).toInt();
    }
    else if (tmp.substring(0,idx_in) == "pres") {
      *value = tmp.substring(idx_in+1).toInt();
    }
    else if (tmp.substring(0,idx_in) == "counter") {
      *counter = tmp.substring(idx_in+1).toInt();
    }
    else if (tmp.substring(0,idx_in) == "screen") {
      msg_buff.str = tmp.substring(idx_in+1);
      msg_buff.state = FULL;
    }
    //else, on continue

loop_init:
    idx = idx2+1;
  }
}

void setup_mqtt(struct mqtt_s *m, unsigned int period, int timer) {
  m->period = period;
  m->timer = timer;
}

//envoyer des donnees pour la bdd
void mqtt_save(struct mqtt_s *m, struct led_s *led, struct photores_s *pres, struct button_s *but) {
  String str;
  if (!waitFor(m->timer, m->period))
    return; //pas encore le moment

  str = String(MQTT_DEVICE_NAME) + "#led:" + String(led->state) + "#counter:" + String(but->count) + "#pres:" + String(pres->value);
  pub_save.publish(str.c_str());
}

//recevoir l'init du client bdd
//callback
void mqtt_init(char *data, uint16_t len) {
  String str = String(data);
  int state = 0;
  int count = 0;
  int value = 0;

  parse_init(&str, &state, &count, &value);
  led_box.state = FULL;

  led.state = state;
  photores.value = value;
  button.count = count;
  is_init = true;
}

//recevoir les chaines pour l'ecran
//callback
void mqtt_command_screen(char *data, uint16_t len) {
  msg_buff.str = String(data);
  msg_buff.state = FULL;
}

//recevoir le changement d'etat de la led
//callback
void mqtt_command_led(char *data, uint16_t len) {
  led_box.value = String(data).toInt() % 2;
  led_box.state = FULL;
}

//permet de lancer la connexion, ou la relancer au besoin
void mqtt_connect() {
  if (mqtt_client.connected())
    return; //deja connecte

  //sinon
  while (mqtt_client.connect() != 0) {
    mqtt_client.disconnect();
    delay(500);    
  }
}

//---------------------------------------

//ISR
void isr_led_ctl(void) { //ISR pour le bouton blanc
  if (led_box.state == EMPTY) {
    led_box.value = (1 - led_box.value);
    led_box.state = FULL;
  }
}

void isr_button_count(void){ //ISR pour le compteur du bouton poussoir
  ++(button.count);
}

//---------------------------------------

void setup() {
  Serial.begin(9600);
  led_box.state = EMPTY;
  msg_buff.state = EMPTY;

  //connexion wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  //preparer les callbacks
  sub_setup.setCallback(mqtt_init);
  sub_screen.setCallback(mqtt_command_screen);
  sub_led.setCallback(mqtt_command_led);

  mqtt_client.subscribe(&sub_setup);
  mqtt_client.subscribe(&sub_screen);
  mqtt_client.subscribe(&sub_led);

  mqtt_connect();

  //on demande l'initialisation des valeurs de nos struct selon la bdd
  pub_init.publish(MQTT_DEVICE_NAME);

  //on attend que le script python de la bdd ai répondu
  while (!is_init) {
    mqtt_connect();
    mqtt_client.processPackets(1000);
  }

  //loop tant que pas init

  setup_led(&led, 0, 100000, LED_BUILTIN);   //setup led
  setup_photores(&photores, 1, 100000, 36); //photores
  setup_oled(&oled, 2, 100000, &msg_buff); //ecran
  setup_mqtt(&save, 10000000, 3); //envoie des donnees -> interval

  //ISR
  pinMode(0, INPUT_PULLUP);
  pinMode(23, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(0), isr_led_ctl, RISING); //bouton blanc pour controler la led
  attachInterrupt(digitalPinToInterrupt(23), isr_button_count, RISING); //compter les appuis du bouton poussoir

  pub_cnct.publish(MQTT_DEVICE_NAME)
}

void loop() {
  loop_led(&led, &led_box);
  loop_photores(&photores);
  loop_oled(&oled, &msg_buff);

  //si pas de reponse au ping, on "deconnecte" pour forcer une reconnexion
  if (!mqtt_client.ping())
    mqtt_client.disconnect();

  mqtt_connect(); //si deja connecté, ne fait rien - permet de garder une connexion


  mqtt_save(&save, &led, &photores, &button);
}
