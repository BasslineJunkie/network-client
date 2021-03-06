﻿/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#include "TransactionsManager.h"

/*!
 *
 * Throws RuntimeError in case if some internal components can't be initialised.
 */
TransactionsManager::TransactionsManager(
    NodeUUID &nodeUUID,
    as::io_service &IOService,
    TrustLinesManager *trustLinesManager,
    ResourcesManager *resourcesManager,
    MaxFlowCalculationTrustLineManager *maxFlowCalculationTrustLineManager,
    MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
    MaxFlowCalculationNodeCacheManager *maxFlowCalculationNodeCacheManager,
    ResultsInterface *resultsInterface,
    StorageHandler *storageHandler,
    PathsManager *pathsManager,
    RoutingTableManager *routingTable,
    Logger &logger,
    SubsystemsController *subsystemsController,
    bool iAmGateway) :

    mNodeUUID(nodeUUID),
    mIOService(IOService),
    mTrustLines(trustLinesManager),
    mResourcesManager(resourcesManager),
    mMaxFlowCalculationTrustLineManager(maxFlowCalculationTrustLineManager),
    mMaxFlowCalculationCacheManager(maxFlowCalculationCacheManager),
    mMaxFlowCalculationNodeCacheManager(maxFlowCalculationNodeCacheManager),
    mResultsInterface(resultsInterface),
    mStorageHandler(storageHandler),
    mPathsManager(pathsManager),
    mLog(logger),
    mSubsystemsController(subsystemsController),
    mRoutingTable(routingTable),
    mIAmGateway(iAmGateway),

    mScheduler(
        new TransactionsScheduler(
            mIOService,
            mLog)),
    mCyclesManager(
        new CyclesManager(
            mNodeUUID,
            mScheduler.get(),
            mIOService,
            mLog,
            mSubsystemsController))
{
    subscribeForCommandResult(
        mScheduler->commandResultIsReadySignal);
    subscribeForSerializeTransaction(
        mScheduler->serializeTransactionSignal);
    subscribeForCloseCycleTransaction(
        mCyclesManager->closeCycleSignal);
    subscribeForBuildCyclesFiveNodesTransaction(
        mCyclesManager->buildFiveNodesCyclesSignal);
    subscribeForBuildCyclesSixNodesTransaction(
        mCyclesManager->buildSixNodesCyclesSignal);
    subscribeForTryCloseNextCycleSignal(
        mScheduler->cycleCloserTransactionWasFinishedSignal);

    try {
        loadTransactionsFromStorage();

    } catch (exception &e) {
        throw RuntimeError(e.what());
    }

    // todo: send current trust line amount to te contractors
}

void TransactionsManager::loadTransactionsFromStorage()
{
    const auto ioTransaction = mStorageHandler->beginTransaction();
    const auto serializedTAs = ioTransaction->transactionHandler()->allTransactions();

    for(const auto kTABufferAndSize: serializedTAs) {
        BaseTransaction::SerializedTransactionType *transactionType =
                new (kTABufferAndSize.first.get()) BaseTransaction::SerializedTransactionType;
        auto TransactionTypeId = *transactionType;
        switch (TransactionTypeId) {
            case BaseTransaction::TransactionType::CoordinatorPaymentTransaction: {
                auto transaction = make_shared<CoordinatorPaymentTransaction>(
                    kTABufferAndSize.first,
                    mNodeUUID,
                    mTrustLines,
                    mStorageHandler,
                    mMaxFlowCalculationCacheManager,
                    mMaxFlowCalculationNodeCacheManager,
                    mResourcesManager,
                    mPathsManager,
                    mLog,
                    mSubsystemsController);
                subscribeForBuildCyclesThreeNodesTransaction(
                    transaction->mBuildCycleThreeNodesSignal);
                prepareAndSchedule(
                    transaction,
                    true,
                    false,
                    true);
                break;
            }
            case BaseTransaction::TransactionType::IntermediateNodePaymentTransaction: {
                auto transaction = make_shared<IntermediateNodePaymentTransaction>(
                    kTABufferAndSize.first,
                    mNodeUUID,
                    mTrustLines,
                    mStorageHandler,
                    mMaxFlowCalculationCacheManager,
                    mMaxFlowCalculationNodeCacheManager,
                    mLog,
                    mSubsystemsController);
                subscribeForBuildCyclesThreeNodesTransaction(
                    transaction->mBuildCycleThreeNodesSignal);
                subscribeForBuildCyclesFourNodesTransaction(
                    transaction->mBuildCycleFourNodesSignal);
                prepareAndSchedule(
                    transaction,
                    false,
                    false,
                    true);
                break;
            }
            case BaseTransaction::TransactionType::ReceiverPaymentTransaction: {
                auto transaction = make_shared<ReceiverPaymentTransaction>(
                    kTABufferAndSize.first,
                    mNodeUUID,
                    mTrustLines,
                    mStorageHandler,
                    mMaxFlowCalculationCacheManager,
                    mMaxFlowCalculationNodeCacheManager,
                    mLog,
                    mSubsystemsController);
                subscribeForBuildCyclesThreeNodesTransaction(
                    transaction->mBuildCycleThreeNodesSignal);
                prepareAndSchedule(
                    transaction,
                    false,
                    false,
                    true);
                break;
            }
            case BaseTransaction::TransactionType::Payments_CycleCloserInitiatorTransaction: {
                auto transaction = make_shared<CycleCloserInitiatorTransaction>(
                    kTABufferAndSize.first,
                    mNodeUUID,
                    mTrustLines,
                    mCyclesManager.get(),
                    mStorageHandler,
                    mMaxFlowCalculationCacheManager,
                    mMaxFlowCalculationNodeCacheManager,
                    mLog,
                    mSubsystemsController);
                prepareAndSchedule(
                    transaction,
                    false,
                    false,
                    true);
                break;
            }
            case BaseTransaction::TransactionType::Payments_CycleCloserIntermediateNodeTransaction: {
                auto transaction = make_shared<CycleCloserIntermediateNodeTransaction>(
                    kTABufferAndSize.first,
                    mNodeUUID,
                    mTrustLines,
                    mCyclesManager.get(),
                    mStorageHandler,
                    mMaxFlowCalculationCacheManager,
                    mMaxFlowCalculationNodeCacheManager,
                    mLog,
                    mSubsystemsController);
                prepareAndSchedule(
                    transaction,
                    false,
                    false,
                    true);
                break;
            }
            default: {
                throw RuntimeError(
                    "TrustLinesManager::loadTransactions. "
                        "Unexpected transaction type identifier.");
            }
        }

    }
}

