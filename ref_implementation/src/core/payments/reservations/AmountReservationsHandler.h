﻿/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_AMOUNTBLOCKSHANDLER_H
#define GEO_NETWORK_CLIENT_AMOUNTBLOCKSHANDLER_H


#include "AmountReservation.h"
#include "../../common/NodeUUID.h"
#include "../../common/exceptions/MemoryError.h"
#include "../../common/exceptions/ValueError.h"
#include "../../common/exceptions/NotFoundError.h"
#include "../../transactions/transactions/base/TransactionUUID.h"

#include <map>


class AmountReservationsHandler {
public:
    AmountReservation::ConstShared reserve(
        const NodeUUID &trustLineContractor,
        const TransactionUUID &transactionUUID,
        const TrustLineAmount &amount,
        const AmountReservation::ReservationDirection direction);

    AmountReservation::ConstShared updateReservation(
        const NodeUUID &trustLineContractor,
        const AmountReservation::ConstShared reservation,
        const TrustLineAmount &newAmount);

    void free(
        const NodeUUID &trustLineContractor,
        const AmountReservation::ConstShared reservation);

    ConstSharedTrustLineAmount totalReserved(
        const NodeUUID &trustLineContractor,
        const AmountReservation::ReservationDirection direction,
        const TransactionUUID *transactionUUID = nullptr) const;

    ConstSharedTrustLineAmount totalReservedOnTrustLine(
        const NodeUUID &trustLineContractor) const;

    bool isReservationPresent(
        const NodeUUID &trustLineContractor) const;

    const vector<AmountReservation::ConstShared> contractorReservations(
        const NodeUUID &contractorUUID,
        const AmountReservation::ReservationDirection direction) const;

protected:
    // One trust line may hold several amount reservations,
    // so the vector<AmountReservation::ConstShared> is used (reservations container).
    //
    // Reservations container would be requested from the map very often,
    // but map's iterator returns copy of the object,
    // so the unique_ptr<vector> is used to get the container without copying it.
    map<NodeUUID, unique_ptr<vector<AmountReservation::ConstShared>>> mReservations;

protected:
    std::vector<AmountReservation::ConstShared> reservations(
        const NodeUUID &trustLineContractor,
        const TransactionUUID *transactionUUID = nullptr) const;
};


#endif //GEO_NETWORK_CLIENT_AMOUNTBLOCKSHANDLER_H
