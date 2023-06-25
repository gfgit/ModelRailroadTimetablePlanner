/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef RSIMPORTWIZARD_H
#define RSIMPORTWIZARD_H

#include <QWizard>

#include <QMap>
#include <QVariant>

#include "utils/types.h"

class RSImportedOwnersModel;
class RSImportedModelsModel;
class RSImportedRollingstockModel;

class RSImportBackendsModel;
class IOptionsWidget;
class ILoadRSTask;

class ImportTask;
class LoadingPage;
class SpinBoxEditorFactory;
class QAbstractItemModel;

class RSImportWizard : public QWizard
{
    Q_OBJECT
public:
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
    inline bool taskRunning() const
    {
        return loadTask || importTask;
    }

    void startImportTask();
    void abortImportTask();

    void goToPrevPageQueued();

public: // Settings
    void setDefaultTypeAndSpeed(RsType t, int speed);

    inline int getImportMode() const
    {
        return importMode;
    }
    void setImportMode(int m);

    inline int getBackendIdx() const
    {
        return backendIdx;
    }

    QAbstractItemModel *getBackendsModel() const;

    IOptionsWidget *createOptionsWidget(int idx, QWidget *parent);
    void setSource(int idx, IOptionsWidget *options);

    ILoadRSTask *createLoadTask(const QMap<QString, QVariant> &arguments, const QString &fileName);

protected:
    bool event(QEvent *e) override;

private slots:
    void onFileChosen(const QString &filename);

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

    // Import options
    int defaultSpeed;
    RsType defaultRsType;
    int importMode;
    int backendIdx;
    RSImportBackendsModel *backends;
    QMap<QString, QVariant> optionsMap;
};

#endif // RSIMPORTWIZARD_H
