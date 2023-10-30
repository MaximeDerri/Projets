CRIOC_TP3_normand_derri

###### tags: `IOC`

# TP3 : Pilotage d'un écran LCD en mode utilisateur et par un driver

## Auteurs
- Normand
- Derri

## Questions préliminaires
*Comment faut-il configurer les GPIOs pour les différents signaux de l'afficheur LCD ?* 
Il faut configurer les GPIO en sortie

*Comment écrire des valeurs vers le LCD ?* 
On envoie les données via nos GPIO aux registres cibles (RS, R/W, E, DB0-DB7)

*Quelles valeurs doivent être envoyées vers l'afficheur pour réaliser l'initialisation ?*
Il faut dans l'ordre:
    - Mettre l'écran en mode 4 bits
    - Définir le nombre de lignes
    - parametrer la police
    - Init le curseur
    - Effacer l'écran
    - Placer le curseur

*Comment demander l'affichage d'un caractère ?*
Il faut envoyer l'octet en deux fois (car mode 4 bits) puis donner la bonne commande

*Comment envoyer des commandes telles que : l'effacement de l'écran, le déplacement du curseur, etc. ?* 
Il faut écrire 0 dans le registre RS (signaler une commande) puis transmettre le code de l'opération sur le bus (toujours en deux fois) puis lever et descendre le flag E a chaque envoie.

## Question 1
*A quoi sert le mot clé volatile*

Le mot-clef `volatile` sert à empecher l'optimisation de variables en registres: on doit aller explicitement chercher la valeur dans l'espace d'adressage par l'intermédiare de load/stores.

## Question 2
*Expliquer la présence des drapeaux dans open() et mmap()*
    
- open()
    - O_RDWR Ouvre le fichier en lecture + écriture
    - O_SYNC Force les écritues synchrones: le processus sera suspendu le temps que l'ecriture soit effectuée
- mmap()
    - PROT_READ     autorise l'accès en lecture
    - PROT_WRITE    autorise l'acces en écriture
    - MAP_SHARED    permet l'utilisation du segment mémoire par les processus fils

## Question 3
*Pourquoi appeler munmap() ?*

Pour détacher le segment mémoire, évitant ainsi une fuite mémoire.

## Question 4
*Expliquer le rôle des masques : LCD_FUNCTIONSET, LCD_FS_4BITMODE, etc.*

- La macro `LCD_FUNCTIONSET` est un masque qui défini `functionset`. Cela implique une certaine interpretation des bits suivants tel que:
    - b4 = DL
    - b2 = F
    - b3 = N
Le ou logique avec les macros `LCD_FS_4BITMODE`, `LCD_FS_2LINE` et `LCD_FS_5x8DOTS` fait entrer l'ecran LCD en mode 4bits, deux lignes et 5*8 points par characteres.

- La macro `LCD_DISPLAYCONTROL` est un masque indiquant qu'on commande l'écran en lui-même:
    - `LCD_DC_DISPLAYON` allume l'ecran
    - `LCD_DC_CURSOROFF` désactive le curseur
- La macro `LCD_ENTRYMODESET` indique une commande concernant le mode d'entrée:
    - `LCD_EM_RIGHT` curseur se déplace vers la droite apres écriture
    - `LCD_EM_DISPLAYNOSHIFT` le texte ne bouge pas en cas de sortie de ligne

 ## Question 5
*Expliquez comment fonctionne cette fonction*

La fonction va écrire l'entrée dans le buffer de  l'écran 20 caractères par lignes puis sauter de ligne si il en reste. 


## Création du driver

* On a du augmenter le delay dans udelay(x) car les char ne s'affichaient pas tous

* On a ajouté LF (\n)

* On a ajouté CR (\r)




