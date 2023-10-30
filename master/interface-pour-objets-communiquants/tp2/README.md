# TP2 Driver Linux sur Raspberrypi

## auteurs:
- Maxime Derri
- François Normand

## Question 1
*Quelle fonction est exécutée lorsqu'on insère le module du noyau ?*

> C'est la fonction de prototype `static int __init mon_module_init(void)`

## Question 2
*Quelle fonction est exécutée lorsqu'on enlève le module du noyau ?*

> C'est la fonction de prototype `static void __exit mon_module_cleanup(void)`

## Question 3
*Résumez dans le CR ce que vous avez fait et ce que vous observez.*

> Nous avons commencé par ajouter le code du module et du Makefile fourni dans l'énoncé du TP2, puis nos avons modifié les variables du Makefile pour correspondre à notre travail.
Puis, nous avons compilé et placé `module.ko` dans `normand-derri/lab2`.
Enfin, on a chargé le module avec `sudo insmod module.ko`
puis fait `lsmod`, qui affiche bien le module.
Pour terminer, on a retiré le module avec `sudo rmmod module`.

## Question 4
*Comment voir que le parametre a bien été lu ?*

> Il suffit de taper la commande `dmesg`. `dmseg` va afficher tous les `printk()` faits dans l'init de notre module.

## Question 5
*Comment savoir que le device a été créé ?*

> etapes:
sudo insmod led0_ND.ko
cat /proc/devices    (-> led0_ND 245)
sudo mknod /dev/led0_ND c 245 0
sudo chmod a+rw /dev/led0_ND

> pour savoir si le device a été crée, on utilise la commande suivante: lsmod | `grep led0_ND`

## Question 6
*Le test de votre driver peut se faire par les commandes suivantes (avant de faire un vrai programme): dites ce que vous observez, en particulier, quelles opérations de votre driver sont utilisées.*

>echo "rien" > /dev/led0_XY
dd bs=1 count=1 < /dev/led0_XY
dmesg
===> dmesg montre bien des actions
on voit une suite `open() write() close() open() read() close()`

## Question 7
*Expliquer comment insdev récupère le numéro major*

> insdev utilise une commande `awk` pour chercher le major dans ``/proc/devices`


>On a utilisé copy_to_user pour retourner le caractere à l'usitilateur pour la lecture du bouton poussoir.
