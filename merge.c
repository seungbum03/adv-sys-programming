//Beom's fread code
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

//define the length of the files : 100MB
#define len 104857600

int readaline_and_out(FILE *fin1, FILE *fin2, FILE *fout, long *line);
char* str_reverse(char *str);

int
main(int argc, char *argv[])
{
    FILE *file1, *file2, *fout;
    long line[3]={0,0,0};
    struct timeval before, after;
    int duration;
    int ret = 1;
    
	if (argc != 4) {
        fprintf(stderr, "usage: %s file1 file2 fout\n", argv[0]);
        goto leave0;
    }
    if ((file1 = fopen(argv[1], "rt")) == NULL) {
        perror(argv[1]);
        goto leave0;
    }
    if ((file2 = fopen(argv[2], "rt")) == NULL) {
        perror(argv[2]);
        goto leave1;
    }
    if ((fout = fopen(argv[3], "wt")) == NULL) {
        perror(argv[3]);
        goto leave2;
    }
   //file open completed

   //finding file size
	gettimeofday(&before, NULL);
    
    readaline_and_out(file1,file2,fout,line);
   
    gettimeofday(&after, NULL);
    
    duration = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
    printf("Processing time = %d.%06d sec\n", duration / 1000000, duration % 1000000);
    printf("File1 = %ld, File2= %ld, Total = %ld Lines\n", line[0], line[1], line[2]);
    ret = 0;
    
leave3:
    fclose(fout);
leave2:
    fclose(file2);
leave1:
    fclose(file1);
leave0:
    return ret; 
}

/* Read a line from fin and write it to fout */
/* return 1 if fin meets end of file */
int
readaline_and_out(FILE *fin1, FILE *fin2, FILE *fout, long *line)
{    
    char *buf1, *buf2;
    char *ptr1, *ptr2;
	char *end_str, *end_str2;
	size_t leng1, leng2;
	//memory allocation
    buf1 = (char*)malloc(sizeof(char)*len);
    buf2 = (char*)malloc(sizeof(char)*len);

	//read file to stream
	leng1 = fread(buf1, len , 1, fin1);
	leng2 = fread(buf2, len, 1, fin2);
	
	ptr1 = strtok_r(buf1, "\n", &end_str);
	ptr2 = strtok_r(buf2, "\n", &end_str2);
	line[0] = 1;
	line[1] = 1;
	while(ptr1 != NULL || ptr2 != NULL){
		
		if(ptr1 != NULL){
			fwrite( str_reverse( ptr1 ), sizeof(char), strlen(ptr1), fout);
			fputc(0x0a, fout);
			line[2]++;
			ptr1 = strtok_r(NULL, "\n", &end_str);
			line[0]++;
		}
	
		if(ptr2 != NULL){
			fwrite( str_reverse( ptr2 ), sizeof(char), strlen(ptr2), fout);
			fputc(0x0a, fout);
			line[2]++;
			ptr2 = strtok_r(NULL, "\n", &end_str2);
			line[1]++;
		}
	}

	free(buf1);
	free(buf2);

	return 0;
}

//function of reversing the string
char*
str_reverse(char *str)
{
	char *p1 = str;
	int s_len = strlen(str);
	char *p2 = str + s_len - 1;

	while (p1 < p2) {
		char tmp = *p1;
		*p1++ = *p2;
		*p2-- = tmp;
	}

	return str;
}
