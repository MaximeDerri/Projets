Test : page_concurrence
But : montrer que l'accès à la page se fait de manière concurrente.
Chaque page est protégée par un mutex. Si un processus A accède à la page 1, puis un processus B accède
aux pages 1 et 2,alors lorsque la page 1 est détenue par le processus A, le processus B doit attendre que
le processus A ait terminé son accès à la page 1 avant de pouvoir obtenir les pages 1 et 2.

Le test comprend 2 fichiers : master.c et slaves.c. Le fichier master simule un serveur, tandis que le fichier
slaves simule 2 clients.
Le maître initialise un tableau de 3 pages, l'esclave A accède à la page 1, et l'esclave B accède aux
pages 1, 2 et 3.

Pour lancer le test, exécutez la commande 'make' pour créer les 2 exécutables : 'master' et 'slaves'.
Lancez d'abord l'exécutable 'master', puis lancez l'exécutable 'slaves'.