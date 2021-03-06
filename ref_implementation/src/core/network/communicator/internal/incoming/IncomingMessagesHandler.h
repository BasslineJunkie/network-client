﻿/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_INCOMINGCONNECTIONSHANDLER_H
#define GEO_NETWORK_CLIENT_INCOMINGCONNECTIONSHANDLER_H

#include "../common/Types.h"
#include "../../internal/incoming/IncomingNodesHandler.h"
#include "../../../../common/exceptions/ValueError.h"
#include "../../../../common/exceptions/ConflictError.h"

#include <boost/asio/steady_timer.hpp>


using namespace std;


class IncomingMessagesHandler {
public:
    signals::signal<void(Message::Shared)> signalMessageParsed;

public:
    IncomingMessagesHandler(
        IOService &ioService,
        UDPSocket &socket,
        Logger &logger)
        noexcept;

    void beginReceivingData()
        noexcept;

protected:
    void handleReceivedInfo(
        const boost::system::error_code &error,
        size_t bytesTransferred)
        noexcept;

    void rescheduleCleaning()
        noexcept;

    static string logHeader()
        noexcept;

    LoggerStream info() const
        noexcept;

    LoggerStream error() const
        noexcept;

    LoggerStream warning() const
        noexcept;

    LoggerStream debug() const
        noexcept;

protected:
    static constexpr const size_t kMaxIncomingBufferSize = Packet::kMaxSize * 2;

protected:
    UDPSocket &mSocket;
    IOService &mIOService;
    Logger &mLog;

    boost::array<byte, kMaxIncomingBufferSize> mIncomingBuffer;
    UDPEndpoint mRemoteEndpointBuffer;

    MessagesParser mMessagesParser;
    IncomingNodesHandler mRemoteNodesHandler;

    boost::asio::deadline_timer mCleaningTimer;
};

#endif //GEO_NETWORK_CLIENT_INCOMINGCONNECTIONSHANDLER_H
