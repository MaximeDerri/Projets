Test : diff_page_acces
But : montrer que les différentes pages peuvent être accédées de manière parallèle.
Toutes les pages sont protégées par des mutex, mais si on accède à des pages différentes, l'accès se fait de manière parallèle.

Le test comprend 2 fichiers : master.c et slaves.c. Le fichier master simule un serveur, tandis que le fichier slaves simule 3 clients.
Le serveur initialise un tableau de 3 pages, et les 3 clients cherchent à accéder aux 3 pages différentes.

Pour lancer le test, exécutez la commande 'make' pour créer les 2 exécutables. Lancez d'abord l'exécutable 'master', puis lancez l'exécutable 'slaves'.