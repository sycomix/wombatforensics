/* $Id: reader.cpp,v 1.11 2008-09-11 01:49:00 salem Exp $
 * This file is part of lfhex.
 * Copyright (C) 2006 Salem Ganzhorn <eyekode@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * Updates to enable editor to open a forensic image.
 * Copyright (C) 2014 Pasquale J. Rinaldi, Jr. <pjrinaldi@gmail.com>
 */ 
#include <stdexcept>
#include <algorithm>
#include <new>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "imgreader.h"
// some systems #define to map fn's to thier 64 bit
                      // equivalents. make sure the header gets processed the
                      // same as the .cc file

ImageReader::ImageReader(off_t npages, off_t pageSize) : _pageSize(pageSize)
{
  _maxPages = _freePages = (npages < 10) ? 10: npages;
  _error   = "";
  _is_open = false;
  _offset  = 0;
  _size    = 0;
  _firstPage = -1;
  _lastPage  = -1;
}

ImageReader::~ImageReader()
{
  if( _is_open )
    close();
}

//
// public methods
//
off_t ImageReader::CurrentPage()
{
    return _offset/_pageSize;
}

bool ImageReader::openimage(TskObject* tskpointer)
{
    tskptr = tskpointer;
    if(is_open())
        close();
    _size = tskptr->imglength; // length in bytes for selected file
    //qDebug() << "image length:" << tskptr->imglength;
    _pageSize = tskptr->blocksize;
    off_t npages = _size/_pageSize;
    if((_size - 1) % _pageSize != 0)
        npages++;
    //qDebug() << "block size:" << _pageSize << "num of pages:" << npages;
    _numpages = npages;
    _data.resize(npages);
    fill(_data.begin(), _data.begin()+npages, (uchar*)0);
    _is_open = true;
    _firstPage = _lastPage = 0;

    //return true;
    return loadimagepage(0);
}

bool ImageReader::close()
{
    if(!is_open())
        return false;
 /*
  if( !is_open() ) {
    _error = "attempted to close non-open reader.";
    return false;
  }
  */

    _error = "";
  /*
  if( EOF == fclose(_fptr) ) {
    _error = strerror(errno);
    return false;
  }
  */
  // free data pages
    vector<uchar*>::iterator itr;
    for( itr = _data.begin(); itr != _data.end(); ++itr )
        delete [] (*itr);

    _is_open = false;
    _firstPage = _lastPage = -1;
    _freePages = _maxPages;
    return true;
}

bool ImageReader::is_open() const
{
  return _is_open;
}

size_t ImageReader::readimage(vector<uchar>& v, size_t numbytes)
{
    int lastPageIdx = 0;
    size_t bytesread;
    // MODIFY THIS TO WHERE IT'S NOT LOADING THE WHOLE IMAGE, BUT ONLY A PAGE'S WORTH..., IF ANYTHING AT ALL HERE
    if(_offset+(int)numbytes >= size())
    {
        _eof = true;
        if(size() == 0)
            v.erase(v.begin(), v.end()); // added to clear the previous values when a file is of size 0
        lastPageIdx = _data.size()-1;
        bytesread = size() - tell();
        numbytes = bytesread;
    }
    else
    {
        lastPageIdx = (_offset+numbytes)/_pageSize;
        bytesread = numbytes;
    }

    if(!numbytes)
        return numbytes;
    v.erase(v.begin(), v.end());
    v.reserve(v.size() + numbytes);
    for(int page = _offset/_pageSize; page <= lastPageIdx; page++)
    {
        try
        {
            loadimagepage(page);
        }
        catch(bad_alloc)
        {
            return(_offset/_pageSize - page)*_pageSize;
        }
        int start = _offset%_pageSize;
        int stop  = (page == lastPageIdx)? start+numbytes: _pageSize;
        for(int i = start; i < stop; i++)
        {
            v.push_back(_data[page][i]);
        }
        numbytes -= stop-start;
        _offset  += stop-start;
    }

    return bytesread;
}

bool ImageReader::eof()
{
  return (is_open())? _eof : 0;
}

off_t ImageReader::seekimage(off_t offset)
{
    if(!is_open())
        return -1;

    _eof = false;
    _offset = max(min(offset, size()-1), (off_t)0);

    return _offset;
}

off_t ImageReader::tell() const
{
  if(!is_open())
    return -1;
  return _offset;
}

off_t ImageReader::size() const
{
  return _size;
}

off_t ImageReader::NumberPages() const
{
    return _numpages;
    //return _maxPages;
}

const char* ImageReader::lastError() const
{
  return _error.c_str();
}

