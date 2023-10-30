#ifndef INTERFACE_H
#define INTERFACE_H

#define MAGIC 'N'
#define HELLOIOCG _IOR(MAGIC, 0, char *) //get pointer
#define HELLOIOCS _IOW(MAGIC, 1, char *) //set pointer


#endif