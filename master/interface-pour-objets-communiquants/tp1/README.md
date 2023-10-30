ioc23_1_normand_derri

# TP1 - Premiers pas sur la plateforme

## Auteurs
François Normand
Maxime Derri

## Question II.1
*Pourquoi passer par la redirection des ports ?*

Le Raspberry Pi est connecté dans un sous réseau (192.168.1.1) accessible uniquement au travers du routeur "Peri".
Le routeur redirige les requêtes vers le sous réseau (selon le port 62200 + X), aux adresses 192.168.1.X port 22 (SSH).

## Question II.2
*Pourquoi placer nos fichiers dans répertoire propre sur une RaspberryPi*

Les cartes, que nous devons partarger, ne disposent que d'un compte utilisateur. On doit donc éviter de malanger nos fichiers avec l'autre binôme...

## Question V.1
*pourquoi pourrait-il être dangereux de se tromper de broche GPIO ?*

imaginons qu'on se trompe de broche et que le composant soit positionné pour un input, si on lui envoie du courrant, cela sera dangereux pour le composant.


## Question V.2
*A quoi correspond `BCM2835_GPIO_BASE` ?*

Il s'aggit de l'adresse de base de l'espace d'adressage physique du processeur ??

## Question V.3
*Que représente `struct gpio_s`?*

Il s'agit d'une abstraction des adresses physiques des registres des broches GPIO mappées dans l'espace d'adressage virtuel du processus. Cela nous permet de  peut les manipuler comme des variables.

## Question V.4
*Dans quel espace d'adressage est mappé `gpio_regs_virt`?*

`gpio_regs_virt`est mappé dans l'espace d'adressage du processus (virtuel).

## Question V.5
*Dans la fonction `gpio_fsel()`, que contient la variable reg ?*

Reg va determiner l'indice du mot 32 bits dans le tableau `gpfsel` mappant le registre 3 bits de notre pin.

## Question V.6
*Dans la fonction `gpio_write()`, pourquoi écrire à deux adresses différentes en fonction de la valeur val ?*

Si le param `val`est à un: on envoie une commande pour mettre sous tension la broche
Sinon, on demande de mettre hors tension la broche.

## Question V.7
*Dans la fonction `gpio_mmap()`, à quoi correspondent les flags de open() ?*

 - O_RDWR  : ouverture en lecture/écriture
 - O_SYNCH : ecriture synchrone. Le procesuss sera bloqué tant que la donnée a ecrire ne sera pas écrite physiquement.

## Question V.8
*Dans la fonction `gpio_mmap()`, commentez les arguments de `map()`*

 - premier parametre: NULL -> mmap choisiera lui même l'adresse de base du segment dans l'espace d'adressage du processus
 - second parametre: RPI_BLOCK_SIZE -> taille de la zone mémoire à mapper.
 - troisième parametre: PROT_READ | PROT_WRITE -> protection appliquées à toutes les pages du segment
 -  quatrième paramètre: MAP_SHARED: on peut partager le segment avec les autres processus .
 -  cinquième param: map_fd -> on map la zone mémoire contenant les adresses de nos GPIOs
 -  sixième param: BCM2835_GPIO_BASE ->adresse a partir on commence à mapper le fd.

## Question V.9
*Que fait la fonction delay() ?*

`Delay()` Endors un processus pour quelques nanosecondes.

## Question V.10
*Pourquoi doit-on utiliser sudo ?*

Certaines instructions de nôtre programme requièrent les droits de super utilisateur tel qu'acceder et mapper /dev/mem






