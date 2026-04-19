#define PATH_MAX 256
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
typedef struct counter
{
    size_t files;
    size_t dirs;
    size_t links;
} counter_t;

static void delete_last_slash(char *path)
{
    if (strcmp(path, ".") == 0)
        return;
    if (strcmp(path, "/") == 0)
        return;
    size_t len = strlen(path);
    while (path[--len] == '/')
    {
        path[len] = '\0';
    }
}
static int print(const char *path, const char *name, int depth)
{
    struct stat st;
    // No es que lo ponga doble, es que lo necesito para me rellene el st
    if (lstat(path, &st))
    {
        fprintf(stderr, "Error on path %s. %s\n", path, strerror(errno));
        return 1;
    }

    // Print indentation
    for (int i = 0; i < depth; i++)
    {
        fputs("    ", stdout);
    }
    // Imprimimos el name(fputs es ligeramente mas rapido que printf)
    fputs(name, stdout);

    // Si es un link termina con un @, si no con un /
    if (S_ISLNK(st.st_mode))
    {
        fputs("@", stdout);
    }
    else if (S_ISDIR(st.st_mode) && strcmp(name, "/") != 0)
    {
        fputs("/", stdout);
    }
    putchar('\n');
    return 0;
}
// La funcion recursiva
static int walk(const char *path, const char *name, int depth, counter_t *counter)
{
    struct stat st;
    DIR *dp;
    struct dirent *de;
    // uso lstat que es igual que stat pero que no hace lo del bucle infinito con los punteros(es lo que llena st)
    if (lstat(path, &st))
    {
        fprintf(stderr, "lstat: Error on path %s. %s\n", path, strerror(errno));
        return 1;
    }
    if (print(path, name, depth) != 0)
        return 1;

    if (S_ISDIR(st.st_mode))
        counter->dirs++;
    else if (S_ISLNK(st.st_mode))
        counter->links++;
    else
        counter->files++;
    
    if (!S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode))
    {
        return 0;
    }

    dp = opendir(path);
    if (dp == NULL)
    {
        fprintf(stderr, "Error: Cannot open this directory. Path: %s. %s\n", path, strerror(errno));
        return 1;
    }
    int error = 0;

    while ((de = readdir(dp)) != NULL)
    {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        char new_path[PATH_MAX]; // esto para asegurarse de que no se pasa del maximo de caracteres permitidos
        int n;
        n = snprintf(new_path, sizeof(new_path), "%s/%s", path, de->d_name); // Lo que hace es copiar el nuevo path en la variable
        if (n < 0 || (size_t)n >= sizeof(new_path))
        {
            fprintf(stderr, "%s/%s: path too long\n", path, de->d_name);
            error = 1;
            continue;
        }
        if (walk(new_path, de->d_name, depth + 1, counter) != 0)
            error = 1;
    }
    if (closedir(dp) != 0)
    {
        fprintf(stderr, "Error: Cannot close this directory. Path:%s. %s", path, strerror(errno));
        error = 1;
    }
    return error;
}
int main(int argc, char **argv)
{
    if (argc > 2)
    { // Compruebo si respetaron el modo de uso
        puts("tree: too many arguments");
        puts("      usage: tree [path]");
    }
    // Creo el puntero que va al inicio del path, si no pasaron argumentos es el .
    if (argc != 1)
        delete_last_slash(argv[1]);
    const char *Path = (argc == 1) ? "." : argv[1];
    counter_t counter = {0,0,0};
    int out = walk(Path, Path, 0,&counter);
    printf("%zu directories    %zu files    %zu links", counter.dirs,counter.files,counter.links);
    return out ? EXIT_FAILURE : EXIT_SUCCESS;
}