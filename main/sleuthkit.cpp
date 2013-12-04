#include "sleuthkit.h"

static std::string getFileType(const char *name)
{
    std::string filename = name;
    size_t pos = filename.rfind('.');
    if (pos != std::string::npos)
    {
        std::string suffix = filename.substr(pos);
        std::string result;
        for (size_t i=0; i < suffix.size(); i++)
        {
            result += (char)tolower(suffix[i]);
        }
        return result;
    }
    else
        return std::string("");
}
QString DisplayTimeUTC(int unixtime)
{
    QDateTime tmptime;
    QString tmpstring = "";
    tmptime.setTime_t(unixtime);
    tmptime.setTimeSpec(Qt::UTC);
    if(tmptime.date().toString(Qt::ISODate).compare("1969-12-31") != 0)
        tmpstring = tmptime.toString(Qt::ISODate);
    
    return tmpstring;
}

SleuthKitPlugin::SleuthKitPlugin(WombatDatabase* wombatcasedata)
{
    wombatdata = wombatcasedata; 
}
void SleuthKitPlugin::Initialize(WombatVariable wombatVariable)
{
    wombatvariable = wombatVariable;
    SetupSystemProperties();
    SetupLog();
    SetupScheduler();
    SetupFileManager();
    SetupImageDatabase();
    SetupBlackboard();
    fprintf(stderr, "SleuthKit Existsi\n");
    qRegisterMetaType<WombatVariable>("WombatVariable");
    connect(this, SIGNAL(SetLogVariable(WombatVariable)), log, SLOT(LogVariable(WombatVariable)), Qt::DirectConnection);
}

void SleuthKitPlugin::SetupImageDatabase()
{
    // initialize dummy database to create copy new imagedb's from.
    try
    {
        initialdb = new TskImgDBSqlite(wombatvariable.datapath.toStdString().c_str(), "initial.db");
        if(initialdb->initialize() != 0)
            fprintf(stderr, "Error initializing StarterDB\n");
        else
        {
            fprintf(stderr, "Starter DB was Initialized Successfully!\n");
        }
        TskServices::Instance().setImgDB(*initialdb);
        fprintf(stderr, "Loading Starter ImageDB was Successful!\n");
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Loading Starter ImageDB: %s\n", ex.message().c_str());
    }
}

void SleuthKitPlugin::OpenEvidence(WombatVariable wombatVariable)
{
    wombatvariable = wombatVariable;
    QString oldstring = wombatvariable.datapath + "initial.db";
    QString newstring = wombatvariable.evidencedirpath + wombatvariable.evidencedbname;
    if(QFile::copy(oldstring.toStdString().c_str(), newstring.toStdString().c_str()))
    {
        fprintf(stderr, "File Copy Was Successful\n");
        // copy was successful
        try
        {
            tmpdb = new TskImgDBSqlite(wombatvariable.evidencedirpath.toStdString().c_str(), wombatvariable.evidencedbname.toStdString().c_str());
        }
        catch(TskException &ex)
        {
            fprintf(stderr, "Tsk New Exception: %s\n", ex.message().c_str());
        }
        try
        {
            if(tmpdb->initialize() != 0)
                fprintf(stderr, "Error intializing Evidence DB\n");
            else
            {
                fprintf(stderr, "Evidence DB was initialized successfully\n");
            }
        }
        catch(TskException &ex)
        {
            fprintf(stderr, "Tsk Open Exception: %s\n", ex.message().c_str());
        }
        try
        {
            TskServices::Instance().setImgDB(*tmpdb);
        }
        catch(TskException &ex)
        {
            fprintf(stderr, "Services Set ImageDB: %s\n", ex.message().c_str());
        }
    }
    else
    {
        fprintf(stderr, "File Copy was NOT successful\n");
        // copy was not successful
        // exit out with error that image already added...
    }
    fprintf(stderr, "Evidence ImgDB Path: %s\n", wombatvariable.evidencepath.toStdString().c_str());
    
    int filecount = 0;
    int processcount = 0;
    TskImageFileTsk imagefiletsk;
    try
    {
        imagefiletsk.open(wombatvariable.evidencepath.toStdString());
        TskServices::Instance().setImageFile(imagefiletsk);
        fprintf(stderr, "Opening Image File was successful!\n");
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Opening Evidence: %s\n", ex.message().c_str());
    }
    try
    {
        imagefiletsk.extractFiles();
        fprintf(stderr, "Extracting Evidence was successful\n");
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Extracting Evidence: %s\n", ex.message().c_str());
    }
    // Get Number of Files found here 
    filecount = TskServices::Instance().getImgDB().getNumFiles();
    emit UpdateStatus(filecount, processcount);
    LOGINFO("Processing Evidence Started");
    wombatdata->InsertMsg(wombatvariable.caseid, wombatvariable.evidenceid, wombatvariable.jobid, 2, "Processing Evidence Started");
    TskSchedulerQueue::task_struct *task;
    TskPipelineManager pipelinemgr;
    TskPipeline* filepipeline;
    TskSchedulerQueue::task_struct* currenttask;
    try
    {
        filepipeline = pipelinemgr.createPipeline(TskPipelineManager::FILE_ANALYSIS_PIPELINE);
    }
    catch(const TskException &ex)
    {
        fprintf(stderr, "Error creating file analysis pipeline: %s\n", ex.message().c_str());
    }
    // HOW ABOUT LOOP OF RUNNABLES...
    //
    /*
    for(int i=0; i < scheduler.mapvector.size(); i++)
    {
        currenttask = scheduler.mapvector[i];
        try
        {
            if(currenttask->task == Scheduler::FileAnalysis && filepipeline && !filepipeline->isEmpty())
            {
                TaskRunner* trunner = new TaskRunner(currenttask, filepipeline);
                QThreadPool::globalInstance()->start(trunner);
                //filepipeline->run(task->id);
            }
            else
            {
                fprintf(stderr, "Skipping task: %d\n", task->task);
            }
            //delete task;
        }
        catch(TskException &ex)
        {
            fprintf(stderr, "TskException: %s\n", ex.message().c_str());
        }

        //processcount++;
        //emit UpdateStatus(filecount, processcount);
        //emit UpdateMessageTable();

        //TaskRunner* trunner = new TaskRunner(scheduler.mapvector[i], filepipeline);
        //QThreadPool::globalInstance()->start(trunner);
    }
    delete currenttask;
    */
    /*
    QFutureWatcher<void> watcher;
    std::vector<TskSchedulerQueue::task_struct* > tmpvector;
    watcher.setFuture(QtConcurrent::map(&tmpvector, &SleuthKitPlugin::TaskMap));
    //watcher.setFuture(QtConcurrent::map(&((std::vector<TskSchedulerQueue::task_struct*>)scheduler.mapvector), &SleuthKitPlugin::TaskMap));
    watcher.waitForFinished();
    */
    // QT CONCURRENT TEST
    while((task = scheduler.nextTask()) != NULL)
    {
        try
        {
            if(task->task == Scheduler::FileAnalysis && filepipeline && !filepipeline->isEmpty())
            {
                filepipeline->run(task->id);
            }
            else
            {
                fprintf(stderr, "Skipping task: %d\n", task->task);
            }
            delete task;
        }
        catch(TskException &ex)
        {
            fprintf(stderr, "TskException: %s\n", ex.message().c_str());
        }
        processcount++;
        emit UpdateStatus(filecount, processcount);
        emit UpdateMessageTable();
    }
    if(filepipeline && !filepipeline->isEmpty())
    {
        filepipeline->logModuleExecutionTimes();
    }
    LOGINFO("Processing Evidence Finished");
    wombatdata->InsertMsg(wombatvariable.caseid, wombatvariable.evidenceid, wombatvariable.jobid, 2, "Processing Evidence Finished");
    fprintf(stderr, "File Count: %d - Process Count: %d\n", filecount, processcount);
    wombatdata->UpdateJobEnd(wombatvariable.jobid, filecount, processcount);
    GetImageTree(wombatvariable, 1);
    LOGINFO("Adding Evidence Finished");
    wombatdata->InsertMsg(wombatvariable.caseid, wombatvariable.evidenceid, wombatvariable.jobid, 2, "Adding Evidence Finished");
}

