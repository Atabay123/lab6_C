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
#include <dirent.h> //this i add, because it is needed to work with directories

//these will be reused, so thats why I keep it here in one place
#define FIRST_NAME "atabay"
#define LAST_NAME  "atabaev"
#define STUDENT_ID "251adb244"

//I use these tokens with names, so the tokenizer and parser can differentiate the numbers, operators, end of lnes and bad inputs
// i have added two tokens, since the parser need also wrok with multiolication and division
//added two more tokens to work with parenthesises
//i have added one more token, so can work with power
enum { T_INT, T_PLUS, T_MINUS, T_STAR, T_SLASH, T_POW, T_LP, T_RP, T_END, T_BAD };


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
    //  i add it, because last version had only for minus and plus
    else if (c == '*') {
    if (p + 1 < line_end && buf[p + 1] == '*') {
        tok = T_POW;
        p += 2;
    }
    else { tok = T_STAR; p++; }
 }// this done, in order to make rogram differentiate between multiplication and power
     else if (c == '/') { tok = T_SLASH; p++; }
       else if (c == '(') { tok = T_LP; p++; } //i add it, so can work with parentheses, i got some idea from Google AI Studio
     else if (c == ')') { tok = T_RP; p++; }
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

static Value parse_primary(void);
static Value parse_power(void);
//i made separate function for multiplication and division ,so they can be calculated before addition and subtraction
static Value parse_term(void) {
    Value left = parse_power();
    if (failed) return left;
    while (tok == T_STAR || tok == T_SLASH) {
        int op = tok;
        next_token();
        Value right = parse_power();
        if (failed) return left;
        if (op == T_STAR) {
            left.i = left.i * right.i;
        } else {
            if (right.i == 0) {//division by zero not allowed
                fail(right.start);
                return left;
            }
            left.i = left.i / right.i;
        }
    }
    return left;
}
static Value parse_expr(void);

// I handle single number, so the parse_expr can then focus on combining numbers witg operators
static Value parse_primary(void) {
    Value v = {0};
    long start = tok_pos;
    //need to remember where this starte in case if it will be needed later
    v.start = start;
    if (tok == T_INT) {
        v.i = tok_int;
        next_token();
    } else if (tok == T_LP) { //if there is new parenthesis, need work with it first
    next_token();
    v = parse_expr();
    if (failed) return v;

    v.start = start;
    if (tok == T_RP) {
        next_token();
    } else {
        fail(tok_pos);
    }
} else {
        fail(tok_pos);
    }
    return v;
}

static long long int_pow(long long base, long long exp) {
    long long result = 1;
    while (exp > 0) {
        if (exp & 1) result = result * base;
        exp = exp >> 1;
        if (exp > 0) base = base * base;
    }
    return result;
}// added to do the power


//it works with power, makes power rigth associative
static Value parse_power(void) {
    Value left = parse_primary();
    if (failed) return left;
    if (tok == T_POW) {
        next_token();
        Value right = parse_power();
        if (failed) return left;
        if (right.i < 0) {
            fail(right.start);
            return left;
        }
        left.i = int_pow(left.i, right.i);
    }
    return left;
}


//i process additions and subtractions from left to right using value already calculated as the left side of the next operation
//and also i changed it to use the parse_term, so the * and / can be used before + and -
static Value parse_expr(void) {
    Value left = parse_term();
    if (failed) return left;

    //I keep processing while there is another plus or minus, so expressions with several operations can be managed
    while (tok == T_PLUS || tok == T_MINUS) {
        int op = tok;
        next_token();
        Value right = parse_term();
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

    long k = start;
     while (k < line_end && (buf[k] == ' ' || buf[k] == '\t')) k++;

     if (k == line_end || buf[k] == '#') continue;

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
    //here i save input directory, output directory, input file if is provided
    const char *dir_arg = NULL;
    const char *out_arg = NULL;
    const char *in_arg = NULL;


//here i check all the arguments provided by user
    for (int i = 1; i < argc; i++) {

         //if user wants to process a folder, i save its path here
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
                        if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value after %s\n", argv[i]);
                return 1;
        }
           dir_arg = argv[++i];
    }else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 >= argc) {// i need it, otherwise program dont know where to save results
                fprintf(stderr, "Error: missing value after %s\n", argv[i]);
                return 1;
            }
            out_arg = argv[++i];
        }else if (argv[i][0] == '-') {//needed in case if program will get unsupported options
            fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
            return 1;
        }



    else {
            if (in_arg != NULL) {
                fprintf(stderr, "Error: more than one input file\n");
                return 1;
            }
            in_arg = argv[i];
        }
    }
    if ((dir_arg == NULL) == (in_arg == NULL)) {  // neither or both
        fprintf(stderr, "Usage: %s [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n", argv[0]);
        return 1;
    }

   
    DIR *dir = NULL;
    if (in_arg != NULL) {
        struct stat st;
        if (stat(in_arg, &st) != 0 || !S_ISREG(st.st_mode)) {
            fprintf(stderr, "Error: cannot read input file '%s'\n", in_arg);
            return 1;
        }
    } else {
        dir = opendir(dir_arg);
        if (dir == NULL) {
            fprintf(stderr, "Error: cannot open folder '%s': %s\n", dir_arg, strerror(errno));
            return 1;
        }
    }

    
    char out_dir[4096];
    if (out_arg != NULL) {
        snprintf(out_dir, sizeof out_dir, "%s", out_arg);
    } else {
        char base[256];
        if (dir_arg != NULL) base_name(dir_arg, base, 0);
        else base_name(in_arg, base, 1);
        snprintf(out_dir, sizeof out_dir, "%s_%s_%s", base, FIRST_NAME, STUDENT_ID);
    }
    if (mkdir(out_dir, 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error: cannot create '%s': %s\n", out_dir, strerror(errno));
        if (dir != NULL) closedir(dir);
        return 1;
    }

    if (in_arg != NULL) return process_file(in_arg, out_dir);

    
    int status = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        size_t n = strlen(entry->d_name);
        if (n <= 4 || strcmp(entry->d_name + n - 4, ".txt") != 0) continue;
        char path[4096];
        snprintf(path, sizeof path, "%s/%s", dir_arg, entry->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (process_file(path, out_dir) != 0) status = 1;
    }
    closedir(dir);//just closed directory after checking all files
    return status;//needed to show whether was all done successfully
}