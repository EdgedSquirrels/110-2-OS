#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

typedef struct nodepr {
    int dir_num;
    int file_num;
} nodepr;

nodepr sub_tree(int fd, char *path, int level, int isend[]) {
    char *p, *ed = path + strlen(path);
    struct dirent de, de2;
    struct stat st;
    struct nodepr nd = {.dir_num = 0, .file_num = 0}, nd2;
    isend[level] = 0;
    int fd2;

    // make sure the buf end with '/' and p will start with its end
    p = path + strlen(path);
    *p++ = '/';
    *p = '\0';

    de.inum = de2.inum = 0;
    while (de.inum == 0 || de.name[0] == '.') {
        if (read(fd, &de, sizeof(de)) != sizeof(de)) {
            *ed = '\0';
            return nd;
        }
            
    }
    while (de2.inum == 0 || de2.name[0] == '.') {
        if (read(fd, &de2, sizeof(de2)) != sizeof(de2)) {
            isend[level] = 1;
            break;
        }
    }

    while (1) {
        // print de.name info
        for (int i = 0; i < level; i++)
            printf("%c   ", "| "[isend[i]]);
        printf("|\n");
        for (int i = 0; i < level; i++)
            printf("%c   ", "| "[isend[i]]);
        printf("+-- %s\n", de.name);

        // run subdirectory
        memcpy(p, de.name, strlen(de.name) + 1);
        // printf("%s(debug--path)\n", buf);
        if ((fd2 = open(path, 0)) < 0 || fstat(fd2, &st) < 0) {
            // open the desired file or directory
            fprintf(2, "%s [error opening dir or file]\n", path);
            *ed = '\0';
            return nd;
        }
        if (st.type == T_DIR) {
            nd.dir_num++;
            nd2 = sub_tree(fd2, path, level + 1, isend);
            nd.dir_num += nd2.dir_num;
            nd.file_num += nd2.file_num;
        }
        else if (st.type == T_FILE)
            nd.file_num++;
        close(fd2);
        // break if it reaches the end
        if (isend[level]) break;
        de = de2;
        de2.inum = 0;
        while (de2.inum == 0 || de2.name[0] == '.') {
            if (read(fd, &de2, sizeof(de2)) != sizeof(de2)) {
                isend[level] = 1;
                break;
            }
        }
    }
    *ed = '\0';
    return nd;
}

void tree(char *path) {
    // TODO
    char buf[256];
    int fd, fds[2], dir_num = 0, file_num = 0;
    int level = 0, isend[10] = {0};
    // struct dirent de;
    struct stat st;
    strcpy(buf, path);


    if ((fd = open(path, 0)) < 0 || fstat(fd, &st) < 0 || st.type != T_DIR) {
        // open the desired file or directory
        printf("%s [error opening dir]\n", buf);
        printf("\n%d directories, %d files\n", dir_num, file_num);
        exit(0);
    }
    printf("%s\n", buf);
    pipe(fds);

    // 2. Fork a child
    if (fork() == 0) {
        close(fds[0]);
        nodepr nd = {.dir_num = 0, .file_num = 0};
        // 3. Child traverse the files and directories under <root_directory>, output in tree-like
        //    format, and analyze the total number of traversed files and directories.
        nd = sub_tree(fd, buf, level, isend);
        close(fd);
        // 4. Child send file_num and dir_num to parent via a pipe
        //    Note: Remember to close the file descriptor
        write(fds[1], &nd.dir_num, sizeof(int));
        write(fds[1], &nd.file_num, sizeof(int));
        close(fds[1]);
        exit(0);
    }
    close(fds[1]);
    read(fds[0], &dir_num, sizeof(int));
    read(fds[0], &file_num, sizeof(int));
    close(fds[0]);
    
    // 5. Parent prints <dir_num> directories, <file_num> files
    //    Note: <root_directory> should not be counted
    // level < 5 and <dir_num> + <file_num> <= 20
    printf("\n%d directories, %d files\n", dir_num, file_num);
    exit(0);
}

int main(int argc, char *argv[]) {
    // 1. Read one argument from the command line
    if (argc < 2) {
        tree(".");
        exit(0);
    }
    for (int i = 1; i < argc; i++)
        tree(argv[i]);
    exit(0);
}

/*
sudo docker run -it -v $(pwd)/xv6:/home/os_mp0/xv6 ntuos/mp0
make qemu
*/

/*
Questions:
./ ../ 要用 i-node 偵測還是用 filename 偵測？
dev file 要算在 file 嗎？
dev file 可以被 traverse 嗎？如果可以的話，需要 tree 裡面的 files 嗎？
是不是要忽略 . 開頭的檔案？
*/