void SleuthKitPlugin::TaskMap(TskSchedulerQueue::task_struct* &task)
{
    TskPipelineManager pipelinemgr;
    TskPipeline* filepipeline;
    try
    {
        filepipeline = pipelinemgr.createPipeline(TskPipelineManager::FILE_ANALYSIS_PIPELINE);
    }
    catch(const TskException &ex)
    {
        fprintf(stderr, "Error creating file analysis pipeline: %s\n", ex.message().c_str());
    }
    try
    {
        if(task->task == Scheduler::FileAnalysis && filepipeline && !filepipeline->isEmpty())
        {
            filepipeline->run(task->id);
        }
        else
        {
            fprintf(stderr, "Skipping task: %d\n", task->task);
        }
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "TskException: %s\n", ex.message().c_str());
    }
    if(filepipeline && !filepipeline->isEmpty())
    {
        filepipeline->logModuleExecutionTimes();
    }
}

void SleuthKitPlugin::SetEvidenceDB(WombatVariable wombatVariable)
{
    wombatvariable = wombatVariable;
    try
    {
        tmpdb = new TskImgDBSqlite(wombatvariable.evidencedirpath.toStdString().c_str(), wombatvariable.evidencedbname.toStdString().c_str());
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Tsk New Exception: %s\n", ex.message().c_str());
    }
    try
    {
        if(tmpdb->open() != 0)
            fprintf(stderr, "Error intializing Evidence DB\n");
        else
        {
            fprintf(stderr, "Evidence DB was initialized successfully\n");
        }
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Tsk Open Exception: %s\n", ex.message().c_str());
    }
    try
    {
        TskServices::Instance().setImgDB(*tmpdb);
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Services Set ImageDB: %s\n", ex.message().c_str());
    }
}

