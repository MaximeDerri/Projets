# Licence 3: Projet de programmation orienté objet - Fractales

Logiciel pour générer les images de fractales.

## Informations

| **Nom**  | **Prénom** |
| -------- | ---------- |
| DERRI    | Maxime     |
| VAIN     | Alexandre  |


## Compiler le projet et lancer le programme

La racine contient un Makefile, mais nous vous recommandons d'utiliser plutôt nos scripts.
* `make` compile et lance le programme.

* `make examples` compile et lance le programme avec les exemples à dessiner.

* `make clean` supprime le répertoir `build` qui contient les fichiers .class


scripts:
* Pour lancer la compilation, utiliser `./compile`.

* Pour lancer le programme, utiliser `./launch`.
Sans arguments, l'interface graphique est utilisée.

Sinon, voici le schéma pour utiliser le programme en ligne de commande:
./launch -{set} values#{x1}:{x2}:{y1}:{y2}:{zoom}:{iter}:{step}:{pow}:{real_component_of_C}:{imaginary_component_of_C}
set est à remplacer par `julia` ou `mandelbrot`.
Exemple: `./launch -julia values#-1.0:1.0:-1.0:1.0:400:1000:0.01:2:0.234:0.1`.

NOTE:  1 <= (zoom * step) <= 10 sinon le fractal ne sera pas réalisé.
Pour plus d'informations sur les contraintes, vous pouvez les retrouver ligne 196 dans `Fractal.java`.

Pour plus d'information sur le mode console, utilisez `./launch -help`.

* Pour lancer le programme et dessiner les exemples de fractales, vous pouvez utiliser `./run-examples`.
Les fractales seront alors dans le répertoire `fractals`.
