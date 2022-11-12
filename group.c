#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_BYTES 8096

#define MY_ENOENT 0 //group not found
#define MY_EIO 1 //I/O error
#define MY_ENOMEM 2 // No memory
#define MY_ENAN 3 // not a number

typedef unsigned int gid_t;

struct group {
    char   *gr_name;        /* group name */
    char   *gr_passwd;      /* group password */
    gid_t   gr_gid;         /* group ID */
    char  **gr_mem;         /* NULL-terminated array of pointers to names of group members */
};

/*START Functions for strings */
 int strcmp(char* str1, char* str2){
	int i=0;
	while(str1[i] != 0 && str2[i] != 0 && str1[i] == str2[i]){
		i++;
	}
	if(str1[i]==0 && str2[i]==0){
		return 0;
	}else if(str1[i]==0){
		return 1;
	}else if(str2[i]==0){
		return -1;
	}else if(str1[i]>str2[i]){
		return 1;
	}else{
		return -1;
	}
}

long unsigned int strlen(char* str){
	int i=0;
	while(str[i]!=0)i++;
	return i;
}

char* strcat(char* dest, char* src){
	int i=0,j=0;
	while(dest[i]!=0)i++;
	while(src[j]!=0)dest[i++]=src[j++];
	dest[i]=0;
	return dest;
}

char* strcpy(char* dest,const char* src){
	int i;
	for(i=0;src[i]!=0;i++){
		dest[i]=src[i];
	}
	dest[i]=0;
	return dest;
}

int strsplit(char *str, char separator,char*** splitted){
	(*splitted) = malloc(sizeof(char *));
	char* tmp = malloc(MAX_BYTES);
	
	int i=0;
	int count=0, substring=0;
	

	for(i;str[i]!=0;i++){
		if(str[i]==separator){
			tmp[count]=0;
			(*splitted)[substring] = malloc(count);
			strcpy((*splitted)[substring],tmp);
			substring++;
			count=0;
		}else{
			tmp[count]=str[i];
			count++;
		}
	}
	tmp[count]=0;
	(*splitted)[substring] = malloc(count);
	strcpy((*splitted)[substring],tmp);
	substring++;
	count=0;

	free(tmp);

	if(substring==0 && strlen(str)>0){
		(*splitted)[0] = malloc(strlen(str)+1);
		strcpy((*splitted)[0],str);
		return 1;
	}

	return substring;
}

int toint(char *str){
	int n=strlen(str);
	int i=0;
	int result=0;
	int sign=1;
	if(str[0]=='-'){
		sign=-1;
		i=1;
	}else if(str[0]=='+'){
		sign=1;
		i=1;
	}else{
		i=0;
	}
	for(i;i<n;i++){
		
		if(str[i]<'0' || str[i]>'9'){
			errno = MY_ENAN;
			return 0;
		}
		int factor=1;
		int j;
		for(j=0;j<n-i-1;j++){
			factor*=10;
		}
		result += (str[i]-48)*factor;
	}

	return result*sign;
}
/*END Functions for strings*/

/*START Functions for group file*/

/**
 * @brief Read groups from /etc/group file, return -1 if error happens and sets errno
 * 
 * 
 * @param groups {struct group **} pointer to array of groups 
 * @return int - number of groups read from /etc/group file
 */
