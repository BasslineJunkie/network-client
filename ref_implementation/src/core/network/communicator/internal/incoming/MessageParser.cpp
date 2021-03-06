/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#include "MessageParser.h"

MessagesParser::MessagesParser(
    Logger *logger)
    noexcept:

    mLog(logger)
{}

pair<bool, Message::Shared> MessagesParser::processBytesSequence(
    BytesShared buffer,
    const size_t count) {

    if (count < kMinimalMessageSize || buffer == nullptr) {
        return messageInvalidOrIncomplete();
    }

    try {
        const Message::SerializedType kMessageIdentifier =
            *(reinterpret_cast<Message::SerializedType*>(buffer.get()));

        switch(kMessageIdentifier) {

        /*
         * System messages
         */
        case Message::System_Confirmation:
            return messageCollected<ConfirmationMessage>(buffer);


        /*
         * Trust lines messages
         */
        case Message::TrustLines_SetIncoming:
            return messageCollected<SetIncomingTrustLineMessage>(buffer);

        case Message::TrustLines_SetIncomingFromGateway:
            return messageCollected<SetIncomingTrustLineFromGatewayMessage>(buffer);

        case Message::TrustLines_CloseOutgoing:
            return messageCollected<CloseOutgoingTrustLineMessage>(buffer);


        /*
         * Payment operations messages
         */
        case Message::Payments_CoordinatorReservationRequest:
            return messageCollected<CoordinatorReservationRequestMessage>(buffer);

        case Message::Payments_CoordinatorReservationResponse:
            return messageCollected<CoordinatorReservationResponseMessage>(buffer);

        case Message::Payments_ReceiverInitPaymentRequest:
            return messageCollected<ReceiverInitPaymentRequestMessage>(buffer);

        case Message::Payments_ReceiverInitPaymentResponse:
            return messageCollected<ReceiverInitPaymentResponseMessage>(buffer);

        case Message::Payments_IntermediateNodeReservationRequest:
            return messageCollected<IntermediateNodeReservationRequestMessage>(buffer);

        case Message::Payments_IntermediateNodeReservationResponse:
            return messageCollected<IntermediateNodeReservationResponseMessage>(buffer);

        case Message::Payments_CoordinatorCycleReservationRequest:
            return messageCollected<CoordinatorCycleReservationRequestMessage>(buffer);

        case Message::Payments_CoordinatorCycleReservationResponse:
            return messageCollected<CoordinatorCycleReservationResponseMessage>(buffer);

        case Message::Payments_IntermediateNodeCycleReservationRequest:
            return messageCollected<IntermediateNodeCycleReservationRequestMessage>(buffer);

        case Message::Payments_IntermediateNodeCycleReservationResponse:
            return messageCollected<IntermediateNodeCycleReservationResponseMessage>(buffer);

        case Message::Payments_ParticipantsVotes:
            return messageCollected<ParticipantsVotesMessage>(buffer);

        case Message::Payments_FinalPathConfiguration:
            return messageCollected<FinalPathConfigurationMessage>(buffer);

        case Message::Payments_FinalPathCycleConfiguration:
            return messageCollected<FinalPathCycleConfigurationMessage>(buffer);

        case Message::Payments_FinalAmountsConfiguration:
            return messageCollected<FinalAmountsConfigurationMessage>(buffer);

        case Message::Payments_FinalAmountsConfigurationResponse:
            return messageCollected<FinalAmountsConfigurationResponseMessage>(buffer);

        case Message::Payments_TTLProlongationRequest:
            return messageCollected<TTLProlongationRequestMessage>(buffer);

        case Message::Payments_TTLProlongationResponse:
            return messageCollected<TTLProlongationResponseMessage>(buffer);

        case Message::Payments_VotesStatusRequest:
            return messageCollected<VotesStatusRequestMessage>(buffer);

        case Message::Payments_ReservationsInRelationToNode:
            return messageCollected<ReservationsInRelationToNodeMessage>(buffer);


        /*
         * Cycles processing messages
         */
        case Message::Cycles_SixNodesMiddleware:
            return messageCollected<CyclesSixNodesInBetweenMessage>(buffer);

        case Message::Cycles_FiveNodesMiddleware:
            return messageCollected<CyclesFiveNodesInBetweenMessage>(buffer);

        case Message::Cycles_SixNodesBoundary:
            return messageCollected<CyclesSixNodesBoundaryMessage>(buffer);

        case Message::Cycles_FiveNodesBoundary:
            return messageCollected<CyclesFiveNodesBoundaryMessage>(buffer);

        case Message::Cycles_ThreeNodesBalancesResponse:
            return messageCollected<CyclesThreeNodesBalancesResponseMessage>(buffer);

        case Message::Cycles_FourNodesBalancesRequest:
            return messageCollected<CyclesFourNodesBalancesRequestMessage>(buffer);

        case Message::Cycles_FourNodesBalancesResponse:
            return messageCollected<CyclesFourNodesBalancesResponseMessage>(buffer);

        case Message::Cycles_ThreeNodesBalancesRequest:
            return messageCollected<CyclesThreeNodesBalancesRequestMessage>(buffer);


        /*
         * Max flow calculation messages
         */
        case Message::MaxFlow_InitiateCalculation:
            return messageCollected<InitiateMaxFlowCalculationMessage>(buffer);

        case Message::MaxFlow_ResultMaxFlowCalculation:
            return messageCollected<ResultMaxFlowCalculationMessage>(buffer);

        case Message::MaxFlow_ResultMaxFlowCalculationFromGateway:
            return messageCollected<ResultMaxFlowCalculationGatewayMessage>(buffer);

        case Message::MaxFlow_CalculationSourceFirstLevel:
            return messageCollected<MaxFlowCalculationSourceFstLevelMessage>(buffer);

        case Message::MaxFlow_CalculationTargetFirstLevel:
            return messageCollected<MaxFlowCalculationTargetFstLevelMessage>(buffer);

        case Message::MaxFlow_CalculationSourceSecondLevel:
            return messageCollected<MaxFlowCalculationSourceSndLevelMessage>(buffer);

        case Message::MaxFlow_CalculationTargetSecondLevel:
            return messageCollected<MaxFlowCalculationTargetSndLevelMessage>(buffer);

        /*
         * RoutingTables
         */
        case Message::RoutingTableRequest:
            return messageCollected<RoutingTableRequestMessage>(buffer);
        case Message::RoutingTableResponse:
            return messageCollected<RoutingTableResponseMessage>(buffer);

        /*
         * Gateway notification Messages
         */
        case Message::GatewayNotification:
            return messageCollected<GatewayNotificationMessage>(buffer);

#ifdef DEBUG
        /*
         * Debug messages
         */
        case Message::Debug:
            return messageCollected<DebugMessage>(buffer);
#endif

        default: {
            warning() << "processBytesSequence: "
                << "Unexpected message identifier occurred (" << kMessageIdentifier << "). Message dropped.";
            return messageInvalidOrIncomplete();
        }

        }

    } catch (exception &) {
        return messageInvalidOrIncomplete();
    }
}

MessagesParser &MessagesParser::operator=(
    const MessagesParser &other)
    noexcept
{
    mLog = other.mLog;
}

pair<bool, Message::Shared> MessagesParser::messageInvalidOrIncomplete()
{
    return make_pair(
        false,
        Message::Shared(nullptr));
}

template <class CollectedMessageType>
pair<bool, Message::Shared> MessagesParser::messageCollected(
    CollectedMessageType message) const
{
    return make_pair(
        true,
        static_pointer_cast<Message>(
            make_shared<CollectedMessageType>(message)));
}

string MessagesParser::logHeader()
    noexcept
{
    return "[MessagesParser]";
}

LoggerStream MessagesParser::warning() const
    noexcept
{
    return mLog->warning(logHeader());
}
