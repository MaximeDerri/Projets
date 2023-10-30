# Proxmox VE sur VMs

## Etape 1
Installer Virtual Box (v7).

## Etape 2
Télécharger l'ISO proxmox.

## Etape 3
Créer une VM proxmox.

CPU:
- Dans les paramètres de la VM, activer `VT-x/AMD-V` dans `system/processor` (si grisé, lancer la commande `VBoxManager modifyvm nom_vm --nested-hw-virt on`).

Network:
- Si ce n'est pas fait, créer une interface réseau dans tools/network.
	- Si une erreur apparait, c'est possiblement à cause du `Secure Boot`. Il faut le désactiver dans le BIOS.
- Ensuite dans les paramètres de la VM:
	- adapter1 `Host-only Adapter` et choisir l'interface (vboxnet0 ?).
	- adapter2: `NAT`.

## Etape 4
Lancer la VM et installer proxmox.

## Etape 5
Eteindre la VM (car repropose de télécharger).
Aller dans paramètre > stockage et supprimer l'ISO.

## Etape 6
Pour pouvoir accéder à l'interface WEB, il va falloir modifier l'`adaper2 NAT` (réseau privé, invisible de l'hote) pour ajouter le port forwarding.
- procédure: https://www.youtube.com/watch?v=Kq_JOGX0MW4.
- Relancer la VM.
- pve login: root, pwd: `fait à l'installation`.
- Lancer la commande `ip a` et récupérer l'adresse ip w.x.y.z de la VM.
- Ouvrir le panneau de configuration de la VM > network > adapter2 (`NAT`) > Port Forwarding.
- ajouter:
`|WEB|TCP|127.0.0.1|8080|w.x.y.z|80|` et  `|SSH|TCP|127.0.0.1|2222|w.x.y.z|22|`.

-> L'interface Web de Proxmox est maintenant accessible.

# Utilisation de l'API RESTProxmox
	- lister les nodes et les ressources: get /cluster/resources --type node
	- lister toutes les VMs/conteneurs et les ressources: get /cluster/resources --type vm
	- lister tous les stockages de tous les nodes: get /cluster/resources --type storage
	- lister les types de CPU pris en charge par un node: get /nodes/{node}/capabilities/qemu/cpu
	- lister les interfaces reseau d'un node: get /nodes/{node}/network
	- lister la config d'une vm (ex: net it, cpu type) get /nodes/{node}/qemu/{vmid}/config
	- lister les pre-condition pour la migration d'une vm (ex: les disques local + taille, vm on/off): get /nodes/{node}/qemu/{vmid}/migrate
	- lancer une migration d'une vm: post /nodes/{node}/qemu/{vmid}/migrate

On pourrait avoir un problème au niveau du stockage:
on a des pool de mémoire pour lvmthin et zfs, donc différents stockages peuvent être basés dessus et mener à faux le fait que chaque stockage ait une source
séparée. Sur mon node 2, j'ai deux stockage lvm-thin qui prennent sur le pool tvmthin data
Idée: utiliser get /nodes/{node}/disk/ directory ou lvm ou lvmthin ou zfs pour obtenir des informations sur l'état des disques d'un node.

	- obtenir des infos sur les disques local du NODE (donc du serveur): get /nodes/{node}/disks/list
	- obtenir le nom de tous les stockages: get /storage {--type (dir | lvm | lvmthin | zfs)}
	- obtenir l'information sur le chemin ou pool associé au stockage: get /storage/{storage}
	==> Permet de faire le lien entre les stockages et les pool/chemin associer
	==> créer des objets dont on indique le pool/chemin et une liste des noms de stockages associés à ce dernier

	- réfléchir si on base l'algo sur l'espace disponible de ces disque, puis en fonction du manque de place on suggère à l'utilisateur
	d'augmenter la capaciter (extend / create)
	
	Dans un premier temps, on considère qu'aucun autre disque ne sera créer par l'algo (on verra pour la suite si on le fait)
	==> indiquer dans des objets les espaces qui pourraient être occupés par les migration sur les disques existants en fonction d'une limite
	à essayer de ne pas dépasser, et dans le cas où ça dépasse, ajouter un nouvel objet de type stockage qui indique que ce stockage n'existe pas.
	L'idée est d'indiquer à l'utilisateur qu'il manque de l'espace pour les migrations et qu'il sera nécessaire de créer de la place.
