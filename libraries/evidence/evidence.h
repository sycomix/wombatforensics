#ifndef EVIDENCE_H
#define EVIDENCE_H

#include <main/interfaces.h>
//#include <sqlite3.h>
#include <QtPlugin>
#include <QObject>
#include <QtWidgets>
#include <cstdio>
#include <string>
#include <QStringList>
//#include <QFileDialog>

class EvidencePlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "wombat.PluginInterface" FILE "evidence.json")
    Q_INTERFACES(PluginInterface)
public:
    //QList<QStringList> PluginActions() const;
    //QList<QStringList> PluginActionIcons() const;
    //QStringList PluginMenus() const;

    void Initialize();
    void Run(QString input);
    void Finalize() {};

    QMap<QString, QMap<QString, QVariant> > plugmap;
};
/*
class EvidencePlugin : public QObject, public EvidenceInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "wombat.EvidenceInterface" FILE "evidence.json")
    Q_INTERFACES(EvidenceInterface)
    
public:
    //Evidence Interface Functions
    QStringList evidenceActions() const;
    QStringList evidenceActionIcons() const;

    QString addEvidence();
    void remEvidence(int currentCaseID);

private:
    int currentcaseid;

};
*/
#endif // EVIDENCE_H
