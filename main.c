#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

char *s = NULL;

int label_counter = 0; // Counter for generating unique labels

// Function to generate unique labels for assembly code
char* generate_label() {
    char* label = (char*)malloc(10 * sizeof(char)); // Allocate space for label
    sprintf(label, ".L%d", label_counter++); // Format label as .L<number>
    return label;
}

// Function to recursively parse and generate assembly code for Brainfuck commands
void do_command(char command, FILE *input, FILE *output) {
    char c;
    long pos;

    switch (command) {
        case '>':
            fprintf(output, "\n\tmovq -8(%%rbp), %%rax\n\taddq $1, %%rax\n\tmovq %%rax, -8(%%rbp)");
            break;
        case '<':
            fprintf(output, "\n\tmovq -8(%%rbp), %%rax\n\tsubq $1, %%rax\n\tmovq %%rax, -8(%%rbp)");
            break;
        case '+':
            fprintf(output, "\n\tmovq -8(%%rbp), %%rax\n\tmovzbl (%%rax), %%edx\n\taddl $1, %%edx\n\tmovb %%dl, (%%rax)");
            break;
        case '-':
            fprintf(output, "\n\tmovq -8(%%rbp), %%rax\n\tmovzbl (%%rax), %%edx\n\tsubl $1, %%edx\n\tmovb %%dl, (%%rax)");
            break;
        case '.':
            fprintf(output, "\n\tmovq -8(%%rbp), %%rax\n\tmovzbl (%%rax), %%eax\n\tmovsbl %%al, %%eax\n\tmovl %%eax, %%edi\n\tcall putchar");
            break;
        case ',':
            fprintf(output, "\n\tcall getchar\n\tmovl %%eax, %%edx\n\tmovq -8(%%rbp), %%rax\n\tmovb %%dl, (%%rax)");
            break;
        case '[': {
            char *start_label = generate_label();
            char *end_label = generate_label();

            fprintf(output, "\n%s:", start_label);
            fprintf(output, "\n\tmovq -8(%%rbp), %%rax\n\tmovzbl (%%rax), %%eax\n\ttestb %%al, %%al\n\tjz %s", end_label);

            while ((c = getc(input)) != EOF && c != ']') {
                do_command(c, input, output);
            }

            fprintf(output, "\n\tjmp %s\n%s:", start_label, end_label);
            free(start_label);
            free(end_label);
            break;
        }
        case ']':
            // In valid Brainfuck, ']' should be handled by the '[' case
            break;
        default:
            // Ignore unsupported characters
            break;
    }
}

char* find_flags(int argc, char **args) {
    for(int i = 0; i < argc; i++) {
        if (strcmp(args[i], "-o") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "expected output name, inserting a.out\n");
                return NULL;
            } else {
                return args[i+1];
            }
        }
    }
}

int main(int argc, char **argv) {
    s = (char *)calloc(INT_MAX, sizeof(char)); // Allocate a reasonable amount of memory
    if (!s) {
        perror("calloc");
        return 1;
    }
    strcpy(s, "");
    char command;
    const char *output_file_name;
    FILE *input;
    FILE *output = fopen("output.s", "wb");
    if (!output) {
        perror("fopen");
        return 1;
    }

    if (argc > 1) {
        input = fopen(argv[1], "r");
        output_file_name = "a.out";
        if (NULL == input) {
                fprintf(stderr, "Unable to open \"%s\"\n", argv[1]);
                exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Usage: %s <input.bf>\n", argv[0]);
        return 0;
    }
    fprintf(output, ".section .text\n.global main\nmain:\n\tpushq %%rbp\n\tmovq %%rsp, %%rbp\n\tsubq $30000, %%rsp\n\tleaq -30000(%%rbp), %%rax\n\tmovl $30000, %%edx\n\tmovl $0, %%esi\n\tmovq %%rax, %%rdi\n\tcall memset\n\tleaq -30000(%%rbp), %%rax\n\tmovq %%rax, -8(%%rbp)\n");

    while ((command = getc(input)) != EOF) {
        do_command(command, input, output);
    }

    fprintf(output, "\n\tmovl $0, %%eax\n\tleave\n\tret\n");
    free(s);
    fclose(input);
    fclose(output);

    char *cmd = (char *)calloc(1024, sizeof(char));
    sprintf(cmd, "gcc output.s -no-pie -w -Wl,-z,noexecstack -o %s\n", output_file_name);
    system(cmd);
    system("rm output.s");
    free(cmd);
    return 0;
}
