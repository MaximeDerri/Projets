Test : test_lecture_ecriture
But : démontrer la cohérence entre la lecture et l'écriture.
Il peut arriver qu'un client souhaite écrire sur une page tandis que d'autres clients veulent lire. Ce test vise à montrer la cohérence entre la lecture et l'écriture sur une page.

Ce test comprend 1 fichier : main.c, qui crée 5 processus, dont 1 qui simule le serveur et 4 qui simulent des clients.
Trois clients cherchent à lire la page et un client cherche à écrire dans la page.

Pour lancer le test, exécutez la commande 'make' pour créer un exécutable main.