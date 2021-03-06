﻿/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_BASETRANSACTION_H
#define GEO_NETWORK_CLIENT_BASETRANSACTION_H

#include "TransactionUUID.h"

#include "../../../common/Types.h"
#include "../../../common/NodeUUID.h"
#include "../../../common/memory/MemoryUtils.h"

#include "../../../network/messages/Message.hpp"
#include "../../../network/messages/base/transaction/ConfirmationMessage.h"
#include "../result/TransactionResult.h"
#include "../result/state/TransactionState.h"
#include "../../../interface/results_interface/result/CommandResult.h"
#include "../../../resources/resources/BaseResource.h"

#include "../../../common/exceptions/RuntimeError.h"

#include "../../../logger/Logger.h"

#include <boost/signals2.hpp>

#include <vector>
#include <deque>
#include <utility>
#include <cstdint>
#include <sstream>


namespace signals = boost::signals2;

// todo: [hsc] consider separating network logic from the base TA logic, for example in separate class.
class BaseTransaction {
public:
    typedef shared_ptr<BaseTransaction> Shared;
    typedef uint16_t SerializedTransactionType;
    typedef uint16_t SerializedStep;

    typedef signals::signal<void(Message::Shared, const NodeUUID&)> SendMessageSignal;
    typedef signals::signal<void(BaseTransaction::Shared)> LaunchSubsidiaryTransactionSignal;
    typedef signals::signal<void(const NodeUUID&, ConfirmationMessage::Shared)> ProcessConfirmationMessageSignal;

public:
    // TODO: add other states shortcuts here
    TransactionResult::Shared resultDone () const;

    TransactionResult::Shared resultFlushAndContinue() const;

    TransactionResult::Shared resultWaitForMessageTypes(
        vector<Message::MessageType> &&requiredMessagesTypes,
        uint32_t noLongerThanMilliseconds) const;

    TransactionResult::Shared resultWaitForResourceTypes(
        vector<BaseResource::ResourceType> &&requiredResourcesType,
        uint32_t noLongerThanMilliseconds) const;

    TransactionResult::Shared resultAwakeAfterMilliseconds(
        uint32_t responseWaitTime) const ;

    TransactionResult::Shared resultContinuePreviousState() const;

    TransactionResult::Shared resultWaitForMessageTypesAndAwakeAfterMilliseconds(
        vector<Message::MessageType> &&requiredMessagesTypes,
        uint32_t noLongerThanMilliseconds) const;

public:
    virtual ~BaseTransaction() = default;

    enum TransactionType {
        SetOutgoingTrustLineTransaction = 100,
        SetIncomingTrustLineTransaction = 101,
        CloseIncomingTrustLineTransaction = 102,
        CloseOutgoingTrustLineTransaction = 103,
        RejectOutgoingTrustLineTransaction = 104,


        // Cycles
        Cycles_ThreeNodesInitTransaction = 200,
        Cycles_ThreeNodesReceiverTransaction = 201,
        Cycles_FourNodesInitTransaction = 202,
        Cycles_FourNodesReceiverTransaction = 203,
        Cycles_FiveNodesInitTransaction = 204,
        Cycles_FiveNodesReceiverTransaction = 205,
        Cycles_SixNodesInitTransaction = 206,
        Cycles_SixNodesReceiverTransaction = 207,

        // Payments
        CoordinatorPaymentTransaction = 301,
        ReceiverPaymentTransaction = 302,
        IntermediateNodePaymentTransaction = 303,
        VoutesStatusResponsePaymentTransaction = 304,
        Payments_CycleCloserInitiatorTransaction = 305,
        Payments_CycleCloserIntermediateNodeTransaction = 306,

        // Max flow calculation
        CollectTopologyTransactionType = 401,
        InitiateMaxFlowCalculationTransactionType = 402,
        ReceiveMaxFlowCalculationOnTargetTransactionType = 403,
        MaxFlowCalculationSourceFstLevelTransactionType = 404,
        MaxFlowCalculationTargetFstLevelTransactionType = 405,
        MaxFlowCalculationSourceSndLevelTransactionType = 406,
        MaxFlowCalculationTargetSndLevelTransactionType = 407,
        ReceiveResultMaxFlowCalculationTransactionType = 408,
        MaxFlowCalculationCacheUpdateTransactionType = 409,
        MaxFlowCalculationStepTwoTransactionType = 410,
        MaxFlowCalculationFullyTransactionType = 411,

        // Contractors
        ContractorsList = 500,
        TrustlinesList = 501,
        TrustLineOne = 502,

