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

void store(FILE * archive, char path[], int isCompressed);
void compress(char path[]);
void walk_archive(FILE * archive, int (*callback)(char *, MetaData, FILE *));
int print_meta(char * filepath, MetaData file_meta, FILE * file);

int main (int argc, char **argv)
{
	FILE * fp;
	int opt, cflag = 0, aflag = 0, xflag = 0, pflag = 0, mflag = 0, jflag = 0;
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
       	 	case 'c': {cflag = 1; pitt_file = opt; DEBUG_PRINT(("C IS FOUND\n")); break;}
	        case 'a': {aflag = 1; pitt_file = opt; DEBUG_PRINT(("A IS FOUND\n")); break;}
	        case 'x': {xflag = 1; pitt_file = opt; DEBUG_PRINT(("X IS FOUND\n")); break;}
	        case 'p': {pflag = 1; pitt_file = opt; DEBUG_PRINT(("P IS FOUND\n")); break;}
	        case 'm': {mflag = 1; pitt_file = opt; DEBUG_PRINT(("M IS FOUND\n")); break;}
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
		if((cflag != 1 && aflag != 1))
		{
			printf("Cannot call flag J without flag a or flag c");
			return 1;
		}
		//compress file(s) at input path
		compress(inputPath);
	}else if (cflag)
	{
		//open new writable file
		fp = fopen(pittrar, "w+");
		DEBUG_PRINT(("Compressing file...\n"));
		if(fp == NULL){
			DEBUG_PRINT(("Program ending\n"));
			exit(0);
		}
		//call store function
		store(fp, inputPath, jflag);
	}else if (aflag)
	{
		DEBUG_PRINT(("At A\n"));
		//call fxn
		//open file at the end (enabling append feature)
		fp = fopen(pittrar, "a+");
		DEBUG_PRINT(("Compressing file...\n"));
		if(fp == NULL){
			DEBUG_PRINT(("Program ending\n"));
			exit(0);
		}
		//call store fucntion
		store(fp, inputPath, jflag);
	}else if (xflag)
	{
		DEBUG_PRINT(("At X\n"));
		//call fxn
	}else if (pflag)
	{
		DEBUG_PRINT(("At P\n"));
		//call fxn
	}else if (mflag)
	{
		fp = fopen(pittrar, "r");
        DEBUG_PRINT(("I AM WALKING\n"));
        walk_archive(fp, print_meta);
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

/******************
 * Callback function requirements:
 *      int functionName(char * file_path, MetaData * file_meta_data, FILE * file);
 *          file_path: Name of the file and relative path.
 *          file_meta_data: data for that file
 *          file: the start of the file, only read up to size
 *      RETURN: Return whether you read from the file or not.
 ********************/
void walk_archive(FILE * archive, int (*callback)(char *, MetaData, FILE *)){
    MetaData data;
    char * path_name;
    while(1){
        printf("I AMHERE\n");
        fread(&data, sizeof(MetaData), 1, archive);
        path_name = (char *)malloc(data.path_name_size);
        fread(&data, data.path_name_size, 1, archive);
        fread(path_name, data.path_name_size, 1, archive);
        callback(path_name, data, archive);
        free(path_name);
        if(feof(archive)){
            return;
        }
    }
}

int print_meta(char * filepath, MetaData file_meta, FILE * file){
    printf("Meta: %d, %d, %d, %d, %s, %s\n", file_meta.type, file_meta.compressed, file_meta.size, file_meta.path_name_size, file_meta.permissions, filepath);
    return 1;
}

void store(FILE * archive, char * path, int isCompressed)
{
	//write meta-->path name
	//if file-->file contents
	//if directory --> recurse contents
	//create metadata
	FILE * path_file;

	struct stat buf;
	MetaData data;
    data.size = 0;
    data.type = 0;
    data.path_name_size = 0;

	stat(path, &buf);
	//populate compressed
	data.compressed = isCompressed;
	//populate name
	data.path_name_size = strlen(path) + 1;

	//populate permissions
    sprintf(data.permissions, (buf.st_mode & S_IRUSR) ? "r" : "-");
    sprintf(data.permissions+1, (buf.st_mode & S_IWUSR) ? "w" : "-");
    sprintf(data.permissions+2, (buf.st_mode & S_IXUSR) ? "x" : "-");
    sprintf(data.permissions+3, (buf.st_mode & S_IRGRP) ? "r" : "-");
    sprintf(data.permissions+4, (buf.st_mode & S_IWGRP) ? "w" : "-");
    sprintf(data.permissions+5, (buf.st_mode & S_IXGRP) ? "x" : "-");
    sprintf(data.permissions+6, (buf.st_mode & S_IROTH) ? "r" : "-");
    sprintf(data.permissions+7, (buf.st_mode & S_IWOTH) ? "w" : "-");
    sprintf(data.permissions+8, (buf.st_mode & S_IXOTH) ? "x" : "-");
	DEBUG_PRINT(("stats: %s, name: %s\n", data.permissions, path));

	//populate type (file or directory)
	if(S_ISDIR(buf.st_mode))
	{
		data.type = DIRECTORY;
	}else
	{
		data.type = FILE_TYPE;
	}

	if(data.type == FILE_TYPE)
	{
		//write meta data to archive
		fwrite((void *)&data, sizeof(data), 1, archive);

		//write path name (string) to archive

		fwrite(path, data.path_name_size, 1, archive);

		//open and read file to buffer "string"
		path_file = fopen(path, "r+");
		fseek(path_file, 0, SEEK_END);
		long fsize = ftell(path_file);
		fseek(path_file, 0, SEEK_SET);
		char *string = malloc(fsize);
		fread(string, fsize, 1, path_file);
		fclose(path_file);

		//write file contents to archive
		fwrite(string, fsize, 1, archive);
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
					fwrite((void *)&data, sizeof(data), 1, archive);
					fwrite(path, data.path_name_size, 1, archive);

					store(archive, (char *)permanent_path, isCompressed);
					free(permanent_path);
				}
			}
		}
	}
}