int getgrarr(struct group ** groups){
	int n=0;

	int fd = open("/etc/group",O_RDONLY);
	if(fd==-1){
		//error opening file
		errno = MY_EIO;
		return -1;
	}

	char *buf = malloc(sizeof(char));
	if(buf == NULL){
		//error allocating memory
		errno = MY_ENOMEM;
		return -1;
	}
	int readBytes;
	int count = 0;
	char* tmp = malloc(MAX_BYTES);
	if(tmp==NULL){
		//error allocating memory
		errno = MY_ENOMEM;
		return -1;
	}
	while((readBytes=read(fd,buf,sizeof(char)))>0){
		if(strcmp(buf,":")==0){
			if(count == 0){
				//name
				(*groups)[n].gr_name = malloc(strlen(tmp)+1);
				if((*groups)[n].gr_name==NULL){
					//error allocating memory
					errno = MY_ENOMEM;
					return -1;
				}
				strcpy((*groups)[n].gr_name,tmp);
				
			}else if(count==1){
				//pwd
				(*groups)[n].gr_passwd = malloc(strlen(tmp)+1);
				if((*groups)[n].gr_passwd==NULL){
					//error allocating memory
					errno = MY_ENOMEM;
					return -1;
				}
				strcpy((*groups)[n].gr_passwd,tmp);
			}else if(count==2){
				//gid
				(*groups)[n].gr_gid = toint(tmp);
			}
			
			tmp[0]=0;
			count++;
		}else if(strcmp(buf,"\n")==0){
			//gmemb
			int members = strsplit(tmp,',',&((*groups)[n].gr_mem));
			(*groups)[n].gr_mem[members] = NULL;
			//next line
			n++;
			(*groups) = realloc((*groups),(n+1)*sizeof(struct group));
			if((*groups)==NULL){
				//error reallocating memory
				errno = MY_ENOMEM;
				return -1;
			}

			count=0;
			tmp[0]=0;
		}else{
			//not a separator, add it to tmp string
			strcat(tmp,buf);
		}
	}

	free(tmp);
	free(buf);
	close(fd);
	return n;
}

/**
 * @brief Free memory used to store array of groups from /etc/group file
 * 
 * @param groups {struct group **} pointer to array of groups 
 * @param n length of groups array
 * @return int 
 */
int freegrarr(struct group** groups, int n){
	int i;

	for(i=0;i<n;i++){
		free((*groups)[i].gr_name);
		free((*groups)[i].gr_passwd);
		int j=0;
		while((*groups)[i].gr_mem[j]!=NULL){
			free((*groups)[i].gr_mem[j]);
			j++;
		}
		free((*groups)[i].gr_mem);
	}

	free(*groups); 

	return 0;
}


/**
 * @brief Reads group of name var{name} from file /etc/group
 * 
 * @param name {char *} name of the group needed
 * @return struct group* - pointer to structure of type struct grou* where is data of group with name var{name} stored
 */
struct group* getgrnam(char* name){
	struct group* foundGroup;
	struct group* groups = malloc(sizeof(struct group));
	int n = getgrarr(&groups);
	if(n==-1){ 
		//errno is set in getgrarr function
		return NULL;
	}
	if(n==0){ 
		//errno is set in getgrarr function
		return NULL;
	}

	int i;
	for(i=0;i<n;i++){
		
		 if(strcmp(groups[i].gr_name,name)==0){
			foundGroup = malloc(sizeof(struct group));

			foundGroup->gr_name = malloc(strlen(groups[i].gr_name)+1);
			if(foundGroup->gr_name == NULL){
				//error allocating memory
				errno = MY_ENOMEM;
				return NULL;
			}
			strcpy(foundGroup->gr_name, groups[i].gr_name);

			foundGroup->gr_passwd = malloc(strlen(groups[i].gr_passwd)+1);
			if(foundGroup->gr_passwd == NULL){
				//error allocating memory
				errno = MY_ENOMEM;
				return NULL;
			}
			strcpy(foundGroup->gr_passwd,groups[i].gr_passwd);

			foundGroup->gr_gid = groups[i].gr_gid;
			
			foundGroup->gr_mem = malloc(sizeof(char **));
			if(foundGroup->gr_mem == NULL){
				//error allocating memory
				errno = MY_ENOMEM;
				return NULL;
			}
			int j=0;
			
			while(groups[i].gr_mem[j] != NULL){
				foundGroup->gr_mem[j] = malloc(sizeof(groups[i].gr_mem[j])+1);
				if(foundGroup->gr_mem[j] == NULL){
					//error allocating memory
					errno = MY_ENOMEM;
					return NULL;
				}
				strcpy(foundGroup->gr_mem[j],groups[i].gr_mem[j]);
				j++;
				
			}
			foundGroup->gr_mem[j] = NULL;
			freegrarr(&groups,n);
			return foundGroup;
		} 
	}

	freegrarr(&groups,n);
	errno = MY_ENOENT;
	return NULL;
}


/**
 * @brief Reads group of id var{id} from file /etc/group
 * 
 * @param gid {int} id of the group needed
 * @return struct group* - pointer to structure of type struct grou* where is data of group with id var{gid} stored
 */
