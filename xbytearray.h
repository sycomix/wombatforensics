#ifndef XBYTEARRAY_H
#define XBYTEARRAY_H

/** \cond docNever */

#include <QtCore>
#include "tskvariable.h"

/*! XByteArray represents the content of QHexEcit.
XByteArray comprehend the data itself and informations to store if it was
changed. The QHexEdit component uses these informations to perform nice
rendering of the data

XByteArray also provides some functionality to insert, replace and remove
single chars and QByteArras. Additionally some functions support rendering
and converting to readable strings.
*/
class XByteArray
{
public:
    explicit XByteArray();

    int AddressOffset();
    void SetAddressOffset(int offset);

    int AddressWidth();
    void SetAddressWidth(int width);
    int LinesPerPage(void);
    int LineCount(void);

    QByteArray & data();
    void SetData(QByteArray data);
    bool OpenImage(TskObject* tskobject);
    bool CloseImage();
    size_t ReadImage(size_t numbytes); // here is where i need to map the QFileDevice (as in # of pages)
    bool LoadImagePage(off_t pageindex);
    off_t SeekImage(off_t offset);

    /*
    bool dataChanged(int i);
    QByteArray dataChanged(int i, int len);
    void setDataChanged(int i, bool state);
    void setDataChanged(int i, const QByteArray & state);
    */
    int RealAddressNumbers();
    int size();

    /*
    QByteArray & insert(int i, char ch);
    QByteArray & insert(int i, const QByteArray & ba);

    QByteArray & remove(int pos, int len);

    QByteArray & replace(int index, char ch);
    QByteArray & replace(int index, const QByteArray & ba);
    QByteArray & replace(int index, int length, const QByteArray & ba);
*/
    QChar AsciiChar(int index);
    //QString ToReadableString(int start=0, int end=-1);
    int bytesperline;

signals:

public slots:

private:
    QByteArray _data;
    //QByteArray _changedData;

    int addressnumbers;                    // wanted width of address area
    int addressoffset;                     // will be added to the real addres inside bytearray
    int realaddressnumbers;                // real width of address area (can be greater then wanted width)
    //int _oldSize;                           // size of data
    TskObject* tskptr;
    off_t imagesize; // bytes per image
    off_t linecount; // number of lines
    off_t blocksize;
    off_t currentoffset;
    off_t firstoffset;
    off_t lastoffset;
    off_t firstloadedoffset;
    off_t lastloadedoffset;
    off_t currentlineindex;
    off_t loadedlinecount;
    off_t blocklinecount;
    off_t firstline, lastline;
    off_t firstloadedline, lastloadedline;
    off_t loadedsize;
    //off_t pagecount;
    //off_t currentpageindex;
    //off_t firstpage, lastpage;
};

/** \endcond docNever */
#endif // XBYTEARRAY_H
