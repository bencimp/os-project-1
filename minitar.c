#include <fcntl.h>
#include <grp.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "minitar.h"

#define NUM_TRAILING_BLOCKS 2
#define MAX_MSG_LEN 512
#define DEBUG 0

//While I was going to write helper functions, I was unsure if they would work with the header provided, as this is the first class I've had to use C for. If I have time, I'll clean them up later.

/*
 * Helper function to compute the checksum of a tar header block
 * Performs a simple sum over all bytes in the header in accordance with POSIX
 * standard for tar file structure.
 */
void compute_checksum(tar_header *header) {
    // Have to initially set header's checksum to "all blanks"
    memset(header->chksum, ' ', 8);
    unsigned sum = 0;
    char *bytes = (char *)header;
    for (int i = 0; i < sizeof(tar_header); i++) {
        sum += bytes[i];
    }
    snprintf(header->chksum, 8, "%7o", sum);
}

/*
 * Populates a tar header block pointed to by 'header' with metadata about
 * the file identified by 'file_name'.
 * Returns 0 on success or 1 if an error occurs
 */
int fill_tar_header(tar_header *header, const char *file_name) {
    memset(header, 0, sizeof(tar_header));
    char err_msg[MAX_MSG_LEN];
    struct stat stat_buf;
    // stat is a system call to inspect file metadata
    if (stat(file_name, &stat_buf) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to stat file %s", file_name);
        perror(err_msg);
        return 1;
    }

    strncpy(header->name, file_name, 100); // Name of the file, null-terminated string
    snprintf(header->mode, 8, "%7o", stat_buf.st_mode & 07777); // Permissions for file, 0-padded octal

    snprintf(header->uid, 8, "%7o", stat_buf.st_uid); // Owner ID of the file, 0-padded octal
    struct passwd *pwd = getpwuid(stat_buf.st_uid); // Look up name corresponding to owner ID
    if (pwd == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up owner name of file %s", file_name);
        perror(err_msg);
        return 1;
    }
    strncpy(header->uname, pwd->pw_name, 32); // Owner  name of the file, null-terminated string

    snprintf(header->gid, 8, "%7o", stat_buf.st_gid); // Group ID of the file, 0-padded octal
    struct group *grp = getgrgid(stat_buf.st_gid); // Look up name corresponding to group ID
    if (grp == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up group name of file %s", file_name);
        perror(err_msg);
        return 1;
    }
    strncpy(header->gname, grp->gr_name, 32); // Group name of the file, null-terminated string

    snprintf(header->size, 12, "%11o", (unsigned)stat_buf.st_size); // File size, 0-padded octal
    snprintf(header->mtime, 12, "%11o", (unsigned)stat_buf.st_mtime); // Modification time, 0-padded octal
    header->typeflag = REGTYPE; // File type, always regular file in this project
    strncpy(header->magic, MAGIC, 6); // Special, standardized sequence of bytes
    memcpy(header->version, "00", 2); // A bit weird, sidesteps null termination
    snprintf(header->devmajor, 8, "%7o", major(stat_buf.st_dev)); // Major device number, 0-padded octal
    snprintf(header->devminor, 8, "%7o", minor(stat_buf.st_dev)); // Minor device number, 0-padded octal

    compute_checksum(header);
    return 0;
}