void TransactionsManager::processCommand(
    BaseUserCommand::Shared command)
{
    // ToDo: sort calls in the call probabilty order.
    // For example, max flows calculations would be called much ofetn, then credit usage transactions.
    // So, why we are checking trust lines commands first, and max flow is only in the middle of the check sequence?

    if (command->identifier() == SetOutgoingTrustLineCommand::identifier()) {
        launchSetOutgoingTrustLineTransaction(
            static_pointer_cast<SetOutgoingTrustLineCommand>(
                command));

    } else if (command->identifier() == CloseIncomingTrustLineCommand::identifier()) {
        launchCloseIncomingTrustLineTransaction(
            static_pointer_cast<CloseIncomingTrustLineCommand>(
                command));

    } else if (command->identifier() == CreditUsageCommand::identifier()) {
        launchCoordinatorPaymentTransaction(
            dynamic_pointer_cast<CreditUsageCommand>(
                command));

    } else if (command->identifier() == InitiateMaxFlowCalculationCommand::identifier()){
        launchInitiateMaxFlowCalculatingTransaction(
            static_pointer_cast<InitiateMaxFlowCalculationCommand>(
                command));

    } else if (command->identifier() == InitiateMaxFlowCalculationFullyCommand::identifier()){
        launchMaxFlowCalculationFullyTransaction(
            static_pointer_cast<InitiateMaxFlowCalculationFullyCommand>(
                command));

    } else if (command->identifier() == TotalBalancesCommand::identifier()){
        launchTotalBalancesTransaction(
            static_pointer_cast<TotalBalancesCommand>(
                command));

    } else if (command->identifier() == HistoryPaymentsCommand::identifier()){
        launchHistoryPaymentsTransaction(
            static_pointer_cast<HistoryPaymentsCommand>(
                command));

    } else if (command->identifier() == HistoryAdditionalPaymentsCommand::identifier()){
        launchAdditionalHistoryPaymentsTransaction(
            static_pointer_cast<HistoryAdditionalPaymentsCommand>(
                command));

    } else if (command->identifier() == HistoryTrustLinesCommand::identifier()){
        launchHistoryTrustLinesTransaction(
            static_pointer_cast<HistoryTrustLinesCommand>(
                command));

    } else if (command->identifier() == HistoryWithContractorCommand::identifier()){
        launchHistoryWithContractorTransaction(
            static_pointer_cast<HistoryWithContractorCommand>(
                command));

    } else if (command->identifier() == GetFirstLevelContractorsCommand::identifier()){
        launchGetFirstLevelContractorsTransaction(
                static_pointer_cast<GetFirstLevelContractorsCommand>(
                        command));

    } else if (command->identifier() == GetTrustLinesCommand::identifier()){
        launchGetTrustlinesTransaction(
            static_pointer_cast<GetTrustLinesCommand>(
                command));

    } else if (command->identifier() == GetTrustLineCommand::identifier()){
        launchGetTrustlineTransaction(
            static_pointer_cast<GetTrustLineCommand>(
                command));

    // BlackList Commands
    } else if (command->identifier() == AddNodeToBlackListCommand::identifier()){

        launchAddNodeToBlackListTransaction(
            static_pointer_cast<AddNodeToBlackListCommand>(
                command));

    } else if (command->identifier() == CheckIfNodeInBlackListCommand::identifier()){
        launchCheckIfNodeInBlackListTransaction(
            static_pointer_cast<CheckIfNodeInBlackListCommand>(
                command));

    } else if (command->identifier() == RemoveNodeFromBlackListCommand::identifier()){
        launchRemoveNodeFromBlackListTransaction(
            static_pointer_cast<RemoveNodeFromBlackListCommand>(
                command));

    } else if (command->identifier() == GetBlackListCommand::identifier()){
        launchGetBlackListTransaction(
            static_pointer_cast<GetBlackListCommand>(
                command));

    } else if (command->identifier() == PaymentTransactionByCommandUUIDCommand::identifier()){
        launchPaymentTransactionByCommandUUIDTransaction(
            static_pointer_cast<PaymentTransactionByCommandUUIDCommand>(
                command));

    } else {
        throw ValueError(
            "TransactionsManager::processCommand: "
                "Unexpected command identifier.");
    }
}

