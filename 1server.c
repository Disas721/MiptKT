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


// Сервер

char *FIFOCL, *filename;
void *buf;


void fifo_kill() {
    
    free (FIFOCL);
    free (filename);
    unlink (FIFOSERV);
    free (buf);

    exit(0);
}


int main() {
    int fdin, fdout, fh, rd = -1, notend, req = 0;
    char stops[] = "stop";
    pid_t chpid;
    // fifo_kill();
    signal(SIGINT, fifo_kill);

    buf = NULL;
    if ((buf = malloc (SZ)) == NULL) {
        fprintf (stderr, "0.0M\n");
        return -1;
    }
    umask (0);
    if (mkfifo (FIFOSERV, 0600) < 0) {                  // открываем фифо для ввода
        perror ("mkfifo failed");
        return errno;
    }

    if ((FIFOCL = malloc (SZ)) == NULL) {               
        fprintf (stderr, "0.0M\n");
        unlink (FIFOSERV);
        return errno;
    }
    if ((filename = malloc (SZ)) == NULL) {
        fprintf (stderr, "0.0M\n");
        unlink (FIFOSERV);
        return errno;
    }


    do {
        if ((fdin = open (FIFOSERV, O_RDONLY)) < 0) {   // Читаем FIFO клиента и filename
            perror ("open server fifo failed");
            unlink (FIFOSERV);
            return errno;
        }
        if ((rd = read (fdin, buf, SZ)) < 0) {
            perror ("fifo read failed");
            unlink (FIFOSERV);
            return errno;
        }
        close (fdin);
        sscanf (buf, "%s %s", FIFOCL, filename);
/*        printf ("%s, %s", FIFOCL, filename);
        fflush (stdout);*/
        if (strcmp (filename, stops) == 0) break;

        if ((chpid = fork()) < 0) {
            perror ("fork failed");
            unlink (FIFOSERV);
            return errno;
        }
        if (chpid == 0) {                                   // делаем fork
    //        printf ("\nSubprocess ID: %d", getpid());
            if ((fdout = open (FIFOCL, O_WRONLY)) < 0) {
                perror ("open cl fifo failed");
                return errno;
            }
            if ((fh = open (filename, O_RDONLY)) < 0) {
                perror ("file open failed");
                if (write (fdout, " !!! SERVER ERROR !!! \n", 24) < 0) {
                    perror ("fifo write failed");
                    close (fdout);
                    return errno;
                }
                close (fdout);
                return errno;
            }

            do {                                            // отправляем содержание файла
                if ((rd = read (fh, buf, SZ)) < 0) {
                    perror ("file read failed");
                    if (write (fdout, " !!! SERVER ERROR !!! \n", 24) < 0) {
                        perror ("fifo write failed");
                        close (fdout);
                        close (fh);
                        return errno;
                    }
                    close (fdout);
                    close (fh);
                    return errno;
                }
                if (rd == 0) break;
                if (write (fdout, buf, rd) < 0) {
                    perror ("fifo write failed");
                    if (write (fdout, " !!! SERVER ERROR !!! \n", 24) < 0) {
                        perror ("fifo write failed");
                        close (fdout);
                        close (fh);
                        return errno;
                    }
                    close (fdout);
                    close (fh);
                    return errno;
                }
                
            } while (1);
            close (fdout);
            close (fh);
            printf ("\nRequest #%d processed. File: %s\n", req + 1, filename);
            fflush (stdout);
            free (FIFOCL);
            free (filename);
            free (buf);
            fflush (stdout);
            return 0;
        }

        notend = strcmp (filename, stops);
        req++;
        
    } while (notend != 0);

    printf ("\nStopping, a total of %d request(s) processed.\n\n", req);
    free (FIFOCL);
    free (filename);
    unlink (FIFOSERV);
    free (buf);

    return 0;
    
}
