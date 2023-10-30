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
 * liberer h
 * liberer chaque commit
 * 
 * @h: pointeur vers l'historique
 */
void freeHistory(struct history *h) {
  struct commit *com = h->commit_list;
  struct commit *c;

  while (com->list.next != &(com->list)) /* on ne peut pas utiliser les macro for de list.h car chaque chainon suivant deppend du precedent et list_del place LIST_POISON1 dans next */
  {
    c = list_next_entry(com, list);
    com->list.next = c->list.next;
    list_del(&(c->list));
    free(c);

  }
  free(com);
  free(h);
}

/**
  * last_commit - retourne l'adresse du dernier commit de l'historique.
  *
  * @h: pointeur vers l'historique
  */
struct commit *last_commit(struct history *h)
{
  return list_prev_entry(h->commit_list, list);
}

/**
  * display_history - affiche tout l'historique, i.e. l'ensemble des commits de
  *                   la liste
  *
  * @h: pointeur vers l'historique a afficher
  */
void display_history(struct history *h)
{
  struct commit *c;
	if(h == NULL || h->commit_list == NULL)
  {
    return;
  }

  printf("Historique de \'Toute une histoire\' :\n");
  list_for_each_entry(c, &(h->commit_list->list), list)
  {
    display_commit(c);
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
  int test = 0;
  struct commit *c;
  struct commit *com = h->commit_list;

  //determiner major
  list_for_each_entry(c, &(com->major_list), major_list)
  {
    if(c->version.major == major)
    {
      test = 1;
      break;
    }
  }

  if((major > 0) && (test == 0)) //dans ce cas, le numero major n'existe pas
  {
    goto end; //forward goto
  }

  //parcourir minor
  if(test == 1)
  {
    com = c;  
  }

  //chercher minor
  list_for_each_entry(c, &(com->list), list)
  {
    if(c->version.major != major)
    {
      break;
    }
    if(c->version.major == major && c->version.minor == minor)
    {
      display_commit(c);
      return;
    }
  }

  end:
  //pas trouvé
  printf("Not here !!!\n");
}
