#ifndef WOMBATFUNCTIONS_H
#define WOMBATFUNCTIONS_H

// Copyright 2013-2019 Pasquale J. Rinaldi, Jr.
// Distrubted under the terms of the GNU General Public License version 2

#include "wombatinclude.h"
#include "globals.h"
#include "tskcpp.h"

std::string GetTime(void);
qint64 GetChildCount(QString filefilter);
void AppendPreviewReport(QString string);
void ReplacePreviewReport(QString string);
void RemovePreviewItem(QString itemid);

void AddItem(QString content, QString section, QString itemid, QString subitemid = "");
void RemItem(QString content, QString section, QString itemid, QString subitemid = "");

QStringList GetChildFiles(QString filefilter);
bool FileExists(const std::string& filename);
TSK_WALK_RET_ENUM RootEntries(TSK_FS_FILE* tmpfile, const char* tmppath, void* tmpptr);
TSK_WALK_RET_ENUM TreeEntries(TSK_FS_FILE* tmpfile, const char* tmppath, void* tmpptr);
void GenerateThumbnails(QString thumbid);
void GenerateHash(QString itemid);
void LoadImagesHash(void);
void SaveImagesHash(void);
void PopulateTreeModel(QString evidstring);
void FileRecurse(QString partitionpath, QString paraddr, QString rootinum);
void AddFileData(QString file, QString partpath, QString rootinum);
QString GetBlockList(TSK_FS_FILE* tmpfile);
void WriteEvidenceProperties(TSK_IMG_INFO* curimginfo, QString evidencepath, QString imgfullpath);
void WriteVolumeProperties(TSK_VS_INFO* curvolinfo, QString volumepath);
void WriteFileSystemProperties(TSK_FS_INFO* curfsinfo, QString partitionpath);
void WriteFileProperties(TSK_FS_FILE* curfileinfo, AddEvidenceVariable* aevar);
void BuildStatFile(TSK_FS_FILE* curfileinfo, const char* tmppath, AddEvidenceVariable* aevar);
void WriteAlternateDataStreamProperties(TSK_FS_FILE* curfileinfo, QString adsname, qint64 adssize, QString attrid, AddEvidenceVariable* aevar);
//void LogMessage(QString message);
TSK_WALK_RET_ENUM GetBlockAddress(TSK_FS_FILE* tmpfile, TSK_OFF_T off, TSK_DADDR_T addr, char* buf, size_t size, TSK_FS_BLOCK_FLAG_ENUM flags, void *ptr);
QString GetFilePermissions(TSK_FS_META* tmpmeta);
QString ConvertGmtHours(int gmtvar);
void ProcessExport(QString curid);
void InitializeEvidenceStructure(QString evidstring);
int SegmentDigits(int);
QString GetFileSystemLabel(TSK_FS_INFO* fsinfo);

#endif // wombatfunctions.h
