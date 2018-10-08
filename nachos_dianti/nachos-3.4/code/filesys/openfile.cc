// openfile.cc 
//	Routines to manage an open Nachos file.  As in UNIX, a
//	file must be open before we can read or write to it.
//	Once we're all done, we can close it (in Nachos, by deleting
//	the OpenFile data structure).
//
//	Also as in UNIX, for convenience, we keep the file header in
//	memory while the file is open.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "filehdr.h"
#include "openfile.h"
#include "system.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector)
{ 
    hdr = new FileHeader;
    hdr->FetchFrom(sector);
    seekPosition = 0;
// lab5
    _sector = sector;
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile()
{
    delete hdr;
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void
OpenFile::Seek(int position)
{
    seekPosition = position;
}	

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

// lab5
int
OpenFile::synch_Read(char *into, int numBytes)
{
//printf("synch_read\n");
    int start = seekPosition;
    int last = hdr->ByteToSector(start);
    for(int i=0;i<numBytes;++i)
    {
        int sector = hdr->ByteToSector(start+i);
        if(sector!=last || i==0)
        {
            synchDisk->cnt_p(sector);
            synchDisk->read_cnt[sector]++;
            if(synchDisk->read_cnt[sector] == 1)
            {
                synchDisk->rw_p(sector);
            }
            last = sector;
            synchDisk->cnt_v(sector);
        }
    } 

    int result = ReadAt(into, numBytes, seekPosition);
    seekPosition += result;

    last = hdr->ByteToSector(start);
    for(int i=0;i<numBytes;++i)
    {
        int sector = hdr->ByteToSector(start+i);
        if(sector!=last || i==0)
        {
            synchDisk->cnt_p(sector);
            synchDisk->read_cnt[sector]--;
            if(synchDisk->read_cnt[sector] == 0)
            {
                synchDisk->rw_v(sector);
            }
            last = sector;
            synchDisk->cnt_v(sector);
        }
    } 

    return result;
}

int
OpenFile::synch_Write(char *into, int numBytes)
{
//printf("synch_write\n");
    int start = seekPosition;
//printf("seekPosition of thread %s(tid: %d): %d\n",currentThread->getName(),currentThread->get_thread_id(),seekPosition);
    int last = hdr->ByteToSector(start);
    for(int i=0;i<numBytes;++i)
    {
//printf("i: %d\n",i);
        int sector = hdr->ByteToSector(start+i);
        if(sector!=last || i==0)
        {
            synchDisk->rw_p(sector);
//printf("i: %d  acquire!!!!!!!!!!!!!!!!!\n",i);
        }
    } 
 
//printf("numBytes start write: %d\n",numBytes);
    int result = WriteAt(into, numBytes, seekPosition);
    seekPosition += result;
//printf("return value in write: %d\n",result);

    last = hdr->ByteToSector(start);
    for(int i=0;i<numBytes;++i)
    {
//printf("i2: %d\n",i);
        int sector = hdr->ByteToSector(start+i);
        if(sector!=last || i==0)
        {
            synchDisk->rw_v(sector);
        }
    } 
//printf("result: %d\n",result);
    return result;
}

int
OpenFile::synch_Write2(char *into, int numBytes)
{
//printf("synch_write2\n");
    int start = seekPosition;
//printf("seekPosition of thread %s(tid: %d): %d\n",currentThread->getName(),currentThread->get_thread_id(),seekPosition);
    int last = hdr->ByteToSector(start);
    for(int i=0;i<numBytes;++i)
    {
//printf("i: %d\n",i);
        int sector = hdr->ByteToSector(start+i);
        if(sector!=last || i==0)
        {
            synchDisk->rw_p(sector);
//printf("i: %d  acquire!!!!!!!!!!!!!!!!!\n",i);
        }
    } 
    //currentThread->Yield();

//printf("numBytes start write: %d\n",numBytes);
    int result = WriteAt(into, numBytes, seekPosition);
    seekPosition += result;
//printf("return value in write: %d\n",result);

    last = hdr->ByteToSector(start);
    for(int i=0;i<numBytes;++i)
    {
//printf("i2: %d\n",i);
        int sector = hdr->ByteToSector(start+i);
        if(sector!=last || i==0)
        {
            synchDisk->rw_v(sector);
        }
    } 
//printf("result: %d\n",result);
    return result;
}

int
OpenFile::Read(char *into, int numBytes)
{
   int result = ReadAt(into, numBytes, seekPosition);
   seekPosition += result;
   return result;
}

int
OpenFile::Write(char *into, int numBytes)
{
//printf("numBytes start write: %d\n",numBytes);
   int result = WriteAt(into, numBytes, seekPosition);
   seekPosition += result;
//printf("return value in write: %d\n",result);
   return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------

int
OpenFile::ReadAt(char *into, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
    	return 0; 				// check request
    if ((position + numBytes) > fileLength)		
	numBytes = fileLength - position;
    DEBUG('f', "Reading %d bytes at %d, from file of length %d.\n", 	
			numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;
//printf("ReadAt::  numBytes: %d firstSector: %d lastSector: %d\n",numBytes, firstSector, lastSector);
    // read in all the full and partial sectors that we need
    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)	
    {
//printf("sector index: %d\n",hdr->ByteToSector(i * SectorSize));
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);
    }
    // copy the part we want
    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);
    delete [] buf;
//printf("return val of READAT: %d\n",numBytes);
    return numBytes;
}

int
OpenFile::WriteAt(char *from, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;
//printf("position: %d     fileLength: %d\n",position, fileLength);
    if ((numBytes <= 0) || (position > fileLength))
    {
        //printf("##################################\n");
	    return 0;				// check request
    }
    if ((position + numBytes) > fileLength)
    {
        //numBytes = fileLength - position;
// lab5
printf("need more space!\n");
        BitMap *bitMap = new BitMap(NumSectors);
        bitMap->FetchFrom(fileSystem->freeMapFile);
        int size = position + numBytes - fileLength;
printf("fileLength before extend: %d\n", hdr->FileLength());
        hdr->extend(bitMap, size);
printf("fileLength after extend: %d\n\n", hdr->FileLength());
        hdr->WriteBack(_sector);
        bitMap->WriteBack(fileSystem->freeMapFile);
    }
    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n", 	
			numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

//printf("WriteAt::  numBytes: %d firstSector: %d lastSector: %d\n",numBytes, firstSector, lastSector);
    buf = new char[numSectors * SectorSize];

    firstAligned = (position == (firstSector * SectorSize));
    lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

// read in first and last sector, if they are to be partially modified
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);	
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
				SectorSize, lastSector * SectorSize);	

// copy in the bytes we want to change 
    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

// write modified sectors back
    for (i = firstSector; i <= lastSector; i++)	
        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);
    delete [] buf;
//printf("return val of writeAt: %d\n", numBytes);
    return numBytes;
}

//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
OpenFile::Length() 
{ 
    return hdr->FileLength(); 
}
