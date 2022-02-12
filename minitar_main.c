#include <stdio.h>
#include <string.h>

#include "file_list.h"
#include "minitar.h"

#define DEBUG 0



int main(int argc, char **argv) {
    if (argc < 3) {
        printf("%s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
        return -1;
    }
    if(DEBUG){   
      for (unsigned int x = 0; x < argc; x ++){
        printf("Argument %d: \"%s\"\n", x, argv[x]);
      }
    }
    file_list_t files;
    file_list_init(&files);
    // TODO: Parse command-line arguments and invoke functions from 'minitar.h'
    // to execute archive operations

    const char *operation = argv[1];
    const char *archiveName = argv[3];
    if (DEBUG)printf("Archive Name: %s\n", archiveName);
    for (int x = 4; x < argc; x ++){
      file_list_add(&files, argv[x]);
      if(DEBUG) printf("File %d: %s\n", x-4, argv[x]);
    }
    char create[] = "-c";
    char append[] = "-a";
    char list[] = "-t";
    char update[] = "-u";
    char extract[] = "-x";
    if(DEBUG)printf("Strings defined\n");
    if (!strcmp(operation, create)){
      //call create function
      //TODO urgently! due Friday 2/4!
      create_archive(archiveName, &files);
    }
    else if (!strcmp(operation, append)){
      //call append function
      //TODO urgently! due Friday 2/4!
      append_files_to_archive(archiveName, &files);
    }
    else if (!strcmp(operation, list)){
      //call list
      get_archive_file_list(archiveName, &files);
    }
    else if (!strcmp(operation, update)){
      //call update
      //update_files_in_archive(archiveName, &files);
    }
    else if (!strcmp(operation, extract)){
      //call extract
      extract_files_from_archive(archiveName);
    }
    else {
      //if not valid command, say so and do nothing
      printf("Invalid command.\n");
    }

    file_list_clear(&files);
    return 0;
}
