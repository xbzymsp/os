// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    name = NULL;
    for (int i = 0; i < tableSize; i++)
    {
        table[i].inUse = FALSE;
        table[i].name = NULL;
        table[i].path = NULL;
        table[i].dir_name = NULL;
    }
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    //printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@22\n");
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
    //printf("##############################33\n");
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
    {
//printf("i in find index: %d\n",i);
        //if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
//printf("table name: %s        find name: %s inuse: %d i: %d\n",table[i].name, name, table[i].inUse,i);
        //if (table[i].inUse && !strcmp(name, table[i].name))
        if(table[i].inUse) 
        {
//printf("name: %s   find name: %s\n",table[i].name, name);
            //strcat(table[i].name, "/\0");
//printf("table name: %s   find name: %s\n",table[i].name, name);
        }

        if(table[i].inUse && !strcmp(name, table[i].name))
        {
//printf("find it!\n");
            return i;
        } 
    }
    return -1;		// name not in directory
}

int
Directory::FindIndex2(char *name)
{
    for (int i = 0; i < tableSize; i++)
    {
        //if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
printf("table name: %s        find name: %s inuse: %d i: %d\n",table[i].name, name, table[i].inUse,i);
        if (table[i].inUse && !strcmp(name, table[i].name))
        {
//printf("find it!\n");
            return i;
        }
    }
    return -1;		// name not in directoryhdr->
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
//printf("find %s\n",name);
    int i = FindIndex(name);

    if (i != -1)
	return table[i].sector;
    return -1;
}

int
Directory::Find2(char *name)
{
    int i = FindIndex2(name);

    if (i != -1)
	return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector, int type)
{ 
    if (FindIndex(name) != -1)
	return FALSE;

    char *path;
    int pos = 0;
    int len = strlen(name);
    for(int i=len-1;i>=0;--i)
    {
        if(name[i]=='/')
        {
            pos =i;
            break;
        }
    }
    path = new char[pos+1];
    for(int i=0;i<pos;++i)
    {
        path[i] = name[i];
    }
    path[pos] = '\0';


    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            //strncpy(table[i].name, name, FileNameMaxLen); 
            //if(type == 'F')
            //char tmp[20];
            //strcpy(tmp,name);
            //strcat(tmp,"/");
//printf("tmp: %s\n",tmp);
            table[i].name = name;
            /*else 
            {
                char *tmp = new char[100];
                char *str = "/\0";
                sprintf(tmp,"%s%s",name,str);
                table[i].name = tmp;
            }  */
            table[i].path = path;
printf("add name: %s path : %s sector: %d\n", table[i].name,table[i].path,newSector);
            table[i].sector = newSector;
//printf("add sector: %d\n",newSector);
            table[i].type = type;
        return TRUE;
	}
    return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    int i = FindIndex(name);

    if (i == -1)
	return FALSE; 		// name not in directory
    table[i].inUse = FALSE;
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
    //printf("dir name: %s\n",name);
   for (int i = 0; i < tableSize; i++)
	if (table[i].inUse)
	    printf("%s\n", table[i].name);
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
	    printf("Name: %s, file_header_Sector: %d\n", table[i].name, table[i].sector);
	    hdr->FetchFrom(table[i].sector);
	    hdr->Print();
	}
    printf("\n");
    delete hdr;
}


Directory::get_correct_dir(char *name)
{
printf("get_correct_dir\n");
//List();
    int dir_sector = -1;
    char *root = "root";
    int test = strcmp(root, name);
//printf("root: %s   name: %s  test: %d\n",root,name,test);
    if(test == 0)
    {
//printf("2342\n");
        return 1;
    }
    else
    {
        int index = Find(name);
//printf("name in get: %s\n",name);
//printf("index in get: %d\n",index);
        if(index != -1)
        {
            return index;
        }
        else
        {
printf("did not find it, now next layer\n");
            bool have_child = FALSE;
            for(int i=0;i<tableSize;++i)
            {
                if(table[i].inUse && (table[i].type == 0))
                {
//printf("name: %s\n",table[i].name);
                    have_child = TRUE;
                    int dir_index = table[i].sector;
printf("next sector: %d\n",dir_index);
                    OpenFile *tmp = new OpenFile(dir_index);
                    Directory *dir = new Directory(200);
                    dir->FetchFrom(tmp);
                    for(int i=0;i<dir->tableSize;++i)
                    {
                        if(dir->table[i].inUse)
                        {
                            printf("name: %s\n",dir->table[i].name);
                        }
                    }
//printf("di gui le!!!!!!\n");
                    dir_sector = dir->get_correct_dir(name);
                    delete tmp;
                    delete dir;
                    return dir_sector;
                }
            }
        }
    }
printf("return : %d\n",dir_sector);
    return dir_sector;
}