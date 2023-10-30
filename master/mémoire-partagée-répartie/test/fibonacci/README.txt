Test sur la suite de Fibonacci.
But : montrer que l'accès à la page se fait de manière concurrente.
Les pages sont protégées par un système de mutex, ce qui permet d'avoir au plus 1 accès à la page simultanément.

Le test comprend un fichier : fibonacci.c qui crée 6 processus, dont 1 simulateur serveur et 5 simulateurs clients.
Les 5 clients cherchent à accéder à la mémoire partagée et calculent la suite de Fibonacci.

Pour lancer le test, exécutez la commande 'make' pour créer un exécutable.