void SleuthKitPlugin::ShowFile(WombatVariable wombatVariable)
{
    QString curtmpfilepath = "";
    wombatvariable = wombatVariable;
    SetEvidenceDB(wombatvariable);
    fprintf(stderr, "file id: %i\n", wombatvariable.fileid);
    if(wombatvariable.fileid >= 0)
        curtmpfilepath = GetFileContents(wombatvariable.fileid);
    else if(wombatvariable.fileid == -1) // image file
    {
        curtmpfilepath = wombatvariable.evidencepath;
        fprintf(stderr, "dd image path: %s\n", wombatvariable.evidencepath.toStdString().c_str());
        // QString imagename = wombatvariable.evidencepath.split("/").last();
    }
    else if(wombatvariable.fileid == -2) // volume file
    {
        curtmpfilepath = GetVolumeFilePath(wombatvariable, wombatvariable.volid);
        fprintf(stderr, "tmp file path: %s\n", wombatvariable.tmpfilepath.toStdString().c_str());
        fprintf(stderr, "vol file path %s\n", curtmpfilepath.toStdString().c_str());
    }
    else if(wombatvariable.fileid == -3) // file system file
    {
        // fs_info sql call provides root_inum and last inum.
        // 
    }
    else
        curtmpfilepath = ""; // set to "" to load nothing.
    emit LoadFileContents(curtmpfilepath);
}

void SleuthKitPlugin::ExportFiles(WombatVariable wombatVariable)
{
    wombatvariable = wombatVariable;
    int processcount = 0;
    for(int i = 0; i < wombatvariable.exportdatalist.size(); i++)
    {
        processcount++;
        wombatvariable.evidencepath = QString::fromStdString(wombatvariable.exportdatalist[i].evidencepath);
        wombatvariable.evidencedbname = QString::fromStdString(wombatvariable.exportdatalist[i].evidencedbname);
        SetEvidenceDB(wombatvariable);
        ExportFile(wombatvariable.exportdatalist[i].fullpath, wombatvariable.exportdatalist[i].id);
        std::string tmpstring = wombatvariable.exportdatalist[i].fullpath + " Exported";
        wombatdata->InsertMsg(wombatvariable.caseid, wombatvariable.evidenceid, wombatvariable.jobid, 2, tmpstring.c_str());
        emit UpdateStatus(wombatvariable.exportdatalist[i].exportcount, processcount);
        emit UpdateMessageTable();
    }
    FinishExport(processcount);
}

void SleuthKitPlugin::FinishExport(int processcount)
{
    LOGINFO("File Export Finished");
    wombatdata->InsertMsg(wombatvariable.caseid, wombatvariable.evidenceid, wombatvariable.jobid, 2, "File Export Finished");
    wombatdata->UpdateJobEnd(wombatvariable.jobid, wombatvariable.exportdatalist[0].exportcount, processcount);
    emit UpdateMessageTable();
}

void SleuthKitPlugin::RefreshTreeViews(WombatVariable wombatVariable)
{
    wombatvariable = wombatVariable;
    QStringList evidencelist = wombatdata->ReturnCaseActiveEvidenceID(wombatvariable.caseid);
    for(int i=0; (i < evidencelist.count() / 3); i++)
    {
        wombatvariable.evidenceid = evidencelist[3*i].toInt();
        wombatvariable.evidencepath = evidencelist[(3*i+1)];
        wombatvariable.evidencedbname = evidencelist[(3*i+2)];
        SetEvidenceDB(wombatvariable);
        GetImageTree(wombatvariable, 0);
    }
}

void SleuthKitPlugin::PopulateCase(WombatVariable wombatVariable)
{
    wombatvariable = wombatVariable;
    QStringList evidencepathlist;
    QStringList evidencejoblist = wombatdata->ReturnCaseEvidenceIdJobIdType(wombatvariable.caseid);
    int isevidencedeleted = 0;
    for(int i=0; i < (evidencejoblist.count() / 3); i++)
    {
        wombatvariable.jobid = evidencejoblist[(3*i)].toInt();
        wombatvariable.jobtype = evidencejoblist[(3*i+1)].toInt();
        wombatvariable.evidenceid = evidencejoblist[(3*i)+2].toInt();
        wombatvariable.evidencepath = wombatdata->ReturnEvidencePath(wombatvariable.evidenceid);
        isevidencedeleted = wombatdata->ReturnEvidenceDeletedState(wombatvariable.evidenceid);
        wombatvariable.evidencedbname = wombatvariable.evidencepath.split("/").last() + ".db";
        emit SetLogVariable(wombatvariable);
        if(isevidencedeleted == 0 && wombatvariable.jobtype == 1)
        {
            SetEvidenceDB(wombatvariable);
            GetImageTree(wombatvariable, 0);
        }
        emit PopulateProgressWindow(wombatvariable);
    }
}