struct group* getgrgid(int gid){
	struct group* foundGroup;
	struct group* groups = malloc(sizeof(struct group));
	int n = getgrarr(&groups);
	if(n==-1){ 
		//errno is set in getgrarr function
		return NULL;
	}
	if(n==0){ 
		//errno is set in getgrarr function
		return NULL;
	}

	int i;
	for(i=0;i<n;i++){
		
		 if(groups[i].gr_gid == gid){
			foundGroup = malloc(sizeof(struct group));

			foundGroup->gr_name = malloc(strlen(groups[i].gr_name)+1);
			if(foundGroup->gr_name == NULL){
				//error allocating memory
				errno = MY_ENOMEM;
				return NULL;
			}
			strcpy(foundGroup->gr_name, groups[i].gr_name);

			foundGroup->gr_passwd = malloc(strlen(groups[i].gr_passwd)+1);
			if(foundGroup->gr_passwd == NULL){
				//error allocating memory
				errno = MY_ENOMEM;
				return NULL;
			}
			strcpy(foundGroup->gr_passwd,groups[i].gr_passwd);

			foundGroup->gr_gid = groups[i].gr_gid;
			
			foundGroup->gr_mem = malloc(sizeof(char **));
			if(foundGroup->gr_mem == NULL){
				//error allocating memory
				errno = MY_ENOMEM;
				return NULL;
			}
			int j=0;
			
			while(groups[i].gr_mem[j] != NULL){
				foundGroup->gr_mem[j] = malloc(sizeof(groups[i].gr_mem[j])+1);
				if(foundGroup->gr_mem[j] == NULL){
					//error allocating memory
					errno = MY_ENOMEM;
					return NULL;
				}
				strcpy(foundGroup->gr_mem[j],groups[i].gr_mem[j]);
				j++;
				
			}
			foundGroup->gr_mem[j] = NULL;
			freegrarr(&groups,n);
			return foundGroup;
		} 
	}

	freegrarr(&groups,n);
	errno = MY_ENOENT;
	return NULL;
}


void help(){
	printf("Format: ./a.out [arg1] [arg2]\n");
	printf("arg1 - option argument\n");
	printf("arg2 - search argument\n");
	printf("\n%-15s %10s\n\n","Options","Description");
	printf("%-15s %10s\n","-n -name","find group by name in [arg2]");
	printf("%-15s %10s\n","-g -gid", "find group by gid in [arg2], arg2 must be int number");
	printf("%-15s %10s\n","-h -help", "show this menu");
}

void checkMyErrno(){
	switch(errno){
		case MY_ENOENT:
		printf("Group not found!\n");
		break;
 		case MY_EIO: 
		printf("Error in I/O operations!\n");
		break;
 		case MY_ENOMEM:
		printf("Not enough memory!\n");
		break;
 		case MY_ENAN :
		printf("Not a number!\n");
		break;
		default:
		printf("Unknown error!\n");
		break;
	}
}

int main(int argc, char** argv){
	if(argc<2){
		help();
		return 1;
	}
	if(strcmp(argv[1],"-h")==0 || strcmp(argv[1],"-help")==0){
		help();
		return 0;
	}
	if(argc!=3){
		help();
		return 1;
	}
	struct group * grupa = NULL;
	if(strcmp(argv[1],"-n")==0 || strcmp(argv[1],"-name")==0){
		grupa = getgrnam(argv[2]);
	}else if(strcmp(argv[1],"-g")==0 || strcmp(argv[1],"-gid")==0){
		int gid = toint(argv[2]);
		if(gid==0 && errno == MY_ENAN){
			printf("gid must be int number\n");
			return 1;
		}
		grupa = getgrgid(gid);
	}

	if(grupa == NULL){
		checkMyErrno();
		return 1;
	}

	printf("Group: %s %s %d \nMembers: ",grupa->gr_name,grupa->gr_passwd, grupa->gr_gid);

	int j=0;
	while(grupa->gr_mem[j] != NULL){
		printf("%s",grupa->gr_mem[j]);
		if(grupa->gr_mem[j+1] != NULL){
			printf(",");
		}
		j++;
	} 
	printf("\n");
	
	
	free(grupa->gr_name);
	free(grupa->gr_passwd);
	j=0;
	while(grupa->gr_mem[j] != NULL){
		free(grupa->gr_mem[j]);
		j++;
	} 
	free(grupa->gr_mem);
	free(grupa);

	return 0;
}