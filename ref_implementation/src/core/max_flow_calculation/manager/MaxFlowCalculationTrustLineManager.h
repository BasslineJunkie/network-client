/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_MAXFLOWCALCULATIONTRUSTLINEMANAGER_H
#define GEO_NETWORK_CLIENT_MAXFLOWCALCULATIONTRUSTLINEMANAGER_H

#include "../../common/NodeUUID.h"
#include "MaxFlowCalculationTrustLineWithPtr.h"
#include "../../common/time/TimeUtils.h"
#include "../../logger/Logger.h"

#include <set>
#include <unordered_set>
#include <unordered_map>
#include <boost/functional/hash.hpp>

class MaxFlowCalculationTrustLineManager {

public:
    typedef unordered_set<MaxFlowCalculationTrustLineWithPtr*> TrustLineWithPtrHashSet;

public:
    MaxFlowCalculationTrustLineManager(
        bool iAmGateway,
        NodeUUID &nodeUUID,
        Logger &logger);

    void addTrustLine(
        MaxFlowCalculationTrustLine::Shared trustLine);

    TrustLineWithPtrHashSet trustLinePtrsSet(
        const NodeUUID &nodeUUID);

    void resetAllUsedAmounts();

    bool deleteLegacyTrustLines();

    size_t trustLinesCounts() const;

    // todo : this code used only for testing and should be deleted in future
    void printTrustLines() const;

    DateTime closestTimeEvent() const;

    void setPreventDeleting(
        bool preventDeleting);

    bool preventDeleting() const;

    void addUsedAmount(
        const NodeUUID &sourceUUID,
        const NodeUUID &targetUUID,
        const TrustLineAmount &amount);

    void makeFullyUsed(
        const NodeUUID &sourceUUID,
        const NodeUUID &targetUUID);

    set<NodeUUID> neighborsOf(
        const NodeUUID &sourceUUID);

    void addGateway(const NodeUUID &gateway);

    const set<NodeUUID> gateways() const;

    void makeFullyUsedTLsFromGatewaysToAllNodesExceptOne(
        const NodeUUID &exceptedNode);

private:
    static const byte kResetTrustLinesHours = 0;
    static const byte kResetTrustLinesMinutes = 12;
    static const byte kResetTrustLinesSeconds = 0;

    static Duration& kResetTrustLinesDuration() {
        static auto duration = Duration(
            kResetTrustLinesHours,
            kResetTrustLinesMinutes,
            kResetTrustLinesSeconds);
        return duration;
    }

private:
    LoggerStream info() const;
    LoggerStream debug() const;

    const string logHeader() const;

private:
    unordered_map<NodeUUID, TrustLineWithPtrHashSet*, boost::hash<boost::uuids::uuid>> msTrustLines;
    map<DateTime, MaxFlowCalculationTrustLineWithPtr*> mtTrustLines;
    Logger &mLog;
    bool mPreventDeleting;
    set<NodeUUID> mGateways;
};

#endif //GEO_NETWORK_CLIENT_MAXFLOWCALCULATIONTRUSTLINEMANAGER_H
