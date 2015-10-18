/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "bdiff.h"
#include "myio.h"
#include "msg.h"

void SameDate( const char *file, const char *as )
{
    auto struct stat            statblk;
    auto struct utimbuf         utimebuf;

    if( stat( as, &statblk ) == 0 ) {
        utimebuf.actime = statblk.st_atime;
        utimebuf.modtime = statblk.st_mtime;
        utime( file, &utimebuf );
    }
}

void MyOpen( MY_FILE *file, FILE *fd, const char *name )
{
    file->fd = fd;
    file->start = 0;
    file->len = 0;
    file->dirty = false;
    file->name = name;
}

void MyClose( MY_FILE *file )
{
    if( file->dirty ) {
        SeekCheck( fseek( file->fd, file->start, SEEK_SET ), file->name );
        if( fwrite( file->buff, 1, file->len, file->fd ) != file->len ) {
            PatchError( ERR_CANT_WRITE, file->name );
        }
    }
    fclose( file->fd );
}

void InBuffer( MY_FILE *file, foff off, size_t len, size_t eob )
{
    if( off < file->start || off+len > file->start+eob ) {
        if( file->dirty ) {
            SeekCheck( fseek( file->fd, file->start, SEEK_SET), file->name );
            if( fwrite( file->buff, 1, file->len, file->fd ) != file->len ) {
                PatchError( ERR_CANT_WRITE, file->name );
            }
        }
        if( ( off & ~(SECTOR_SIZE - 1) ) + BUFFER_SIZE > off + len ) {
            off &= ~(SECTOR_SIZE - 1);
        }
        SeekCheck( fseek( file->fd, off, SEEK_SET ), file->name );
        file->start = off;
        file->len = fread( file->buff, 1, BUFFER_SIZE, file->fd );
        file->dirty = false;
    }
}

void Input( MY_FILE *file, void *tmp, foff off, size_t len )
{
    InBuffer( file, off, len, file->len );
    memcpy( tmp, &file->buff[off - file->start], len );
}

void Output( MY_FILE *file, void *tmp, foff off, size_t len )
{
    InBuffer( file, off, len, BUFFER_SIZE );
    memcpy( &file->buff[off - file->start], tmp, len );
    if( file->len < off - file->start + len ) {
        file->len = (unsigned)( off - file->start + len );
    }
    file->dirty = true;
}
