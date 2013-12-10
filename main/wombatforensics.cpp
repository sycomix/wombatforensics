#include "wombatforensics.h"

WombatForensics::WombatForensics(QWidget *parent) : QMainWindow(parent), ui(new Ui::WombatForensics)
{
    ui->setupUi(this);
    threadpool = QThreadPool::globalInstance();
    wombatcasedata = new WombatDatabase();
    wombatprogresswindow = new ProgressWindow(wombatcasedata);
    isleuthkit = new SleuthKitPlugin(wombatcasedata);
    connect(wombatprogresswindow, SIGNAL(HideProgressWindow(bool)), this, SLOT(HideProgressWindow(bool)), Qt::DirectConnection);
    connect(isleuthkit, SIGNAL(UpdateStatus(int, int)), this, SLOT(UpdateProgress(int, int)), Qt::QueuedConnection);
    connect(isleuthkit, SIGNAL(UpdateMessageTable()), this, SLOT(UpdateMessageTable()), Qt::QueuedConnection);
    connect(isleuthkit, SIGNAL(ReturnImageNode(QStandardItem*)), this, SLOT(GetImageNode(QStandardItem*)), Qt::QueuedConnection);
    qRegisterMetaType<WombatVariable>("WombatVariable");
    qRegisterMetaType<FileExportData>("FileExportData");
    connect(this, SIGNAL(LogVariable(WombatVariable)), isleuthkit, SLOT(GetLogVariable(WombatVariable)), Qt::QueuedConnection);
    connect(wombatcasedata, SIGNAL(DisplayError(QString, QString, QString)), this, SLOT(DisplayError(QString, QString, QString)), Qt::DirectConnection);
    connect(isleuthkit, SIGNAL(LoadFileContents(QString)), this, SLOT(LoadFileContents(QString)), Qt::QueuedConnection);
    connect(isleuthkit, SIGNAL(PopulateProgressWindow(WombatVariable)), this, SLOT(PopulateProgressWindow(WombatVariable)), Qt::QueuedConnection);
    wombatprogresswindow->setModal(false);
    wombatvariable.caseid = 0;
    wombatvariable.evidenceid = 0;
    wombatvariable.jobtype = 0;
    InitializeAppStructure();
    InitializeSleuthKit();
}

void WombatForensics::HideProgressWindow(bool checkedstate)
{
    ui->actionView_Progress->setChecked(checkedstate);
}

std::string WombatForensics::GetTime()
{
    struct tm *newtime;
    time_t aclock;

    time(&aclock);   // Get time in seconds
    newtime = localtime(&aclock);   // Convert time to struct tm form 
    char timeStr[64];
    snprintf(timeStr, 64, "%.2d/%.2d/%.2d %.2d:%.2d:%.2d",
        newtime->tm_mon+1,newtime->tm_mday,newtime->tm_year % 100, 
        newtime->tm_hour, newtime->tm_min, newtime->tm_sec);

    return timeStr;
}

void WombatForensics::InitializeAppStructure()
{
    QString homePath = QDir::homePath();
    homePath += "/WombatForensics/";
    wombatvariable.settingspath = homePath + "settings";
    wombatvariable.datapath = homePath + "data/";
    wombatvariable.casespath = homePath + "cases/";
    wombatvariable.tmpfilepath = homePath + "tmpfiles";
    bool mkPath = (new QDir())->mkpath(wombatvariable.settingspath);
    if(mkPath == false)
        DisplayError("2.0", "App Settings Folder Failed.", "App Settings Folder was not created.");
    mkPath = (new QDir())->mkpath(wombatvariable.datapath);
    if(mkPath == false)
        DisplayError("2.1", "App Data Folder Failed.", "Application Data Folder was not created.");
    mkPath = (new QDir())->mkpath(wombatvariable.casespath);
    if(mkPath == false)
        DisplayError("2.2", "App Cases Folder Failed.", "App Cases Folder was not created.");
    mkPath = (new QDir())->mkpath(wombatvariable.tmpfilepath);
    if(mkPath == false)
        DisplayError("2.2", "App TmpFile Folder Failed.", "App TmpFile Folder was not created.");
    QString logPath = wombatvariable.datapath + "WombatLog.db";
    bool logFileExist = wombatcasedata->FileExists(logPath.toStdString());
    if(!logFileExist)
    {
        const char* errstring = wombatcasedata->CreateLogDB(logPath);
        if(strcmp(errstring, "") != 0)
            DisplayError("1.0", "Log File Error", errstring);
    }
    QString tmpPath = wombatvariable.datapath + "WombatCase.db";
    bool doesFileExist = wombatcasedata->FileExists(tmpPath.toStdString());
    if(!doesFileExist)
    {
        const char* errstring = wombatcasedata->CreateCaseDB(tmpPath);
        if(strcmp(errstring, "") != 0)
            DisplayError("1.0", "File Error", errstring);
    }
    else
    {
        const char* errstring = wombatcasedata->OpenCaseDB(tmpPath);
        if(strcmp(errstring, "") != 0)
            DisplayError("1.1", "SQL", errstring);
    }
    if(wombatcasedata->ReturnCaseCount() == 0)
    {
        ui->actionOpen_Case->setEnabled(false);
    }
    else if(wombatcasedata->ReturnCaseCount() > 0)
    {
        ui->actionOpen_Case->setEnabled(true);
    }
    else
    {
        DisplayError("1.0", "Case Count", "Invalid Case Count returned.");
    }
    QList<int> sizelist;
    sizelist.append(height()/2);
    sizelist.append(height()/2);
    ui->splitter->setSizes(sizelist);
    SetupDirModel();
    SetupHexPage();
    SetupToolbar();
    
}