void TransactionsManager::processMessage(
    Message::Shared message)
{
    // ToDo: sort calls in the call probability order.
    // For example, max flows calculations would be called much oftetn, then credit usage transactions.

    /*
     * Max flow
     */
    if (message->typeID() == Message::MessageType::MaxFlow_InitiateCalculation) {
        launchReceiveMaxFlowCalculationOnTargetTransaction(
            static_pointer_cast<InitiateMaxFlowCalculationMessage>(message));

    } else if (message->typeID() == Message::MessageType::MaxFlow_ResultMaxFlowCalculation) {
        try {
            mScheduler->tryAttachMessageToCollectTopologyTransaction(message);
        } catch (NotFoundError &) {
            launchReceiveResultMaxFlowCalculationTransaction(
                static_pointer_cast<ResultMaxFlowCalculationMessage>(message));
        }

    } else if (message->typeID() == Message::MessageType::MaxFlow_ResultMaxFlowCalculationFromGateway) {
        try {
            mScheduler->tryAttachMessageToCollectTopologyTransaction(message);
        } catch (NotFoundError &) {
            launchReceiveResultMaxFlowCalculationTransactionFromGateway(
                static_pointer_cast<ResultMaxFlowCalculationGatewayMessage>(message));
        }

    } else if (message->typeID() == Message::MessageType::MaxFlow_CalculationSourceFirstLevel) {
        launchMaxFlowCalculationSourceFstLevelTransaction(
            static_pointer_cast<MaxFlowCalculationSourceFstLevelMessage>(message));

    } else if (message->typeID() == Message::MessageType::MaxFlow_CalculationTargetFirstLevel) {
        launchMaxFlowCalculationTargetFstLevelTransaction(
            static_pointer_cast<MaxFlowCalculationTargetFstLevelMessage>(message));

    } else if (message->typeID() == Message::MessageType::MaxFlow_CalculationSourceSecondLevel) {
        launchMaxFlowCalculationSourceSndLevelTransaction(
            static_pointer_cast<MaxFlowCalculationSourceSndLevelMessage>(message));

    } else if (message->typeID() == Message::MessageType::MaxFlow_CalculationTargetSecondLevel) {
        launchMaxFlowCalculationTargetSndLevelTransaction(
            static_pointer_cast<MaxFlowCalculationTargetSndLevelMessage>(message));

    /*
     * Payments
     */
    } else if (message->typeID() == Message::Payments_ReceiverInitPaymentRequest) {
        launchReceiverPaymentTransaction(
            static_pointer_cast<ReceiverInitPaymentRequestMessage>(message));

    } else if (message->typeID() == Message::Payments_IntermediateNodeReservationRequest) {
        // It is possible, that transaction was already initialised
        // by the ReceiverInitPaymentRequest.
        // In this case - message must be simply attached to it,
        // no new transaction must be launched.
        try {
            mScheduler->tryAttachMessageToTransaction(message);

        } catch (NotFoundError &) {
            launchIntermediateNodePaymentTransaction(
                    static_pointer_cast<IntermediateNodeReservationRequestMessage>(message));
        }

    } else if (message->typeID() == Message::Payments_IntermediateNodeCycleReservationRequest) {
        // It is possible, that transaction was already initialised
        // by the ReceiverInitPaymentRequest.
        // In this case - message must be simply attached to it,
        // no new transaction must be launched.
        try {
            mScheduler->tryAttachMessageToTransaction(message);

        } catch (NotFoundError &) {
            launchCycleCloserIntermediateNodeTransaction(
                static_pointer_cast<IntermediateNodeCycleReservationRequestMessage>(message));
        }
    } else if(message->typeID() == Message::MessageType::Payments_VotesStatusRequest){
        launchVotesResponsePaymentsTransaction(
            static_pointer_cast<VotesStatusRequestMessage>(message));

    /*
     * Cycles
     */
    } else if (message->typeID() == Message::MessageType::Cycles_SixNodesMiddleware) {
        launchSixNodesCyclesResponseTransaction(
            static_pointer_cast<CyclesSixNodesInBetweenMessage>(message));

    } else if (message->typeID() == Message::MessageType::Cycles_FiveNodesMiddleware) {
        launchFiveNodesCyclesResponseTransaction(
            static_pointer_cast<CyclesFiveNodesInBetweenMessage>(message));

    } else if (message->typeID() == Message::MessageType::Cycles_ThreeNodesBalancesRequest){
        launchThreeNodesCyclesResponseTransaction(
            static_pointer_cast<CyclesThreeNodesBalancesRequestMessage>(message));

    } else if (message->typeID() == Message::MessageType::Cycles_FourNodesBalancesRequest){
        launchFourNodesCyclesResponseTransaction(
            static_pointer_cast<CyclesFourNodesBalancesRequestMessage>(message));

    /*
     * Trust lines
     */
    } else if (message->typeID() == Message::TrustLines_SetIncoming) {
        launchSetIncomingTrustLineTransaction(
            static_pointer_cast<SetIncomingTrustLineMessage>(message));

    } else if (message->typeID() == Message::TrustLines_SetIncomingFromGateway) {
        launchSetIncomingTrustLineTransaction(
            static_pointer_cast<SetIncomingTrustLineFromGatewayMessage>(message));

    } else if (message->typeID() == Message::TrustLines_CloseOutgoing) {
        launchCloseOutgoingTrustLineTransaction(
            static_pointer_cast<CloseOutgoingTrustLineMessage>(message));

    } else if (message->typeID() == Message::System_Confirmation) {
        launchRejectOutgoingTrustLineTransaction(
            static_pointer_cast<ConfirmationMessage>(message));

    /*
     * RoutingTable
    */
    } else if (message->typeID() == Message::RoutingTableRequest) {
        launchRoutingTableResponseTransaction(
            static_pointer_cast<RoutingTableRequestMessage>(message));
    /*
     * Gateway notification
     */
    } else if (message->typeID() == Message::GatewayNotification) {
        launchGatewayNotificationReceiverTransaction(
            static_pointer_cast<GatewayNotificationMessage>(message));

    } else {
        mScheduler->tryAttachMessageToTransaction(message);
    }
}