void SleuthKitPlugin::SetupSystemProperties()
{
    QString tmpPath = wombatvariable.settingspath;
    tmpPath += "/tsk-config.xml";
    QFile tmpFile(tmpPath);
    if(!tmpFile.exists()) // if tsk-config.xml does not exist, create and write it here
    {
        if(tmpFile.open(QFile::WriteOnly | QFile::Text))
        {   
            QXmlStreamWriter xml(&tmpFile);
            xml.setAutoFormatting(true);
            xml.writeStartDocument();
            xml.writeStartElement("TSK_FRAMEWORK_CONFIG");
            xml.writeStartElement("CONFIG_DIR");
            xml.writeCharacters(wombatvariable.settingspath);
            xml.writeEndElement();
            xml.writeStartElement("MODULE_DIR");
            xml.writeCharacters("/usr/local/lib");
            xml.writeEndElement();
            xml.writeStartElement("MODULE_CONFIG_DIR");
            xml.writeCharacters("/usr/local/share/tsk");
            xml.writeEndElement();
            xml.writeStartElement("OUT_DIR");
            xml.writeCharacters(wombatvariable.tmpfilepath);
            xml.writeEndElement();
            xml.writeStartElement("SYSTEM_OUT_DIR");
            xml.writeCharacters("#OUT_DIR#/SystemOutput");
            xml.writeEndElement();
            xml.writeStartElement("PIPELINE_CONFIG_FILE");
            xml.writeCharacters("#CONFIG_DIR#/tsk-pipe.xml");
            xml.writeEndElement();
            xml.writeEndElement();
            xml.writeEndDocument();
        }
        else
        {
            fprintf(stderr, "Could not open file to write\n");
        }
        tmpFile.close();
    }
    try
    {
        systemproperties = new TskSystemPropertiesImpl();
        systemproperties->initialize(tmpPath.toStdString());
        TskServices::Instance().setSystemProperties(*systemproperties);
        fprintf(stderr, "Configuration File Loading was successful!\n");
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Loading Config File config file: %s\n", ex.message().c_str());
    }
    tmpPath = wombatvariable.settingspath;
    tmpPath += "/tsk-pipe.xml";
    QFile pipeFile(tmpPath);
    fprintf(stderr, "PipPath: %s\n", tmpPath.toStdString().c_str());
    if(!pipeFile.exists()) // if tsk-pipe.xml does not exist, create and write it here
    {
        if(pipeFile.open(QFile::WriteOnly | QFile::Text))
        {
            QXmlStreamWriter pxml(&pipeFile);
            pxml.setAutoFormatting(true);
            pxml.writeStartDocument();
            pxml.writeStartElement("PIPELINE_CONFIG");
            pxml.writeStartElement("PIPELINE");
            pxml.writeAttribute("type", "FileAnalysis");
            pxml.writeStartElement("MODULE");
            pxml.writeAttribute("order", "1");
            pxml.writeAttribute("type", "plugin");
            pxml.writeAttribute("location", "tskHashCalcModule");
            pxml.writeEndElement();
            pxml.writeStartElement("MODULE");
            pxml.writeAttribute("order", "2");
            pxml.writeAttribute("type", "plugin");
            pxml.writeAttribute("location", "tskFileTypeSigModule");
            pxml.writeEndElement();
            pxml.writeEndElement();
            pxml.writeEndElement();
            pxml.writeEndDocument();
        }
        else
            fprintf(stderr, "Could not open file for writing\n");
        pipeFile.close();
    }
    // make the output directories
    if(!(new QDir())->mkpath(QString::fromStdString(GetSystemProperty(TskSystemProperties::OUT_DIR))))
        fprintf(stderr, "out_dir failed\n");
    if(!(new QDir())->mkpath(QString::fromStdString(GetSystemProperty(TskSystemProperties::SYSTEM_OUT_DIR))))
        fprintf(stderr, "system_out_dir failed\n");
    if(!(new QDir())->mkpath(QString::fromStdString(GetSystemProperty(TskSystemProperties::MODULE_OUT_DIR))))
        fprintf(stderr, "module_out_dir failed\n");
}
void SleuthKitPlugin::SetupLog()
{
    QString tmpPath = wombatvariable.datapath + "/tsk-log.txt";
    try
    {
        log = new TskLog(wombatvariable.datapath.toStdString());
        log->open(tmpPath.toStdString().c_str());
        TskServices::Instance().setLog(*log);
        fprintf(stderr, "Loading Log File was successful!\n");
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Loading Log File: %s\n", ex.message().c_str());
    }
}

void SleuthKitPlugin::SetupBlackboard()
{
    try
    {
        TskServices::Instance().setBlackboard((TskBlackboard &)TskDBBlackboard::instance());
        fprintf(stderr, "Loading Blackboard was successful!\n");
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Loading Blackboard: %s\n", ex.message().c_str());
    }
}

void SleuthKitPlugin::SetupScheduler()
{
    try
    {
        TskServices::Instance().setScheduler(scheduler);
        fprintf(stderr, "Loading Scheduler was successful!\n");
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Loading Scheduler: %s\n", ex.message().c_str());
    }
}

void SleuthKitPlugin::SetupFileManager()
{
    try
    {
        TskServices::Instance().setFileManager(fileManager->instance());
        fprintf(stderr, "Loading File Manager was successful!\n");
    }
    catch(TskException &ex)
    {
        fprintf(stderr, "Loading FileManager: %s\n", ex.message().c_str());
    }
}

void SleuthKitPlugin::threadFinished()
{
    fprintf(stderr, "The Thread Finished. ");
}