void WombatForensics::InitializeSleuthKit()
{
    ThreadRunner* initrunner = new ThreadRunner(isleuthkit, "initialize", wombatvariable);
    threadpool->start(initrunner);
    threadpool->waitForDone();
}

void WombatForensics::AddEvidence()
{
    QString evidenceFilePath = QFileDialog::getOpenFileName(this, tr("Select Evidence Item"), tr("./"));
    if(evidenceFilePath != "")
    {
        wombatprogresswindow->show();
        wombatprogresswindow->ClearTableWidget();
        wombatvariable.jobtype = 1; // add evidence
        // DETERMINE IF THE EVIDENCE NAME EXISTS, IF IT DOES THEN PROMPT USER THAT ITS OPEN ALREADY. IF THEY WANT TO OPEN A SECOND COPY
        // THEN SET NEWEVIDENCENAME EVIDENCEFILEPATH.SPLIT("/").LAST() + "COPY.DB"
        QString evidenceName = evidenceFilePath.split("/").last();
        evidenceName += ".db";
        wombatvariable.evidenceid = wombatcasedata->InsertEvidence(evidenceName, evidenceFilePath, wombatvariable.caseid);
        wombatvariable.evidenceidlist.append(wombatvariable.evidenceid);
        wombatvariable.evidencepath = evidenceFilePath;
        wombatvariable.evidencepathlist << wombatvariable.evidencepath;
        wombatvariable.evidencedbname = evidenceName;
        wombatvariable.evidencedbnamelist << wombatvariable.evidencedbname;
        wombatvariable.jobid = wombatcasedata->InsertJob(wombatvariable.jobtype, wombatvariable.caseid, wombatvariable.evidenceid);
        emit LogVariable(wombatvariable);
        QString tmpString = evidenceName;
        tmpString += " - ";
        tmpString += QString::fromStdString(GetTime());
        QStringList tmpList;
        tmpList << tmpString << QString::number(wombatvariable.jobid);
        wombatprogresswindow->UpdateAnalysisTree(0, new QTreeWidgetItem(tmpList));
        wombatprogresswindow->UpdateFilesFound("0");
        wombatprogresswindow->UpdateFilesProcessed("0");
        wombatprogresswindow->UpdateAnalysisState("Adding Evidence to Database");
        LOGINFO("Adding Evidence Started");
        wombatcasedata->InsertMsg(wombatvariable.caseid, wombatvariable.evidenceid, wombatvariable.jobid, 2, "Adding Evidence Started");
        ThreadRunner* trun = new ThreadRunner(isleuthkit, "openevidence", wombatvariable);
        threadpool->start(trun);
    }
}

void WombatForensics::RemEvidence()
{
    wombatprogresswindow->ClearTableWidget();
    wombatvariable.jobtype = 2; // remove evidence
    QStringList evidenceList;
    evidenceList.clear();
    // populate case list here
    evidenceList = wombatcasedata->ReturnCaseActiveEvidence(wombatvariable.caseid);
    bool ok;
    QString item = QInputDialog::getItem(this, tr("Remove Existing Evidence"), tr("Select Evidence to Remove: "), evidenceList, 0, false, &ok);
    if(ok && !item.isEmpty()) // open selected case
    {
        wombatvariable.evidenceid = wombatcasedata->ReturnEvidenceID(item);
        wombatvariable.jobid = wombatcasedata->InsertJob(wombatvariable.jobtype, wombatvariable.caseid, wombatvariable.evidenceid);
        emit LogVariable(wombatvariable);
        QString tmpstring = item.split("/").last() + " - " + QString::fromStdString(GetTime());
        QStringList tmplist;
        tmplist << tmpstring << QString::number(wombatvariable.jobid);
        wombatprogresswindow->UpdateAnalysisTree(2, new QTreeWidgetItem(tmplist));
        wombatprogresswindow->UpdateFilesFound("");
        wombatprogresswindow->UpdateFilesProcessed("");
        wombatprogresswindow->UpdateAnalysisState("Removing Evidence");
        LOGINFO("Removing Evidence Started");
        wombatcasedata->InsertMsg(wombatvariable.caseid, wombatvariable.evidenceid, wombatvariable.jobid, 2, "Removing Evidence Started");
        UpdateMessageTable();
        wombatcasedata->RemoveEvidence(item);
        wombatprogresswindow->UpdateProgressBar(25);
        QString tmppath = wombatvariable.evidencedirpath + item.split("/").last() + ".db";
        if(QFile::remove(tmppath))
        {
        }
        else
            emit DisplayError("2.1", "Evidence DB File was NOT Removed", "");
        wombatprogresswindow->UpdateProgressBar(50);
        UpdateCaseData(wombatvariable);
        wombatprogresswindow->UpdateProgressBar(75);
        LOGINFO("Removing Evidence Finished");
        wombatcasedata->InsertMsg(wombatvariable.caseid, wombatvariable.evidenceid, wombatvariable.jobid, 2, "Removing Evidence Finished");
        wombatcasedata->UpdateJobEnd(wombatvariable.jobid, 0, 0);
        UpdateMessageTable();
        wombatprogresswindow->UpdateAnalysisState("Removing Evidence Finished");
        wombatprogresswindow->UpdateProgressBar(100);
        UpdateMessageTable();
    }
}

