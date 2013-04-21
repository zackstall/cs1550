#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#define DIRECTORY 1
#define FILE_TYPE 0


#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

typedef struct{
	int type;
	int compressed;
	int size;
	int path_name_size;
	char permissions[10];
} MetaData;

void compress(char path[]);

int main (int argc, char **argv)
{
	FILE * fp;
	int opt, cflag, aflag, xflag, pflag, mflag, jflag;
	int length, i;
	char pitt_file;
	char * pittrar;
	char * inputPath;
	char keys[] = ".";

	while ((opt = getopt(argc, argv, "jc::c::ja::a::x::p::m::")) != -1)
    {
        switch (opt)
        {
			case 'j': {jflag = 1; pitt_file = opt; DEBUG_PRINT(("J IS FOUND\n")); break;}
       	 	case 'c': {cflag = 1; pitt_file = opt; DEBUG_PRINT(("C IS FOUND\n"));break;}
	        case 'a': {aflag = 1; pitt_file = opt; break;}
	        case 'x': {xflag = 1; pitt_file = opt; break;}
	        case 'p': {pflag = 1; pitt_file = opt; break;}
	        case 'm': {mflag = 1; pitt_file = opt; break;}
        }
    }

	DEBUG_PRINT(("pittfile: %c\n", pitt_file));

	//get file to be modified
	inputPath = argv[3];

	//check to see if given archive file matches .pitt format
	pittrar = argv[2];
	length = strlen(pittrar);
	i = strcspn(pittrar,keys);
	DEBUG_PRINT(("pitt archive file is %s, i is %d\n", pittrar, i));
	if(i == length)
	{
		printf("Invalid archive location\n");
		return 1;
	}else if(strcmp("pitt", pittrar+i+1))
	{
		printf("Invalid archive location\n");
		return 1;
	}





	if (jflag)
	{
		fp = fopen(pittrar, "w+");
		//call fxn
		DEBUG_PRINT(("Compressing file...\n"));
		if(fp == NULL){
			DEBUG_PRINT(("Program ending\n"));
			exit(0);
		}
		compress(inputPath);
	}else if (cflag)
	{
		//call fxn
	}else if (aflag)
	{
		//call fxn
	}else if (xflag)
	{
		//call fxn
	}else if (pflag)
	{
		//call fxn
	}else if (mflag)
	{
		//call fxn
	}

}

void compress(char * path)
{
	//create metadata
	struct stat buf;
	stat(path, &buf);

    if(!S_ISDIR(buf.st_mode))
	{
		if(path[strlen(path) - 1] == 'Z' && path[strlen(path) - 2] == '.')
		{
			DEBUG_PRINT(("This is already compressed\n"));
			return;
		}
		int childId = fork();
		char * argv[] = {"compress", "-f", path, NULL};
		int wait_status;
		if(childId >= 0) // fork was successful
		    {
		        if(childId == 0) // child process
		        {
					DEBUG_PRINT(("Compressing execvp %s  \n", path));
					execv("compress", argv);
					perror("Didn't work:");
					DEBUG_PRINT(("THIS CANT HAPPEN\n"));
		        }
		        else //Parent process
		        {
					wait(&wait_status);
		        }
		    }
		    else // fork failed
		    {
		        printf("\n Fork failed, quitting!\n");
		        return;
		    }
	}else
	{
		DIR * newPath;
		newPath = opendir(path);
		if(newPath != NULL)
		{
			struct dirent * entry;
			while((entry = readdir(newPath)) != NULL)
			{
				const char * d_name;

				d_name = entry->d_name;
				if(strcmp(d_name, ".") != 0 && strcmp(d_name, "..") != 0)
				{
					char * permanent_path = (char *) malloc(strlen(d_name) + strlen(path) + 2);
					strcat(permanent_path, path);
					strcat(permanent_path, "/");
					strcat(permanent_path, d_name);

					compress((char *)permanent_path);
					free(permanent_path);
				}
			}
		}
	}

}