void SleuthKitPlugin::GetImageTree(WombatVariable wombatvariable, int isAddEvidence)
{
    QString imagename = wombatvariable.evidencepath.split("/").last();
    std::vector<TskFileRecord> fileRecordVector;
    std::list<TskVolumeInfoRecord> volRecordList;
    std::list<TskFsInfoRecord> fsInfoRecordList;
    QString fullPath = "";
    QString currentVolPath = "";
    QString currentFsPath = "";
    TskFileRecord tmpRecord;
    TskVolumeInfoRecord volRecord;
    TskFsInfoRecord fsInfoRecord;
    QStandardItem *fsNode;
    QStandardItem *volNode;
    QStandardItem *imageNode = new QStandardItem(imagename);
    imageNode->setIcon(QIcon(":/basic/treeimage"));
    int ret;
    uint64_t tmpId;
    volRecordList.clear();
    fileRecordVector.clear();
    fsInfoRecordList.clear();
    // also need to get the partitions and volumes as nodes.
    ret = TskServices::Instance().getImgDB().getVolumeInfo(volRecordList);
    //fprintf(stderr, "volrecordlist count: %d\n", volRecordList.count());
    foreach(volRecord, volRecordList) // populates all vol's and fs's.
    {
        // if volflag = 0, get description
        // if volflag = 1, list as unallocated
        fprintf(stderr, "Vol Description: %s - VolFlags: %d\n", volRecord.description.c_str(), volRecord.flags);
        if(volRecord.flags >= 0 && volRecord.flags <= 2)
        {
            if(volRecord.flags == 1)
            {
                volNode = new QStandardItem(QString::fromUtf8(volRecord.description.c_str()));
                volNode->setIcon(QIcon(":/basic/treefilemanager"));
                currentVolPath = QString::fromUtf8(volRecord.description.c_str()) + "/";
            }
            else if(volRecord.flags == 0)
            {
                volNode = new QStandardItem("unallocated space");
                volNode->setIcon(QIcon(":/basic/treefilemanager"));
                currentVolPath = "unallocated space/";
            }
            else if(volRecord.flags == 2)
            {
                volNode = new QStandardItem(QString::fromUtf8(volRecord.description.c_str()));
                volNode->setIcon(QIcon(":/basic/treefilemanager"));
                currentVolPath = QString::fromUtf8(volRecord.description.c_str());
                currentVolPath += "/";
            }
            else
            {
                // don't display anything
            }
            // for each volrecord, get fsinfo list
            imageNode->appendRow(volNode);
            ret = TskServices::Instance().getImgDB().getFsInfo(fsInfoRecordList);
            foreach(fsInfoRecord, fsInfoRecordList)
            {
                if(fsInfoRecord.vol_id == volRecord.vol_id)
                {
                    QString fsType = "";
                    // NEED TO DO SWITCH/CASE HERE TO GET FSTYPE
                    if(fsInfoRecord.fs_type == 1)
                        fsType = "NTFS";
                    else if(fsInfoRecord.fs_type == 3)
                        fsType = "FAT12";
                    else if(fsInfoRecord.fs_type == 4)
                        fsType = "FAT16";
                    else if(fsInfoRecord.fs_type == 5)
                        fsType = "FAT32";
                    else if(fsInfoRecord.fs_type == 7)
                        fsType = "UFS1";
                    else if(fsInfoRecord.fs_type == 8)
                        fsType = "UFS1B";
                    else if(fsInfoRecord.fs_type == 9)
                        fsType = "UFS2";
                    else if(fsInfoRecord.fs_type == 11)
                        fsType = "EXT2";
                    else if(fsInfoRecord.fs_type == 12)
                        fsType = "EXT3";
                    else if(fsInfoRecord.fs_type == 14)
                        fsType = "SWAP";
                    else if(fsInfoRecord.fs_type == 16)
                        fsType = "RAW";
                    else if(fsInfoRecord.fs_type == 18)
                        fsType = "ISO9660";
                    else if(fsInfoRecord.fs_type == 20)
                        fsType = "HFS";
                    else if(fsInfoRecord.fs_type == 22)
                        fsType = "EXT4";
                    else if(fsInfoRecord.fs_type == 23)
                        fsType = "YAFFS2";
                    else if(fsInfoRecord.fs_type == 25)
                        fsType = "UNSUPPORTED";
                    else
                        fsType = QString::number(fsInfoRecord.fs_type);
                    fsNode = new QStandardItem(fsType);
                    fsNode->setIcon(QIcon(":/basic/treepartition"));
                    currentFsPath = fsType + "/";
                    volNode->appendRow(fsNode);
                    //BEGIN FILE ADD CODE
                    std::vector<uint64_t> fileidVector;
                    //Create custom function to access this...
                    sqlite3* tmpImgDB;
                    QString tmpImgDbPath = wombatvariable.evidencedirpath + imagename + ".db";
                    QList<int> objectidlist;
                    objectidlist.clear();
                    if(sqlite3_open(tmpImgDbPath.toStdString().c_str(), &tmpImgDB) == SQLITE_OK)
                    {
                        sqlite3_stmt* stmt;
                        if(sqlite3_prepare_v2(tmpImgDB, "SELECT file_id FROM fs_files WHERE fs_id = ? ORDER BY file_id", -1, &stmt, 0) == SQLITE_OK)
                        {
                            if(sqlite3_bind_int(stmt, 1, fsInfoRecord.fs_id) == SQLITE_OK)
                            {
                                while(sqlite3_step(stmt) == SQLITE_ROW)
                                {
                                    uint64_t fileId = (uint64_t)sqlite3_column_int(stmt, 0);
                                    fileidVector.push_back(fileId);
                                    if(isAddEvidence == 1)
                                        objectidlist.append(wombatdata->InsertObject(wombatvariable.caseid, wombatvariable.evidenceid, (int)fileId));
                                }
                                sqlite3_finalize(stmt);
                            }
                            else
                            {
                                //std::wstringstream infoMessage;
                                //infoMessage << L"Get FsFileIds Failed: " << sqlite3_errmsg(imgdb);
                                //LOGERROR(infoMessage.c_str());
    
                            }
                        }
                        else
                        {
                            //std::wstringstream infoMessage;
                            //infoMessage << L"Get FsFileIds Failed: " << sqlite3_errmsg(imgdb);
                            //LOGERROR(infoMessage.c_str());
                        }
                    }
                    sqlite3_close(tmpImgDB);
                    //end create custom function to access fileid
                    QList<QList<QStandardItem*> > treeList;
                    foreach(tmpId, fileidVector)
                    {
                        TskFileRecord tmprecord;
                        ret = TskServices::Instance().getImgDB().getFileRecord(tmpId, tmprecord);
                        fprintf(stderr, "%s\n", tmprecord.md5.c_str());
                        fileRecordVector.push_back(tmprecord);
                    }
                    for(int i=0; i < (int)fileRecordVector.size(); i++)
                    {
                        QStandardItem* tmpitem;
                        QList<QStandardItem*> sleuthList;
                        fullPath = imagename + "/" + currentVolPath + currentFsPath;
                        if(fileRecordVector[i].parentFileId == 0)
                        {
                            fullPath += "root/";
                            sleuthList << new QStandardItem(QString("root"));
                        }
                        else
                        {
                            fullPath += "root/" + QString(fileRecordVector[i].fullPath.c_str());
                            sleuthList << new QStandardItem(QString(fileRecordVector[i].name.c_str()));
                        }
                        tmpitem = new QStandardItem(QString::number(wombatdata->ReturnObjectID(wombatvariable.caseid, wombatvariable.evidenceid, (int)fileRecordVector[i].fileId)));
                        tmpitem->setCheckable(true);
                        sleuthList << tmpitem;
                        //sleuthList << new QStandardItem(QString::number((int)fileRecordVector[i].fileId));
                        sleuthList << new QStandardItem(fullPath);
                        sleuthList << new QStandardItem(QString::number(fileRecordVector[i].size));
                        if(sqlite3_open(tmpImgDbPath.toStdString().c_str(), &tmpImgDB) == SQLITE_OK)
                        {
                            sqlite3_stmt* stmt;
                            if(sqlite3_prepare_v2(tmpImgDB, "select value_text from blackboard_attributes where attribute_type_id = 62 and obj_id = ?;", -1, &stmt, 0) == SQLITE_OK)
                            {
                                if(sqlite3_bind_int(stmt, 1, (int)fileRecordVector[i].fileId) == SQLITE_OK)
                                {
                                    int ret = sqlite3_step(stmt);
                                    if(ret == SQLITE_ROW || ret == SQLITE_DONE)
                                    {
                                        sleuthList << new QStandardItem(QString((const char*)sqlite3_column_text(stmt, 0)));
                                    }
                                    sqlite3_finalize(stmt);
                                }
                                else
                                {
                                    //std::wstringstream infoMessage;
                                    //infoMessage << L"Get FsFileIds Failed: " << sqlite3_errmsg(imgdb);
                                    //LOGERROR(infoMessage.c_str());
                                }
                            }
                            else
                            {
                                //std::wstringstream infoMessage;
                                //infoMessage << L"Get FsFileIds Failed: " << sqlite3_errmsg(imgdb);
                                //LOGERROR(infoMessage.c_str());
                            }
                        }
                        sqlite3_close(tmpImgDB);
                        sleuthList << new QStandardItem(QString::fromStdString(getFileType(fileRecordVector[i].name.c_str())));
                        sleuthList << new QStandardItem(DisplayTimeUTC(fileRecordVector[i].crtime));
                        sleuthList << new QStandardItem(DisplayTimeUTC(fileRecordVector[i].atime));
                        sleuthList << new QStandardItem(DisplayTimeUTC(fileRecordVector[i].mtime));
                        sleuthList << new QStandardItem(DisplayTimeUTC(fileRecordVector[i].ctime));
                        sleuthList << new QStandardItem(QString(fileRecordVector[i].md5.c_str()));
                        fprintf(stderr, "%i - %s\n", i, fileRecordVector[i].md5.c_str());
                        treeList.append(sleuthList);
                    }
                    for(int i = 0; i < (int)fileRecordVector.size(); i++)
                    {
                        // NEED TO EXPAND FOR OTHER ICONS
                        QStandardItem* tmpItem2 = ((QStandardItem*)treeList[i].first());
                        if(((TskFileRecord)fileRecordVector[i]).dirType == 3)
                        {
                            tmpItem2->setIcon(QIcon(":/basic/treefolder"));
                        }
                        else
                        {
                            tmpItem2->setIcon(QIcon(":/basic/treefile"));
                        }
                        if(((TskFileRecord)fileRecordVector[i]).parentFileId == 0)
                        {
                            fsNode->appendRow(treeList[i]);
                        }
                    }
                    for(int i=0; i < (int)fileRecordVector.size(); i++)
                    {
                        tmpRecord = fileRecordVector[i];
                        if(tmpRecord.parentFileId > 0)
                        {
                            ((QStandardItem*)treeList[tmpRecord.parentFileId-1].first())->appendRow(treeList[i]);
                        }
                    }
                    //END FILE ADD CODE
                }
            }
        }
    }
    emit ReturnImageNode(imageNode);
}