int WombatForensics::StandardItemCheckState(QStandardItem* tmpitem, int checkcount)
{
    int curcount = checkcount;
    QModelIndex curindex = tmpitem->index();
    if(tmpitem->hasChildren())
    {
        for(int i=0; i < tmpitem->rowCount(); i++)
        {
            curcount = StandardItemCheckState(tmpitem->child(i,0), curcount);
        }
    }
    if(curindex.sibling(curindex.row(),1).flags().testFlag(Qt::ItemIsUserCheckable))
    {
        if(tmpitem->parent()->child(curindex.row(), 1)->checkState())
            curcount++;
    }
    
    return curcount;
}

std::vector<FileExportData> WombatForensics::SetFileExportProperties(QStandardItem* tmpitem, FileExportData tmpexport, std::vector<FileExportData> tmpexportlist)
{
    QModelIndex curindex = tmpitem->index();
    if(tmpitem->hasChildren())
    {
        for(int i=0; i < tmpitem->rowCount(); i++)
        {
            tmpexportlist = SetFileExportProperties(tmpitem->child(i,0), tmpexport, tmpexportlist);
        }
    }
    if(curindex.sibling(curindex.row(), 1).flags().testFlag(Qt::ItemIsUserCheckable))
    {
        if(tmpitem->parent()->child(curindex.row(), 1)->checkState())
        {
            // update evidence/file info for each item bit.
            tmpexport.id = curindex.sibling(curindex.row(), 1).data().toString().toInt(); // unique object id
            wombatvariable.evidenceid = wombatcasedata->ReturnObjectEvidenceID(tmpexport.id); // evidence id
            QStringList currentevidencelist = wombatcasedata->ReturnEvidenceData(wombatvariable.evidenceid); // evidence data
            tmpexport.evidencepath = currentevidencelist[0].toStdString(); // evidence path
            tmpexport.evidencedbname = currentevidencelist[1].toStdString(); // evidence db name
            tmpexport.name = curindex.sibling(curindex.row(), 0).data().toString().toStdString(); // file name
            if(tmpexport.pathstatus == FileExportData::include)
            {
                // export path with original path
                tmpexport.fullpath = tmpexport.exportpath + "/" + curindex.sibling(curindex.row(), 2).data().toString().toStdString();
            }
            else if(tmpexport.pathstatus == FileExportData::exclude)
            {
                tmpexport.fullpath = tmpexport.exportpath + "/" + tmpexport.name; // export path without original path
            }
            //fprintf(stderr, "export full path checked: %s\n", tmpexport.fullpath.c_str());
    
            tmpexportlist.push_back(tmpexport);
        }
    }

    return tmpexportlist;
}
std::vector<FileExportData> WombatForensics::SetListExportProperties(QStandardItem* tmpitem, FileExportData tmpexport, std::vector<FileExportData> tmpexportlist)
{
    QModelIndex curindex = tmpitem->index();
    if(tmpitem->hasChildren())
    {
        for(int i = 0; i < tmpitem->rowCount(); i++)
        {
            tmpexportlist = SetListExportProperties(tmpitem->child(i, 0), tmpexport, tmpexportlist);
        }
    }
    // get every item.
    tmpexport.id = curindex.sibling(curindex.row(), 1).data().toString().toInt(); // unique object id
    wombatvariable.evidenceid = wombatcasedata->ReturnObjectEvidenceID(tmpexport.id); // evidence id
    QStringList currentevidencelist = wombatcasedata->ReturnEvidenceData(wombatvariable.evidenceid); // evidence data
    tmpexport.evidencepath = currentevidencelist[0].toStdString(); // evidence path
    tmpexport.evidencedbname = currentevidencelist[1].toStdString(); // evidence db name
    tmpexport.name = curindex.sibling(curindex.row(), 0).data().toString().toStdString(); // file name
    if(tmpexport.pathstatus == FileExportData::include)
    {
        // export path with original path
        tmpexport.fullpath = tmpexport.exportpath + "/" + curindex.sibling(curindex.row(), 2).data().toString().toStdString();
    }
    else if(tmpexport.pathstatus == FileExportData::exclude)
    {
        tmpexport.fullpath = tmpexport.exportpath + "/" + tmpexport.name; // export path without original path
    }
    //fprintf(stderr, "export full path listed: %s\n", tmpexport.fullpath.c_str());
    tmpexportlist.push_back(tmpexport);

    return tmpexportlist;
}
int WombatForensics::StandardItemListCount(QStandardItem* tmpitem, int listcount)
{
    int curcount = listcount;
    if(tmpitem->hasChildren())
    {
        for(int i=0; i < tmpitem->rowCount(); i++)
        {
            curcount = StandardItemListCount(tmpitem->child(i,0), curcount);
        }
    }
    curcount++;

    return curcount;
}

