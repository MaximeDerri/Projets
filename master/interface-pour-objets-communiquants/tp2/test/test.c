#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//BP et LED dans le même driver (read / write)
int main()
{  
   char led, bp;
   int fdled0 = open("/dev/led0_ND", O_RDWR);
   if (fdled0 < 0) {
      fprintf(stderr, "Erreur d'ouverture des pilotes LED ou Boutons\n");
      return 1;
   }
   do { 
      led = (led == '0') ? '1' : '0';
      write( fdled0, &led, 1);
      printf("test\n");
      read(fdled0, &bp, 1);
      sleep( 1);
      printf("read: %c\n", bp);
   } while (bp == '1');
   return 0;
}