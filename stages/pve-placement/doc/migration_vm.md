# Détail des migrations des VMs Qemu sur cluster Proxmox (offline migration et live migration)



## 1) Début
- Commande depuis l'utilitaire `qm` ou l'UI Web


## 2) Récupération des informations envoyés par JSON (API REST)
`PVE::CLI::qm.pm -> PVE::API2::Qemu` (L.4373).
- Récupération d'attributs et vérification (nom des noeuds, id VM, état online/offline de la VM, ip, etc...).
- Etape du stockage et travail sur l'attribut `targetstorage` avec `PVE::JSONSchema::parse_idmap()`.
	- Utilisation de `PVE::Tools::split_list()` qui utilise comme séparateur `,` et `;`.
	- Parcours de la liste:
		- Si on trouve '1', alors plus tard on saura qu'il faudra faire correspondre le nom des stockages sources et cibles.
		- Si on trouve juste un nom de stockage, alors la migration des stockages se fera vers celui-ci (si tout se passe bien - possibles conflits sur des formats ou type de stockage).
		- Sinon on a probablement un élément de la forme 'src_storage_name:tgt_storage_name', donc les éléments présents sur la source seront envoyés sur la cible.
- Verification des permissions, droits d'accès et du status des stockages.
- Identification de deux cas de migrations: high availability et migration 'classique'.
- Préparation de la tache et lancement.


## 3) Vérouillage
`PVE::GuestHelpers::guest_migration_lock()` -> `PVE::Tools::lock_file()` -> `PVE::Tools::lock_file_full()`.
- Lock sur un fichier de migration qui porte le nom `/var/lock/pve-manager/pve-migrate-$vmid`.
- Exécution de migrate() passée en paramètre de la chaine.


## 4) Migration
`PVE::QemuMigrate->migrate()` dépend d'un autre fichier `PVE::AbstractMigrate` - utilisé comme classe abstraite et contient la fonction migrate().
- Récupération des informations du réseau pour effectuer la migration.
- `PVE::QemuMigrate->lock_vm()` -> `PVE::AbstractConfig::lock_config()` -> `PVE::AbstractConfig::lock_config_full()` -> `PVE::AbstractConfig::lock_file_full_wrapper()` -> `PVE::Tools::lock_file_full()` pour vérouiller la vm (fichier de configuration) `/var/lock/qemu-server/lock-$vmid.conf`.

### 4.a| Préparation de la migration avec PVE::QemuMigrate->prepare()
- Vérification de l'état des noeuds et de la VM (existance, online / offline).
- Vérification des taches qui tournent sur la VM pour savoir si la migration peut avoir lieu (replication job).
- Vérification de taches bloquantes effectuées par la VM pou savoir si la migration peut avoir lieu.
- Vérification de l'utilisation de mapped devices et mapped loal devices.
- Vérification des stockages et de la disponibilitée.
- Préparation du tunnel d'échange (websocket tunnel) et test ssh.
- Retourne l'état de la VM (online ou offline).

### 4.b| Etape 1 avec PVE::QemuMigrate->phase1()
- Place le verrou de migration dans le fichier de configuration.
- Mettre à jour la taille des images dans le fichier de configuration.
- S'il y a réplication - par snapshot avec `PVE::QemuMigrate::handle_replication()` -> `PVE::Replication::run_replication()` -> `PVE::Replication::run_replication_nolock()` -> `PVE::Replication::replicate()`.
    - Si la vm est online, on va mettre à jour les bitmap des stockages (calcul du delta - éléments non à jour de la réplication).
	- Lancement de la mise à jour des répliques.