void WombatForensics::ExportEvidence()
{
    int checkcount = 0;
    int listcount = 0;

    QStandardItem* rootitem = wombatdirmodel->invisibleRootItem();
    for(int i = 0; i < rootitem->rowCount(); i++)
    {
        QStandardItem* imagenode = rootitem->child(i,0);
        for(int j = 0; j < imagenode->rowCount(); j++)
        {
            QStandardItem* volumenode = imagenode->child(j,0);
            for(int k = 0; k < volumenode->rowCount(); k++)
            {
                QStandardItem* fsnode = volumenode->child(k,0);
                for(int m = 0; m < fsnode->rowCount(); m++)
                {
                    QStandardItem* filenode = fsnode->child(m,0);
                    checkcount = StandardItemCheckState(filenode, checkcount);
                    listcount = StandardItemListCount(filenode, listcount);
                }
            }
        }
    }
    //fprintf(stderr, "checked count: %i\n", checkcount);
    //fprintf(stderr, "listed item count: %i\n", listcount);
    exportdialog = new ExportDialog(this, checkcount, listcount);
    connect(exportdialog, SIGNAL(FileExport(FileExportData)), this, SLOT(FileExport(FileExportData)), Qt::DirectConnection);
    exportdialog->show();
}

void WombatForensics::FileExport(FileExportData exportdata)
{
    std::vector<FileExportData> exportevidencelist;
    if(exportdata.filestatus == FileExportData::selected)
    {
        exportdata.exportcount = 1;
        exportdata.id = curselindex.sibling(curselindex.row(), 1).data().toString().toInt(); // unique objectid
        wombatvariable.evidenceid = wombatcasedata->ReturnObjectEvidenceID(exportdata.id); // evidence id
        QStringList currentevidencelist = wombatcasedata->ReturnEvidenceData(wombatvariable.evidenceid); // evidence data
        exportdata.evidencepath = currentevidencelist[0].toStdString(); // evidence path
        exportdata.evidencedbname = currentevidencelist[1].toStdString(); // evidence db name
        exportdata.name = curselindex.sibling(curselindex.row(),0).data().toString().toStdString(); // file name
        if(exportdata.pathstatus == FileExportData::include)
        {
            exportdata.fullpath = exportdata.exportpath;
            exportdata.fullpath += "/";
            exportdata.fullpath += curselindex.sibling(curselindex.row(), 2).data().toString().toStdString(); // export path with original path
        }
        else if(exportdata.pathstatus == FileExportData::exclude)
        {
            exportdata.fullpath = exportdata.exportpath + "/" + exportdata.name; // export path without original path
        }
        //fprintf(stderr, "export selected full path: %s\n", exportdata.fullpath.c_str());

        exportevidencelist.push_back(exportdata);
    }
    else if(exportdata.filestatus == FileExportData::checked)
    {
        QStandardItem* rootitem = wombatdirmodel->invisibleRootItem();
        for(int i = 0; i < rootitem->rowCount(); i++) // loop over images
        {
            QStandardItem* imagenode = rootitem->child(i,0);
            for(int j = 0; j < imagenode->rowCount(); j++) // loop over volume(s)
            {
                QStandardItem* volumenode = imagenode->child(j,0); // loop over partition(s)
                for(int k = 0; k < volumenode->rowCount(); k++)
                {
                    QStandardItem* fsnode = volumenode->child(k,0); // file system node
                    for(int m = 0; m < fsnode->rowCount(); m++)
                    {
                        QStandardItem* filenode = fsnode->child(m,0); // file system root node
                        exportdata.exportcount = StandardItemCheckState(filenode, exportdata.exportcount);
                        exportevidencelist = SetFileExportProperties(filenode, exportdata, exportevidencelist);
                    }
                }
            }
        }
    }
    else if(exportdata.filestatus == FileExportData::listed)
    {
        QStandardItem* rootitem = wombatdirmodel->invisibleRootItem();
        for(int i = 0; i < rootitem->rowCount(); i++) // loop over images
        {
            QStandardItem* imagenode = rootitem->child(i,0);
            for(int j = 0; j < imagenode->rowCount(); j++) // loop over volumes
            {
                QStandardItem* volumenode = imagenode->child(j,0); // loop over partitions
                for(int k = 0; k < volumenode->rowCount(); k++)
                {
                    QStandardItem* fsnode = volumenode->child(k,0); // file system node
                    for(int m = 0; m < fsnode->rowCount(); m++)
                    {
                        QStandardItem* filenode = fsnode->child(m,0); // file system root node
                        exportdata.exportcount = StandardItemListCount(filenode, exportdata.exportcount);
                        exportevidencelist = SetListExportProperties(filenode, exportdata, exportevidencelist);
                    }
                }
            }
        }
    }
    wombatvariable.exportdatalist = exportevidencelist;
    wombatprogresswindow->ClearTableWidget();
    wombatvariable.jobtype = 3; // export files
    wombatvariable.jobid = wombatcasedata->InsertJob(wombatvariable.jobtype, wombatvariable.caseid, wombatvariable.evidenceid);
    emit LogVariable(wombatvariable);
    QString tmpString = "File Export - " + QString::fromStdString(GetTime());
    QStringList tmpList;
    tmpList << tmpString << QString::number(wombatvariable.jobid);
    wombatprogresswindow->UpdateAnalysisTree(1, new QTreeWidgetItem(tmpList));
    wombatprogresswindow->UpdateFilesFound(QString::number(wombatvariable.exportdata.exportcount));
    wombatprogresswindow->UpdateFilesProcessed("0");
    wombatprogresswindow->UpdateAnalysisState("Exporting Files");
    LOGINFO("File Export Started");
    wombatcasedata->InsertMsg(wombatvariable.caseid, wombatvariable.evidenceid, wombatvariable.jobid, 2, "File Export Started");
    ThreadRunner* trun = new ThreadRunner(isleuthkit, "exportfiles", wombatvariable);
    threadpool->start(trun);
}