void SleuthKitPlugin::ExportFile(std::string exportpath, int objectID)
{
    int fileID = wombatdata->ReturnObjectFileID(objectID); // file id
    TskImageFileTsk currentimagefiletsk;
    try
    {
        currentimagefiletsk.open(wombatvariable.evidencepath.toStdString());
        TskServices::Instance().setImageFile(currentimagefiletsk);
    }
    catch(TskException ex)
    {
        fprintf(stderr, "Error setfile: %s\n", ex.what());
    }
    int ihandle;
    try
    {
        ihandle = TskServices::Instance().getImageFile().openFile(fileID);
        fprintf(stderr, "getimagefile works...");
    }
    catch(TskException ex)
    {
        fprintf(stderr, "Error getFile: %s\n", ex.what());
    }
    TskFile* tfile;
    try
    {
        tfile = TskServices::Instance().getFileManager().getFile((uint64_t)fileID);
        fprintf(stderr, "get file from image works\n");
    }
    catch(TskException ex)
    {
        fprintf(stderr, "Get File Error: %s\n", ex.what());
    }
    if(tfile->isDirectory())
    {
        bool tmpdir = (new QDir())->mkpath(QString::fromStdString(exportpath));
        if(!tmpdir)
            fprintf(stderr, "%s creation failed.\n");
        fprintf(stderr, "need to make the directory here...\n");
    }
    else
    {
        try
        {
            if(!tfile->exists())
            {
                TskServices::Instance().getFileManager().saveFile(tfile);
            }
        }
        catch(TskException ex)
        {
            fprintf(stderr, "read file/write to fail failed %s\n", ex.what());
        }
        catch(std::exception ex)
        {
            fprintf(stderr, "read file/write to fail %s\n", ex.what());
        }
        std::wstringstream ws;
        ws << exportpath.c_str();
        std::wstring wexportpath = ws.str();
        try
        {
            TskServices::Instance().getFileManager().copyFile(tfile, wexportpath);
        }
        catch(TskException ex)
        {
            fprintf(stderr, "copy file to export location failed %s\n", ex.what());
        }
        catch(std::exception ex)
        {
            fprintf(stderr, "copy file to export location failed %s\n", ex.what());
        }
    }
}   

