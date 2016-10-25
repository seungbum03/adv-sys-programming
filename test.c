//Beom's fread code!
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

	gettimeofday(&before, NULL);
    
    if((readaline_and_out(file1,file2,fout,line))){
			fprintf(stderr, "Error was occured!!\n");
			goto leave3;
	}
   
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

/*===============================================================
Read entire texts from fin1, fin2 to streams named buf1, buf2.
Using strtok_r(), break a each strings into a sequence of tokens.
Then reverse a string, write to a file name fout.

return 1 if it occurs error.
return 0 if it finished.
=================================================================*/
int
readaline_and_out(FILE *fin1, FILE *fin2, FILE *fout, long *line)
{    
    char *buf1, *buf2;
    char *ptr1, *ptr2;
	char *end_str, *end_str2;
	size_t leng1, leng2;
	//memory allocation
    if((buf1 = (char*)malloc(sizeof(char)*len)) == NULL){
		fprintf(stderr, "Malloc error\n");
		return 1;
	}
    if((buf2 = (char*)malloc(sizeof(char)*len)) == NULL){
		fprintf(stderr, "Malloc error\n");
		return 1;
	}

	//read file to stream
	if((leng1 = fread(buf1, len , 1, fin1)) == -1){
		fprintf(stderr, "fread error\n");
		return 1;
	}
	if((leng2 = fread(buf2, len, 1, fin2)) == -1){
		fprintf(stderr, "fread error\n");
		return 1;
	}
	
	//devide a string with "\n"
	ptr1 = strtok_r(buf1, "\n", &end_str);
	ptr2 = strtok_r(buf2, "\n", &end_str2);
	line[0] = 1;
	line[1] = 1;

	while(ptr1 != NULL || ptr2 != NULL){
		//fwrite the devided & reversed strings to fout
		if(ptr1 != NULL){
			fprintf(fout, "%s\n", ptr1);
			line[2]++;
			ptr1 = strtok_r(NULL, "\n", &end_str);
			line[0]++;
		}
	
		if(ptr2 != NULL){
			fprintf(fout, "%s\n", ptr2);
			line[2]++;
			ptr2 = strtok_r(NULL, "\n", &end_str2);
			line[1]++;
		}
	}

	free(buf1);
	free(buf2);

	return 0;
}


/*====================================================
                reverse the string.
char *str -> the string that we want to reverse
	   	 	  It returns char pointer
======================================================*/
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
