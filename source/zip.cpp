/*
	Purpose of this file is to simulate the fopen/fclose/fread/fseek stdio functions
	for use with compressed N64 rom images.
*/

// Disable SAFESEH to use the old library
#pragma comment(lib, "ZLIB.LIB")

#include <stdio.h>
#include <string.h>
#include "unzip.h"
#include "zconf.h"
#include "zlib.h"

unz_file_info file_info;

int FindN64Rom (unzFile fp) {
	char szFileName[256];
	if (fp == NULL)
		return -1;
	if (unzGoToFirstFile(fp) == UNZ_OK) {
		do {
			if (unzGetCurrentFileInfo(fp, 
					&file_info, 
					szFileName, 
					256, 
					NULL, 
					0, 
					NULL, 
					0) == UNZ_OK) {
				if(stricmp(&szFileName[strlen(szFileName) - 4], ".bin") == 0
					|| stricmp(&szFileName[strlen(szFileName) - 4], ".v64") == 0
					|| stricmp(&szFileName[strlen(szFileName) - 4], ".n64") == 0
					|| stricmp(&szFileName[strlen(szFileName) - 4], ".u64") == 0
					|| stricmp(&szFileName[strlen(szFileName) - 4], ".z64") == 0
					|| stricmp(&szFileName[strlen(szFileName) - 4], ".pal") == 0
					|| stricmp(&szFileName[strlen(szFileName) - 4], ".usa") == 0
					|| stricmp(&szFileName[strlen(szFileName) - 4], ".jap") == 0
					|| stricmp(&szFileName[strlen(szFileName) - 4], ".eur") == 0
					|| stricmp(&szFileName[strlen(szFileName) - 4], ".rom") == 0) {
						if(unzOpenCurrentFile(fp)== UNZ_OK) {
							return 0;
						}
				}
			} else {
				return -3;
			}
		} while(unzGoToNextFile(fp) == UNZ_OK);
	} else {
		return -4;
	}
	return -5;
}

FILE *zipopen (const char *fname, const char *flags) {
	FILE *fp;
	fp = (FILE *)unzOpen (fname);
	if (FindN64Rom ((unzFile)fp))
		return NULL;
	return fp;
}

int zipclose (FILE *fp) {
	unzClose ((unzFile)fp);
	return 0;
}

int zipseek (FILE *fp, long offset, int flags) {
	if (flags == SEEK_SET) {
		unzOpenCurrentFile(fp);
	} else if (flags == SEEK_END) {
		unzGetCurrentFileInfo(fp, 
				&file_info, 
				NULL, 
				0, 
				NULL, 
				0, 
				NULL, 
				0);
	}
	return 0;
}

long ziptell (FILE *fp) {
	return (file_info.uncompressed_size);
}

size_t zipread (void *data, size_t size, size_t count, FILE *fp) {
	return (unzReadCurrentFile(fp, data, size*count));
}
/*
void ReadZippedRomData(char* RomPath)
{
    unzFile fp;
    unsigned long gROMLength; //size in bytes of the ROM
    free(ROM_Image);
    
  if(fp = unzOpen(RomPath))
  {
    char szFileName[256];
    if(unzGoToFirstFile(fp) == UNZ_OK)
    {
      do
      {
        unz_file_info file_info;
        if(unzGetCurrentFileInfo(fp,
                         &file_info,
                         szFileName,
                         256,
                         NULL,
                         0,
                         NULL,
                         0) == UNZ_OK)
        {
          if(stricmp(&szFileName[strlen(szFileName) - 4], ".bin") == 0
            || stricmp(&szFileName[strlen(szFileName) - 4], ".v64") == 0
            || stricmp(&szFileName[strlen(szFileName) - 4], ".rom") == 0)
          {
            gROMLength = file_info.uncompressed_size; //get size of ROM

            if (((gROMLength & 0xFFFF)) == 0)
                gAllocationLength = gROMLength;
            else
                gAllocationLength = gROMLength + (0x10000 - (gROMLength & 0xFFFF));

            if (( ROM_Image = (uint8*)calloc( gAllocationLength, sizeof( uint8 ))) == NULL ) 
            {
                DisplayError("Error: Could not allocate rom image buffer.\n" );
                unzClose(fp);
                exit( 0 );
            }

            InitMemoryLookupTables();

            if(unzOpenCurrentFile(fp)== UNZ_OK)
            {
                if(unzReadCurrentFile(fp, ROM_Image, sizeof( uint8 ) * gROMLength) == (int)(sizeof( uint8 ) * gROMLength))
                {
                    ByteSwap(gAllocationLength, ROM_Image);
                
                    memcpy((uint8*)&SP_DMEM, ROM_Image, 0x1000);
                    unzClose(fp);
                    return(1);
                }
                else
                {
                    DisplayError("File could not be read. gROMLength = %08X\n", gROMLength );
                    unzClose(fp);
                    exit(0);
                }
                unzCloseCurrentFile(fp);
            }
            else
            {
                DisplayError("File could not be read: CRC Error in zip." );
                unzClose(fp);
                exit(0);
            }
          }
        }
        else
        {
              DisplayError("File could not unzipped." );
              unzClose(fp);
              exit(0);
        }
      }
      while(unzGoToNextFile(fp) == UNZ_OK);
    }
    unzClose(fp);
  }
}
*/