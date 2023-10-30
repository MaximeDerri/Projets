# Licence 3: Projet programmation réseau - GhostLab

## Informations

| **Nom**  | **Prénom** |
| -------- | ---------- |
| DERRI    | Maxime     |
| VAIN     | Alexandre  |

## Utilisation :

#### Compilation du client

```
make
```

#### Execution du client

```
./client id tcp_port udp_port
```
avec `id` l'identifiant du joueur `tcp_port` pour la connection au serveur et `udp_port` pour le joueur.

```
./client id udp_port
```
avec `id` l'identifiant du joueur, `udp_port` pour le joueur et tcp_port est préparé avec une macro (on à défini le port `4501` comme port par défaut).

#### Execution du serveur

```
./server tcp_port
```
lance le serveur sur le port `tcp_port`

```
./server
```
lance le serveur sur le port de défaut (on à défini le port `4501` comme port par défaut).

## Partie du client

#### 1) Paramétrer le client
Si le client est utilisé en `LOCAL`,il faut aller dans le fichier
`src/client/include/game_infos.h` et mettre la macro `LOCALHOST à 1`.

Si le client est utilisé en `salle de TP`,il faut aller dans le fichier
`src/client/include/game_infos.h` et mettre la macro `LOCALHOST à 0`.

LOCALHOST == 1:
* connection tcp sur localhost.
* SO_REUSEPORT pour le multicast.

LOCALHOST == 0:
* connection tcp sur lulu.
* SO_REUSEADDR pour le multicast.


#### 3) Affichage
L'interface graphique n'étant visiblement pas comprise dans la notation, le projet n'en possède donc pas.

Cependant, pour que l'affichage reste lisible, nous utilisons des couleurs dans le terminal.

Aussi, au début du jeu (quand la partie commence, après START), un `xterm` est lancé avec fork() et execlp():
* des appels système sont utilisés dans game_infos.c pour préparer le tube nommé `/tmp/ghostlab/textual-channel`, lancé avec xterm en lecture.
* un thread va s'occuper de réceptionner les messages udp / mc avec select(), puis d'autres fonctions vont vérifier les données et les écrire ou non dans le tube nommé (pour afficher dans le xterm).


#### 4) Interractions utilisateur - client
Une liste des commandes se trouve tout en haut du fichier `src/client/src/command.c`.
Il est possible d'écrire `help` (ou quelque chose de faux) pour faire écrire l'aide dans le terminal.


## Extensions :
(ici, '_' fait référence à un espace).

#### 1) Connaitre les joueurs qui ont fait START ou non
    - Le client envoie `START?_m***` pour demander au serveur.
    - Le serveur peut répondre:
        - `STAR!_m_n***` suivi de n requêtes `STARP_id_t***`.
        - `DUNNO***` si la partie m n'existe pas.
    - Valeurs:
        - m est codé sur un octet et correspond au numéro de partie.
        - n est codé sur un octet et correspond au nombre de requêtes / joueurs.
        - t est codé sur un octet et précise si le joueur est prêt (t > 0) OU non prêt (t == 0).
        - id est l'identifiant du joueur, comme dans le sujet.


Au lancement d'une partie, 2 objets sont donnés aux joueurs. Chaque joueur reçoit:
* Une bombe, qui détruit les cases autour du joueur. La bombe explose autour du joueur sur 3*3 cases. Le joueur est au centre et les contours du labyrinthe ne sont pas atteints.
* Un radar, qui donne la vision d'un carré autour du joueur. Le radar affiche autour du joueur tout le labyrinthe sur une distance maximal de 255*255. Un petit labyrinthe sera donc complètement affiché.
    

#### 2) Faire exploser sa bombe
    - Le client envoie `EXPL?***` pour essayer de faire explosere la bombe.
    - Le serveur peut réponde:
        - `EXPL!***` si effectué.
        - `EXPLN***` si impossible / non effectué.
        - `GOBYE***` si la partie est fini.
    - Si la bombe explose, le serveur multi-diffuse `BOMB!_id_x_y+++`.
    - Valeurs:
        - id est l'identifiant du joueur, comme dans le sujet.
        - x et y sont codés sur 3 octets, comme dans le sujet.

#### 3) Utiliser son radar
    - Le client envoie `RADA?***` pour essayer de l'utiliser.
    - Le serveur peut réponde:
        - `RADA!_n_y***` suivi de n requêtes `CASES_c***`.
        - `RADAN***` si impossible / non effectué.
        - `GOBYE***` si la partie est fini.
    - Valeurs:
        - n est codé sur un octet et correspond au nombre de requêtes à suivre.
        - y est codé sur un octet et précise la taille de c.
        - c est codé sur y octets, et correspond à une partie de ligne dans le labyrinthe.
    - L'affichage produit est de n ligne de y colonnes:
        - `P` correspond à la position d'un joueur (lanceur et les autres).
        - `G` correspond à la position d'un fantôme.
        - `#` correspond à la position d'un murs.
        - ` ` correspond à un chemin.