        // TotalBalances
        TotalBalancesTransactionType = 600,

        // History
        HistoryPaymentsTransactionType = 700,
        HistoryTrustLinesTransactionType = 701,
        HistoryWithContractorTransactionType = 702,
        HistoryAdditionalPaymentsTransactionType = 703,

        // FindPath
        FindPathByMaxFlowTransactionType = 800,

        //RoutingTable
        RoutingTableInitTransactionType = 900,
        RoutingTableResponceTransactionType = 901,

        //BlackList
        AddNodeToBlackListTransactionType = 1000,
        CheckIfNodeInBlackListTransactionType = 1001,
        RemoveNodeFromBlackListTransactionType = 1002,
        GetBlackListTransactionType = 1003,

        // Transactions
        TransactionByCommandUUIDType = 1100,

        // Gateway notification
        GatewayNotificationSenderType = 1200,
        GatewayNotificationReceiverType = 1201,
    };

public:
    virtual const TransactionType transactionType() const;

    const TransactionUUID &currentTransactionUUID () const;

    const NodeUUID &currentNodeUUID () const;

    const int currentStep() const;

    void recreateTransactionUUID();

    void pushContext(
        Message::Shared message);

    void pushResource(
        BaseResource::Shared resource);

    virtual pair<BytesShared, size_t> serializeToBytes() const;

    virtual TransactionResult::SharedConst run() = 0;

protected:
    BaseTransaction(
        const TransactionType type,
        Logger &log);

    BaseTransaction(
        const TransactionType type,
        const TransactionUUID &transactionUUID,
        Logger &log);

    BaseTransaction(
        const TransactionType type,
        const NodeUUID &nodeUUID,
        Logger &log);

    BaseTransaction(
        const TransactionType type,
        const TransactionUUID &transactionUUID,
        const NodeUUID &nodeUUID,
        Logger &log);

    BaseTransaction(
        BytesShared buffer,
        const NodeUUID &nodeUUID,
        Logger &log);

    // TODO: convert to hpp?
    template <typename ContextMessageType>
    inline shared_ptr<ContextMessageType> popNextMessage()
    {
        const auto message = static_pointer_cast<ContextMessageType>(mContext.front());
        mContext.pop_front();
        return message;
    }

    template<typename ResourceType>
    inline shared_ptr<ResourceType> popNextResource() {

        const auto resource = static_pointer_cast<ResourceType>(mResources.front());
        mResources.pop_front();
        return resource;
    }

    // TODO: convert to hpp?
    template <typename MessageType, typename... Args>
    inline void sendMessage(
        const NodeUUID &addressee,
        Args&&... args) const
    {
        const auto message = make_shared<MessageType>(args...);
        outgoingMessageIsReadySignal(
            message,
            addressee);
    }

    inline void sendMessage(
        const NodeUUID &addressee,
        const Message::Shared message) const
    {
        outgoingMessageIsReadySignal(
            message,
            addressee);
    }

    void processConfirmationMessage(
        const NodeUUID &contractorUUID,
        const ConfirmationMessage::Shared confirmationMessage)
    {
        processConfirmationMessageSignal(
            contractorUUID,
            confirmationMessage);
    }

    void launchSubsidiaryTransaction(
      BaseTransaction::Shared transaction);

    void clearContext();

    // todo: [hsc] consider using bytes serializer / deserializer
    virtual void deserializeFromBytes(
        BytesShared buffer);

    static const size_t kOffsetToInheritedBytes();

    TransactionResult::SharedConst transactionResultFromCommand(
        CommandResult::SharedConst result) const;

    virtual const string logHeader() const = 0;
    LoggerStream info() const;
    LoggerStream warning() const;
    LoggerStream error() const;
    LoggerStream debug() const;

public:
    mutable SendMessageSignal outgoingMessageIsReadySignal;
    mutable LaunchSubsidiaryTransactionSignal runSubsidiaryTransactionSignal;
    mutable ProcessConfirmationMessageSignal processConfirmationMessageSignal;

protected:
    static const uint16_t mkStandardConnectionTimeout = 1500;   //milliseconds
    static const uint16_t mkWaitingForResponseTime = 3000;

protected:
    TransactionType mType;
    TransactionUUID mTransactionUUID;
    NodeUUID mNodeUUID;
    deque<Message::Shared> mContext;
    deque<BaseResource::Shared> mResources;
    SerializedStep mStep = 1;
    uint8_t mVotesRecoveryStep = 0;

    Logger &mLog;
};


#endif //GEO_NETWORK_CLIENT_BASETRANSACTION_H
