/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_CYCLESSIXNODESINITTRANSACTION_H
#define GEO_NETWORK_CLIENT_CYCLESSIXNODESINITTRANSACTION_H

#include "base/CyclesBaseFiveSixNodesInitTransaction.h"
#include "../../../../network/messages/cycles/SixAndFiveNodes/CyclesSixNodesInBetweenMessage.hpp"
#include "../../../../network/messages/cycles/SixAndFiveNodes/CyclesSixNodesBoundaryMessage.hpp"
#include "../../../../paths/lib/Path.h"

class CyclesSixNodesInitTransaction :
        public CyclesBaseFiveSixNodesInitTransaction{

public:
    CyclesSixNodesInitTransaction(
        const NodeUUID &nodeUUID,
        TrustLinesManager *manager,
        CyclesManager *cyclesManager,
        StorageHandler *storageHandler,
        Logger &logger);

    const BaseTransaction::TransactionType transactionType() const;

protected:
    const string logHeader() const;

protected:
    TransactionResult::SharedConst runCollectDataAndSendMessagesStage();
    TransactionResult::SharedConst runParseMessageAndCreateCyclesStage();

};
#endif //GEO_NETWORK_CLIENT_CYCLESSIXNODESINITTRANSACTION_H
