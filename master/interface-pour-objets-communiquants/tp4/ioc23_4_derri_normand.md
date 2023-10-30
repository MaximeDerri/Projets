###### tags: `IOC`

# TP4 Programmation ESP32 / Arduino 

## Auteurs:
- Maxime Derri
- François Normand


## Question 1
*Les fonctions loop_Tache() et setup_Tache() peuvent avoir des variables locales mais leur état n'est pas conservé entre deux instances d'exécutions. Elles peuvent aussi avoir des variables static mais ces variables ont une valeur unique même si la tâche est en plusieurs exemplaires et que donc elle utilise la même fonction loop_Tache(). Les variables static ne sont pas conseillées parce qu'elles ne peuvent être initialisées dans la fonction setup_Tache(). Expliquez pourquoi dans le CR pour vérifier que vous avez compris.*

> Il n'est pas efficace d'utiliser static car:
> - La variable en question ne peut être définie dans setup_tache(), sinon elle y serait liée
> - L'utilisation d'une variable static dans une loop_tache() impliquerai que cette fonction seul utilise cette variable, et donc il faudrait autant de fonction que de tache.

## Question 2
*Que contient le tableau waitForTimer[] et à quoi sert-il ?*

> Le tableau waitForTimer[] contient les différents timer des taches (initialisé static, d'une taille MAX_WAIT_FOR_TIMER qui définie le nombre de tache).
l'argument timer sert d'identifiant dans le tableau afin de récupérer le timer qui doit être utilisé dans la fonction waitFor().

## Question 3
*Si on a deux tâches indépendantes avec la même période, pourquoi ne peut-on pas utiliser le même timer dans waitFor() ?*

> Il est nécessaire d'avoir un timer par tache, car:
> - on vérifie si le temps stocké + la période = le temps actuel, et si oui on obtient un entier positif
> - à chaque appel, le temps est retourné et mis à jour. Si deux taches appelent succesivement la fonction, l'une recevra un signal (entier) attendue, et l'autre non, et il risque donc de ne pas s'exécuter.

## Question 4
*Dans quel cas la fonction waitFor() peut rendre 2 ?*

> Une tache 2 peut obtenir 2 de waitFor(), si la tache 1 qui s'exécute avant à pris plus de temps que prévue, et s'étale sur la période de tache 2 (on a donc plus qu'une période d'écoulée).

## notes

> Il semble y avoir un problème avec le fichier ssd1306_128x64_i2c, et il faut utiliser le modèle 128x32 en changeant:
> - Wire.begin(4, 15); au début de setup()
> - changer la macro OLED_RESET et mettre la valeur 16
> - changer la macro SCREEN_HEIGHT à 64 (étrange...)
> 
# Utilisation de l'écran OLED

## Question 5
*Extraire de ce code, ce qui est nécessaire pour juste afficher un compteur qui s'incrémente toutes les 1 seconde sur l'écran OLED. Vous devez ajouter une tâche nommée oled dans votre programme en conservant celles déjà dans votre sketch (programme Arduino). L'idée, c'est d'avoir plein de tâches ensemble.*

> Voir ecran_oled1.ino

# Communication inter-tâches

## Question 6
*Dans le texte précédent, quel est le nom de la boîte à lettre et comment est-elle initialisée ?*

> enum {EMPTY, FULL};

> struct mailbox_s {
  int state;
  int val;
};

> struct mailbox_s mb = {.state = EMPTY};

> La boîte à lettre est une structure mailbox_s.
Cette structure est initialisée à sa déclaration, avec l'état EMPTY (issue d'une énumération).

## Question 7
*Ajouter une tâche nommée lum qui lit toutes les 0,5 seconde le port analogique [...] (par analogRead()) sur lequel se trouve la photo-résistance et qui sort sa valeur dans une boite à lettre. Cette boite à lettre sera connectée à la tâche oled. Vous afficher la valeur en pourcentage de 0% à 100% en utilisant la fonction map()*

> voir com_inter_tache.ino

## Question 8
*Mofifier la tâche Led pour que la fréquence de clignotement soit inversement proportionnel à la lumière reçue (moins il y a de lumière plus elle clignote vite). La tâche Led devra donc se brancher avec la tâche lum avec une nouvelle boite à lettre. Il doit y avoir deux boites sortant de lum, l'une vers oled l'autre vers led.*

> voir com_inter_tache.ino

# Gestion des interrutions

## Question 9
*Ajouter une tâche qui arrête le clignotement de la LED si vous recevez un s depuis le clavier. Vous devez ajouter une tâche ISR, et la connecter à la tâche led par une nouvelle boîte à lettre.*

> Nous n'avons pas réussi dans ISR a capturer un evenement du clavier.
> Dans loop(), une fonction va vérifier si il y a des caractères a lire, et si elle en trouve alors on set à 1 un boolean. Quand une interruption intervient (le bouton blanc sur l'ESP), l'ISR va vérifier le boolean et provoquer l'extinction de la led par la boite a lettre

## Question 10
*Représenter le graphe de tâches final sur un dessin en utilisant le langage de ​graphviz (regarder ​ce graphe bi-parti. C'est un graphe biparti avec des ronds pour les tâches et des rectangles pour les boites à lettres.*
>digraph GrapheTaches {
    fontname="Helvetica,Arial,sans-serif"
    node [fontname="Helvetica,Arial,sans-serif"]
    edge [fontname="Helvetica,Arial,sans-serif"]
    node [shape=box];  mb; mb2; isr;
    node [shape=circle,fixedsize=true,width=0.9];  ArduinoIde; Led; Mess; Oled; Lum; ISR; Loop;
    ArduinoIde -> Loop;
    Loop -> Led;
    Loop -> Mess;
    Loop -> Oled;
    Loop -> Lum;
    ArduinoIde -> ISR;
    ISR -> isr;
    isr -> Led;
    Lum -> mb
    Lum-> mb2
    mb -> Oled
    mb2 -> Led
    Mess -> ArduinoIde
    overlap=false
    fontsize=12;
}
(Le rendu est dans les fichiers de l'archive)
