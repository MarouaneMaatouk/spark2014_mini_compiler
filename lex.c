#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ada.h"

//Lexical elements----------------------------------
// numeric | comment | strings | identifier | specialKey(Op)
// TODO: [redefine operators], pragma, add based_literal
//--------------------------------------------------
char * str;
int i = 0;


//Translate macro value to macro name:
void getMacro() {
  if(token.type == EOF) {
    printf("T_EOF\n");
    return;
  }
  FILE *f = fopen("ada.h" , "r");
  char buffer[400];
  while (fgets(buffer, sizeof(buffer), f) != NULL) {
       char identifier[400];
       if(token.type >= 257 && token.type <= 804) {
         int valueP;
         if (sscanf (buffer, "#define %s %d", identifier, &valueP) == 2) {
           if(token.type == valueP) {
             printf("%s\n", identifier);
             break;
           }
         }
       }else {
         char valueC;
         if (sscanf (buffer, "#define %s '%c'", identifier, &valueC) == 2) {
           if(token.type == valueC) {
             printf("%s\n", identifier);
             break;
           }
         }
       }

    }
}
//Check if a str is a reserved keyword
int isReserved(char * strn) {
  FILE *f = fopen("ada.h" , "r");
  char buffer[400];
  while (fgets(buffer, sizeof(buffer), f) != NULL) {
       char identifier[400];
       int valueP;
       if (sscanf (buffer, "#define T_%s %d", identifier, &valueP) == 2) {
           if(strcmp(strn, identifier) == 0) {
             return valueP;
           }
       }
    }
    return 0;
}

void rmSpace() {
  char c = fgetc(fp);
  while(c == ' ' || c == '\n'|| c == '\t'|| c == '\r')
    c = fgetc(fp);
  ungetc(c, fp);
}

// operators:----------------------------

void operators() {
  rmSpace();
  memset(str, '\0' , 1024);
  i = 0;
  char c = fgetc(fp);

  if (c == EOF) {
    token.type = c;
    strcpy(token.val.stringValue , "EOF");
    return;
  }
  if(c == '<' || c == '>' || c == '/' || c == ':' || c == '*') {
    str[i] = c; i++;
    char cc  = fgetc(fp);
    if(cc == '=') {
      str[i] = cc; i++;

      if(strcmp(":=",str) == 0) token.type = 800;
      else if(strcmp("<=",str) == 0) token.type = 801;
      else if(strcmp(">=",str) == 0) token.type = 802;
      else if(strcmp("/=",str) == 0) token.type = 803;
    }
    else if(cc == '*'){
      str[i] = cc; i++;
      token.type = 804;
    }
    else {
      ungetc(cc,fp);
      token.type = c;
    }
    strcpy(token.val.stringValue,str);
  }
  else {
    switch (c) {
      case ',' :  case '.': case ';' : case '+' : case '-' :
      case '/' : case '(' : case ')' : case '*' : case '&':
                str[i] = c; i++;
                token.type = c;
                strncpy(token.val.stringValue,str,1);
                break;
      default:
              ungetc(c,fp);
    }
  }
  memset(str, '\0' , 1024);
}
// T_NUMERIC
// numeric_literal ::= decimal_literal [| based_literal]
//decimal_literal ::= numeral [ "." numeral ] [ exponent ]
//exponent ::= ( "E" [ "+" ] numeral ) | ( "E" "-" numeral )


int numeral() {
  int result = 0;
  char c = fgetc(fp);

  while(isdigit(c)) {
    result = 1;
    str[i] = c; i++;
    c = fgetc(fp);

    if(c == '_') {
      str[i] = c; i++;
      c = fgetc(fp);
      if(isdigit(c)) continue;
      else {result = 0; break;}
    }
    else if(isdigit(c)) continue;
    else {
      ungetc(c,fp);
      break;
    }
  }
  if(result == 0) {
    ungetc(c,fp);
    //printf("%c\n", fgetc(fp));
    /*for(j = strlen(str); j >= 0; j--) {
      ungetc(str[j],fp);
      memset(str, '\0', 1024);
      i = 0;
    }*/
  }
  return result;
}

