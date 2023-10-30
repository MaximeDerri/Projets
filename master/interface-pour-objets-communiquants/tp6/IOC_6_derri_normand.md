TP6: Modèle client-serveur

###### tags: `IOC`

# TP6: Modèle client-serveur

# NOTE: 
> Nous avons déjà vue les sockets dans l'UE PSCR.

## Question 1
*commentez l’exemple dans le code en trouvant la documentation.*
> voir le code commenté

## Question 2
*Créez une nouvelle application permettant de recueillir le vote de personne concernant le choix de l’heure d’hiver ou d’été. Vous avez deux programmes à écrire. Le premier permet de voter, c’est le client. Le second permet de recueillir le vote, c’est le serveur.*
> On utilise un .h qui contient l'interface pour le protocol, ainsi que des macro pour les tailles des données / requetes.
> Pour stopper le serveur, on utilise plutôt une variable global "run" qu'on met à 1.
Puis, on modifie le signal SIGINT pour placer 0 dans cette variable.
le while qui tourne sur le serveur pour accepter / traiter les requetes tourne sur cette variable
afin de terminer plus proprement.
Nous avons également utilisé poll afin de ne pas bloquer éternellement en attendant une requete, afin
de reboucler et verifier la variable "run" (continuer ou non).

> On aurait également pu implémenter le serveur comme suit:
> * le while accepte les requetes et créer un thread par tache à traiter
> * implémentation de mutex pour garentir la cohérence des données
> * On pourrait éventuellement se souvenir des participants pour ne pas prendre en compte leurs votes plusieurs fois, il suffirait de déclarer un tableau de tableau ([][]) et d'allouer chaque cases avec
malloc.
> * Une autre amélioration des performances (vis à vis du stockage des participants ci dessus et du parcours de la structure) serait d'utiliser un arbre RADIX ou un ABR (mais possibilitée colision du à la fonction de hachage) ou une table de hachage.
