

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv) {

    const char *path = ".";
    int watchedfd;

    struct stat fileStats;

    // Read and print the file
    ssize_t readReturn;
    char chunk[4096];

    if (argc >= 2) {
        path = argv[1];
    } else {
        printf("Please provide a file to watch!\n");
        return 1;
    }

    watchedfd = open(path, O_RDONLY);

    if (watchedfd <= -1) {
        printf("Please provide a valid file to watch!\n");
        return 1;
    }

    if (fstat(watchedfd, &fileStats) == -1) {
        printf("Error getting file stats!\n");
        printf("errno: %d\n", errno);
        close(watchedfd);
        return 1;
    }

    if (!S_ISREG(fileStats.st_mode)) {
        printf("File is not a regular file!\n");
        // printf("errno: %d\n", errno);
        close(watchedfd);
        return 1;
    }


    int kqueuefd = kqueue();

    if (kqueuefd == -1) {
        printf("Creation of kqueue failed!\n");
        printf("errno: %d\n", errno);
        close(watchedfd);
        return 1;
    }


    struct kevent vnodekevent;
    EV_SET(&vnodekevent, watchedfd, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE | NOTE_DELETE | NOTE_RENAME, 0, NULL);

    int keventResult = kevent(kqueuefd, &vnodekevent, 1, NULL, 0, NULL);
    if (keventResult <= -1) {
        printf("Registration of kevent failed!\n");
        printf("errno: %d\n", errno);
        close(watchedfd);
        close(kqueuefd);
        return 1;
    }

    while (1) {
        printf("Listening for events, current file: %s\n", path);
        
        if (lseek(watchedfd, 0, SEEK_SET) == -1) {
            printf("Failed to seek to start of file!\n");
            printf("errno: %d\n", errno);
            close(watchedfd);
            close(kqueuefd);
            return 1;
        }

        while ((readReturn = read(watchedfd, chunk, sizeof chunk)) > 0) {
            if (write(STDOUT_FILENO, chunk, readReturn) <= -1) {
                printf("Failed to write to stdout!");
                printf("errno: %d\n", errno);
                close(watchedfd);
                close(kqueuefd);
                return 1;
            }
        }
        if (readReturn <= -1) {
            printf("Specified path is not a readable file!");
            printf("errno: %d\n", errno);
            close(watchedfd);
            close(kqueuefd);
            return 1;
        }

        struct kevent occurred;
        int receivedEventValue = kevent(kqueuefd, NULL, 0, &occurred, 1, NULL);


        if (receivedEventValue <= -1) {
            printf("Receiving of kevent failed!\n");
            printf("errno: %d\n", errno);
            close(watchedfd);
            close(kqueuefd);
            return 1;
        }

        // printf("%u\n", occurred.fflags);

        // if (occurred.fflags & NOTE_WRITE) {
        //     printf("The file has been written to!\n");
        // }

        if ((occurred.fflags & NOTE_DELETE) || (occurred.fflags & NOTE_RENAME)) {
            printf("\nFile has been deleted or renamed, aborting!\n");
            break;
        } else {
            system("clear");
        }

    }

    close(watchedfd);

    // forgor the queue is an fd too lol
    close(kqueuefd);

    return 0;

}