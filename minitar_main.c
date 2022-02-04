#include <stdio.h>
#include <string.h>

#include "file_list.h"
#include "minitar.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("%s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
        return 0;
    }

    file_list_t files;
    file_list_init(&files);
    // TODO: Parse command-line arguments and invoke functions from 'minitar.h'
    // to execute archive operations

    const char *operation = argv[1];
    const char *archiveName = argv[3];
    printf("%s\n", archiveName);
    for (int x = 4; x < argc; x ++){
      file_list_add(&files, argv[x]);
    }
    char create[] = "-c";
    char append[] = "-a";
    char list[] = "-l";
    char update[] = "-u";
    char extract[] = "-x";



    if (strcmp(operation, create)){
      //call create function
    }
    else if (strcmp(operation, append)){
      //call append
    }
    else if (strcmp(operation, list)){
      //call list
    }
    else if (strcmp(operation, update)){
      //call upadte
    }
    else if (strcmp(operation, extract)){
      //call extract
    }
    else {
      //if not valid command, say so and do nothing
      printf("Invalid command");
    }
                                      

    file_list_clear(&files);
    return 0;
}