QStringList SleuthKitPlugin::GetVolumeContents(WombatVariable wombatVariable)
{
    wombatvariable = wombatVariable;
    SetEvidenceDB(wombatvariable);
    std::list<TskVolumeInfoRecord> vollist;
    QStringList voldesclist;
    TskVolumeInfoRecord volrecord;
    int ret;
    vollist.clear();
    voldesclist.clear();
    ret = TskServices::Instance().getImgDB().getVolumeInfo(vollist);
    foreach(volrecord, vollist)
    {
        voldesclist << QString::fromUtf8(volrecord.description.c_str()) << QString::number(volrecord.vol_id);
    }
    return voldesclist;
}

QString SleuthKitPlugin::GetVolumeFilePath(WombatVariable wombatVariable, int volID)
{
    wombatvariable = wombatVariable;
    sqlite3* tmpImgDB;
    int ret;
    QString tmpImgDbPath = wombatvariable.evidencedirpath + wombatvariable.evidencepath.split("/").last() + ".db";
    uint64_t secstart = -1;
    uint64_t seclength = -1;
    if(sqlite3_open(tmpImgDbPath.toStdString().c_str(), &tmpImgDB) == SQLITE_OK)
    {
        sqlite3_stmt* stmt;
        if(sqlite3_prepare_v2(tmpImgDB, "SELECT sect_start, sect_len FROM vol_info WHERE vol_id = ?", -1, &stmt, 0) == SQLITE_OK)
        {
            if(sqlite3_bind_int(stmt, 1, volID) == SQLITE_OK)
            {
                ret = sqlite3_step(stmt);
                if(ret == SQLITE_ROW || ret == SQLITE_DONE)
                {
                    secstart = sqlite3_column_int64(stmt, 0);
                    seclength = sqlite3_column_int64(stmt, 1);
                }
            }
        }
        sqlite3_finalize(stmt);
    }
    TskImageFileTsk currentimagefiletsk;
    uint64_t bytelen = 512*(seclength - 1);
    uint64_t bytestart = 512*secstart;
    //char volbuffer[bytelen];
    char* volbuffer;
    try
    {
        currentimagefiletsk.open(wombatvariable.evidencepath.toStdString());
        TskServices::Instance().setImageFile(currentimagefiletsk);
    }
    catch(TskException ex)
    {
        fprintf(stderr, "Error set image file: %s\n", ex.what());
    }
    int retval;
    try
    {
        //SectorRun::SectorRun    
        // need to figure out why this fails... and returns -1
        //retval = TskServices::Instance().getImageFile().getByteData(bytestart, bytelen, volbuffer);
        //retval = TskServices::Instance().getImageFile().getSectorData(secstart, seclength-1, volbuffer);
        //fprintf(stderr, "sector data return value: %i\n", retval);
        if (retval == -1)
        {
        //std::wstringstream message;
        //message << L"TskImageFileTsk::getByteData - tsk_img_read -- start: " 
            //<< byte_start << " -- len: " << byte_len
            //<< "(" << tsk_error_get() << ")" << std::endl;
        //LOGERROR(message.str());
        //return -1;
        }
    }
    catch(TskException ex)
    {
        fprintf(stderr, "Error getting sector data: %s\n", ex.what());
    }
    if(retval > 0)
    {
        //std::string bufstring(volbuffer);
        //FILE* tmpfile;
        //tmpfile = fopen("/home/pasquale/WombatForensics/tmpfiles/volbyte.dat", "wb");
        //fwrite(volbuffer, sizeof(char), sizeof(volbuffer), tmpfile);
        //fclose(tmpfile);
        
        ofstream tmpfile("/home/pasquale/WombatForensics/tmpfiles/volbyte.dat", ios::out | ios::binary);
        //ofstream tmpfile(returnpath.toStdString().c_str(), ios::out | ios::binary);
        tmpfile.write(volbuffer, sizeof(volbuffer));
        tmpfile.close();
        
    }

    return "/home/pasquale/WombatForensics/tmpfiles/volbyte.dat";
}

