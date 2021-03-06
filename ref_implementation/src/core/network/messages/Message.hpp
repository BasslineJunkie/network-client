﻿/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_MESSAGE_H
#define GEO_NETWORK_CLIENT_MESSAGE_H

#include "../../common/memory/MemoryUtils.h"
#include "../communicator/internal/common/Packet.hpp"

#include <limits>


using namespace std;


class Message {
public:
    typedef shared_ptr<Message> Shared;
    typedef uint16_t SerializedType;

public:
    enum MessageType {
        /*
         * System messages types
         */
        System_Confirmation = 0,

        /*
         * Trust lines
         */
        // these messages has destination uuid, that's why they should be checked on communicator
        // and their types should be added on Communicator::onMessageReceived if condition
        TrustLines_SetIncoming = 100,
        TrustLines_CloseOutgoing = 101,
        TrustLines_SetIncomingFromGateway = 102,

        /*
         * Payments messages
         */
        Payments_ReceiverInitPaymentRequest = 201,
        Payments_ReceiverInitPaymentResponse = 202,
        Payments_CoordinatorReservationRequest = 203,
        Payments_CoordinatorReservationResponse = 204,
        Payments_IntermediateNodeReservationRequest = 205,
        Payments_IntermediateNodeReservationResponse = 206,

        Payments_CoordinatorCycleReservationRequest = 207,
        Payments_CoordinatorCycleReservationResponse = 208,
        Payments_IntermediateNodeCycleReservationRequest = 209,
        Payments_IntermediateNodeCycleReservationResponse = 210,

        Payments_FinalAmountsConfiguration = 211,
        Payments_FinalAmountsConfigurationResponse = 212,

        Payments_ParticipantsVotes = 213,
        Payments_VotesStatusRequest = 214,
        Payments_FinalPathConfiguration = 215,
        Payments_FinalPathCycleConfiguration = 216,
        Payments_TTLProlongationRequest = 217,
        Payments_TTLProlongationResponse = 218,

        Payments_ReservationsInRelationToNode = 219,

        /*
         * Cycles
         */
        Cycles_ThreeNodesBalancesRequest = 300,
        Cycles_ThreeNodesBalancesResponse = 301,
        Cycles_FourNodesBalancesRequest = 302,
        Cycles_FourNodesBalancesResponse = 303,
        Cycles_FiveNodesBoundary = 304,
        Cycles_FiveNodesMiddleware = 305,
        Cycles_SixNodesBoundary = 306,
        Cycles_SixNodesMiddleware = 307,

        /*
         * Max flow
         */
        MaxFlow_InitiateCalculation = 400,
        MaxFlow_CalculationSourceFirstLevel = 401,
        MaxFlow_CalculationTargetFirstLevel = 402,
        MaxFlow_CalculationSourceSecondLevel = 403,
        MaxFlow_CalculationTargetSecondLevel = 404,
        MaxFlow_ResultMaxFlowCalculation = 405,
        MaxFlow_ResultMaxFlowCalculationFromGateway = 406,

        /*
         * Empty slot with codes 500-599
         */

        /*
         * Routing table
         */
        RoutingTableRequest = 600,
        RoutingTableResponse = 601,

        /*
         * Gateway notification
         */
        GatewayNotification = 700,

        /*
         * DEBUG
         */
        // Obvious, that we have to set this code
        Debug = 6666,
    };

public:
    virtual ~Message() = default;

    /*
     * Returns max allowed size of the message in bytes.
     */
    static size_t maxSize()
    {
        return
             numeric_limits<PacketHeader::PacketIndex>::max() * Packet::kMaxSize -
            (numeric_limits<PacketHeader::PacketIndex>::max() * PacketHeader::kSize);
    }

    /*
     * Some of derived classes are used for various transactions responses.
     *
     * Transactions scheduler requires mechanism to know
     * which response to attach to which transaction.
     * The simplest way to do this - is to attach response to the transaction by it's UUID
     * (transactions scheduler checks if "transactionUUID" of the response is equal to the UUID of the transaction).
     *
     * But transaction UUID may be redundant is various cases
     * (for example, in routing tables responses, max flow calculation responses,
     * and several other).
     *
     * This method(s) makes it possible for the transactions scheduler to know,
     * how to decide which response should be attached to which transaction,
     * and implement custom attach logic for each one response type.
     *
     *
     * WARN:
     * Derived classes of specific responses must override one of this methods.
     */
    virtual const bool isTransactionMessage() const
    {
        return false;
    }

    virtual const MessageType typeID() const = 0;

    /**
     * @throws bad_alloc;
     */
    virtual pair<BytesShared, size_t> serializeToBytes() const
        noexcept(false)
    {
        // todo: [hsc] use bytes serializer, if possible in all descendant messages.
        // todo: [hsc] check if bytes serializer is really faster/safer, than raw pointers usage.

        const SerializedType kMessageType = typeID();
        auto buffer = tryMalloc(sizeof(kMessageType));

        memcpy(
            buffer.get(),
            &kMessageType,
            sizeof(kMessageType));

        return make_pair(
            buffer,
            sizeof(kMessageType));
    }

protected:
    virtual void deserializeFromBytes(
        BytesShared buffer)
    {}

    virtual const size_t kOffsetToInheritedBytes() const
    {
        return sizeof(SerializedType);
    }
};

#endif //GEO_NETWORK_CLIENT_MESSAGE_H