void WombatForensics::UpdateCaseData(WombatVariable wvariable)
{
    // refresh views here
    wombatdirmodel->clear();
    QStringList headerList;
    headerList << "Name" << "Unique ID" << "Full Path" << "Size (Bytes)" << "Created (UTC)" << "Accessed (UTC)" << "Modified (UTC)" << "Status Changed (UTC)" << "MD5 Hash";
    wombatdirmodel->setHorizontalHeaderLabels(headerList);
    ui->dirTreeView->setModel(wombatdirmodel);
    ThreadRunner* trun = new ThreadRunner(isleuthkit, "refreshtreeviews", wombatvariable);
    threadpool->start(trun);
}

void WombatForensics::UpdateProgress(int filecount, int processcount)
{
    int curprogress = (int)((((float)processcount)/(float)filecount)*100);
    wombatprogresswindow->UpdateFilesFound(QString::number(filecount));
    wombatprogresswindow->UpdateFilesProcessed(QString::number(processcount));
    wombatprogresswindow->UpdateProgressBar(curprogress);
}

void WombatForensics::UpdateMessageTable()
{
    wombatprogresswindow->ClearTableWidget();
    QStringList tmplist = wombatcasedata->ReturnMessageTableEntries(wombatvariable.jobid);
    wombatprogresswindow->UpdateMessageTable(tmplist);
}

void WombatForensics::PopulateProgressWindow(WombatVariable wvariable)
{
    int treebranch = 0;
    QString tmpstring;
    //wombatvariable = wvariable;
    QStringList joblist = wombatcasedata->ReturnJobDetails(wvariable.jobid);
    if(wvariable.jobtype == 1 || wvariable.jobtype == 2)
        tmpstring = wvariable.evidencedbname + " - " + joblist[0];
    else if(wvariable.jobtype == 3)
        tmpstring = "File Export - " + joblist[0];
    else
        tmpstring = wvariable.evidencedbname + " - " + joblist[0];
    QStringList tmplist;
    tmplist << tmpstring << QString::number(wvariable.jobid);
    if(wvariable.jobtype == 1) treebranch = 0;
    else if(wvariable.jobtype == 2) treebranch = 2;
    else treebranch = 1;
    wombatprogresswindow->UpdateAnalysisTree(treebranch,  new QTreeWidgetItem(tmplist));
    if(wvariable.jobtype == 2)
    {
        wombatprogresswindow->UpdateProgressBar(100);
        wombatprogresswindow->UpdateFilesFound("");
        wombatprogresswindow->UpdateFilesProcessed("");
    }
    else
    {
        wombatprogresswindow->UpdateFilesFound(joblist[1]);
        wombatprogresswindow->UpdateFilesProcessed(joblist[2]);
        int finalprogress = (int)((((float)joblist[2].toInt())/(float)joblist[1].toInt())*100);
        wombatprogresswindow->UpdateProgressBar(finalprogress);
    }
    wombatprogresswindow->UpdateAnalysisState(joblist[3]);
    UpdateMessageTable();
}


void WombatForensics::DisplayError(QString errorNumber, QString errorType, QString errorValue)
{
    QString tmpString = errorNumber;
    tmpString += ". SqlError: ";
    tmpString += errorType;
    tmpString += " Returned ";
    tmpString += errorValue;
    QMessageBox::warning(this, "Error", tmpString, QMessageBox::Ok);
}

void WombatForensics::GetImageNode(QStandardItem* imagenode)
{
    QStandardItem* currentroot = wombatdirmodel->invisibleRootItem();
    currentroot->appendRow(imagenode);
    ui->dirTreeView->setModel(wombatdirmodel);
    ResizeColumns(wombatdirmodel);
    UpdateMessageTable();

}

void WombatForensics::ResizeColumns(QStandardItemModel* currentmodel)
{
    for(int i=0; i < currentmodel->columnCount(); i++)
    {
        // may need to compare treeview->model() == currentmodel) to determine what to set it to.
        // depending on the design though, i may not need multiple layouts since the columns can be sorted.
        // have to see as i go. for now its good.
        ui->dirTreeView->resizeColumnToContents(i);
    }
}

void WombatForensics::SetupDirModel(void)
{
    wombatdirmodel = new QStandardItemModel();
    QStringList headerList;
    headerList << "Name" << "Unique ID" << "Full Path" << "Size (Bytes)" << "Signature" << "Extension" << "Created (UTC)" << "Accessed (UTC)" << "Modified (UTC)" << "Status Changed (UTC)" << "MD5 Hash";
    wombatdirmodel->setHorizontalHeaderLabels(headerList);
    QStandardItem *evidenceNode = wombatdirmodel->invisibleRootItem();
    ui->dirTreeView->setModel(wombatdirmodel);
    ResizeColumns(wombatdirmodel);
    connect(ui->dirTreeView, SIGNAL(clicked(QModelIndex)), this, SLOT(dirTreeView_selectionChanged(QModelIndex)));
    connect(ui->dirTreeView, SIGNAL(expanded(const QModelIndex &)), this, SLOT(ResizeViewColumns(const QModelIndex &)));
}

