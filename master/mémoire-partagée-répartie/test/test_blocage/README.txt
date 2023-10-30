Test : test_blocage
But : s'assurer que l'accès en lecture ne provoque pas d'interblocage.
Ce test vise à résoudre un problème d'interblocage lié à l'accès en lecture.

Le test comprend 2 fichiers : maitre.c et slave.c. Le fichier maitre simule un maître, tandis que le fichier slave simule 3 esclaves.
Les esclaves accèdent à la mémoire partagée puis se terminent.

Pour lancer le test, exécutez la commande 'make' qui créera 2 exécutables : maitre et slave. Lancez d'abord l'exécutable 'maitre', puis lancez l'exécutable 'slave'.