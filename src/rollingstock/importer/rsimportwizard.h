#ifndef RSIMPORTWIZARD_H
#define RSIMPORTWIZARD_H

#include <QWizard>

#include <QMap>
#include <QVariant>

#include "utils/types.h"

class RSImportedOwnersModel;
class RSImportedModelsModel;
class RSImportedRollingstockModel;

class ILoadRSTask;
class ImportTask;
class LoadingPage;
class SpinBoxEditorFactory;
class QAbstractItemModel;
class IOptionsWidget;

class RSImportWizard : public QWizard
{
    Q_OBJECT
public:
    enum class ImportSource
    {
        OdsImport = 0,
        SQLiteImport,
        NSources
    };

    enum Pages
    {
        OptionsPageIdx = 0,
        ChooseFileIdx,
        LoadFileIdx,
        SelectOwnersIdx,
        SelectModelsIdx,
        SelectRsIdx,
        ImportRsIdx,
        NPages
    };

    enum ExtraCodes
    {
        RejectWithoutAsking = 3
    };

    explicit RSImportWizard(bool resume, QWidget *parent = nullptr);
    ~RSImportWizard() override;

    void done(int result) override;
    virtual bool validateCurrentPage() override;
    virtual int nextId() const override;

    bool startLoadTask(const QString &fileName);
    void abortLoadTask();
    inline bool taskRunning() const { return loadTask || importTask; }

    void startImportTask();
    void abortImportTask();

    void goToPrevPageQueued();

public: //Settings
    void setDefaultTypeAndSpeed(RsType t, int speed);

    inline int getImportMode() const { return importMode; }
    void setImportMode(int m);

    inline ImportSource getImportSource() const { return importSource; }

    inline QAbstractItemModel *getSourcesModel() const
    {
        return sourcesModel;
    }

    IOptionsWidget *createOptionsWidget(ImportSource source, QWidget *parent);
    void setSource(ImportSource source, IOptionsWidget *options);

    ILoadRSTask *createLoadTask(const QMap<QString, QVariant>& arguments, const QString &fileName);

protected:
    bool event(QEvent *e) override;
    void initializePage(int id) override;
    void cleanupPage(int id) override;

private:
    RSImportedOwnersModel *ownersModel;
    RSImportedModelsModel *modelsModel;
    RSImportedRollingstockModel *listModel;
    SpinBoxEditorFactory *spinFactory;

    ILoadRSTask *loadTask;
    ImportTask *importTask;
    LoadingPage *loadFilePage;
    LoadingPage *importPage;
    bool isStoppingTask;

    //Import options
    int defaultSpeed;
    RsType defaultRsType;
    int importMode;
    ImportSource importSource;
    QAbstractItemModel *sourcesModel;
    QMap<QString, QVariant> optionsMap;
};

#endif // RSIMPORTWIZARD_H