//
// Protected member fn's
//

bool ImageReader::loadimagepage(off_t pageIdx)
{
    off_t retval = 0;
    if(!is_open())
        return false;
    /*
    if(_data[pageIdx] != 0)
        return true;
    */
    if(!nFreePages())
    {
        if(abs(_firstPage - pageIdx) > abs(_lastPage - pageIdx))
            while(!freePage(_firstPage++));
        else
            while(!freePage(_lastPage--));
    }
    _data[pageIdx] = new uchar[_pageSize];
    --nFreePages();
    if(tskptr->objecttype <= 5)
    {
        retval = tsk_img_read(tskptr->readimginfo, pageIdx*_pageSize, (char*)_data[pageIdx], _pageSize);
    }
    if(retval > 0)
    {
        if(pageIdx < _firstPage)
            _firstPage = pageIdx;
        if(pageIdx > _lastPage)
            _lastPage = pageIdx;
    }

    return 1;
}

bool ImageReader::loadPage(off_t pageIdx)
{
  if( !is_open() )
    return false;
  if(_data[pageIdx] != 0)
    return true;

  if( !nFreePages() ) {
    // free the page which is the furthest away from the page we are loading

    // this could be trouble if off_t is unsigned!
    if( abs(_firstPage - pageIdx) > abs(_lastPage - pageIdx) ) 
      while(!freePage(_firstPage++));
    else
      while(!freePage(_lastPage--));
  }
  _data[pageIdx] = new uchar [_pageSize];
  // page added, subtract from num available
  --nFreePages();

  fseek(_fptr,pageIdx*_pageSize, SEEK_SET);

  off_t retval = fread(_data[pageIdx], sizeof(uchar), _pageSize, _fptr);

  if( retval ) {
    if( pageIdx < _firstPage )
      _firstPage = pageIdx;

    if( pageIdx > _lastPage ) 
      _lastPage = pageIdx;
  }
  return retval;
}

bool ImageReader::freePage(off_t pageIdx)
{
  // check range
  if( pageIdx < 0 || (unsigned)pageIdx >= _data.size() || !_data[pageIdx] )
    return false;
  
  delete [] _data[pageIdx];
  _data[pageIdx] = 0;
  ++nFreePages(); // page freed
  return true;
}

uchar ImageReader::operator[] (off_t offset)
{
  if( !is_open() )
    throw logic_error("ImageReader::operator[] - attempt to access non-open file.");
  if( offset < 0 )
    throw out_of_range("ImageReader::operator[] - "
		       "attempt to access negative offset.");
  if( offset >= size() ) 
    throw out_of_range("ImageReader::operator[] - "
		       "attempt to access past end of file");

  off_t page_idx = offset/_pageSize;
  bool loaded = loadPage( page_idx );
  assert( loaded );
  return _data[page_idx][ offset%_pageSize ];
}

off_t
ImageReader::nFreePages() const
{
    //qDebug() << "Free Pages" << _freePages;
   return _freePages;
}

off_t&
ImageReader::nFreePages()
{
   return _freePages;
}

bool
ImageReader::dataIsAtOffset( const vector<uchar>& data, off_t pos )
{
    size_t i = 0;
    while( i < data.size() ) {
	if( data[i] != (*this)[pos+i] ) {
	    return false;
	}
	++i;
    }
    return true;
}


off_t
ImageReader::findIndex( off_t start, const vector<uchar>& data, off_t stop )
{
    off_t pos = start;
    while( pos <= stop ) {
	if( data[0] == (*this)[pos] ) {
	    if( dataIsAtOffset( data, pos ) ) {
		return pos;
	    }
	}
	++pos;
    }
    return size();
}

off_t
ImageReader::rFindIndex( off_t start, const vector<uchar>& data, off_t stop )
{
    off_t pos = start;
    while( pos >= stop ) {
	if( data[0] == (*this)[pos] ) {
	    if( dataIsAtOffset( data, pos ) ) {
		return pos;
	    }
	}
	--pos;
    }
    return size();
}

void ImageReader::SetData(vector<uchar*> tmpdata)
{
    _data = tmpdata;
}

//
// ReadBuffer friends and aquaintances (only for debugging)
//

#if 0
ostream& operator<< (ostream&out, const ReadBuffer& buff)
{
  ios_base::fmtflags old_flags = out.flags();
  out.flags(old_flags | ios::hex | ios::showbase);
  for(size_t i = 0; i < buff.size(); i++)
    out << buff[i];
  // restore old flags
  out.flags(old_flags);
  return out;
}
#endif