// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
//printf("allocate\n");
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	return FALSE;		// not enough space

// init
    for (int i = 0; i < numSectors; i++)
	dataSectors[i] = freeMap->Find();
    return TRUE; 

// lab5 

    int left;
    int second_num;    // how many second indexes we need
    if(numSectors < NumDirect)  // only need direct
    {
//printf("only need direct index!!!!!!!!!!!!!!!!!!!\n");
        for (int i = 0; i < numSectors; i++)
	    dataSectors[i] = freeMap->Find();
        return TRUE;
    }
    else
    {
//printf("need second index!!!!!!!!!!!!!!!!!!!!!!1\n");
        for(int i=0;i<NumDirect;++i)
        {
            dataSectors[i] = freeMap->Find();
        }
        left = numSectors - NumDirect;
        second_num = left/32 + 1;
        for(int i=0;i<second_num;++i)
        {
            dataSectors[NumDirect + i] = freeMap->Find();
            int num = (left < 32)? left : 32;
            int *second = new int[num];
            for(int j=0;j<num;++j)
            {
                second[j] = freeMap->Find();
            }
            synchDisk->WriteSector(dataSectors[NumDirect + i], (char*)second);
            delete second;
            left -= 32;
        }
        return TRUE;
    }
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    for (int i = 0; i < numSectors; i++) {
	ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	freeMap->Clear((int) dataSectors[i]);
    }  

// lab5
//printf("deallocate\n");
    if(numSectors <= NumDirect)  // only need direct index
    {
        for (int i = 0; i < numSectors; i++) {
	    ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	    freeMap->Clear((int) dataSectors[i]);
        }
        return;
    }
    else
    {
        int left;
        for (int i = 0; i < NumDirect; i++) {
	    ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	    freeMap->Clear((int) dataSectors[i]);
        }
        left = numSectors - NumDirect;
        int second_num = left/32 + 1;
        for(int i=0;i<second_num;++i)
        {
            int num = (left < 32)? left : 32;
            char *tmp = new char[SectorSize];
            synchDisk->ReadSector(dataSectors[NumDirect + i],tmp);
            int *second = (int *)tmp;
            for(int j=0;j<num;++j)    // deallocate second first
            {
                ASSERT(freeMap->Test((int) second[j]));  // ought to be marked!
	            freeMap->Clear((int) second[j]);
            }
            ASSERT(freeMap->Test((int) dataSectors[NumDirect + i]));  // ought to be marked!
	        freeMap->Clear((int) dataSectors[NumDirect + i]);
            left -= 32;
        }
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    return(dataSectors[offset / SectorSize]);
// lab5
    int sector = offset / SectorSize;
    if(sector < NumDirect)
    {
        return dataSectors[sector];
    }
    else
    {
        sector -= NumDirect;
        int index = sector / 32;
        char *tmp = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect + index], tmp);
        int *second = (int *)tmp;
        int second_sector = sector - index * 32;
        int ans = second[second_sector];
        delete second;
        return ans;
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    /*
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
    */
// lab5
    printf("file_size: %d bytes\n", FileLength());
    int left = numSectors;
    if(numSectors <= NumDirect)
    {
        printf("file_sector: ");
        for(int i=0;i<numSectors;++i)
        {
            printf("%d ",dataSectors[i]);
        }
        printf("\n");
        return;
    }
    else
    {
        printf("sector of direct index: ");
        for(int i=0;i<NumDirect;++i)
        {
            printf("%d ",dataSectors[i]);
        }
        left -= NumDirect;
        int second_num = left/32 + 1;
        for(int i=0;i<second_num;++i)
        {
            int num = (left < 32)? left : 32;
            int *result;
            printf("\nsecond_sector %d of second index: %d\n",dataSectors[NumDirect+i],i+1);
            char *tmp = new char[32];
            synchDisk->ReadSector(dataSectors[NumDirect+i],tmp);
            printf("\tsector of second index %d: ",i+1);
            result = (int*)tmp;
            for(int j=0;j<num;++j)
            {
                printf("%d ",result[j]);
            }
            delete tmp;
            left -= 32;
        }
    }
} 



bool
FileHeader::extend(BitMap *bitMap, int fileSize)
{
    int sector = divRoundUp(fileSize, SectorSize);
//printf("sector in extend: %d\n",sector);
//printf("numBits: %d\n",bitMap->numBits);
    //bitMap->Print();
    if(bitMap->NumClear() < sector)
    {
        printf("not enough space!\n");
        return FALSE;
    }
    int already_sector = FileLength();
    already_sector = divRoundUp(already_sector, SectorSize);
    if(already_sector + sector > NumDirect)
    {
        printf("too big file!\n");
        return FALSE;
    }
    for(int i=0;i<sector;++i)
    {
        numBytes += SectorSize;
        numSectors++;
        dataSectors[numSectors-1] = bitMap->Find();
    }
//printf("numBytes in extend: %d\n\n",numBytes);
//printf("\n");
    return TRUE;
}