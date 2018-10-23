#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define RWX_UGO (S_IRWXU | S_IRWXG | S_IRWXO)

const unsigned int BLOCK_SIZE = 512;

char* ERRORS_TEXT[] = 
{
    "",
    "open failed",
    "fstat failed",
    "malloc failed",
    "read failed",
    "write failed",
    "lseek failed",
    "error writing file hole"
    "close source failed",
    "close destination failed"
};

enum ERRORS
{
    ERR_SUCCESS,
    ERR_OPEN,
    ERR_FSTAT,
    ERR_MALLOC,
    ERR_READ,
    ERR_WRITE,
    ERR_LSEEK,
    ERR_WRITE_FILE_HOLE,
    ERR_CLOSE_SOURCE,
    ERR_CLOSE_DESTINATION
} state;

int main(int argc, char* argv[])
{
    state = ERR_SUCCESS;
    if (3 != argc)
    {
        printf("Usage: %s [source] [destination]", argv[0]);
        goto cleanup;
    }


    int fd_src = open(argv[1], O_RDONLY, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
    if (-1 == fd_src)
    {
        state = ERR_OPEN;
        goto cleanup;
    }
    
    struct stat src_stats = {0};
    if (-1 == fstat(fd_src, &src_stats))
    {
        state = ERR_FSTAT;
        goto cleanup;
    }

    int fd_dst = open(argv[2], O_CREAT|O_WRONLY|O_TRUNC, (src_stats.st_mode & RWX_UGO)); // only taking the RWXRWXRWX permissions
    if (-1 == fd_dst)
    {
        state = ERR_OPEN;
        goto cleanup;
    }

    unsigned int raw_size = (src_stats.st_blocks * BLOCK_SIZE);
    char * buf = malloc(raw_size);
    if (NULL == buf)
    {
        state = ERR_MALLOC;
        goto cleanup;
    }

    ssize_t bytes_read = read(fd_src, buf, raw_size);
    if (-1 == bytes_read)
    {
        state = ERR_READ;
        goto cleanup;
    }
    ssize_t bytes_written = write(fd_dst, buf, bytes_read);
    if (-1 == bytes_written)
    {
        state = ERR_WRITE;
        goto cleanup;
    }

    if (-1 == lseek(fd_dst, (raw_size-bytes_written-1), SEEK_END))
    {
        state = ERR_LSEEK;
        goto cleanup;
    }

    if (-1 == write(fd_dst, "", 1)) // make the of the file the same size of it on disk.
    {
        state = ERR_WRITE_FILE_HOLE;
        goto cleanup;
    }

    if (-1 == close(fd_src))
    {
        state = ERR_CLOSE_SOURCE;
        goto cleanup;
    }

    if (-1 == close(fd_dst))
    {
        state = ERR_CLOSE_DESTINATION;
        goto cleanup;
    }

    free(buf);
    
cleanup:
    printf("%s\n", ERRORS_TEXT[state]);
    return state;
}
