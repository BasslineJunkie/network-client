/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_INITIATEMAXFLOWCALCULATIONTRANSACTION_H
#define GEO_NETWORK_CLIENT_INITIATEMAXFLOWCALCULATIONTRANSACTION_H

#include "../base/BaseCollectTopologyTransaction.h"
#include "../../../interface/commands_interface/commands/max_flow_calculation/InitiateMaxFlowCalculationCommand.h"

#include "../../../trust_lines/manager/TrustLinesManager.h"
#include "../../../max_flow_calculation/manager/MaxFlowCalculationTrustLineManager.h"
#include "../../../max_flow_calculation/cashe/MaxFlowCalculationCacheManager.h"
#include "../../../max_flow_calculation/cashe/MaxFlowCalculationNodeCacheManager.h"
#include "../../../max_flow_calculation/cashe/MaxFlowCalculationNodeCache.h"

#include "../../../network/messages/max_flow_calculation/InitiateMaxFlowCalculationMessage.h"
#include "../../../network/messages/max_flow_calculation/MaxFlowCalculationSourceFstLevelMessage.h"
#include "../../../network/messages/max_flow_calculation/ResultMaxFlowCalculationMessage.h"

#include "CollectTopologyTransaction.h"
#include "MaxFlowCalculationStepTwoTransaction.h"

#include <set>

class InitiateMaxFlowCalculationTransaction : public BaseCollectTopologyTransaction {

public:
    typedef shared_ptr<InitiateMaxFlowCalculationTransaction> Shared;

public:
    InitiateMaxFlowCalculationTransaction(
        NodeUUID &nodeUUID,
        InitiateMaxFlowCalculationCommand::Shared command,
        TrustLinesManager *trustLinesManager,
        MaxFlowCalculationTrustLineManager *maxFlowCalculationTrustLineManager,
        MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
        MaxFlowCalculationNodeCacheManager *maxFlowCalculationNodeCacheManager,
        bool iAmGateway,
        Logger &logger);

    InitiateMaxFlowCalculationCommand::Shared command() const;

protected:
    const string logHeader() const;

private:
    TransactionResult::SharedConst sendRequestForCollectingTopology();

    TransactionResult::SharedConst processCollectingTopology();

    TrustLineAmount calculateMaxFlow(
        const NodeUUID &contractorUUID);

    TrustLineAmount calculateMaxFlowUpdated(
        const NodeUUID &contractorUUID);

    void calculateMaxFlowOnOneLevel();

    void calculateMaxFlowOnOneLevelUpdated();

    TrustLineAmount calculateOneNode(
        const NodeUUID& nodeUUID,
        const TrustLineAmount& currentFlow,
        byte level);

    TransactionResult::SharedConst resultOk(
        bool finalMaxFlows,
        vector<pair<NodeUUID, TrustLineAmount>> &maxFlows);

    TransactionResult::SharedConst resultProtocolError();

private:
    static const byte kMaxPathLength = 5;
    static const uint32_t kWaitMillisecondsForCalculatingMaxFlow = 2000;

private:
    InitiateMaxFlowCalculationCommand::Shared mCommand;
    vector<NodeUUID> mForbiddenNodeUUIDs;
    byte mCurrentPathLength;
    TrustLineAmount mCurrentMaxFlow;
    NodeUUID mCurrentContractor;
    size_t mCountProcessCollectingTopologyRun;
    bool mIAmGateway;
    MaxFlowCalculationTrustLineManager::TrustLineWithPtrHashSet mFirstLevelTopology;
    vector<NodeUUID> mAlreadyCalculated;
};


#endif //GEO_NETWORK_CLIENT_INITIATEMAXFLOWCALCULATIONTRANSACTION_H
