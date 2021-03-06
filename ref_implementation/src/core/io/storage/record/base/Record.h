/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_RECORD_H
#define GEO_NETWORK_CLIENT_RECORD_H

#include "../../../../common/Types.h"
#include "../../../../transactions/transactions/base/TransactionUUID.h"
#include "../../../../common/time/TimeUtils.h"

class Record {
public:
    typedef shared_ptr<Record> Shared;

public:
    enum RecordType {
        TrustLineRecordType = 1,
        PaymentRecordType
    };

public:
    virtual const bool isTrustLineRecord() const;

    virtual const bool isPaymentRecord() const;

    const Record::RecordType recordType() const;

    const TransactionUUID operationUUID() const;

    const DateTime timestamp() const;

    const NodeUUID contractorUUID() const;

protected:
    Record();

    Record(
        const Record::RecordType recordType,
        const TransactionUUID &operationUUID,
        const NodeUUID &contractorUUID);

    Record(
        const Record::RecordType recordType,
        const TransactionUUID &operationUUID,
        const NodeUUID &contractorUUID,
        const GEOEpochTimestamp geoEpochTimestamp);

public:
    static const size_t kOperationUUIDBytesSize = 16;

private:
    RecordType mRecordType;
    TransactionUUID mOperationUUID;
    DateTime mTimestamp;
    NodeUUID mContractorUUID;
};

#endif //GEO_NETWORK_CLIENT_RECORD_H
