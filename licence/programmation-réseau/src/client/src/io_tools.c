#include "io_tools.h"


void drain_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// to avoid pbs from scanf()...
int stdin_listen(char *com, int *n, char *id, char *msg) { // com need to have a length > than 2
    *n = -1;
    int r = 0;
    int len_nbr = 3; // 3 integer chars -> [0, 999]
    int len_buff = 0;
    char nbr[len_nbr + 1];

    if (id == NULL || msg == NULL)
        len_buff = 7;
    else
        len_buff = 213;
    char buff[len_buff];

    r = read(STDIN_FILENO, buff, sizeof(char) * len_buff);
    if (r < 0) {
        perror("Error from read() in stdin_listen()");
        return -2;
    }
    // com
    if(running == 0) //end
        return 1;

    if (r < 2 || buff[1] == '\n') {
        printf("%sA command is 2 chars long%s\n", RED, RESET);
        return -1;
    }
    memcpy(com, buff, sizeof(char) * COM_LEN);
    com[COM_LEN] = '\0';

    // preparation for a future STDIN clean - we accept every entree and we cut to the max length
    // adding a \0' brand to indicate that we had not read all the input - need to clean
    if (buff[r - 1] != '\n')
        buff[r - 1] = '\0';
    if (r <= COM_LEN + 1) // \n
        return 0; // only com, and stdin is ok

    int i = COM_LEN + 1; //+1 for the fill char between com and nbr (' ' or anything else)
    int cpt = 0;

    // getting nbr
    while (buff[i] >= '0' && buff[i] <= '9') {
        i++;
        cpt++;
    }
    // if we don't have cpt > 0 -> id/msg is maybe find.
    // if we don't have buf[i] == '\n' or '\0' -> id/msg is maybe find (we can't try to truncate then)
    if (cpt > 0 && (buff[i] == '\n' || buff[i] == '\0')) { // nbr is find and cpt is the length
        if (cpt > 3)
            cpt = 3;
        memcpy(nbr, buff + (COM_LEN + 1), sizeof(char) * cpt);
        nbr[cpt] = '\0';
        *n = (int)strtol(nbr, NULL, 10);
    }
    else
        cpt = 0; //maybe nbr is ID ?
    // exit ?
    if (cpt > 0 || id == NULL || msg == NULL) { // we have n, or we are not able to read id/msg
        if (buff[r - 1] != '\n')
            drain_stdin(); // if we have truncate, we need to clean stdin for the next read
        return 0;
    }

    // else, getting id / mess
    i = COM_LEN + 1;
    cpt = 0;
    if(strcmp(com, "am") != 0) { //am don't take id as arg
        while (cpt < ID_LEN) {
            if (buff[i] == '\n' || buff[i] == '\0' || buff[i] == ' ') { // bad id
                if (buff[r - 1] != '\n')
                    drain_stdin();
                return -1;
            }
            cpt++;
            i++;
        }
        memcpy(id, buff + (COM_LEN + 1), sizeof(char) * cpt); // id
        id[ID_LEN] = '\0';
        if (buff[i] != ' ') { // verif
            if (buff[r - 1] != '\n')
                drain_stdin();
            return -1;
        }
        i++; //space
    }

    cpt = r - 1 - i; // we select the length, witout '\0' (clear) or '\n' at the end of buff - will truncate at 200 char
    if (cpt <= 0)
        return -1;                             // no msg
    if(cpt > 200)
        cpt = 200;
    memcpy(msg, buff + i, sizeof(char) * cpt); // msg
    msg[cpt] = '\0';

    if (buff[r - 1] != '\n')
        drain_stdin();
    return 0;
}
