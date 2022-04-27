#ifndef READFILE_H
#define READFILE_H

#include <stdio.h>
#include <string.h>

struct text_obj {
    char ** text; 
    int numOfLines;
};
typedef struct text_obj Text_obj;

Text_obj* text_obj_destroy(Text_obj* self) {
   for (int i = 0; i < self->numOfLines; i++) {
      free(self->text[i]);
   } 
   free(self->text);
   free(self);
   return NULL;
}

Text_obj* file_get_lines(){
   FILE *fp;
   int numOfLines = 0;
   int counter = 0;
   int ch=0;
   fp = fopen("userRecords.txt" , "r"); //open a file for reading
   while(!feof(fp)){
        ch = fgetc(fp);
        if(ch == '\n')
        {
          numOfLines++;
        }
   }
   fclose(fp);
   
   Text_obj* text_obj = malloc(sizeof(Text_obj));
   
   char **text = malloc(numOfLines * sizeof(char*)); // Allocate row pointers
    for(int i = 0; i < numOfLines; i++)
      text[i] = malloc(512 * sizeof(char));  // Allocate each row separately 
   
   fp = fopen("userRecords.txt" , "r"); //open a file in reading mode
   if(fp == NULL) {
      perror("Error opening file");
      return NULL;
   }
   while( fgets (text[counter], 128, fp)!=NULL ) {
      // Removes trailing newline
        if ((strlen(text[counter]) > 0) && (text[counter][strlen (text[counter]) - 1] == '\n')){
            text[counter][strlen (text[counter]) - 1] = '\0';
        }
      counter++;
   }
   fclose(fp);
   
   text_obj->text = text;
   text_obj->numOfLines = numOfLines;
   
   
   return text_obj;
    
}

void file_insert_lines(char newLine []){
   FILE *fp;
   fp = fopen("userRecords.txt" , "a"); //open a file in append mode
   fprintf(fp, "%s\n", newLine);
   fclose(fp);
    
}

#endif