int decimal_literal() {
  rmSpace();
  i = 0;
  int result = numeral();
  char c = fgetc(fp);
  if(c == '.' && result == 1) {
    str[i] = c; i++;
    result = numeral();
    if(result == 0) {
      ungetc('.',fp);
      str[i-1] = '\0';
    }
    result = 1;
  }
  if (c == 'E' && result == 1) {          //exponent case
    str[i] = c; i++;
    c = fgetc(fp);
    if(c == '+' || c == '-') {
      str[i] = c; i++;
      result = numeral();
    }
    else {
      result = numeral();
    }
  }
  if(result == 1) {
    token.type = T_NUMERIC;
    strcpy(token.val.stringValue,str);
  }else ungetc(c,fp);

  memset(str, '\0' , 1024);
  return result;
}


// comment ::= --.*\n
int comment() {
  rmSpace();
  str = calloc(1024 , sizeof(char));
  i = 0;
  char c = fgetc(fp);
  if(c == '-'){
    str[i] = c; i++;
    c = fgetc(fp);
    if(c == '-') {
      str[i] = c; i++;
      while(c != '\n') {
        c = fgetc(fp);
        str[i] = c; i++;
      }
      //str[i] = '\0';
      token.type = T_COMMENT;
      strcpy(token.val.stringValue,str);
    }
    else {
      ungetc(c,fp);
      str[i] = '\0';
    }
  }else{
    ungetc(c,fp);
  }
  memset(str, '\0' , 1024);
}

// string_literal ::= "quotation mark" { string_element } "quotation mark"
// string_element ::= "pair of quotation mark" | graphic_character

int string_element() {
  char c = fgetc(fp);
  if(c == '"'){
    str[i] = c; i++;
    c = fgetc(fp);
    if(c == '"'){
      str[i] = c; i++;
      string_element();
    }else {
      i = i-1;
      ungetc(c,fp);
      ungetc('"',fp);
      return -1;
    }
  }
  else{
    while(c != '"' && c != '\n') {
      str[i] = c; i++;
      c = fgetc(fp);
    }
    ungetc(c,fp);
    if(c == '"') {
      string_element();
    }

  }
}

int string_literal() {
    rmSpace();
    i = 0;
    char c = fgetc(fp);
    if(c == '"') {
      str[i] = c; i++;
      string_element();
      c = fgetc(fp);

      if(c == '"') {
        str[i] = c; i++;
        token.type = T_STRING;
        strcpy(token.val.stringValue,str);
        memset(str, '\0' , 1024);
        return 1;
      }
      else {
        int j;
        for(j = strlen(str); j >= 0; j--) {
          ungetc(str[j],fp);
        }
      }
    }
    ungetc(c,fp);
    memset(str, '\0' , 1024);
    return -1;
}



//id ::= letter {["_"] (letter | digit)}
//identifier() verify identifier & reserved Keywords
int identifier() {
  rmSpace();
  i = 0;
  char c = fgetc(fp);
  if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
    str[i] = c; i++;
    c = fgetc(fp);
    while((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || isdigit(c)) {
      str[i] = c; i++;
      c = fgetc(fp);
      if(c == '_') {
        str[i] = c; i++;
        c = fgetc(fp);
      }
      else {
        continue;
      }
    }
    int valueP = isReserved(str);
    if(valueP) token.type = valueP;
    else token.type = T_IDENTIFIER;
    strcpy(token.val.stringValue,str);
    ungetc(c ,fp);
  }
  else {
    ungetc(c ,fp);
  }
  memset(str, '\0' , 1024);
}



int scanToken() {
  if(fp == NULL) return -1;
  token.type = T_UNKNOWN;
  memset(token.val.stringValue , '\0' , 100);

  if(token.type == T_UNKNOWN) decimal_literal();
  if(token.type == T_UNKNOWN) identifier();
  if(token.type == T_UNKNOWN) string_literal();
  if(token.type == T_UNKNOWN) comment();
  if(token.type == T_UNKNOWN) operators();
}






int main(int argc, char const *argv[]) {

  fp = fopen("dummy.txt" , "r+");
  str = calloc(1024 , sizeof(char));

  do {
    scanToken();
    //printf("value:: %s\n", token.val.stringValue);
    getMacro();

  }while(token.type != T_UNKNOWN && token.type != T_EOF);

  //printf("nextChar:: %c\n", fgetc(fp));

  return 0;
}
