#include "wombatframework.h"

WombatFramework::WombatFramework(WombatVariable* wombatvariable)
{
    wombatptr = wombatvariable;
}
WombatFramework::~WombatFramework()
{
}

void WombatFramework::OpenEvidenceImage() // open current evidence image
{
    const TSK_TCHAR** images;
    images = (const char**)malloc(wombatptr->evidenceobject.fullpathvector.size()*sizeof(char*));
    int i = 0;
    for(std::vector<std::string>::iterator list_iter = wombatptr->evidenceobject.fullpathvector.begin(); list_iter != wombatptr->evidenceobject.fullpathvector.end(); list_iter++)
    {
        images[i++] = (*list_iter).c_str();
    }
    wombatptr->evidenceobject.imageinfo = tsk_img_open(wombatptr->evidenceobject.itemcount, images, TSK_IMG_TYPE_DETECT, 0);
    free(images);
}

void WombatFramework::OpenVolumeSystem() // open current volume system
{
    wombatptr->evidenceobject.volinfo = tsk_vs_open(wombatptr->evidenceobject.imageinfo, 0, TSK_VS_TYPE_DETECT);
}

void WombatFramework::GetVolumeSystemName() // get the volume system name
{
    wombatptr->volumeobject.name = QString::fromUtf8(tsk_vs_type_todesc(wombatptr->evidenceobject.volinfo->vstype));
    wombatptr->volumeobjectvector.append(wombatptr->volumeobject);
}

void WombatFramework::OpenEvidenceImages() // open all evidence images.
{
    for(int j = 0; j < wombatptr->evidenceobjectvector.count(); j++)
    {
        const TSK_TCHAR** images;
        images = (const char**)malloc(wombatptr->evidenceobjectvector[j].fullpathvector.size()*sizeof(char*));
        int i = 0;
        for(std::vector<std::string>::iterator list_iter = wombatptr->evidenceobjectvector[j].fullpathvector.begin(); list_iter != wombatptr->evidenceobjectvector[j].fullpathvector.end(); list_iter++)
        {
            images[i++] = (*list_iter).c_str();
        }
        wombatptr->evidenceobjectvector[j].imageinfo = tsk_img_open(wombatptr->evidenceobjectvector[j].itemcount, images, TSK_IMG_TYPE_DETECT, 0);
        free(images);
    }
}

void WombatFramework::BuildEvidenceModel()
{
    // COME BACK TO QTCONCURRENT AS I GET FARTHER ALONG AND RESEARCH MORE
    /*
    QVector<ItemObject> itemvector;
    for(int i = 0; i < wombatptr->evidenceobjectvector.count(); i++)
    {
        itemvector.append(wombatptr->evidenceobjectvector[i]);
    }
    QFutureWatcher<void> watcher;
    watcher.setFuture(QtConcurrent::map(itemvector, &WombatFramework::OpenEvidenceImages));
    //watcher.setFuture(QtConcurrent::map(wombatptr->evidenceobjectvector, WombatFramework::OpenEvidenceImages));
    watcher.waitForFinished();
    */
    OpenEvidenceImages(); // PROBABLY PUT THIS IN A MULTI-THREAD ENVIRONMENT.
    for(int i=0; i < wombatptr->evidenceobjectvector.count(); i++) // for each evidence image info file.
    {
        if(wombatptr->evidenceobjectvector[i].imageinfo == NULL)
            fprintf(stderr, "Image didn't open. add to log file as error.\n");
        else
            fprintf(stderr, "Image %s opened. add to log file as info.\n", wombatptr->evidenceobjectvector[i].fullpathvector[0].c_str());
    }
    // NEED TO LAUNCH THIS IN A NEW THREAD TO KEEP GUI RESPONSIVE
    // NEED TO OPEN THE IMAGE - img_open.c [tsk_img_open()]
    // NEED TO GET THE METADATA FOR THE IMAGE/VOLUMES/PARTITIONS/FILES SO I CAN POPULATE THE DIRECTORY TREE INFORMATION


    // NEED TO ADD THE EVIDENCE ITEM TO THE DATABASE
    // POPULATE THE WOMBATVARPTR FOR THE EVIDENCEOBJECT VECTOR
    // NEED TO CREATE THE EVIDENCE TSK DATABASE (EXTRACT EVIDENCE ACCORDING TO MODULES)
    // NEED TO BUILD DIRMODEL AS I GO AND POPULATE DIRTREEVIEW AS I GO WITH EACH FILE
    // FOR NOW I WON'T BUILD MODULES, I'LL JUST DESIGN A MULTI-THREADED APPROACH FOR IT AND ABSTRACT TO PLUGGABLE MODULES LATER

    /*
    QFutureWatcher<void> watcher;
    std::vector<TskSchedulerQueue::task_struct* > tmpvector;
    watcher.setFuture(QtConcurrent::map(&tmpvector, &SleuthKitPlugin::TaskMap));
    //watcher.setFuture(QtConcurrent::map(&((std::vector<TskSchedulerQueue::task_struct*>)scheduler.mapvector), &SleuthKitPlugin::TaskMap));
    watcher.waitForFinished();
    */
    // QT CONCURRENT TEST
}