void TransactionsManager::launchSetOutgoingTrustLineTransaction(
    SetOutgoingTrustLineCommand::Shared command)
{
    prepareAndSchedule(
        make_shared<SetOutgoingTrustLineTransaction>(
            mNodeUUID,
            command,
            mTrustLines,
            mStorageHandler,
            mMaxFlowCalculationCacheManager,
            mMaxFlowCalculationNodeCacheManager,
            mSubsystemsController,
            mIAmGateway,
            mLog),
        true,
        false,
        true);
}

void TransactionsManager::launchCloseIncomingTrustLineTransaction(
    CloseIncomingTrustLineCommand::Shared command)
{
    prepareAndSchedule(
        make_shared<CloseIncomingTrustLineTransaction>(
            mNodeUUID,
            command,
            mTrustLines,
            mStorageHandler,
            mMaxFlowCalculationCacheManager,
            mMaxFlowCalculationNodeCacheManager,
            mSubsystemsController,
            mLog),
        true,
        false,
        true);
}

void TransactionsManager::launchSetIncomingTrustLineTransaction(
    SetIncomingTrustLineMessage::Shared message)
{
    prepareAndSchedule(
        make_shared<SetIncomingTrustLineTransaction>(
            mNodeUUID,
            message,
            mTrustLines,
            mStorageHandler,
            mMaxFlowCalculationCacheManager,
            mMaxFlowCalculationNodeCacheManager,
            mIAmGateway,
            mLog),
        true,
        false,
        true);
}

void TransactionsManager::launchSetIncomingTrustLineTransaction(
    SetIncomingTrustLineFromGatewayMessage::Shared message)
{
    prepareAndSchedule(
        make_shared<SetIncomingTrustLineTransaction>(
            mNodeUUID,
            message,
            mTrustLines,
            mStorageHandler,
            mMaxFlowCalculationCacheManager,
            mMaxFlowCalculationNodeCacheManager,
            mIAmGateway,
            mLog),
        true,
        false,
        true);
}

void TransactionsManager::launchCloseOutgoingTrustLineTransaction(
    CloseOutgoingTrustLineMessage::Shared message)
{
    prepareAndSchedule(
        make_shared<CloseOutgoingTrustLineTransaction>(
            mNodeUUID,
            message,
            mTrustLines,
            mStorageHandler,
            mMaxFlowCalculationCacheManager,
            mMaxFlowCalculationNodeCacheManager,
            mLog),
        true,
        false,
        true);
}

