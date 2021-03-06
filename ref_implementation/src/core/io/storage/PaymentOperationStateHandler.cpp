/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#include "PaymentOperationStateHandler.h"

PaymentOperationStateHandler::PaymentOperationStateHandler(
    sqlite3 *dbConnection,
    const string &tableName,
    Logger &logger):

    mDataBase(dbConnection),
    mTableName(tableName),
    mLog(logger)
{
    string query = "CREATE TABLE IF NOT EXISTS " + mTableName +
        " (transaction_uuid BLOB NOT NULL, "
            "state BLOB NOT NULL, "
            "state_bytes_count INT NOT NULL, "
            "recording_time INT NOT NULL);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::creating table: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
    } else {
        throw IOError("PaymentOperationStateHandler::creating table: "
                          "Run query; sqlite error: " + to_string(rc));
    }
    query = "CREATE UNIQUE INDEX IF NOT EXISTS " + mTableName
            + "_transaction_uuid_idx on " + mTableName + " (transaction_uuid);";
    rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::creating index for TransactionUUID: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
    } else {
        throw IOError("PaymentOperationStateHandler::creating index for TransactionUUID: "
                          "Run query; sqlite error: " + to_string(rc));
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
}

void PaymentOperationStateHandler::saveRecord(
    const TransactionUUID &transactionUUID,
    BytesShared state,
    size_t stateBytesCount)
{
    string query = "INSERT OR REPLACE INTO " + mTableName +
        " (transaction_uuid, state, state_bytes_count, recording_time) VALUES(?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::insert: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_blob(stmt, 1, transactionUUID.data, NodeUUID::kBytesSize, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::insert: "
                          "Bad binding of TransactionUUID; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_blob(stmt, 2, state.get(), (int)stateBytesCount, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::insert: "
                          "Bad binding of State; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_int(stmt, 3, (int)stateBytesCount);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::insert: "
                          "Bad binding of State bytes count; sqlite error: " + to_string(rc));
    }
    GEOEpochTimestamp timestamp = microsecondsSinceGEOEpoch(utc_now());
    rc = sqlite3_bind_int64(stmt, 4, timestamp);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::insert: "
                          "Bad binding of Timestamp; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_step(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE) {
#ifdef STORAGE_HANDLER_DEBUG_LOG
        info() << "prepare inserting is completed successfully";
#endif
    } else {
        warning() << "PaymentOperationStateHandler::insert: Run query; sqlite error: " << rc;
        throw IOError("PaymentOperationStateHandler::insert: "
                          "Run query; sqlite error: " + to_string(rc));
    }
}

void PaymentOperationStateHandler::deleteRecord(
    const TransactionUUID &transactionUUID)
{
    string query = "DELETE FROM " + mTableName + " WHERE transaction_uuid = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::delete: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_blob(stmt, 1, transactionUUID.data, NodeUUID::kBytesSize, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::delete: "
                          "Bad binding of TransactionUUID; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_step(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE) {
#ifdef STORAGE_HANDLER_DEBUG_LOG
        info() << "prepare deleting is completed successfully";
#endif
    } else {
        throw IOError("PaymentOperationStateHandler::delete: "
                          "Run query; sqlite error: " + to_string(rc));
    }
}

pair<BytesShared, size_t> PaymentOperationStateHandler::byTransaction(
    const TransactionUUID &transactionUUID)
{
    string query = "SELECT state, state_bytes_count FROM " + mTableName + " WHERE transaction_uuid = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::byTransaction: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_blob(stmt, 1, transactionUUID.data, NodeUUID::kBytesSize, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        throw IOError("PaymentOperationStateHandler::byTransaction: "
                          "Bad binding of TransactionUUID; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        size_t stateBytesCount = (size_t)sqlite3_column_int(stmt, 1);
        BytesShared state = tryMalloc(stateBytesCount);
        memcpy(
            state.get(),
            sqlite3_column_blob(stmt, 0),
            stateBytesCount);
        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);
        return make_pair(state, stateBytesCount);
    } else {
        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);
        throw NotFoundError("PaymentOperationStateHandler::byTransaction: "
                                "There are now records with requested transactionUUID");
    }
}

LoggerStream PaymentOperationStateHandler::info() const
{
    return mLog.info(logHeader());
}

LoggerStream PaymentOperationStateHandler::warning() const
{
    return mLog.warning(logHeader());
}

const string PaymentOperationStateHandler::logHeader() const
{
    stringstream s;
    s << "[PaymentOperationStateHandler]";
    return s.str();
}