void WombatForensics::SetupHexPage(void)
{
    // hex editor page
    QBoxLayout* mainlayout = new QBoxLayout(QBoxLayout::TopToBottom, ui->hexPage);
    QHBoxLayout* hexLayout = new QHBoxLayout();
    hstatus = new QStatusBar(ui->hexPage);
    hstatus->setSizeGripEnabled(false);
    selectedoffset = new QLabel("Offset: 00");
    selectedhex = new QLabel("Length: 0");
    selectedascii = new QLabel("Ascii: ");
    selectedinteger = new QLabel("Integer: ");
    selectedfloat = new QLabel("Float: ");
    selecteddouble = new QLabel("Double: ");
    hstatus->addWidget(selectedoffset);
    hstatus->addWidget(selectedhex);
    hstatus->addWidget(selectedascii);
    hstatus->addWidget(selectedinteger);
    hstatus->addWidget(selectedfloat);
    hstatus->addWidget(selecteddouble);
    hexwidget = new HexEditor(ui->hexPage);
    hexwidget->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    hexwidget->setObjectName("bt-hexview");
    hexwidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    hexLayout->addWidget(hexwidget);
    hexvsb = new QScrollBar(hexwidget);
    hexLayout->addWidget(hexvsb);
    hexvsb->setRange(0, 0);
    mainlayout->addLayout(hexLayout);
    mainlayout->addWidget(hstatus);

    connect(hexwidget, SIGNAL(rangeChanged(off_t,off_t)), this, SLOT(setScrollBarRange(off_t,off_t)));
    connect(hexwidget, SIGNAL(topLeftChanged(off_t)), this, SLOT(setScrollBarValue(off_t)));
    connect(hexwidget, SIGNAL(offsetChanged(off_t)), this, SLOT(setOffsetLabel(off_t)));
    connect(hexvsb, SIGNAL(valueChanged(int)), hexwidget, SLOT(setTopLeftToPercent(int)));
    connect(hexwidget, SIGNAL(selectionChanged(const QString &)), this, SLOT(UpdateSelectValue(const QString&)));
}

void WombatForensics::SetupToolbar(void)
{
    // setup actiongroups and initial settings here.
    viewgroup = new QActionGroup(this);
    viewgroup->addAction(ui->actionViewHex);
    viewgroup->addAction(ui->actionViewTxt);
    viewgroup->addAction(ui->actionViewOmni);
    ui->actionViewHex->setChecked(true);
    connect(viewgroup, SIGNAL(triggered(QAction*)), this, SLOT(ViewGroupTriggered(QAction*)));
}

WombatForensics::~WombatForensics()
{
    delete ui;
}

void WombatForensics::closeEvent(QCloseEvent* event)
{
    wombatprogresswindow->close();
    RemoveTmpFiles();
    //const char* errmsg = wombatcasedata->CloseCaseDB();
}

void WombatForensics::RemoveTmpFiles()
{
    QString homePath = QDir::homePath();
    homePath += "/WombatForensics/";
    homePath += "tmpfiles/";
    QDir tmpdir(homePath);
    if(!tmpdir.removeRecursively())
    {
        DisplayError("2.7", "Tmp File Removal", "All tmp files may not have been removed. Please manually remove them to avoid evidence contamination.");
    }
}

void WombatForensics::on_actionNew_Case_triggered()
{
    int ret = QMessageBox::Yes;
    // determine if a case is open
    if(wombatvariable.caseid > 0)
    {
        ret = QMessageBox::question(this, tr("Close Current Case"), tr("There is a case already open. Are you sure you want to close it?"), QMessageBox::Yes | QMessageBox::No);
    }
    if (ret == QMessageBox::Yes)
    {
        // create new case here
        bool ok;
        QString text = QInputDialog::getText(this, tr("New Case Creation"), "Enter Case Name: ", QLineEdit::Normal, "", &ok);
        if(ok && !text.isEmpty())
        {
            wombatvariable.caseid = wombatcasedata->InsertCase(text);

            QString tmpTitle = "Wombat Forensics - ";
            tmpTitle += text;
            this->setWindowTitle(tmpTitle); // update application window.
            // make Cases Folder
            QString userPath = wombatvariable.casespath;
            userPath += QString::number(wombatvariable.caseid);
            userPath += "-";
            userPath += text;
            userPath += "/";
            bool mkPath = (new QDir())->mkpath(userPath);
            if(mkPath == true)
            {
                wombatvariable.casedirpath = userPath;
            }
            else
            {
                DisplayError("2.0", "Cases Folder Creation Failed.", "New Case folder was not created.");
            }
            userPath = wombatvariable.casedirpath;
            userPath += "evidence/";
            mkPath = (new QDir())->mkpath(userPath);
            if(mkPath == true)
            {
                wombatvariable.evidencedirpath = userPath;
            }
            else
            {
                DisplayError("2.0", "Case Evidence Folder Creation Failed", "Failed to create case evidence folder.");
            }
            if(wombatcasedata->ReturnCaseCount() > 0)
            {
                ui->actionOpen_Case->setEnabled(true);
            }
        }
    }
}

