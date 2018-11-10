#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const int buff_size = 1024;

void doWrite(int fd, const char *buff, int len){
        size_t idx = 0;
        ssize_t wcnt;
        do {
                wcnt = write(fd,buff + idx, len - idx);
                if (wcnt == -1){ /* error */
                        perror("write");
                        exit(1);
                }
                idx += wcnt;
        } while (idx < len);
}

void write_file(int fd, const char *infile, int fd_in){
        char buff[buff_size];
        ssize_t rcnt;
        for (;;){
                rcnt = read(fd_in,buff,sizeof(buff)-1);
                if (rcnt == 0) /* end-of-file */
                        break;
                if (rcnt == -1){ /* error */
                        perror("read");
                        exit(1);
                }
                doWrite(fd, buff, rcnt);
        }
        close(fd_in);
}

int main(int argc, char **argv){
        char* output = "fconc.out"; /* default output file name */
        if(argc == 1 || argc == 2 || argc >=5){ /* incorrect call of program message */
                printf("Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n");
        }
        else if (argc >= 3){
                if (argc == 4){ /* optional output file name as third input */
                        output = argv[3];
                }
                int flags1 = O_RDONLY;
                int fd_infile1 = open(argv[1], O_RDONLY);
                int fd_infile2 = open(argv[2], O_RDONLY);
                if (fd_infile1 == -1 || fd_infile2 == -1){
                        perror("open");
                        exit(1);
                }
                if (fd_infile1 != -1 && fd_infile2 != -1){ /* both files can be opened */
                        int fd_outfile, oflags, mode, new_fd;
                        int done = 0;
                        int input = 1;
                        oflags = O_CREAT | O_WRONLY | O_TRUNC;
                        if ((strcmp(argv[1], output) == 0) || (strcmp(argv[2], output) == 0)){ /* exception if input file(s) == output file */
                                done = 1;
                                flags1 = O_RDWR;
                                if (strcmp(argv[1], output) == 0){
                                        input = 1;
                                }
                                if (strcmp(argv[2], output) == 0){
                                        input = 2;
                                }
                                printf("Warning: input file: %s has been used as an output file and is overwitten!\n", output);
                                output = "dummy.in";
                                oflags = O_CREAT | O_RDWR | O_TRUNC;
                        }
                        mode = S_IRUSR | S_IWUSR;
                        fd_outfile = open(output, oflags, mode);
                        if (fd_outfile == -1){
                                perror("open");
                                exit(1);
                        }
                        else { /* output file can also be opened */
                                write_file(fd_outfile, argv[1], fd_infile1);
                                write_file(fd_outfile, argv[2], fd_infile2);
                                close(fd_outfile);
                                if (done == 1){
                                        fd_outfile = open(output, O_RDONLY, mode);
                                        new_fd = open(argv[input], flags1, mode);
                                        write_file(new_fd, output, fd_outfile);
                                        close(new_fd);
                                }
                        }
                }
        }
        return 0;
}
