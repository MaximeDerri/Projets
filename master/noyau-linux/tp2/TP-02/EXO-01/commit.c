#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "commit.h"

static int nextId = 0;
static struct commit_ops c_minor = {.extract=extract_minor, .display=display_minor_commit};
static struct commit_ops c_major = {.extract=extract_major, .display=display_major_commit};

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

  if(minor == 0)
  {
    com->com_ops = &c_major;
  }
  else
  {
    com->com_ops = &c_minor;
  }

  INIT_LIST_HEAD(&(com->list));
  INIT_LIST_HEAD(&(com->major_parent));
  INIT_LIST_HEAD(&(com->major_list));

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
    list_add(&(new->list), &(from->list));
    struct commit *c;
    
    /* verifier si on a ajouté une nouvelle majeur ou non */
    if(new->version.major != list_prev_entry(new, list)->version.major) /* nouvelle majeur */
    {
      struct commit *prev = NULL;
      struct commit *next = NULL;
      //chercher la majeur precedente
      prev = list_prev_entry(from, major_parent);

      
      //chercher la majeur suivante
      list_for_each_entry(c, &(new->list), list)
      {
        if(c->version.major != new->version.major)
        {
          next = list_entry(&(c->major_list), struct commit, major_list);
          break;
        }
      }
      
      //refaire les liens
      prev->major_list.next = &(new->major_list);
      new->major_list.next = &(next->major_list);
      new->major_list.prev = &(prev->major_list);
      next->major_list.prev = &(new->major_list);

    }
    else /* sinon, on fait le lien vers la version majeur */
    {
      list_for_each_entry_reverse(c, &(new->list), list)
      {
        if((list_entry(&(c->list), struct commit, list)->version.major == 0 && list_entry(&(c->list), struct commit, list)->version.minor == 0)) /* cas 0 */
        {
          new->major_parent.prev = &(c->major_parent); //lien vers 0.O
          new->major_parent.next = &(c->major_parent); //lien vers 0.0
          break;
        }
        if(c->version.major != new->version.major)
        {
          c = (list_next_entry(c, list));
          new->major_parent.prev = c->major_parent.next; /* il faut une ref sur la major_parent list */
          new->major_parent.next = c->major_parent.next; /* on peut referencer les deux car le but est de remonter vers le majeur */
          break;
        }
      } //for
    } //else

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
 * extract minor commit
 * 
 * @c: commit
 * @return: commit extrait
*/
struct commit *extract_minor(struct commit *c)
{
  list_del(&(c->list)); /* extraction simple */
  return c;
}

/**
 * extract major commit et retire les commits minor
 *
 * 
 * @c: commit
 * @return: commit major extrait
*/
struct commit *extract_major(struct commit *c)
{
  struct commit *tmp = c;
  struct commit *head = c;

  while(head->version.major == list_next_entry(head, list)->version.major)
  {
    tmp = list_next_entry(head, list);
    head->list.next = tmp->list.next;
    tmp->list.next->prev = &(head->list);
    //list_del(&(tmp->list));
  }

  /* dernier commit à retirer */
  head->list.prev->next = head->list.next;
  head->list.next->prev = head->list.prev;
  
  /* mettre a jour la major_list */
  head->major_list.prev->next = head->major_list.next;
  head->major_list.next->prev = head->major_list.prev;
  return c;
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
  return (*(victim->com_ops->extract))(victim);
}

/**
 * affichage pour les versions majeur
 * 
 * @c: commit
*/
void display_major_commit(struct commit *c)
{
  printf("\t%ld:\t",c->id);
  printf(" ### version %d : ", c->version.major);
  printf("\t%s", c->comment);
  printf(" ###\n");
}

void display_minor_commit(struct commit *c)
{
  printf("\t%ld:\t",c->id);
  display_version(&c->version, is_unstable_bis);
  printf("\t%s\n", c->comment);
}

/**
  * display_commit - affiche un commit : "2:  0-2 (stable) 'Work 2'"
  *
  * @c: commit qui sera affiche
  */
void display_commit(struct commit *c)
{
  (*(c->com_ops->display))(c);
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