void WombatForensics::on_actionOpen_Case_triggered()
{
    int ret = QMessageBox::Yes;
    // determine if a case is open
    if(wombatvariable.caseid > 0)
    {
        ret = QMessageBox::question(this, tr("Close Current Case"), tr("There is a case already open. Are you sure you want to close it?"), QMessageBox::Yes | QMessageBox::No);
    }
    if (ret == QMessageBox::Yes)
    {
        // open case here
        QStringList caseList;
        caseList.clear();
        // populate case list here
        caseList = wombatcasedata->ReturnCaseNameList();
        bool ok;
        QString item = QInputDialog::getItem(this, tr("Open Existing Case"), tr("Select the Case to Open: "), caseList, 0, false, &ok);
        if(ok && !item.isEmpty()) // open selected case
        {
            wombatvariable.caseid = wombatcasedata->ReturnCaseID(item);
            QString tmpTitle = "Wombat Forensics - ";
            tmpTitle += item;
            this->setWindowTitle(tmpTitle);
            QString userPath = wombatvariable.casespath;
            userPath += QString::number(wombatvariable.caseid);
            userPath += "-";
            userPath += item;
            userPath += "/";
            bool mkPath = (new QDir())->mkpath(userPath);
            if(mkPath == true)
            {
                wombatvariable.casedirpath = userPath;
            }
            else
            {
                DisplayError("2.0", "Cases Folder Check Failed.", "Existing Case folder did not exist.");
            }
            userPath = wombatvariable.casedirpath;
            userPath += "evidence/";
            mkPath = (new QDir())->mkpath(userPath);
            if(mkPath == true)
            {
                wombatvariable.evidencedirpath = userPath;
            }
            else
            {
                DisplayError("2.0", "Case Evidence Folder Check Failed", "Case Evidence folder did not exist.");
            }
            ThreadRunner* trun = new ThreadRunner(isleuthkit, "populatecase", wombatvariable);
            threadpool->start(trun);
        }
    }
}

void WombatForensics::ViewGroupTriggered(QAction* selaction)
{
    if(selaction == ui->actionViewHex)
    {
        // show the correct viewer page from stacked widget
        //fprintf(stderr, "Hex Button Text: %s\n", selaction->text().toStdString().c_str());
    }
    else if(selaction == ui->actionViewTxt)
    {
        //fprintf(stderr, "Text Button Text: %s\n", selaction->text().toStdString().c_str());
    }
    else if(selaction == ui->actionViewOmni)
    {
        //fprintf(stderr, "Omni Button Text: %s\n", selaction->text().toStdString().c_str());
    }
}

void WombatForensics::on_actionView_Progress_triggered(bool checked)
{
    if(!checked)
        wombatprogresswindow->hide();
    else
        wombatprogresswindow->show();
}

void WombatForensics::dirTreeView_selectionChanged(const QModelIndex &index)
{
    // QString imagename = wombatvariable.evidencepath.split("/").last();
    QString tmptext = "";
    QString sigtext = "";
    tmptext = index.sibling(index.row(), 1).data().toString();
    if(tmptext != "")
    {
        wombatvariable.evidenceid = wombatcasedata->ReturnObjectEvidenceID(tmptext.toInt());
        QStringList currentevidencelist = wombatcasedata->ReturnEvidenceData(wombatvariable.evidenceid);
        wombatvariable.evidencepath = currentevidencelist[0];
        wombatvariable.evidencedbname = currentevidencelist[1];
        wombatvariable.fileid = wombatcasedata->ReturnObjectFileID(tmptext.toInt());
        sigtext = index.sibling(index.row(), 4).data().toString(); // signature value which i need to compare to the xml of known values
        int omnivalue = DetermineOmniView(sigtext);
        if(omnivalue == 0)
        {
            //ui->fileViewTabWidget->setTabEnabled(2, false); // where i disable the omni button 
        }
        else
        {
            //ui->fileViewTabWidget->setTabEnabled(2, true); // where i enable the omni button
            if(omnivalue == 1)
                ui->viewerstack->setCurrentIndex(0);
            else if(omnivalue == 2)
                ui->viewerstack->setCurrentIndex(1);
            else if(omnivalue == 3)
                ui->viewerstack->setCurrentIndex(2);
        }
    }
    else
    {
        tmptext = index.sibling(index.row(), 0).data().toString();
        QStringList evidenceidlist = wombatcasedata->ReturnCaseActiveEvidenceID(wombatvariable.caseid);
        QStringList volumedesclist = isleuthkit->GetVolumeContents(wombatvariable);
        for(int i=0; i < evidenceidlist.count() / 3; i++)
        {
            if(tmptext.compare(evidenceidlist[3*i+1].split("/").last()) == 0)
            {
                wombatvariable.evidenceid = evidenceidlist[3*i].toInt();
                wombatvariable.evidencepath = evidenceidlist[3*i+1];
                wombatvariable.evidencedbname = evidenceidlist[3*i+2];
            }
        }
        // need to do other tmptext.compare's to see whether it's volume or fs...
        if(tmptext.compare(wombatvariable.evidencepath.split("/").last()) == 0)
        {
            wombatvariable.fileid = -1;
        }
        else // try one parent and see if it is a volume...
        {
            QString parenttext = index.parent().sibling(index.row(), 0).data().toString();
            for(int i=0; i < evidenceidlist.count() / 3; i++)
            {
                if(parenttext.compare(evidenceidlist[3*i+1].split("/").last()) == 0) // volume
                {
                    wombatvariable.evidenceid = evidenceidlist[3*i].toInt();
                    wombatvariable.evidencepath = evidenceidlist[3*i+1];
                    wombatvariable.evidencedbname = evidenceidlist[3*i+2];
                }
            }
            bool isvolume = false;
            for(int i=0; i < volumedesclist.count() / 2; i++)
            {
                if(tmptext.compare(volumedesclist[i]) == 0)
                {
                    isvolume = true;
                    wombatvariable.volid = volumedesclist[2*i+1].toInt();
                }
            }
            if(isvolume == true)
            {
                wombatvariable.fileid = -2;
            }
            else // try a file system fileid = -3
            {
            }
        }
    }
    ThreadRunner* tmprun = new ThreadRunner(isleuthkit, "showfile", wombatvariable);
    threadpool->start(tmprun);
}

