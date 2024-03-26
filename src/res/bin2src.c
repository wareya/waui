#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char ** argv)
{
    if (argc < 3)
        return -1;
    
    FILE * out_file = fopen(argv[1], "w");
    
    for (int i = 2; i < argc; i++)
    {
        char * fname = argv[i];
        FILE * in_file = fopen(fname, "rb");
        
        // strip path
        int last_slash = 0;
        for (int j = 0; fname[j] != 0; j++)
        {
            if (fname[j] == '/' || fname[j] == '\\')
                last_slash = j;
        }
        fname += last_slash + 1;
        // convert non-var-safe to var-safe
        int needs_prefix = fname[0] >= '0' && fname[0] <= '9';
        for (int j = 0; fname[j] != 0; j++)
        {
            char c = fname[j];
            if (c >= '0' && c <= '9')
                continue;
            if (c >= 'A' && c <= 'Z')
                continue;
            if (c >= 'a' && c <= 'z')
                continue;
            fname[j] = '_';
        }
        
        
        fprintf(out_file, "const unsigned char %s%s[] =\n", needs_prefix ? "_" : "", fname);
        fprintf(out_file, "{\n    ");
        
        int c = 0;
        unsigned long long n = 0;
        while ((c = fgetc(in_file)) != EOF)
        {
            fprintf(out_file, "%d,", c);
            n += 1;
            if (n % 32 == 0)
                fprintf(out_file, "\n    ");
        }
        fprintf(out_file, "\n};\n");
        fprintf(out_file, "unsigned long long %s%s_size = %lld;\n\n", needs_prefix ? "_" : "", fname, n);
        
        fclose(in_file);
    }
    
    fclose(out_file);
}