void TransactionsManager::launchRejectOutgoingTrustLineTransaction(
    ConfirmationMessage::Shared message)
{
    auto transaction = make_shared<RejectOutgoingTrustLineTransaction>(
        mNodeUUID,
        message,
        mTrustLines,
        mStorageHandler,
        mLog);
    prepareAndSchedule(
        transaction,
        true,
        false,
        false);
    subscribeForProcessingConfirmationMessage(
        transaction->processConfirmationMessageSignal);
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchInitiateMaxFlowCalculatingTransaction(
    InitiateMaxFlowCalculationCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<InitiateMaxFlowCalculationTransaction>(
                mNodeUUID,
                command,
                mTrustLines,
                mMaxFlowCalculationTrustLineManager,
                mMaxFlowCalculationCacheManager,
                mMaxFlowCalculationNodeCacheManager,
                mIAmGateway,
                mLog),
            true,
            true,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchMaxFlowCalculationFullyTransaction(
    InitiateMaxFlowCalculationFullyCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<MaxFlowCalculationFullyTransaction>(
                mNodeUUID,
                command,
                mTrustLines,
                mMaxFlowCalculationTrustLineManager,
                mMaxFlowCalculationCacheManager,
                mMaxFlowCalculationNodeCacheManager,
                mLog),
            true,
            true,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchReceiveMaxFlowCalculationOnTargetTransaction(
    InitiateMaxFlowCalculationMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<ReceiveMaxFlowCalculationOnTargetTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mMaxFlowCalculationCacheManager,
                mLog),
            false,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchReceiveResultMaxFlowCalculationTransaction(
    ResultMaxFlowCalculationMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<ReceiveResultMaxFlowCalculationTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mMaxFlowCalculationTrustLineManager,
                mLog),
            false,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchReceiveResultMaxFlowCalculationTransactionFromGateway(
    ResultMaxFlowCalculationGatewayMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<ReceiveResultMaxFlowCalculationTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mMaxFlowCalculationTrustLineManager,
                mLog),
            false,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchMaxFlowCalculationSourceFstLevelTransaction(
    MaxFlowCalculationSourceFstLevelMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<MaxFlowCalculationSourceFstLevelTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mLog,
                mIAmGateway),
            false,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchMaxFlowCalculationTargetFstLevelTransaction(
    MaxFlowCalculationTargetFstLevelMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<MaxFlowCalculationTargetFstLevelTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mLog,
                mIAmGateway),
            false,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchMaxFlowCalculationSourceSndLevelTransaction(
    MaxFlowCalculationSourceSndLevelMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<MaxFlowCalculationSourceSndLevelTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mMaxFlowCalculationCacheManager,
                mLog,
                mIAmGateway),
            false,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchMaxFlowCalculationTargetSndLevelTransaction(
    MaxFlowCalculationTargetSndLevelMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<MaxFlowCalculationTargetSndLevelTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mMaxFlowCalculationCacheManager,
                mLog,
                mIAmGateway),
            false,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchCoordinatorPaymentTransaction(
    CreditUsageCommand::Shared command)
{
    auto transaction = make_shared<CoordinatorPaymentTransaction>(
        mNodeUUID,
        command,
        mTrustLines,
        mStorageHandler,
        mMaxFlowCalculationCacheManager,
        mMaxFlowCalculationNodeCacheManager,
        mResourcesManager,
        mPathsManager,
        mLog,
        mSubsystemsController);
    subscribeForBuildCyclesThreeNodesTransaction(
        transaction->mBuildCycleThreeNodesSignal);
    prepareAndSchedule(transaction, true, false, true);
}

void TransactionsManager::launchReceiverPaymentTransaction(
    ReceiverInitPaymentRequestMessage::Shared message)
{
    auto transaction = make_shared<ReceiverPaymentTransaction>(
        mNodeUUID,
        message,
        mTrustLines,
        mStorageHandler,
        mMaxFlowCalculationCacheManager,
        mMaxFlowCalculationNodeCacheManager,
        mLog,
        mSubsystemsController);
    subscribeForBuildCyclesThreeNodesTransaction(
        transaction->mBuildCycleThreeNodesSignal);
    prepareAndSchedule(transaction, false, false, true);
}

void TransactionsManager::launchIntermediateNodePaymentTransaction(
    IntermediateNodeReservationRequestMessage::Shared message)
{
    auto transaction = make_shared<IntermediateNodePaymentTransaction>(
        mNodeUUID,
        message,
        mTrustLines,
        mStorageHandler,
        mMaxFlowCalculationCacheManager,
        mMaxFlowCalculationNodeCacheManager,
        mLog,
        mSubsystemsController);
    subscribeForBuildCyclesThreeNodesTransaction(
        transaction->mBuildCycleThreeNodesSignal);
    subscribeForBuildCyclesFourNodesTransaction(
        transaction->mBuildCycleFourNodesSignal);
    prepareAndSchedule(transaction, false, false, true);
}

void TransactionsManager::onCloseCycleTransaction(
    Path::ConstShared cycle)
{
    try {
        prepareAndSchedule(
            make_shared<CycleCloserInitiatorTransaction>(
                mNodeUUID,
                cycle,
                mTrustLines,
                mCyclesManager.get(),
                mStorageHandler,
                mMaxFlowCalculationCacheManager,
                mMaxFlowCalculationNodeCacheManager,
                mLog,
                mSubsystemsController),
            true,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchCycleCloserIntermediateNodeTransaction(
    IntermediateNodeCycleReservationRequestMessage::Shared message)
{
    try{
        prepareAndSchedule(
            make_shared<CycleCloserIntermediateNodeTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mCyclesManager.get(),
                mStorageHandler,
                mMaxFlowCalculationCacheManager,
                mMaxFlowCalculationNodeCacheManager,
                mLog,
                mSubsystemsController),
            false,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }

}

void TransactionsManager::launchVotesResponsePaymentsTransaction(
    VotesStatusRequestMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<VotesStatusResponsePaymentTransaction>(
                mNodeUUID,
                message,
                mStorageHandler,
                mScheduler->isTransactionInProcess(
                    message->transactionUUID()),
                mLog),
            false,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchTotalBalancesTransaction(
    TotalBalancesCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<TotalBalancesTransaction>(
                mNodeUUID,
                command,
                mTrustLines,
                mLog),
            true,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchHistoryPaymentsTransaction(
    HistoryPaymentsCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<HistoryPaymentsTransaction>(
                mNodeUUID,
                command,
                mStorageHandler,
                mLog),
            true,
            false,
            false);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchAdditionalHistoryPaymentsTransaction(
    HistoryAdditionalPaymentsCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<HistoryAdditionalPaymentsTransaction>(
                mNodeUUID,
                command,
                mStorageHandler,
                mLog),
            true,
            false,
            false);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}


/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchHistoryTrustLinesTransaction(
    HistoryTrustLinesCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<HistoryTrustLinesTransaction>(
                mNodeUUID,
                command,
                mStorageHandler,
                mLog),
            true,
            false,
            false);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

/*!
 *
 * Throws MemoryError.
 */
void TransactionsManager::launchHistoryWithContractorTransaction(
    HistoryWithContractorCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<HistoryWithContractorTransaction>(
                mNodeUUID,
                command,
                mStorageHandler,
                mLog),
            true,
            false,
            false);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchGetFirstLevelContractorsTransaction(
    GetFirstLevelContractorsCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<GetFirstLevelContractorsTransaction>(
                mNodeUUID,
                command,
                mTrustLines,
                mLog),
            true,
            false,
            false);
    } catch (ValueError &e){
        throw ValueError(e.message());
    }
}


void TransactionsManager::launchGetTrustlinesTransaction(
    GetTrustLinesCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<GetFirstLevelContractorsBalancesTransaction>(
                mNodeUUID,
                command,
                mTrustLines,
                mLog),
            true,
            false,
            false);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }

}

void TransactionsManager::launchGetTrustlineTransaction(
    GetTrustLineCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<GetFirstLevelContractorBalanceTransaction>(
                mNodeUUID,
                command,
                mTrustLines,
                mLog),
            true,
            false,
            false);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }

}

void TransactionsManager::launchFindPathByMaxFlowTransaction(
    const TransactionUUID &requestedTransactionUUID,
    const NodeUUID &destinationNodeUUID)
{
    try {
        prepareAndSchedule(
            make_shared<FindPathByMaxFlowTransaction>(
                mNodeUUID,
                destinationNodeUUID,
                requestedTransactionUUID,
                mPathsManager,
                mResourcesManager,
                mTrustLines,
                mMaxFlowCalculationTrustLineManager,
                mMaxFlowCalculationCacheManager,
                mMaxFlowCalculationNodeCacheManager,
                mLog),
            true,
            true,
            false);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchPaymentTransactionByCommandUUIDTransaction(
    PaymentTransactionByCommandUUIDCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<PaymentTransactionByCommandUUIDTransaction>(
                mNodeUUID,
                command,
                mScheduler->paymentTransactionByCommandUUID(
                    command->paymentTransactionCommandUUID()),
                mLog),
            false,
            false,
            false);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchGatewayNotificationSenderTransaction()
{
    try {
        prepareAndSchedule(
            make_shared<GatewayNotificationSenderTransaction>(
                mNodeUUID,
                mTrustLines,
                mStorageHandler,
                mIAmGateway,
                mLog),
            false,
            false,
            true);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchGatewayNotificationReceiverTransaction(
    GatewayNotificationMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<GatewayNotificationReceiverTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mStorageHandler,
                mLog),
            false,
            false,
            true);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::attachResourceToTransaction(
    BaseResource::Shared resource) {

    mScheduler->tryAttachResourceToTransaction(
        resource);
}

void TransactionsManager::subscribeForSubsidiaryTransactions(
    BaseTransaction::LaunchSubsidiaryTransactionSignal &signal)
{
    signal.connect(
        boost::bind(
            &TransactionsManager::onSubsidiaryTransactionReady,
            this,
            _1));
}

void TransactionsManager::subscribeForOutgoingMessages(
    BaseTransaction::SendMessageSignal &signal)
{
    // ToDo: connect signals of transaction and core directly (signal -> signal)
    // Boost allows this type of connectivity.
    signal.connect(
        boost::bind(
            &TransactionsManager::onTransactionOutgoingMessageReady,
            this,
            _1,
            _2));
}

void TransactionsManager::subscribeForSerializeTransaction(
    TransactionsScheduler::SerializeTransactionSignal &signal)
{
    signal.connect(
        boost::bind(
            &TransactionsManager::onSerializeTransaction,
            this,
            _1));
}

void TransactionsManager::subscribeForCommandResult(
    TransactionsScheduler::CommandResultSignal &signal)
{
    signal.connect(
        boost::bind(
            &TransactionsManager::onCommandResultReady,
            this,
            _1));
}

void TransactionsManager::subscribeForBuildCyclesThreeNodesTransaction(
    BasePaymentTransaction::BuildCycleThreeNodesSignal &signal)
{
    signal.connect(
        boost::bind(
            &TransactionsManager::onBuidCycleThreeNodesTransaction,
            this,
            _1));
}

void TransactionsManager::subscribeForBuildCyclesFourNodesTransaction(
    BasePaymentTransaction::BuildCycleFourNodesSignal &signal)
{
    signal.connect(
        boost::bind(
            &TransactionsManager::onBuildCycleFourNodesTransaction,
            this,
            _1));
}

void TransactionsManager::subscribeForBuildCyclesFiveNodesTransaction(
    CyclesManager::BuildFiveNodesCyclesSignal &signal)
{
    signal.connect(
        boost::bind(
                &TransactionsManager::onBuildCycleFiveNodesTransaction,
            this));
}

void TransactionsManager::subscribeForBuildCyclesSixNodesTransaction(
    CyclesManager::BuildSixNodesCyclesSignal &signal)
{
    signal.connect(
        boost::bind(
                &TransactionsManager::onBuildCycleSixNodesTransaction,
            this));
}

void TransactionsManager::subscribeForCloseCycleTransaction(
    CyclesManager::CloseCycleSignal &signal)
{
    signal.connect(
        boost::bind(
            &TransactionsManager::onCloseCycleTransaction,
            this,
            _1));
}

void TransactionsManager::subscribeForTryCloseNextCycleSignal(
    TransactionsScheduler::CycleCloserTransactionWasFinishedSignal &signal)
{
    signal.connect(
        boost::bind(
            &TransactionsManager::onTryCloseNextCycleSlot,
            this));
}

void TransactionsManager::subscribeForProcessingConfirmationMessage(
    BaseTransaction::ProcessConfirmationMessageSignal &signal)
{
    signal.connect(
        boost::bind(
            &TransactionsManager::onProcessConfirmationMessageSlot,
            this,
            _1,
            _2));
}

void TransactionsManager::onTransactionOutgoingMessageReady(
    Message::Shared message,
    const NodeUUID &contractorUUID)
{
    transactionOutgoingMessageReadySignal(
        message,
        contractorUUID);
}

/*!
 * Writes received result to the outgoing results fifo.
 *
 * Throws RuntimeError - in case if result can't be processed.
 */
void TransactionsManager::onCommandResultReady(
    CommandResult::SharedConst result)
{
    try {
        auto message = result->serialize();

        info() << "Result for command " + result->identifier();

        if (result->identifier() == HistoryPaymentsCommand::identifier() or
                result->identifier() == HistoryTrustLinesCommand::identifier() or
                result->identifier() == HistoryWithContractorCommand::identifier() or
                result->identifier() == GetTrustLinesCommand::identifier()) {
            auto shortMessage = result->serializeShort();
            info() << "CommandResultReady: " << shortMessage;
        } else {
            info() << "CommandResultReady: " << message;
        }

        mResultsInterface->writeResult(
            message.c_str(),
            message.size());

    } catch (...) {
        throw RuntimeError(
            "TransactionsManager::onCommandResultReady: "
                "Error occurred when command result has accepted");
    }
}

void TransactionsManager::onSubsidiaryTransactionReady(
    BaseTransaction::Shared transaction)
{
    subscribeForSubsidiaryTransactions(
        transaction->runSubsidiaryTransactionSignal);

    subscribeForOutgoingMessages(
        transaction->outgoingMessageIsReadySignal);

    mScheduler->postponeTransaction(
        transaction,
        50);
}

void TransactionsManager::onBuidCycleThreeNodesTransaction(
    set<NodeUUID> &contractorsUUID)
{
    for (const auto &contractorUUID : contractorsUUID) {
        launchThreeNodesCyclesInitTransaction(
            contractorUUID);
    }
}

void TransactionsManager::onBuildCycleFourNodesTransaction(
    set<NodeUUID> &creditors)
{
    for (const auto &kCreditor : creditors) {
        launchFourNodesCyclesInitTransaction(
            kCreditor);
    }
}

void TransactionsManager::onBuildCycleFiveNodesTransaction()
{
    launchFiveNodesCyclesInitTransaction();
}

void TransactionsManager::onBuildCycleSixNodesTransaction()
{
    launchSixNodesCyclesInitTransaction();
}

void TransactionsManager::onTryCloseNextCycleSlot()
{
    mCyclesManager->closeOneCycle(true);
}

void TransactionsManager::onProcessConfirmationMessageSlot(
    const NodeUUID &contractorUUID,
    ConfirmationMessage::Shared confirmationMessage)
{
    ProcessConfirmationMessageSignal(
        contractorUUID,
        confirmationMessage);
}

/**
 *
 * @throws bad_alloc;
 */
void TransactionsManager::prepareAndSchedule(
    BaseTransaction::Shared transaction,
    bool regenerateUUID,
    bool subsidiaryTransactionSubscribe,
    bool outgoingMessagesSubscribe)
{
    if (outgoingMessagesSubscribe)
        subscribeForOutgoingMessages(
            transaction->outgoingMessageIsReadySignal);
    if (subsidiaryTransactionSubscribe)
        subscribeForSubsidiaryTransactions(
            transaction->runSubsidiaryTransactionSignal);

    while (true) {
        try {

            mScheduler->scheduleTransaction(
                transaction);
            return;

        } catch(bad_alloc &) {
            throw bad_alloc();

        } catch(ConflictError &e) {
            warning() << "prepareAndSchedule:" << "TransactionUUID: " << transaction->currentTransactionUUID()
                                              << " New TransactionType:" << transaction->transactionType()
                                              << " Recreate.";
            if (regenerateUUID) {
                transaction->recreateTransactionUUID();
            } else {
                warning() << "prepareAndSchedule:" << "TransactionUUID: " << transaction->currentTransactionUUID()
                                                  << " New TransactionType:" << transaction->transactionType()
                                                  << "Exit.";
                return;
            }
        }
    }
}

void TransactionsManager::launchThreeNodesCyclesInitTransaction(
    const NodeUUID &contractorUUID)
{
    try {
        prepareAndSchedule(
            make_shared<CyclesThreeNodesInitTransaction>(
                mNodeUUID,
                contractorUUID,
                mTrustLines,
                mRoutingTable,
                mCyclesManager.get(),
                mStorageHandler,
                mLog),
            true,
            false,
            true);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }

}

void TransactionsManager::launchThreeNodesCyclesResponseTransaction(
    CyclesThreeNodesBalancesRequestMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<CyclesThreeNodesReceiverTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mLog),
            false,
            false,
            true);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchSixNodesCyclesInitTransaction()
{
    try {
        prepareAndSchedule(
            make_shared<CyclesSixNodesInitTransaction>(
                mNodeUUID,
                mTrustLines,
                mCyclesManager.get(),
                mStorageHandler,
                mLog),
            true,
            false,
            true);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchSixNodesCyclesResponseTransaction(
    CyclesSixNodesInBetweenMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<CyclesSixNodesReceiverTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mLog),
            false,
            false,
            true
        );
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchFiveNodesCyclesInitTransaction()
{
    try {
        prepareAndSchedule(
            make_shared<CyclesFiveNodesInitTransaction>(
                mNodeUUID,
                mTrustLines,
                mCyclesManager.get(),
                mStorageHandler,
                mLog),
            true,
            false,
            true);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchFiveNodesCyclesResponseTransaction(
    CyclesFiveNodesInBetweenMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<CyclesFiveNodesReceiverTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mLog),
            false,
            false,
            true
        );
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchFourNodesCyclesInitTransaction(
    const NodeUUID &creditorUUID)
{
    try {
        prepareAndSchedule(
            make_shared<CyclesFourNodesInitTransaction>(
                mNodeUUID,
                creditorUUID,
                mTrustLines,
                mRoutingTable,
                mCyclesManager.get(),
                mStorageHandler,
                mLog),
            true,
            false,
            true);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchFourNodesCyclesResponseTransaction(
    CyclesFourNodesBalancesRequestMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<CyclesFourNodesReceiverTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mLog),
            false,
            false,
            true);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::onSerializeTransaction(
    BaseTransaction::Shared transaction)
{
    const auto kTransactionTypeId = transaction->transactionType();
    const auto ioTransaction = mStorageHandler->beginTransaction();
    switch (kTransactionTypeId) {
        case BaseTransaction::TransactionType::CoordinatorPaymentTransaction: {
            const auto kChildTransaction = static_pointer_cast<CoordinatorPaymentTransaction>(transaction);
            const auto transactionBytesAndCount = kChildTransaction->serializeToBytes();
            ioTransaction->transactionHandler()->saveRecord(
                kChildTransaction->currentTransactionUUID(),
                transactionBytesAndCount.first,
                transactionBytesAndCount.second);
            break;
        }
        case BaseTransaction::TransactionType::IntermediateNodePaymentTransaction: {
            const auto kChildTransaction = static_pointer_cast<IntermediateNodePaymentTransaction>(transaction);
            const auto transactionBytesAndCount = kChildTransaction->serializeToBytes();
            ioTransaction->transactionHandler()->saveRecord(
                kChildTransaction->currentTransactionUUID(),
                transactionBytesAndCount.first,
                transactionBytesAndCount.second);
            break;
        }
        case BaseTransaction::TransactionType::ReceiverPaymentTransaction: {
            const auto kChildTransaction = static_pointer_cast<ReceiverPaymentTransaction>(transaction);
            const auto transactionBytesAndCount = kChildTransaction->serializeToBytes();
            ioTransaction->transactionHandler()->saveRecord(
                kChildTransaction->currentTransactionUUID(),
                transactionBytesAndCount.first,
                transactionBytesAndCount.second);
            break;
        }
        case BaseTransaction::TransactionType::Payments_CycleCloserInitiatorTransaction: {
            const auto kChildTransaction = static_pointer_cast<CycleCloserInitiatorTransaction>(transaction);
            const auto transactionBytesAndCount = kChildTransaction->serializeToBytes();
            ioTransaction->transactionHandler()->saveRecord(
                kChildTransaction->currentTransactionUUID(),
                transactionBytesAndCount.first,
                transactionBytesAndCount.second);
            break;
        }
        case BaseTransaction::TransactionType::Payments_CycleCloserIntermediateNodeTransaction: {
            const auto kChildTransaction = static_pointer_cast<CycleCloserIntermediateNodeTransaction>(transaction);
            const auto transactionBytesAndCount = kChildTransaction->serializeToBytes();
            ioTransaction->transactionHandler()->saveRecord(
                kChildTransaction->currentTransactionUUID(),
                transactionBytesAndCount.first,
                transactionBytesAndCount.second);
            break;
        }
        default: {
            throw RuntimeError(
                "TrustLinesManager::onSerializeTransaction. "
                    "Unexpected transaction type identifier.");
        }
    }
}

void TransactionsManager::launchRoutingTableResponseTransaction(
    RoutingTableRequestMessage::Shared message)
{
    try {
        prepareAndSchedule(
            make_shared<RoutingTableResponseTransaction>(
                mNodeUUID,
                message,
                mTrustLines,
                mLog),
            false,
            false,
            true);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchRoutingTableRequestTransaction()
{
    try {
        prepareAndSchedule(
            make_shared<RoutingTableInitTransaction>(
                mNodeUUID,
                mTrustLines,
                mRoutingTable,
                mLog),
            false,
            false,
            true);
    } catch (ConflictError &e){
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchAddNodeToBlackListTransaction(
    AddNodeToBlackListCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<AddNodeToBlackListTransaction>(
                mNodeUUID,
                command,
                mStorageHandler,
                mTrustLines,
                mSubsystemsController,
                mLog),
            true,
            false,
            true);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }

}

void TransactionsManager::launchCheckIfNodeInBlackListTransaction(
    CheckIfNodeInBlackListCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<CheckIfNodeInBlackListTransaction>(
                mNodeUUID,
                command,
                mStorageHandler,
                mLog),
            true,
            false,
            false);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchRemoveNodeFromBlackListTransaction(
    RemoveNodeFromBlackListCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<RemoveNodeFromBlackListTransaction>(
                mNodeUUID,
                command,
                mStorageHandler,
                mLog),
            true,
            false,
            false);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

void TransactionsManager::launchGetBlackListTransaction(
    GetBlackListCommand::Shared command)
{
    try {
        prepareAndSchedule(
            make_shared<GetBlackListTransaction>(
                mNodeUUID,
                command,
                mStorageHandler,
                mLog),
            true,
            false,
            false);
    } catch (ConflictError &e) {
        throw ConflictError(e.message());
    }
}

#ifdef TESTS
void TransactionsManager::setMeAsGateway()
{
    mIAmGateway = true;
}
#endif

string TransactionsManager::logHeader()
    noexcept
{
    return "[TransactionsManager]";
}

LoggerStream TransactionsManager::error() const
    noexcept
{
    return mLog.error(logHeader());
}

LoggerStream TransactionsManager::warning() const
    noexcept
{
    return mLog.warning(logHeader());
}

LoggerStream TransactionsManager::info() const
    noexcept
{
    return mLog.info(logHeader());
}