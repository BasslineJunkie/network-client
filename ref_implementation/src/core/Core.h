﻿/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_CORE_H
#define GEO_NETWORK_CLIENT_CORE_H

#include "common/Types.h"
#include "common/NodeUUID.h"

#include "settings/Settings.h"
#include "network/communicator/Communicator.h"
#include "interface/commands_interface/interface/CommandsInterface.h"
#include "interface/results_interface/interface/ResultsInterface.h"
#include "trust_lines/manager/TrustLinesManager.h"
#include "resources/manager/ResourcesManager.h"
#include "transactions/manager/TransactionsManager.h"
#include "max_flow_calculation/manager/MaxFlowCalculationTrustLineManager.h"
#include "max_flow_calculation/cashe/MaxFlowCalculationCacheManager.h"
#include "max_flow_calculation/cashe/MaxFlowCalculationNodeCacheManager.h"
#include "delayed_tasks/MaxFlowCalculationCacheUpdateDelayedTask.h"
#include "delayed_tasks/NotifyThatIAmIsGatewayDelayedTask.h"
#include "io/storage/StorageHandler.h"
#include "paths/PathsManager.h"

#include "logger/Logger.h"

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/signals2.hpp>




#include "network/messages/debug/DebugMessage.h"
#include "subsystems_controller/SubsystemsController.h"


#include <sys/prctl.h>


using namespace std;

namespace as = boost::asio;
namespace signals = boost::signals2;

class Core {

public:
    Core(
        char* pArgv)
        noexcept;

    ~Core();

    int run();

private:
    int initSubsystems();

    int initSettings();

    int initLogger();

    int initCommunicator(
        const json &conf);

    int initCommandsInterface();

    int initResultsInterface();

    int initTrustLinesManager();

    int initMaxFlowCalculationTrustLineManager();

    int initMaxFlowCalculationCacheManager();

    int initMaxFlowCalculationNodeCacheManager();

    int initResourcesManager();

    int initTransactionsManager();

    int initDelayedTasks();

    int initStorageHandler();

    int initPathsManager();

    int initSubsystemsController();

    int initRoughtingTable();

    void connectCommunicatorSignals();

    void connectCommandsInterfaceSignals();

    void connectDelayedTasksSignals();

    void connectResourcesManagerSignals();

    void connectSignalsToSlots();

    void connectRoutingTableSignals();

    void onCommandReceivedSlot(
        BaseUserCommand::Shared command);

    void onMessageReceivedSlot(
        Message::Shared message);

    void onMessageSendSlot(
        Message::Shared message,
        const NodeUUID &contractorUUID);

    void onUpdateRoutingTableSlot();

    void onProcessConfirmationMessageSlot(
        const NodeUUID &contractorUUID,
        ConfirmationMessage::Shared confirmationMessage);

    void onPathsResourceRequestedSlot(
        const TransactionUUID &transactionUUID,
        const NodeUUID &destinationNodeUUID);

    void onResourceCollectedSlot(
        BaseResource::Shared resource);

    void onGatewayNotificationSlot();

    void writePIDFile();

    void updateProcessName();

    /**
     * Sends notification about current outgoing trust line amount to each contractor.
     * This is needed to prevent trust lines borders desyncronization, that might occure in 2 cases:
     *
     * 1. Outgoing trust line amount was changed outside of the engine.
     * 2. Outgoing trust line amount was changed on this node, but the remote node does't received the message.
     *    (this case is valid for the nodes, that was present in the network before the moment,
     *     when forced messages delivering mechanism was added into the engine).
     */
    void notifyContractorsAboutCurrentTrustLinesAmounts();

protected:
    static string logHeader()
    noexcept;

    LoggerStream warning() const
    noexcept;

    LoggerStream error() const
    noexcept;

    LoggerStream info() const
    noexcept;

    LoggerStream debug() const
    noexcept;

protected:
    // This pointer is used to modify executable command description.
    // By default, it would point to the standard argv[0] char sequence;
    char* mCommandDescriptionPtr;

    NodeUUID mNodeUUID;
    as::io_service mIOService;
    bool mIAmGateway;

    unique_ptr<Logger> mLog;
    unique_ptr<Settings> mSettings;
    unique_ptr<Communicator> mCommunicator;
    unique_ptr<CommandsInterface> mCommandsInterface;
    unique_ptr<ResultsInterface> mResultsInterface;
    unique_ptr<TrustLinesManager> mTrustLinesManager;
    unique_ptr<ResourcesManager> mResourcesManager;
    unique_ptr<TransactionsManager> mTransactionsManager;
    unique_ptr<MaxFlowCalculationTrustLineManager> mMaxFlowCalculationTrustLimeManager;
    unique_ptr<MaxFlowCalculationCacheManager> mMaxFlowCalculationCacheManager;
    unique_ptr<MaxFlowCalculationNodeCacheManager> mMaxFlowCalculationNodeCacheManager;
    unique_ptr<MaxFlowCalculationCacheUpdateDelayedTask> mMaxFlowCalculationCacheUpdateDelayedTask;
    unique_ptr<NotifyThatIAmIsGatewayDelayedTask> mNotifyThatIAmIsGatewayDelayedTask;
    unique_ptr<StorageHandler> mStorageHandler;
    unique_ptr<PathsManager> mPathsManager;
    unique_ptr<SubsystemsController> mSubsystemsController;
    unique_ptr<RoutingTableManager> mRoutingTable;
};

#endif //GEO_NETWORK_CLIENT_CORE_H
