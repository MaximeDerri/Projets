#include<stdlib.h>
#include<stdio.h>

#include"version.h"

int is_unstable(struct version *v)
{
	return 1 & ((char *)v)[sizeof(unsigned short)]; //(un)stable depend du parametre minor | faire +6
}

int is_unstable_bis(struct version *v) {
	return 1 == v->minor % 2;
}

void display_version(struct version *v, int(*is_unstable_ptr)(struct version *v))
{
	printf("%2u.%lu %s", v->major, v->minor,
			     is_unstable_ptr(v) ? "(unstable)" : "(stable)  ");
}

int cmp_version(struct version *v, unsigned short major, unsigned long minor)
{
	return v->major == major && v->minor == minor;
}