- Transfert des images hors ligne avec `PVE::QemuLigrate::sync_offline_local_volumes()` (donc, si la migration est hors ligne, toutes les images sont envoyées d'ici).
    - Récupération des images hors lignes.
	- Si le type est zfspool ou btrfs, il semblerait qu'une snapshot soit utilisée.
	- Utilisation de open3() pour échanger avec la cible et fournir une commande qui servira au noeud cible pour demander au noeud source d'envoyer le contenu des images.
	- Création sur la cible des images au bon format par une commande.
	- A la fin, la cible envoie une série de commande pour récupérer le contenu des images.
- Si l'étape échoue, cleanup avec `PVE::QemuMigrate::phase1_cleanup()`


### 4.C| Etape 2 avec PVE::QemuMigrate::phase2() uniquement si la VM est en ligne (live migration)
- Pour les migrations au sein du même cluster `PVE::QemuMigrate::phase2_start_local_cluster()`.
	- Démarrage de la VM sur la cible. Utilisation du protocol NBD pour rendre les images disponibles sur la connexion et pouvoir les transférer. ATTENTION, la VM a démarrée mais n'a pas encore pris le relai
- Transfert des images avec `PVE::QemuServer::qemu_drive_mirror`. lance une commande appelée `drive-mirror`.
- Prépare la migration avec les commandes `migrate-set-parameters` et `client-migrate-info`.
- Déclanche la migration de la VM avec la commande `migrate` et surveillance dans une boucle.
- Vérification de l'état du transfert de la RAM de manière régulière avec la commande `query-migrate`.
- Utilisation d'un cache `xbzrle` de qemu. Ce cache est utilisé afin de transmettre uniquement les mise à jour des pages et pas les pages complètes, afin de moins utiliser le réseau et d'augmenter la vitesse de la live migration.
- En cas de problème, cleanup avec `PVE::QemuMigrate::phase2_cleanup()`.

> `drive_mirror`, `migrate-set-parameters` et `client_migrate_info` sont des commandes provenant du projet qemu, car on voit sur le dépot github de qemu une section migration.

> Qemu propose deux mécanismes appelés precopy et postcopy pour réaliser le transfert de l'état de la VM et de la mémoire.  
> Postcopy:
> - La VM du noeud source est bloquée.
> - L'état du CPU et de ses devices sont envoyés à la VM du noeud destination qui va ensuite prendre le relai sur l'exécution de la VM même. De plus, une list des pages de la mémoire est faite.
> - Puis, il y a deux cas:
>		- Soit la VM cible ne fait pas de défaut de page (c'est à dire que la page demandée à déjà été transférée). Dans ce cas, la source continue de parcourir sa liste de page qui doivent être envoyés.
>		- Soit la VM cible fait un défaut de page (et active un mécanisme du noyau appelé `userfaultfd` - utilisé pour gérer les défauts de page en mode utilisateur). Dans ce cas, la page manquante et les suivantes qui n'ont pas encore été envoyés le sont. Plusieurs pages sont envoyés car les suivantes ont plus de chance d'être utilisés par la suite (localité spatiale).  
>
> Precopy:
> - Copier tout la mémoire sur la VM du noeud cible.
> - Copier les pages qui ont été modifiées pendant le précédent transfert. Ce point est relancé tant que le nombre de page modifié atteint un plafond.
> - La VM du noeud source est bloquée.
> - Les dernières pages modifiées, l'état du CPU et des devices sont envoyés.
> - La VM du noeud cible prend pleinement la main.

> Il semblerait que Proxmox utilise la precopy. Je n'ai pas trouvé grand chose pour appuyer mes propos, que cela provienne des sources ou des forums de Proxmox.
> Cependant, la Précopy me semble être l'option utilisée car pendant la live migration, même quand les images ont été transférées et que Proxmox indique que l'état de la VM est en transfert, des appels à l'API ont montrés que la VM source était en pleine charge (transferts) et qu'elle était toujours attachée au noeud de la source.
> Aussi, on voit bien l'icone de la VM sur l'UI Web changer de noeud seulement à la toute fin.

> Pour plus d'information, voir:
> * https://github.com/qemu/qemu/blob/master/docs/xbzrle.txt.
> * https://www.qemu.org/docs/master/devel/migration.html.
> * https://www.linux-kvm.org/images/e/ed/2011-forum-yabusame-postcopy-migration.pdf page`5` et `7 à 10`.


### 4.d| Etape 3 avec PVE::QemuMigrate::phase3()
- Cette fonction ne fait rien.
- cleanup sur la source avec `PVE::Qemu::Migrate::cleanup_phase3()`.


### 4.e| Fin avec PVE::QemuMigrate::final_cleanup()
- Cette fonction ne fait rien.


## Sources principales pour comprendre les étapes des migrations

* https://github.com/proxmox/qemu-server/blob/master/qm utilitaire.
* https://github.com/proxmox/qemu-server/blob/master/PVE/CLI/qm.pm.
* https://github.com/proxmox/qemu-server/blob/master/PVE/API2/Qemu.pm L.4373 - lancement de la migration.
* https://github.com/proxmox/qemu-server/blob/master/PVE/QemuMigrate.pm.
* https://github.com/proxmox/pve-guest-common/blob/master/src/PVE/AbstractMigrate.pm.
* https://github.com/proxmox/pve-access-control/blob/master/src/PVE/RPCEnvironment.pm.
* https://github.com/proxmox/pve-common/blob/master/src/PVE/RESTEnvironment.pm.
* https://github.com/proxmox/pve-common/blob/master/src/PVE/Tools.pm.
* https://github.com/proxmox/pve-common/blob/master/src/PVE/JSONSchema.pm.
* https://github.com/proxmox/pve-storage/blob/master/src/PVE/Storage.pm.
* https://github.com/proxmox/pve-guest-common/blob/master/src/PVE/Replication.pm.
* https://github.com/proxmox/pve-guest-common/blob/master/src/PVE/GuestHelpers.pm.


