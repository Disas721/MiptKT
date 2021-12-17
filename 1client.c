#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#define SZ 4096
#define FIFOSERV "./fifo.serv"

//клиент



int main(int argc, char **argv) {
    int fdin, fdout, rd = -1, pid_l = 0, file_l;
    pid_t pid;
    char *FIFOCL, *req, stops[] = "stop";

    if (argc < 2) {                                 // проверка на файл
        printf ("no file");
        return 0;
    }
    file_l = strlen (argv[1]);                      // получаем длину 

    if ((pid = getpid()) < 0) {                     // получаем pid
        perror ("getpid failed");
        return errno;
    }
    for (pid_t t = pid; t > 0; ++pid_l) {           // длина pid
        t /= 10;
    }

    if ((FIFOCL = malloc (7 + pid_l)) == NULL) {    // define input fifo filename
        fprintf (stderr, "0.0M\n");
        return -1;
    }
    sprintf (FIFOCL, "./fifo.%d", pid);

    if ((req = malloc (pid_l + file_l + 8)) == NULL) {      // создаем запрос к серверу
        fprintf (stderr, "0.0M\n");
        return -1;
    }
    sprintf (req, "%s %s", FIFOCL, argv[1]);

    umask (0);                                      // create fifo for input
    if (mkfifo (FIFOCL, 0600) < 0) {
        perror ("client mkfifo failed");
        return errno;
    }

    if ((fdout = open (FIFOSERV, O_WRONLY)) < 0) {  // отправляем запрос серверу
        perror ("open serv fifo failed");
        unlink (FIFOCL);
        return errno;
    }
    if (write (fdout, req, SZ) < 0) {
        perror ("request write failed");
        unlink (FIFOCL);
        return errno;
    }
    close (fdout);

    if (strcmp (argv[1], stops) == 0) {             // "soft" shutdown
        unlink (FIFOCL);
        return 0;
    }

    void *buf = NULL;                               // get answer from server
    if ((buf = malloc (SZ)) == NULL) {
        fprintf (stderr, "0.0M\n");
        unlink (FIFOCL);
        return -1;
    }
    if ((fdin = open (FIFOCL, O_RDONLY)) < 0) {
        perror ("open cl fifo failed");
        unlink (FIFOCL);
        return errno;
    }
    do {
        if ((rd = read (fdin, buf, SZ)) < 0) {
            perror ("fifo read failed");
            unlink (FIFOCL);
            return errno;
        }
        if (rd == 0) break;
        if (write (1, buf, rd) < 0) {     //1 -- стандартный поток вывода
            perror ("out write failed");
            unlink (FIFOCL);
            return errno;
        }
    } while (rd != 0);
    close (fdin);
    unlink (FIFOCL);
    free (buf);

//    printf ("%d, %d, %s, %d, %s\n", pid, pid_l, FIFOCL, file_l, req);

    return 0;
}
