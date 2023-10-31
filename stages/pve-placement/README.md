# pve-placement

dépot du travail effectué par Maxime Derri lors d'un stage d'été 2023 (master 1ère année) au lip6,
sur le sujet `répartition statique et dynamique de machines virtuelles sur cluster Proxmox`.

## Context

De nos jours, il existe plusieurs solutions de virtualisation (VMware, Proxmox, etc...).
Ces solutions proposent des fonctionnalités permettant la migration de machines virtuelles (VM) et Clusters (CT) entre les noeuds du cluster.
Plusieurs éléments sont à prendre en compte pour les migrations entre les noeuds du cluster:
- espaces de stockage,
- performance des disques pour les transferts,
- utilisation de la RAM,
- utilisation CPU,
- Le réseau,
- CPUs supportés par les systèmes d'exploitation des VMs et CTs,
- Migration de données entre différents système de fichiers des noeuds (des routes peuvent ne pas exister).

On s'intéresse dans ce travail à l'implémentation d'algoritmes de placement afin de réaliser un équilibrage sur les noeuds des clusters Proxmox.
Le problème à résoudre est en lien avec le `bin pack problem`, connu pour être NP-complet.
Il sera donc nécessaire d'utiliser des algorithmes d'approximation.

Il y a deux points à réaliser:
- Losque l'on souhaite éteindre un noeud (pour le modifier ou faire des mises à jours par exemple), il faut pouvoir répartir les VMs et CTs sur
les autres noeuds.
- Pouvoir proposer un nouveau schéma d'organisation des noeuds d'un cluster afin d'optimiser les ressources.

Les migrations doivent être limitées afin de réduire le coût des transferts.


## Utilisation de l'outil

L'outil est developpé en python3, il n'y a rien à compiler. Cependant, des packets sont à installer:
- proxmoxer, paramiko et requests  (avec pip par exemple)
- SSH

Pour connaitre les commandes de l'outil, utilisez `pvep` ou `pvep --help`.
