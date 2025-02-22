#include "driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lexer.h"

#define PREPROCESSED_EXTENSION 'i'
#define ASSEMBLY_EXTENSION 'S'

void ChangeFileExtension(char *out_file, char extension) {
  while (*out_file != '\0') {
    ++out_file;
  }
  --out_file;
  *out_file = extension;
}

void RemoveFileExtension(char *out_file) {
  while (*out_file != '\0') {
    ++out_file;
  }
  out_file -= 2;
  *out_file = '\0';
}

void Preprocess(char *file_name) {
  int rc = fork();
  char *file_name_copy = strdup(file_name);
  char *outfile = strdup(file_name);
  // Swap the file `extension type to .i from .c
  ChangeFileExtension(outfile, PREPROCESSED_EXTENSION);
  if (rc < 0) {
    fprintf(stderr, "Failed to preprocess the file");
    exit(2);
  }
  if (rc == 0) {
    char *argv[] = {"gcc", "-E", "-P", file_name_copy, "-o", outfile, NULL};
    execvp("/usr/bin/gcc", argv);
  } else {
    wait(&rc);
  }
  free(file_name_copy);
  free(outfile);
}

// Replace with actual compiler implementation eventually
void InternalCompile(char *file_name) {
  FILE *fp = fopen(file_name, "r");
  struct TokenList token_list = Lex(fp);
  struct Token last_token = token_list.tokens[token_list.length - 1];
  if (last_token.type != tEof) {
    fprintf(stderr, "Failed to compile, got last token type %d and val %s",
            last_token.type, last_token.value);
    exit(2);
  }
}

void AssembleAndLink(char *file_name) {
  int rc = fork();
  char *file_name_copy = strdup(file_name);
  char *outfile = strdup(file_name);
  // Swap the file `extension type to .i from .c
  RemoveFileExtension(outfile);
  if (rc < 0) {
    fprintf(stderr, "Failed to preprocess the file");
    exit(2);
  }
  if (rc == 0) {
    char *argv[] = {"gcc", file_name_copy, "-o", outfile, NULL};
    execvp("/usr/bin/gcc", argv);
  } else {
    wait(&rc);
  }
  free(file_name_copy);
  free(outfile);
}

void CleanTemporaryFiles(char *file_name) {

}

void Compile(char *file_name) {
  Preprocess(file_name);
  ChangeFileExtension(file_name, PREPROCESSED_EXTENSION);
  InternalCompile(file_name);
  ChangeFileExtension(file_name, ASSEMBLY_EXTENSION);
  AssembleAndLink(file_name);
}