int WombatForensics::DetermineOmniView(QString currentSignature)
{
    int retvalue = 0;
    QString tmppath = wombatvariable.settingspath;
    tmppath += "/tsk-magicview.xml";
    QFile magicfile(tmppath);
    if(magicfile.exists()) // if the xml exists, read it.
    {
        if(magicfile.open(QFile::ReadOnly | QFile::Text))
        {
            QXmlStreamReader reader(&magicfile);
            while(!reader.atEnd())
            {
                reader.readNext();
                if(reader.isStartElement() && reader.name() == "signature")
                {
                    if(currentSignature.toLower().compare(reader.readElementText().toLower()) == 0)
                    {
                        retvalue = reader.attributes().value("viewer").toString().toInt();
                    }
                }
            }
            if(reader.hasError())
                fprintf(stderr, "Reader Error: %s\n", reader.errorString().toStdString().c_str());
            magicfile.close();
        }
        else
            fprintf(stderr, "Couldn't Read the omni file\n");
    }
    else
        fprintf(stderr, "Couldn't find the omni file\n");
    return retvalue;
}

void WombatForensics::UpdateSelectValue(const QString &txt)
{
    // set hex (which i'll probably remove anyway since it's highlighted in same window)
    int sellength = txt.size()/2;
    QString tmptext = "Length: " + QString::number(sellength);
    selectedhex->setText(tmptext);
    // get initial bytes value and then update ascii
    std::vector<uchar> bytes;
    Translate::HexToByte(bytes, txt);
    QString ascii;
    Translate::ByteToChar(ascii, bytes);
    tmptext = "Ascii: " + ascii;
    selectedascii->setText(tmptext);
    QString strvalue;
    uchar * ucharPtr;
    // update the int entry:
    // pad right with 0x00
    int intvalue = 0;
    ucharPtr = (uchar*) &intvalue;
    memcpy(&intvalue,&bytes.begin()[0], min(sizeof(int),bytes.size()));
    strvalue.setNum(intvalue);
    tmptext = "Int: " + strvalue;
    selectedinteger->setText(tmptext);
    // update float entry;
    float fvalue;
    ucharPtr = (uchar*)(&fvalue);
    if(bytes.size() < sizeof(float) )
    {
        for(unsigned int i= 0; i < sizeof(float); ++i)
        {
            if( i < bytes.size() )
            {
                *ucharPtr++ = bytes[i];
            }
            else
            {
                *ucharPtr++ = 0x00;
            }
        }
    }
    else
    {
        memcpy(&fvalue,&bytes.begin()[0],sizeof(float));
    }
    strvalue.setNum( fvalue );
    tmptext = "Float: " + strvalue;
    selectedfloat->setText(tmptext);
    // update double
    double dvalue;
    ucharPtr = (uchar*)&dvalue;
    if(bytes.size() < sizeof(double) )
    {
        for(unsigned int i= 0; i < sizeof(double); ++i)
        {
            if( i < bytes.size() )
            {
                *ucharPtr++ = bytes[i];
            }
            else
            {
                *ucharPtr++ = 0x00;
            }
        }
    }
    else
    {
        memcpy(&dvalue,&bytes.begin()[0],sizeof(double));
    }
    strvalue.setNum( dvalue );
    tmptext = "Double: " + strvalue;
    selecteddouble->setText(tmptext);
}

void WombatForensics::LoadFileContents(QString filepath)
{
    if(filepath != "")
    {
        QFileInfo pathinfo(filepath);
        if(!pathinfo.isDir())
        {
            LoadHexModel(filepath);
            LoadTxtContent(filepath);
            LoadOmniContent(filepath); // possibly add a view type here: 1 - web, 2 - pic, 3 - vid
        }
        else
        {
            //txtwidget->setPlainText("");
        }
    }
    else
    {
        //txtwidget->setPlainText("");
        // load nothing here...
    }
}

void WombatForensics::LoadHexModel(QString tmpFilePath)
{
    hexwidget->open(tmpFilePath);
    hexwidget->set2BPC();
    hexwidget->setBaseHex();
}
void WombatForensics::LoadTxtContent(QString asciiText)
{
    /*
    QFile tmpFile(asciiText);
    tmpFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream stream(&tmpFile);
    txtwidget->setPlainText(stream.readAll());
    tmpFile.close();
    */
}

void WombatForensics::LoadOmniContent(QString filePath)
{
}

void WombatForensics::setOffsetLabel(off_t pos)
{
  QString label;
  label = "Offset: ";
  char    buffer[64];
#if _LARGEFILE_SOURCE
  sprintf(buffer,"0x%lx",pos);
#else
  sprintf(buffer,"0x%x",pos);
#endif
  label += buffer;
  selectedoffset->setText(label);
}

void WombatForensics::setScrollBarRange(off_t low, off_t high)
{
   (void)low;(void)high;
   // range must be contained in the space of an integer, just do 100
   // increments
   hexvsb->setRange(0,100);
}

void WombatForensics::setScrollBarValue(off_t pos)
{
  // pos is the topLeft pos, set the scrollbar to the
  // location of the last byte on the page
  // Note: offsetToPercent now rounds up, so we don't
  // have to worry about if this is the topLeft or bottom right
  hexvsb->setValue(hexwidget->offsetToPercent(pos));
}
