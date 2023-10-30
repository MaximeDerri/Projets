#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "../lcd_mod/lcd_ND.h"

struct pos_cursor setpos;

int main() 
{
    int lcd = open("/dev/lcd_ND", O_WRONLY);
    char buf[256];
    int i, x, y;

    for(i = 0; i<256; i++){
        buf[i] = '\0';
    }

    while(1){
        fgets(buf, 256, stdin);

        if(! strcmp("clear\n", buf)){
            ioctl(lcd, LCDIOCT_CLEAR);
            perror("1");
            continue;
        }
        
        if(! strcmp("setcursor\n", buf)){
            x = fgetc(stdin);
            y = fgetc(stdin);

            setpos.line   = y;
            setpos.column = x;

            ioctl(lcd, LCDIOCT_SETXY, &setpos);
            perror("2");
            continue;
        }

        write(lcd, buf, 256);
    }

    return 0;
}