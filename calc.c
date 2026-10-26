// Atabay Atabaev 251ADB244
// https://github.com/Atabay123/lab6_C.git
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: I am using Google AI Studio, chat link: 

//here i define the POSIX interface, so below I can use POSIX declarations 
#define _POSIX_C_SOURCE 200809L

//I need these headers, because I will work with: i/o, dynamic memory, string, charcter classifcation, error informations, work with directory, and some job with POSIX interface
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

//these will be reused, so thats why I keep it here in one place
#define FIRST_NAME "atabay"
#define LAST_NAME  "atabaev"
#define STUDENT_ID "251adb244"

//I use these tokens with names, so the tokenizer and parser can differentiate the numbers, operators, end of lnes and bad inputs
enum { T_INT, T_PLUS, T_MINUS, T_END, T_BAD };


//I made it global, so the different functions can process the same input consistently, like input buffer, tokenizer state, and error infromation
static char *buf;
static long len;
static long p;        
static long line_end; 
static int tok;       
static long tok_pos;   
static long long tok_int;
static int failed;     
static long fail_pos;

typedef struct {
    long long i;
    long start; 
} Value;


//I only save the first error, since i need to report where exactly the error in input occured, instead of replacing with later error
static void fail(long pos) {
    if (!failed) {
        failed = 1;
        fail_pos = pos;
    }
}
//I asked AI Google AI Studio, like to give some idea to do tokenizer

//i read next token here so parser can work with numbers and operators instead of checking every character itself
static void next_token(void) {
    //I skip spaces and tabs here, since they should not impact the expression
    while (p < line_end && (buf[p] == ' ' || buf[p] == '\t')) p++;

    // i use T_END when there is nothing else on this line, so parser will know thatit reached the end of expression
    if (p >= line_end) {
        tok = T_END;
        tok_pos = line_end + 1;
        return;
    }

    //i keep here the position of token, so it posssible to point to character if it will be clear that it is invalid
    tok_pos = p + 1;
    char c = buf[p];
    if (c == '+') { tok = T_PLUS; p++; }
    else if (c == '-') { tok = T_MINUS; p++; }
    else if (isdigit((unsigned char)c)) {
        //I build number digit by digit, because the input contains some text
        tok_int = 0;
        while (p < line_end && isdigit((unsigned char)buf[p])) {
            tok_int = tok_int * 10 + (buf[p] - '0');
            p++;
        }
        tok = T_INT;
    }
    //i mark any other charactrs aas bad, so parser can report error instead of treating it as valid token
    else { tok = T_BAD; p++; }  
}

// I handle single number, so the parse_expr can then focus on combining numbers witg operators
static Value parse_primary(void) {
    Value v = {0};
    //need to remember where this starte in case if it will be needed later
    v.start = tok_pos;
    if (tok == T_INT) {
        v.i = tok_int;
        next_token();
    } else {
       
        //i report an error when number is expected, but current token not number
        fail(tok_pos);
    }
    return v;
}



//i process additions and subtractions from left to right using value already calculated as the left side of the next operation
static Value parse_expr(void) {
    Value left = parse_primary();
    if (failed) return left;

    //I keep processing while there is another plus or minus, so expressions with several operations can be managed
    while (tok == T_PLUS || tok == T_MINUS) {
        int op = tok;
        next_token();
        Value right = parse_primary();
        if (failed) return left;

         //I check which operator was saved before reading the next number, because next_token changes the current token
        if (op == T_PLUS) left.i = left.i + right.i;
        else left.i = left.i - right.i;
    }
    return left;
}


static char *out_text;
static long out_len;
static long out_cap;

//I just use bigger buffer when result becomes very long, so no need to guess the maximum size in advance
static void add_text(const char *s) {
    long n = strlen(s);

        //i increase the buffer size when there is not enough space, because the output can contain many lines
    if (out_len + n + 1 > out_cap) {
        while (out_len + n + 1 > out_cap) out_cap = out_cap * 2;
        char *bigger = realloc(out_text, out_cap);
        if (bigger == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            exit(1);
        }
        out_text = bigger;
    }

     //i copy the new result after the previous one and update the length so the next resilt goes in the right place
    memcpy(out_text + out_len, s, n + 1);
    out_len = out_len + n;
}



//I take just the file name from the path because I need the output file name to be based on the input file, not its whole path
static void base_name(const char *path, char *out, int strip_txt) {
    long end = strlen(path);

    //here just remove the / so the path ending with / not make any issue
    while (end > 1 && path[end - 1] == '/') end--;
    long start = end;
    while (start > 0 && path[start - 1] != '/') start--;
    long n = end - start;
    if (n > 255) n = 255;
    memcpy(out, path + start, n);
    out[n] = '\0';

    // I remove the txt extension when requested so it does not appear two time in the output file name
    if (strip_txt && n > 4 && strcmp(out + n - 4, ".txt") == 0) out[n - 4] = '\0';
}

static int process_file(const char *in_path, const char *out_dir) {
    // i check the file opened before reading it, because otherwise there is no input to be processed
    FILE *f = fopen(in_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: cannot open '%s': %s\n", in_path, strerror(errno));
        return 1;
    }
   
        //first i find the file size so I can allocate enough memory to read the input
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);
    buf = malloc(len + 1);
    if (buf == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        fclose(f);
        return 1;
    }
    len = fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    out_cap = 1024;
    out_len = 0;
    out_text = malloc(out_cap);
    if (out_text == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        free(buf);
        return 1;
    }
    out_text[0] = '\0';

    failed = 0;
    long cursor = 0;
    while (cursor < len && !failed) {
        long start = cursor;
        while (cursor < len && buf[cursor] != '\n') cursor++;
        line_end = cursor;
        if (cursor < len) cursor++;  
        p = start;
        next_token();
        Value v = parse_expr();

        if (!failed && tok != T_END) fail(tok_pos);
        if (failed) break;

        char text[64];
        snprintf(text, sizeof text, "%lld\n", v.i);
        add_text(text);
    }

    char base[256];
    base_name(in_path, base, 1);
    char out_path[4096];
    snprintf(out_path, sizeof out_path, "%s/%s_%s_%s_%s.txt",
             out_dir, base, FIRST_NAME, LAST_NAME, STUDENT_ID);

    int status = 0;
    FILE *out = fopen(out_path, "w");
    if (out == NULL) {
        fprintf(stderr, "Error: cannot write '%s': %s\n", out_path, strerror(errno));
        status = 1;
    } else {
       
        if (failed) fprintf(out, "ERROR:%ld\n", fail_pos);
        else fwrite(out_text, 1, out_len, out);
        if (ferror(out) || fclose(out) != 0) {
            fprintf(stderr, "Error: write failed for '%s'\n", out_path);
            status = 1;
        }
    }
    free(buf);
    free(out_text);
    return status;
}

//i  handle the command line arguments here , so the user can specify the output directory and input file when running the program
int main(int argc, char **argv) {
    const char *out_dir = NULL;
    const char *in_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) out_dir = argv[++i];
        else in_path = argv[i];
    }


      //check that both required arguments were provided so the program does not try to process a missing path
    if (out_dir == NULL || in_path == NULL) {
        fprintf(stderr, "Usage: %s -o OUTDIR input.txt\n", argv[0]);
        return 1;
    }

    if (mkdir(out_dir, 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error: cannot create '%s': %s\n", out_dir, strerror(errno));
        return 1;
    }
    return process_file(in_path, out_dir);
}