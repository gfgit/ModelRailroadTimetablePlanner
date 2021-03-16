#include "mergemodelsdialog.h"
#include "ui_mergemodelsdialog.h"

#include "app/session.h"

#include "rollingstock/rsmodelsmatchmodel.h"

#include "utils/rs_types_names.h"

#include <QMessageBox>
#include "utils/sqldelegate/customcompletionlineedit.h"

#include <QDebug>

MergeModelsDialog::MergeModelsDialog(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MergeModelsDialog),
    mDb(db),
    q_getModelInfo(mDb, "SELECT suffix,max_speed,axes,type,sub_type FROM rs_models WHERE id=?")
{
    ui->setupUi(this);

    model = new RSModelsMatchModel(mDb, this);
    sourceModelEdit = new CustomCompletionLineEdit(model);
    destModelEdit = new CustomCompletionLineEdit(model);

    sourceModelEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    destModelEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    ui->gridLayout->addWidget(sourceModelEdit, 1, 0);
    ui->gridLayout->addWidget(destModelEdit, 2, 0);

    connect(sourceModelEdit, &CustomCompletionLineEdit::dataIdChanged, this, &MergeModelsDialog::sourceModelChanged);
    connect(destModelEdit, &CustomCompletionLineEdit::dataIdChanged, this, &MergeModelsDialog::destModelChanged);

    ui->removeSourceCheckBox->setChecked(AppSettings.getRemoveMergedSourceModel());
}

MergeModelsDialog::~MergeModelsDialog()
{
    delete ui;
}

void MergeModelsDialog::done(int r)
{
    if(r == QDialog::Accepted)
    {
        db_id sourceModelId = 0;
        db_id destModelId = 0;
        QString tmp;
        sourceModelEdit->getData(sourceModelId, tmp);
        destModelEdit->getData(destModelId, tmp);

        //Check input is valid
        if(!sourceModelId || !destModelId || sourceModelId == destModelId)
        {
            QMessageBox::warning(this, tr("Invalid Models"), tr("Models must not be null and must be different"));
            return; //We don't want the dialog to be closed
        }

        if(mergeModels(sourceModelId, destModelId, ui->removeSourceCheckBox->isChecked()))
        {
            //Operation succeded, inform user
            QMessageBox::information(this,
                                     tr("Merging completed"),
                                     tr("Models merged succesfully."));
        }
        else
        {
            QMessageBox::warning(this, tr("Error while merging"), tr("Some error occurred while merging models."));
            //Accept dialog to close it, so don't return here
        }
    }

    QDialog::done(r);
}

void MergeModelsDialog::sourceModelChanged(db_id modelId)
{
    fillModelInfo(ui->sourceModelInfo, modelId);
}

void MergeModelsDialog::destModelChanged(db_id modelId)
{
    fillModelInfo(ui->destModelInfo, modelId);
}

void MergeModelsDialog::fillModelInfo(QLabel *label, db_id modelId)
{
    if(!modelId)
    {
        label->setText(tr("No model set"));
        return;
    }

    q_getModelInfo.bind(1, modelId);
    if(q_getModelInfo.step() != SQLITE_ROW)
    {
        label->setText(tr("Error"));
        q_getModelInfo.reset();
        return;
    }

    auto r = q_getModelInfo.getRows();
    QString suffix = r.get<QString>(0);
    int maxSpeedKmH = r.get<int>(1);
    int axes = r.get<int>(2);
    RsType type = RsType(r.get<int>(3));
    RsEngineSubType subType = RsEngineSubType(r.get<int>(4));

    QString typeStr = RsTypeNames::name(type);
    if(type == RsType::Engine)
    {
        typeStr.append(", ");
        typeStr.append(RsTypeNames::name(subType));
    }

    if(!suffix.isEmpty())
    {
        QString tmp;
        tmp.reserve(suffix.size() + 8);
        tmp.append("<b>"); //3 char
        tmp.append(suffix);
        tmp.append("</b> "); //5 char
        suffix = tmp;
    }

    label->setText(tr("%1Axes: <b>%2</b> Max.Speed: <b>%3 km/h</b><br>Type: <b>%4</b>")
                   .arg(suffix)
                   .arg(axes)
                   .arg(maxSpeedKmH)
                   .arg(typeStr));

    adjustSize();

    model->autoSuggest(QString()); //Reset query

    q_getModelInfo.reset();
}

/* Merge sourceModel in destModel:
 * - all rollingstock of model 'sourceModel' will be changed to model 'destModel'
 * - if removeSource is true then at the end of the operation 'sourceModel' is deleted from the database
 *
 * If fails returns -1
 * If succeds returns the number of rollingstock pieces that have been changed
 *
 * BIG TODO: say we have RS
 *  Aln773.1251
 *  Aln668.1251
 *
 *  Then we merge Aln773 in Aln668, the result is:
 *  Aln668.1251
 *  Aln668.1251 Two RS identical
 *
 *  We should prevent that by canceling the merge or by changing the number or letting user choose
 */
int MergeModelsDialog::mergeModels(db_id sourceModelId, db_id destModelId, bool removeSource)
{
    if(sourceModelId == destModelId)
        return false; //Error: must be different models

    command q_mergeModels(mDb, "UPDATE rs_list SET model_id=? WHERE model_id=?");
    q_mergeModels.bind(1, destModelId);
    q_mergeModels.bind(2, sourceModelId);
    int ret = q_mergeModels.execute();
    q_mergeModels.reset();

    if(ret != SQLITE_OK)
    {
        qDebug() << "Merging Models" << sourceModelId << destModelId;
        qDebug() << "DB Error:" << ret << mDb.error_msg() << mDb.extended_error_code();
        return false;
    }

    if(removeSource)
    {
        command q_removeSource(mDb, "DELETE FROM rs_models WHERE id=?");
        q_removeSource.bind(1, sourceModelId);
        ret = q_removeSource.execute();
        if(ret != SQLITE_OK)
        {
            qDebug() << "Removing model" << sourceModelId;
            qDebug() << "DB Error:" << ret << mDb.error_msg() << mDb.extended_error_code();
        }
    }

    return true;
}
