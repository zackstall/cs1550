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
int print_heirarchy(char * filepath, MetaData file_meta, FILE * file);
int expand_archive(char * filepath, MetaData file_meta, FILE * file);

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
	}
	if (cflag)
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
		fp = fopen(pittrar, "r");
		walk_archive(fp, expand_archive);
	}else if (pflag)
	{
		//use 
		DEBUG_PRINT(("At P\n"));
		//call fxn
		fp = fopen(pittrar, "r");
		walk_archive(fp, print_heirarchy);
	}else if (mflag)
	{
		fp = fopen(pittrar, "r");
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
        int count;
        count = fread(&data, sizeof(MetaData), 1, archive);
        if(count <  1){
            return;
        }
        path_name = (char *)malloc(data.path_name_size);
        fread(path_name, data.path_name_size, 1, archive);
        callback(path_name, data, archive);
        free(path_name);
        if(feof(archive)){
            return;
        }
    }
}

void print_shortened_path(char * path)
{
	int i;
	for(i = strlen(path) - 1; i >= 0; i--)
	{
		if(path[i] == '/'){
			printf("File: %s, ", (path + i + 1));
			return;
		}
	}
}

int print_meta(char * filepath, MetaData file_meta, FILE * file)
{
	print_shortened_path(filepath);
    printf("Meta: %d, %d, %d, %d, %s\n", file_meta.type, file_meta.compressed, file_meta.size, file_meta.path_name_size, file_meta.permissions);

    char * extra = (char *)malloc(file_meta.size);
    fread(extra, file_meta.size, 1, file);
    free(extra);
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

		//write path name (string) to archive


		//open and read file to buffer "string"
		path_file = fopen(path, "r+");
		if(path_file == NULL)
		{
			DEBUG_PRINT(("FILE IS DEAD %s\n", path));
			exit(0);
		}
		fseek(path_file, 0, SEEK_END);
		long fsize = ftell(path_file);
		fseek(path_file, 0, SEEK_SET);
		char *string = malloc(fsize);
		fread(string, fsize, 1, path_file);
        data.size = fsize;
		fclose(path_file);

        DEBUG_PRINT(("writing file..%s\n", path));
		//write file contents to archive
        printf("Meta: %d, %d, %d, %d, %s, %s\n", data.type, data.compressed, data.size, data.path_name_size, data.permissions, path);
		fwrite((void *)&data, sizeof(data), 1, archive);
		fwrite(path, data.path_name_size, 1, archive);
		fwrite(string, fsize, 1, archive);
	}else
	{
		DIR * newPath;
		newPath = opendir(path);
		if(newPath != NULL)
		{
            DEBUG_PRINT(("writing folder..%s\n", path));
            fwrite((void *)&data, sizeof(data), 1, archive);
            fwrite(path, data.path_name_size, 1, archive);

            struct dirent * entry;
			while((entry = readdir(newPath)) != NULL)
			{
				const char * d_name;

				d_name = entry->d_name;
				if(strcmp(d_name, ".") != 0 && strcmp(d_name, "..") != 0)
				{
                    char * permanent_path = (char *) malloc(strlen(d_name) + strlen(path) + 2);
					memset(permanent_path, 0, strlen(d_name) + strlen(path) + 2);
                    strcat(permanent_path, path);
					DEBUG_PRINT(("permanent path: %s\n", permanent_path));

                    strcat(permanent_path, "/");
                    strcat(permanent_path, d_name);
					DEBUG_PRINT(("permanent path: %s\n", permanent_path));
					store(archive, (char *)permanent_path, isCompressed);
					free(permanent_path);
				}
			}
		}
	}
}

int print_heirarchy(char * filepath, MetaData file_meta, FILE * file)
{
    printf("%s\n", filepath);
    char * extra = (char *)malloc(file_meta.size);
    fread(extra, file_meta.size, 1, file);
    free(extra);
    return 1;
}

int expand_archive(char * filepath, MetaData file_meta, FILE * file)
{
	
	FILE * fp;
    char * file_contents = (char *)malloc(file_meta.size);
   if(file_meta.type == DIRECTORY){
		mkdir(filepath, S_IRWXU);
	}
	else{
		FILE * f;
	 	fread(file_contents, file_meta.size, 1, file);
		f = fopen(filepath, "w");
		if(f != NULL){
			fwrite(file_contents, file_meta.size, 1, f);
			fclose(f);
		}
		if(file_meta.compressed){
			int childId = fork();
			char * argv[] = {"compress", "-d", filepath, NULL};
			int wait_status;
			if(childId >= 0) // fork was successful
			    {
				if(childId == 0) // child process
				{
						DEBUG_PRINT(("Compressing execvp %s  \n", filepath));
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

			}
	    free(file_contents);		
	}

    return 1;
}
