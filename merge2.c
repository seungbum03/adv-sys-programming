//Beom's test
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

int readaline_and_out(FILE *fin1, FILE *fin2, FILE *fout);
char* str_reverse(char *str);

int
main(int argc, char *argv[])
{
    FILE *file1, *file2, *fout;
    int eof1 = 0, eof2 = 0;
    long line1 = 0, line2 = 0, lineout = 0;
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
	
    gettimeofday(&before, NULL);
    
	/*do {
        if (!eof1) {
            if (!readaline_and_out(file1, fout)) {
                line1++; lineout++;
            } else
                eof1 = 1;
        }
        if (!eof2) {
            if (!readaline_and_out(file2, fout)) {
                line2++; lineout++;
            } else
                eof2 = 1;
        }
    } while (!eof1 || !eof2);
   */
    readaline_and_out(file1,file2,fout);
   
    gettimeofday(&after, NULL);
    
    duration = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
    printf("Processing time = %d.%06d sec\n", duration / 1000000, duration % 1000000);
    printf("File1 = %ld, File2= %ld, Total = %ld Lines\n", line1, line2, lineout);
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
readaline_and_out(FILE *fin1, FILE *fin2, FILE *fout)
{    
    int ch, count = 0;
    char *buf1;
	char *buf2;
    char *ptr1;
	char *ptr2;

	size_t leng1, leng2;
    int i=0,j=0;
	long len1, len2;

	//finding file size
	fseek(fin1, 0, SEEK_END);
	len1 = ftell(fin1);
	fseek(fin1, SEEK_CUR, 0);

	fseek(fin2, 0, SEEK_END);
	len2 = ftell(fin2);
	fseek(fin2, SEEK_CUR, 0);
	
	printf("len1 : %d, len2 : %d\n",len1,len2);

	//memory allocation
    buf1 = (char*)malloc(sizeof(char)*len1);
    buf2 = (char*)malloc(sizeof(char)*len2);

	//read file to stream
	leng1 = fread(buf1, len1, 10, fin1);
	leng2 = fread(buf2, len2, 10, fin2);

	printf("%s", buf1);
	printf("%s\n", buf2);
	
	ptr1 = strtok(buf1, "\n");
	ptr2 = strtok(buf2, "\n");

	while( ptr1 != NULL && ptr2 != NULL){
		if(ptr1 != NULL){
			printf("%s\n", str_reverse(ptr1));
			ptr1 = strtok(NULL, "\n");
		}
		if(ptr2 != NULL){
			printf("%s\n", str_reverse(ptr2));
			ptr2 = strtok(NULL, "\n");
		}
	}
	
	//while( ptr1 != EOF && ptr2 != EOF){



    return 0;
}

char*
str_reverse(char *str)
{
	char *p1 = str;
	int len = strlen(str);
	char *p2 = str + len - 1;

	while (p1 < p2) {
		char tmp = *p1;
		*p1++ = *p2;
		*p2-- = tmp;
	}

	return str;
}