/*
* Removes 'nbytes' bytes from the file identified by 'file_name'
* Returns 0 upon success, -1 upon error
* Note: This function uses lower-level I/O syscalls (not stdio), which we'll learn about later
*/
int remove_trailing_bytes(const char *file_name, size_t nbytes) {
    char err_msg[MAX_MSG_LEN];
    // Note: ftruncate does not work with O_APPEND
    int fd = open(file_name, O_WRONLY);
    if (fd == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to open file %s", file_name);
        perror(err_msg);
        return 1;
    }
    //  Seek to end of file - nbytes
    off_t current_pos = lseek(fd, -1 * nbytes, SEEK_END);
    if (current_pos == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to seek in file %s", file_name);
        perror(err_msg);
        close(fd);
        return 1;
    }
    // Remove all contents of file past current position
    if (ftruncate(fd, current_pos) == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to truncate file %s", file_name);
        perror(err_msg);
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}


int create_archive(const char *archive_name, const file_list_t *files) {
    // TODO: Not yet implemented. Due Friday 2/4.
  if(DEBUG) printf("Entered create_archive function...\n");

  /*
    For each file in the archive, we need to:
    Generate a header
    Add the header to the archive file
    Digest the file in half kilobyte (512-byte) chunks, adding each to the file, stopping at the last
    For the last chunk, pad it with empty bytes until it reaches the 512 number, then push it to the file
    After that, repeat the first step (header chunk generation)
    When every file has been generated, add a kilobyte of zeros to the end (two 512-byte chunks), then close the file.
   */

  //Open the new archive file
  FILE *archiveOut = fopen(archive_name, "wb");

  //Get the first element of the linked list
  node_t *file = files->head;

  //For each file in the archive
  while(file != NULL){
    //get the next file
    //if(DEBUG)printf("Inside of while loop...\n");
    //file = file->next;
    //Generate a header
    if(DEBUG) printf("Generating header...\n");
    tar_header myHeader;
    fill_tar_header(&myHeader, file->name);

    //make a char buffer to hold the bytes (move this into the inner loop so that it always works?)
    //unsigned char buffer[512] = {0};

    //Add the header to the archive file
    if(DEBUG)printf("Adding header to archive file...\n");
    fwrite(&myHeader,sizeof(myHeader),1,archiveOut);

    //Open the file, reading in binary
    if(DEBUG)printf("Opening file, reading in binary mode...\n");
    FILE *fileIn = fopen(file->name, "rb");

    //Get file size and number of blocks that are going to be read (minus the one that will be padded)
    if(DEBUG)printf("Getting file size...\n");
    fseek(fileIn, 0, SEEK_END);
    int sz = ftell(fileIn);
    fseek(fileIn, 0, SEEK_SET);

    //int sizeOfLastBlock = sz % 512; //Possibly superfluous?
    int numberOfBlocks = (sz / 512)+1; //Probably good enough on its own
    if(DEBUG)printf("Number of blocks: %d.\n",numberOfBlocks);

    //Iterate through the file, 512 bytes at a time
    for (int x = 0; x < numberOfBlocks; x++){
      unsigned char buffer[512] = {0};
      fread(buffer, sizeof(buffer), 1, fileIn);
      //for (unsigned int y = 0; y < 512; y ++){
      //printf(" %s," buffer[y]);
      //}
      printf("\n\n%s", buffer);
      fwrite(&buffer, sizeof(buffer), 1, archiveOut);
    }
    //If there are 512 bytes or more left in the file, print them into the file

    //If there are fewer than 512 bytes but more than zero remaning, print them into the file padded with zeros to make the block size match up
    //Unneccessary? since we are able to just do the above stuff 
    if(DEBUG)printf("Getting next file...\n");
    file = file->next;
    //End loop
  }
  //Finally, add the end file blocks (two blocks of zeroes)
  unsigned char endBlock[1024] = {0};
  fwrite(&endBlock,sizeof(endBlock),1,archiveOut);

  //Close file
  fclose(archiveOut);

  return 0;
}

int append_files_to_archive(const char *archive_name, const file_list_t *files) {
  // TODO: Not yet implemented. Due Friday 2/4.
  if(DEBUG) printf("Entered append_files_to_archive function...\n");

  //the following logic should truncate all of the remaining bytes.
  if(DEBUG) printf("Checking for empty bytes...\n");
  FILE *archiveCheck = fopen(archive_name, "rb");
  char emptyBuffer[1024] = {0};
  char checkBuffer[1024] = {0};
  fseek(archiveCheck, -1024, SEEK_END);
  fread(checkBuffer, sizeof(checkBuffer), 1, archiveCheck);
  if(!(strcmp(emptyBuffer, checkBuffer))){
    remove_trailing_bytes(archive_name, 1024);
  }
  fclose(archiveCheck);
  if(DEBUG) printf("Truncated trailing bytes.\n");



  /*
    For each file in the archive, we need to:
    Generate a header
    Add the header to the archive file
    Digest the file in half kilobyte (512-byte) chunks, adding each to the file, stopping at the last
    For the last chunk, pad it with empty bytes until it reaches the 512 number, then push it to the file
    After that, repeat the first step (header chunk generation)
    When every file has been generated, add a kilobyte of zeros to the end (two 512-byte chunks), then close the file.
   */

  //Open the new archive file
  FILE *archiveOut = fopen(archive_name, "ab");
  
  //Get the first element of the linked list
  node_t *file = files->head;
  
  fseek(archiveOut, 0, SEEK_END);
  if(DEBUG) printf("archiveOut opened.\n");
  //For each file in the archive
  while(file != NULL){
    //get the next file
    //if(DEBUG)printf("Inside of while loop...\n");
    //file = file->next;
    //Generate a header
    if(DEBUG) printf("Generating header...\n");
    tar_header myHeader;
    fill_tar_header(&myHeader, file->name);

    //make a char buffer to hold the bytes (move this into the inner loop so that it always works?)
    //unsigned char buffer[512] = {0};

    //Add the header to the archive file
    if(DEBUG)printf("Adding header to archive file...\n");
    fwrite(&myHeader,sizeof(myHeader),1,archiveOut);

    //Open the file, reading in binary
    if(DEBUG)printf("Opening file, reading in binary mode...\n");
    FILE *fileIn = fopen(file->name, "rb");

    //Get file size and number of blocks that are going to be read (minus the one that will be padded)
    if(DEBUG)printf("Getting file size...\n");
    fseek(fileIn, 0, SEEK_END);
    int sz = ftell(fileIn);
    fseek(fileIn, 0, SEEK_SET);

    //int sizeOfLastBlock = sz % 512; //Possibly superfluous?
    int numberOfBlocks = (sz / 512)+1; //Probably good enough on its own
    if(DEBUG)printf("Number of blocks: %d.\n",numberOfBlocks);

    //Iterate through the file, 512 bytes at a time
    for (int x = 0; x < numberOfBlocks; x++){
      unsigned char buffer[512] = {0};
      fread(buffer, sizeof(buffer), 1, fileIn);
      //for (unsigned int y = 0; y < 512; y ++){
      //printf(" %s," buffer[y]);
      //}
      printf("\n\n%s", buffer);
      fwrite(&buffer, sizeof(buffer), 1, archiveOut);
    }
    //If there are 512 bytes or more left in the file, print them into the file

    //If there are fewer than 512 bytes but more than zero remaning, print them into the file padded with zeros to make the block size match up
    //Unneccessary? since we are able to just do the above stuff 
    if(DEBUG)printf("Getting next file...\n");
    file = file->next;
    //End loop
  }
  //Finally, add the end file blocks (two blocks of zeroes)
  unsigned char endBlock[1024] = {0};
  fwrite(&endBlock,sizeof(endBlock),1,archiveOut);

  //Close file
  fclose(archiveOut);

  return 0;
}

int get_archive_file_list(const char *archive_name, file_list_t *files) {
    // TODO: Not yet implemented
  if(DEBUG) printf("Entered get_archive_file_list function...\n");

  FILE *archiveIn = fopen(archive_name, "rb");

  //This code copied from the check for appending files to the archive. It will have to be edited, obviously, but it's fit for purpose right now.
  char emptyBuffer[1024] = {0};
  /*char checkBuffer[1024] = {0};
  fseek(archiveCheck, -1024, SEEK_END);
  fread(checkBuffer, sizeof(checkBuffer), 1, archiveCheck);
  if(!(strcmp(emptyBuffer, checkBuffer))){
    remove_trailing_bytes(archive_name, 1024);
  }
  fclose(archiveCheck);*/

  //Condition for this while loop TBD, but should be pretty clear once the loop body is written
  //For each file,
  while(1){
   //Make a buffer to hold the information and another to hold the size;
    char nameBuffer[100] = {0};
    char sizeBuffer[12] = {0};

    //Read in the first 100 bytes as the character name
    fread(nameBuffer, sizeof(nameBuffer), 1, archiveIn);

    //printf("%s\n", nameBuffer);
    //If the name is empty, break
    if (DEBUG) printf("Comparing strings to empty string ...\n");
    if (!(strcmp(nameBuffer, emptyBuffer))){
      break;
    }

    //Print it out to the terminal
    if(DEBUG) printf("%s\n", nameBuffer);

    //Re-read the writeup: we're not supposed to do that. Instead, push it to the file list
    file_list_add(files, nameBuffer);

    /*char breaker = '\0';
    int n = 0;
    while (nameBuffer[n] != breaker){
      printf("%c", nameBuffer[n]);
      n ++;
    }
    printf("\n");
    */
    //Read the file size. We can't use the same cheat that we used previously, because we are just one file. As a result, we have to instead read the file size from the header and interpret the octal and turn it into a decimal number, then skip forward that many blocks of 512 bytes, rounded up.
    fseek(archiveIn, 24, SEEK_CUR);
    int preConversion, postConversion, holder, count;
    postConversion = 0;
    count = 0;
    fread(sizeBuffer, sizeof(sizeBuffer), 1, archiveIn);
    if (DEBUG) printf("Pre-conversion octal: %s\n", sizeBuffer);
    preConversion = atoi(sizeBuffer);
    //This code converts the number from octal to decimal
    while(preConversion > 0){
      holder = preConversion % 10;
      preConversion /= 10;
      postConversion += (holder * pow(8, count));
      count ++;
    }

    if (DEBUG) printf("Post-conversion to Octal: %d\n", postConversion);
    int skipBytes = postConversion + (512-(postConversion%512));

    //Skip the appropriate number of bytes to get to the next header, which should be the remaining length of the header plus filesize divided by 512 rounded up
    fseek(archiveIn, (skipBytes + 376), SEEK_CUR);
    //Loop
    //in this case, it's entirely passive, this loop relying on being broken out of by 
    

  }
  return 0;
}

int extract_files_from_archive(const char *archive_name) {
    // TODO: Not yet implemented
  
  //This is basically just an amalgamation of the other functions I've written

  //Open the archive file
  FILE *archiveIn = fopen(archive_name, "rb");
  char emptyBuffer[1024] = {0};
  while(1){
    //Find the name, fopen that file in clear overwrite mode
    //Make a buffer to hold the information and another to hold the size;
    char nameBuffer[100] = {0};
    char sizeBuffer[12] = {0};
    
    //Read in the first 100 bytes as the character name
    fread(nameBuffer, sizeof(nameBuffer), 1, archiveIn);
    if (DEBUG) printf("Comparing strings to empty string ...\n");
    if (!(strcmp(nameBuffer, emptyBuffer))){
      break;
    }
    
    //Find the length, store it
    fseek(archiveIn, 24, SEEK_CUR);
    int preConversion, postConversion, holder, count;
    postConversion = 0;
    count = 0;
    fread(sizeBuffer, sizeof(sizeBuffer), 1, archiveIn);
    if (DEBUG) printf("Pre-conversion octal: %s\n", sizeBuffer);
    preConversion = atoi(sizeBuffer);
    //This code converts the number from octal to decimal
    while(preConversion > 0){
      holder = preConversion % 10;
      preConversion /= 10;
      postConversion += (holder * pow(8, count));
      count ++;
    }
    
    if (DEBUG) printf("Post-conversion to Octal: %d\n", postConversion);
    int skipBytes = (512-(postConversion%512));
    
    //Go to the end of the header
    fseek(archiveIn, 376, SEEK_CUR);
    //read in bytes to match the file length and write it all to the file
    FILE *outFile = fopen(nameBuffer, "wb");
    //Read the remaining bytes
    char buffer[1];
    for(int x = 0; x < postConversion; x ++){
      fread(buffer, sizeof(buffer), 1, archiveIn);
      fwrite(buffer, sizeof(buffer), 1, outFile);
    }
    
    fseek(archiveIn, skipBytes, SEEK_CUR);
    //loop it good
  }
    return 0;
}

