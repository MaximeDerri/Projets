#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "commit.h"

static int nextId = 0;

/**
  * new_commit - alloue et initialise une structure commit correspondant aux
  *              parametres
  *
  * @major: numero de version majeure
  * @minor: numero de version mineure
  * @comment: pointeur vers une chaine de caracteres contenant un commentaire
  *
  * @return: retourne un pointeur vers la structure allouee et initialisee
  */
struct commit *new_commit(unsigned short major, unsigned long minor,
			  char *comment)
{
	struct commit *com = malloc(sizeof(struct commit));
  if(com == NULL)
  {
    perror("malloc");
    exit(1);
  }
  com->version.major = major;
  com->version.minor = minor;
  com->comment = comment;
  com->id = nextId;
  nextId++;
  com->next = com;
  com->prev = com;
  return com;
}

/**
  * insert_commit - insere sans le modifier un commit dans la liste doublement
  *                 chainee
  *
  * @from: commit qui deviendra le predecesseur du commit insere
  * @new: commit a inserer - seuls ses champs next et prev seront modifies
  *
  * @return: retourne un pointeur vers la structure inseree
  */
static struct commit *insert_commit(struct commit *from, struct commit *new)
{ 
  if(from == NULL)
  {
    from = new;
  }
  else
  {
	from->next->prev = new;
  new->next = from->next;
  new->prev = from;
  from->next = new;
  }
	return new;
}

/**
  * add_minor_commit - genere et insere un commit correspondant a une version
  *                    mineure
  *
  * @from: commit qui deviendra le predecesseur du commit insere
  * @comment: commentaire du commit
  *
  * @return: retourne un pointeur vers la structure inseree
  */
struct commit *add_minor_commit(struct commit *from, char *comment)
{
	struct commit *com = new_commit(from->version.major, (from->version.minor)+1, comment);
	return insert_commit(from, com);
}

/**
	* add_major_commit - genere et insere un commit correspondant a une version
  *                    majeure
  *
  * @from: commit qui deviendra le predecesseur du commit insere
  * @comment: commentaire du commit
  *
  * @return: retourne un pointeur vers la structure inseree
  */
struct commit *add_major_commit(struct commit *from, char *comment)
{
	struct commit *com = new_commit((from->version.major)+1, 0, comment);
	return insert_commit(from, com);
}

/**
  * del_commit - extrait le commit de l'historique
  *
  * @victim: commit qui sera sorti de la liste doublement chainee
  *
  * @return: retourne un pointeur vers la structure extraite
  */
struct commit *del_commit(struct commit *victim)
{
	/* TODO : Exercice 3 - Question 5 */
  victim->next->prev = victim->prev;
  victim->prev->next = victim->next;

	return victim;
}

/**
  * display_commit - affiche un commit : "2:  0-2 (stable) 'Work 2'"
  *
  * @c: commit qui sera affiche
  */
void display_commit(struct commit *c)
{
	/* TODO : Exercice 3 - Question 4 */
  printf("\t%ld:\t",c->id);
  display_version(&c->version, is_unstable_bis);
  printf("\t%s\n", c->comment);
}

/**
  * commitOf - retourne le commit qui contient la version passee en parametre
  *
  * @version: pointeur vers la structure version dont on recherche le commit
  *
  * @return: un pointeur vers la structure commit qui contient 'version'
  *
  * Note:      cette fonction continue de fonctionner meme si l'on modifie
  *            l'ordre et le nombre des champs de la structure commit.
  */
struct commit *commitOf(struct version *version)
{
	/* TODO : Exercice 2 - Question 2 */
  struct commit com;
  int offset = (void *)&com.version - (void *)&com;
	return (struct commit *)((void *)version-offset);
}
