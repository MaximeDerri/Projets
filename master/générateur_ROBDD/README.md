# Master 1: Projet d'algorithmique avancée - ROBDD

Membres:
* Yael Zarrouk
* Maxime Derri


## Installation et configuration

* Lancer `make install` à la racine du projet pour l'installation (si la bibliothèque BigInt demande une version, juste appuyer sur entrer)
* Lancer `make clean` à la racine du projet pour nettoyer
* Lancer la commande `make` à la racine du projet pour compiler
* Lancer la commande `make cleanup` à la racine du projet pour tout nettoyer (executable, bibliothèque, etc...)


### CONFIGURATION dans tree_code/src/Makefile:
* Les macro `N1` et `N2` définissent un interval sur le nombre d'arbres à construire pour l'extrapolation (f10 et f11)

* La macro NBTHREAD permet de régler le degré de parallélisme. si NBTHREAD > 1, alors le programme utilisera un pool de thread
  si plusieurs arbres sont à construire

* La macro HT_LIST_SIZE permet de limiter le nombre de buckets de la table de hachage, en supposant que dans le meilleur des cas il y a HT_LIST_SIZE nodes par buckets

* La macro KEEP_DOT permet supprimer les .dot si 0

* La macro SHOW_MODE provoque l'affichage dans le terminal si 0, sinon utilise python (graphiques)

* La macro PATH_GRAPH spécifie un répertoire où stocker les fichiers des graphes générés
  * /!\ ATTENTION: cette macro n'est pas initialisée car dépend des répertoires de l'utilisateur, il faut la configurer si besoin /!\
  * /!\ sinon, il faut executer le programme depuis la racine où est stocké l'executable afin de trouver le repertoire par defaut `./graphs/` /!\

* La macro AUTH_MULT autorise les doublons dans le choix des valeurs aléatoires si la macro vaut une valeur autre que 0 (pour f10 et f11)

* La macro INT_LENGTH permet de configurer la taille en nombre de caractères, des valeurs à générer

* La macro PY_PATH sert à retrouver l'interpreteur python pour fork/exec (par défaut: /usr/bin/python3). utiliser `which python3' permet d'obtenir le chemin.


* Il faut penser à bien délimiter les bornes MIN MAX pour ne pas boucler à l'infini (ou utiliser AUTH_MULT) si ce n'est pas la f9.

* Il faut adapter la macro INT_LENGTH à la situation, car la bibliothèque BigInt peut devenir plus lente si la taille des valeurs à générer (en caractère) est bien plus grande
  que le nombre de variables... Nous sommes certains que cela provient de la définition des opérateurs de la bibliothèque BigInt.




## Commandes

executable:
*   ./tree-analyzer

options:
*   -h         -> aide

*   -n         -> print a random BigInt (size defined in Makefile)

*   -e [-g] N V  -> lance l'etude de l'arbre unique representé par N sur V variables
                 -> affichage de l\'etude dans le terminal
                 -> -g pour tracer le graph

*   -E < -9 | -10 | -11 > V1-V2  -> lance l'etude complète pour entre V1 et V2 variables
                                 -> figure  9: tous les cas
                                 -> figure 10: extrapolation
                                 -> figure 11: statistiques




## Autre

Langages : C++ et python3

* Les temps en millisecondes ajoutés en secondes au temps total de calcul est arrondi à l'inférieur

* Pour tracer les graphes, il faut installer GraphViz.

* Les graphes sont générés dans le répertoir graphs

* Les fichiers des figures 11 générés sont sous format csv, avec comme délimiteur ';'
