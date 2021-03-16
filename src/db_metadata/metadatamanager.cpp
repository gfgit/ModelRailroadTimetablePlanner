#include "metadatamanager.h"

#include <sqlite3pp/sqlite3pp.h>

constexpr char sql_get_metadata[] = "SELECT val FROM metadata WHERE name=?";
constexpr char sql_has_metadata_key[] = "SELECT 1 FROM metadata WHERE name=? AND val NOT NULL";
constexpr char sql_set_metadata[] = "REPLACE INTO metadata(name, val) VALUES(?, ?)";

MetaDataManager::MetaDataManager(sqlite3pp::database &db) :
    mDb(db)
{

}

MetaDataKey::Result MetaDataManager::hasKey(const MetaDataManager::Key &key)
{
    MetaDataKey::Result result = MetaDataKey::Result::NoMetaDataTable;
    if(!mDb.db())
        return result;

    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(mDb.db(), sql_has_metadata_key, sizeof (sql_has_metadata_key) - 1, &stmt, nullptr);
    if(rc != SQLITE_OK)
        return result;

    rc = sqlite3_bind_text(stmt, 1, key.str, key.len, SQLITE_STATIC);
    if(rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return result;
    }

    rc = sqlite3_step(stmt);

    if(rc == SQLITE_ROW)
    {
        result = MetaDataKey::Result::ValueFound;
    }
    else if(rc == SQLITE_OK || rc == SQLITE_DONE)
    {
        result = MetaDataKey::Result::ValueNotFound;
    }
    else
    {
        result = MetaDataKey::Result::NoMetaDataTable;
    }

    sqlite3_finalize(stmt);
    return result;
}

MetaDataKey::Result MetaDataManager::getInt64(qint64 &out, const Key& key)
{
    MetaDataKey::Result result = MetaDataKey::Result::NoMetaDataTable;
    if(!mDb.db())
        return result;

    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(mDb.db(), sql_get_metadata, sizeof (sql_get_metadata) - 1, &stmt, nullptr);
    if(rc != SQLITE_OK)
        return result;

    rc = sqlite3_bind_text(stmt, 1, key.str, key.len, SQLITE_STATIC);
    if(rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return result;
    }

    rc = sqlite3_step(stmt);

    if(rc == SQLITE_ROW)
    {
        if(sqlite3_column_type(stmt, 0) == SQLITE_NULL)
        {
            result = MetaDataKey::Result::ValueIsNull;
        }
        else
        {
            result = MetaDataKey::Result::ValueFound;
            out = sqlite3_column_int64(stmt, 0);
        }
    }
    else if(rc == SQLITE_OK || rc == SQLITE_DONE)
    {
        result = MetaDataKey::Result::ValueNotFound;
    }
    else
    {
        result = MetaDataKey::Result::NoMetaDataTable;
    }

    sqlite3_finalize(stmt);
    return result;
}

MetaDataKey::Result MetaDataManager::setInt64(qint64 in, bool setToNull, const Key& key)
{
    MetaDataKey::Result result = MetaDataKey::Result::NoMetaDataTable;
    if(!mDb.db())
        return result;

    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(mDb.db(), sql_set_metadata, sizeof (sql_set_metadata) - 1, &stmt, nullptr);
    if(rc != SQLITE_OK)
        return result;

    rc = sqlite3_bind_text(stmt, 1, key.str, key.len, SQLITE_STATIC);
    if(rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return result;
    }

    if(setToNull)
        rc = sqlite3_bind_null(stmt, 2);
    else
        rc = sqlite3_bind_int64(stmt, 2, in);
    if(rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return result;
    }

    rc = sqlite3_step(stmt);

    if(rc == SQLITE_OK || rc == SQLITE_DONE)
    {
        result = MetaDataKey::Result::ValueFound;
    }
    else
    {
        result = MetaDataKey::Result::NoMetaDataTable;
    }

    sqlite3_finalize(stmt);
    return result;
}

MetaDataKey::Result MetaDataManager::getString(QString &out, const Key& key)
{
    MetaDataKey::Result result = MetaDataKey::Result::NoMetaDataTable;
    if(!mDb.db())
        return result;

    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(mDb.db(), sql_get_metadata, sizeof (sql_get_metadata) - 1, &stmt, nullptr);
    if(rc != SQLITE_OK)
        return result;

    rc = sqlite3_bind_text(stmt, 1, key.str, key.len, SQLITE_STATIC);
    if(rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return result;
    }

    rc = sqlite3_step(stmt);

    if(rc == SQLITE_ROW)
    {
        if(sqlite3_column_type(stmt, 0) == SQLITE_NULL)
        {
            result = MetaDataKey::Result::ValueIsNull;
        }
        else
        {
            result = MetaDataKey::Result::ValueFound;
            const int len = sqlite3_column_bytes(stmt, 0);
            const char *text = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 0));
            out = QString::fromUtf8(text, len);
        }
    }
    else if(rc == SQLITE_OK || rc == SQLITE_DONE)
    {
        result = MetaDataKey::Result::ValueNotFound;
    }
    else
    {
        result = MetaDataKey::Result::NoMetaDataTable;
    }

    sqlite3_finalize(stmt);
    return result;
}

MetaDataKey::Result MetaDataManager::setString(const QString& in, bool setToNull, const Key& key)
{
    MetaDataKey::Result result = MetaDataKey::Result::NoMetaDataTable;
    if(!mDb.db())
        return result;

    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(mDb.db(), sql_set_metadata, sizeof (sql_set_metadata) - 1, &stmt, nullptr);
    if(rc != SQLITE_OK)
        return result;

    rc = sqlite3_bind_text(stmt, 1, key.str, key.len, SQLITE_STATIC);
    if(rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return result;
    }

    QByteArray arr = in.toUtf8();

    if(setToNull)
        rc = sqlite3_bind_null(stmt, 2);
    else
    {
        rc = sqlite3_bind_text(stmt, 2, arr.data(), arr.size(), SQLITE_STATIC);
    }
    if(rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return result;
    }

    rc = sqlite3_step(stmt);

    if(rc == SQLITE_OK || rc == SQLITE_DONE)
    {
        result = MetaDataKey::Result::ValueFound;
    }
    else
    {
        result = MetaDataKey::Result::NoMetaDataTable;
    }

    sqlite3_finalize(stmt);
    return result;
}
