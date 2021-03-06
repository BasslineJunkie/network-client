/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#include "MaxFlowCalculationCache.h"

MaxFlowCalculationCache::MaxFlowCalculationCache(
    const vector<pair<NodeUUID, ConstSharedTrustLineAmount>> &outgoingFlows,
    const vector<pair<NodeUUID, ConstSharedTrustLineAmount>> &incomingFlows)
{
    for (auto &nodeUUIDAndFlow : outgoingFlows) {
        mOutgoingFlows.insert(nodeUUIDAndFlow);
    }
    for (auto &nodeUUIDAndFlow : incomingFlows) {
        mIncomingFlows.insert(nodeUUIDAndFlow);
    }
}

// check if incoming flow already cached
bool MaxFlowCalculationCache::containsIncomingFlow(
    const NodeUUID &nodeUUID,
    ConstSharedTrustLineAmount flow)
{
    auto nodeUUIDAndFlow = mIncomingFlows.find(nodeUUID);
    // if not present then insert
    if (nodeUUIDAndFlow == mIncomingFlows.end()) {
        if (*flow == TrustLine::kZeroAmount()) {
            return true;
        } else {
            mIncomingFlows.insert(
                make_pair(
                    nodeUUID,
                    flow));
            return false;
        }
    } else {
        auto sharedFlow = (*nodeUUIDAndFlow).second;
        // if flow present but now it is zero, then delete
        if (*flow.get() == TrustLine::kZeroAmount()) {
            mIncomingFlows.erase(nodeUUIDAndFlow);
            return false;
        }
        // if flow differs then update
        if (*sharedFlow.get() != *flow.get()) {
            mIncomingFlows.erase(nodeUUIDAndFlow);
            mIncomingFlows.insert(
                make_pair(
                    nodeUUID,
                    flow));
            return false;
        }
    }
    return true;
}

// check if inoutgoing flow already cached
bool MaxFlowCalculationCache::containsOutgoingFlow(
    const NodeUUID &nodeUUID,
    const ConstSharedTrustLineAmount flow)
{
    auto nodeUUIDAndFlow = mOutgoingFlows.find(nodeUUID);
    // if not present then insert
    if (nodeUUIDAndFlow == mOutgoingFlows.end()) {
        if (*flow == TrustLine::kZeroAmount()) {
            return true;
        } else {
            mOutgoingFlows.insert(
                make_pair(
                    nodeUUID,
                    flow));
            return false;
        }
    } else {
        auto sharedFlow = (*nodeUUIDAndFlow).second;
        // if flow present but now it is zero, then delete
        if (*flow.get() == TrustLine::kZeroAmount()) {
            mOutgoingFlows.erase(nodeUUIDAndFlow);
            return false;
        }
        // if flow differs then update
        if (*sharedFlow.get() != *flow.get()) {
            mOutgoingFlows.erase(nodeUUIDAndFlow);
            mOutgoingFlows.insert(
                make_pair(
                    nodeUUID,
                    flow));
            return false;
        }
    }
    return true;
}