QString SleuthKitPlugin::GetFileContents(int fileID)
{
    TskImageFileTsk currentimagefiletsk;
    QString returnpath = "";
    try
    {
        currentimagefiletsk.open(wombatvariable.evidencepath.toStdString());
        TskServices::Instance().setImageFile(currentimagefiletsk);
    }
    catch(TskException ex)
    {
        fprintf(stderr, "Error setfile: %s\n", ex.what());
    }
    int ihandle;
    try
    {
        ihandle = TskServices::Instance().getImageFile().openFile(fileID);
        fprintf(stderr, "getimagefile works...");
    }
    catch(TskException ex)
    {
        fprintf(stderr, "Error getFile: %s\n", ex.what());
    }
    TskFile* tfile;
    try
    {
        tfile = TskServices::Instance().getFileManager().getFile((uint64_t)fileID);
        fprintf(stderr, "get file from image works\n");
    }
    catch(TskException ex)
    {
        fprintf(stderr, "Get File Error: %s\n", ex.what());
    }
    // Saves the directory data stream from the image to a tmp file
    if(tfile->isDirectory())
    {
        try
        {
            const int FILE_BUFFER_SIZE = 8192;
            QString tmpfile = QString::fromStdWString(TskServices::Instance().getFileManager().getPath((uint64_t)fileID));
            tmpfile += "dir.dat";
            Poco::File destFile(tmpfile.toStdString());
            //If the destination file exists it is replaced
            if (destFile.exists())
            {
                destFile.remove();
            }
            // We read the content from the file and write it to the target
            tfile->open();
            // Create a new empty file.
            destFile.createFile();
            // Call File.read() to get the file content and write to new file.
            Poco::FileOutputStream fos(destFile.path());
            char buffer[FILE_BUFFER_SIZE];
            int bytesRead = 0;
            // Remember the offset the file was at when we were called.
            TSK_OFF_T savedOffset = tfile->tell();
            // Reset to start of file to ensure all content is saved.
            tfile->seek(0, std::ios_base::beg);
            do
            {
                memset(buffer, 0, FILE_BUFFER_SIZE);
                bytesRead = tfile->read(buffer, FILE_BUFFER_SIZE);
                if (bytesRead > 0)
                    fos.write(buffer, bytesRead);
            }
            while (bytesRead > 0);
            // Flush and close the output stream.
            fos.flush();
            fos.close();
            // Restore the saved offset.
            tfile->seek(savedOffset, std::ios_base::beg);
            // Close the file
            tfile->close();
            returnpath = tmpfile;
        }
        catch(TskException ex)
        {
            fprintf(stderr, "dir save buffer to dir.dat file failed\n");
        }
        catch(std::exception ex)
        {
            fprintf(stderr, "dir save buffer to dir.dat file failed\n");
        }
    }
    else
    {
        try
        {
            if(!tfile->exists())
            {
                TskServices::Instance().getFileManager().saveFile(tfile);
            }
        }
        catch(TskException ex)
        {
            fprintf(stderr, "read file/write to fail failed %s\n", ex.what());
        }
        catch(std::exception ex)
        {
            fprintf(stderr, "read file/write to fail %s\n", ex.what());
        }
        int objectid = wombatdata->ReturnObjectID(wombatvariable.caseid, wombatvariable.evidenceid, fileID);
        if(objectid != (int)fileID)
        {
            QString oldfile = QString::fromStdWString(TskServices::Instance().getFileManager().getPath((uint64_t)fileID));
            QString filepath = oldfile.left(oldfile.lastIndexOf("/"));
            QString newfile = filepath + "/" + QString::number(objectid);
            QFile tmpfile(oldfile);
            if(tmpfile.rename(newfile))
            {
                fprintf(stderr, "new file: %s\n", newfile.toStdString().c_str());
                returnpath = newfile;
            }
            else
            {
                fprintf(stderr, "old file value: %s\n", oldfile.toStdString().c_str());
                fprintf(stderr, "new file failed: %s\n", newfile.toStdString().c_str());
                // i'll need to return or log an error here somehow.
            }
        }
        else
        {
            returnpath = QString::fromStdWString(TskServices::Instance().getFileManager().getPath((uint64_t)fileID));
            fprintf(stderr, "old == new: %s\n", returnpath.toStdString().c_str());
        }
    }

    return returnpath;
}
