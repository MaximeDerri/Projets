#include<stdlib.h>
#include<stdio.h>

#include"history.h"

/**
  * new_history - alloue, initialise et retourne un historique.
  *
  * @name: nom de l'historique
  */
struct history *new_history(char *name)
{
  struct history *hist = malloc(sizeof(struct history));
  if(hist == NULL)
  {
    perror("malloc");
    exit(1);
  }
  hist->name = name;
  hist->commit_count = 0;
  hist->commit_list = new_commit(0, 0, "DO NOT PRINT ME !!!");

	return hist;
}

/**
  * last_commit - retourne l'adresse du dernier commit de l'historique.
  *
  * @h: pointeur vers l'historique
  */
struct commit *last_commit(struct history *h)
{
	return h->commit_list->prev;
}

/**
  * display_history - affiche tout l'historique, i.e. l'ensemble des commits de
  *                   la liste
  *
  * @h: pointeur vers l'historique a afficher
  */
void display_history(struct history *h)
{
	if(h == NULL || h->commit_list == NULL)
  {
    return;
  }
  
  struct commit *com = h->commit_list->next;
  printf("Historique de \'Toute une histoire\' :\n");

  while(com != h->commit_list)
  {
    display_commit(com);
    com = com->next;
  }

}

/**
  * infos - affiche le commit qui a pour numero de version <major>-<minor> ou
  *         'Not here !!!' s'il n'y a pas de commit correspondant.
  *
  * @major: major du commit affiche
  * @minor: minor du commit affiche
  */
void infos(struct history *h, int major, unsigned long minor)
{
	struct commit *com = h->commit_list->next;
  while(com != h->commit_list)
  {
    if(com->version.major == major && com->version.minor == minor)
    {
      display_version(&com->version, is_unstable_bis);
      return;
    }
    com = com->next;
  }
  
  printf("Not here !!!\n");
}
