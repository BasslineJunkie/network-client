/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#include "HistoryPaymentsCommand.h"

HistoryPaymentsCommand::HistoryPaymentsCommand(
    const CommandUUID &uuid,
    const string &commandBuffer):

    BaseUserCommand(
        uuid,
        identifier())
{
    const auto minCommandLength = 17;
    if (commandBuffer.size() < minCommandLength) {
        throw ValueError("HistoryPaymentsCommand::parse: "
                             "Can't parse command. Received command is to short.");
    }
    size_t tokenSeparatorPos = commandBuffer.find(
        kTokensSeparator);
    string historyFromStr = commandBuffer.substr(
        0,
        tokenSeparatorPos);
    if (historyFromStr.at(0) == '-') {
        throw ValueError("HistoryPaymentsCommand::parse: "
                             "Can't parse command. 'from' token can't be negative.");
    }
    try {
        mHistoryFrom = std::stoul(historyFromStr);
    } catch (...) {
        throw ValueError("HistoryPaymentsCommand::parse: "
                             "Can't parse command. Error occurred while parsing  'from' token.");
    }

    size_t nextTokenSeparatorPos = commandBuffer.find(
        kTokensSeparator,
        tokenSeparatorPos + 1);
    string historyCountStr = commandBuffer.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (historyCountStr.at(0) == '-') {
        throw ValueError("HistoryPaymentsCommand::parse: "
                             "Can't parse command. 'count' token can't be negative.");
    }
    try {
        mHistoryCount = std::stoul(historyCountStr);
    } catch (...) {
        throw ValueError("HistoryPaymentsCommand::parse: "
                             "Can't parse command. Error occurred while parsing 'count' token.");
    }

    tokenSeparatorPos = nextTokenSeparatorPos;
    nextTokenSeparatorPos = commandBuffer.find(
        kTokensSeparator,
        tokenSeparatorPos + 1);
    string timeFromStr = commandBuffer.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (timeFromStr == kNullParameter) {
        mIsTimeFromPresent = false;
    } else {
        mIsTimeFromPresent = true;
        try {
            int64_t timeFrom = std::stoul(timeFromStr);
            mTimeFrom = pt::time_from_string("1970-01-01 00:00:00.000");
            mTimeFrom += pt::microseconds(timeFrom);
        } catch (...) {
            throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'timeFrom' token.");
        }
    }

    tokenSeparatorPos = nextTokenSeparatorPos;
    nextTokenSeparatorPos = commandBuffer.find(
        kTokensSeparator,
        tokenSeparatorPos + 1);
    string timeToStr = commandBuffer.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (timeToStr == kNullParameter) {
        mIsTimeToPresent = false;
    } else {
        mIsTimeToPresent = true;
        try {
            int64_t timeTo = std::stoul(timeToStr);
            mTimeTo = pt::time_from_string("1970-01-01 00:00:00.000");
            mTimeTo += pt::microseconds(timeTo);
        } catch (...) {
            throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'timeTo' token.");
        }
    }

    tokenSeparatorPos = nextTokenSeparatorPos;
    nextTokenSeparatorPos = commandBuffer.find(
        kTokensSeparator,
        tokenSeparatorPos + 1);
    string lowBoundaryAmountStr = commandBuffer.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (lowBoundaryAmountStr == kNullParameter) {
        mIsLowBoundartAmountPresent = false;
    } else {
        mIsLowBoundartAmountPresent = true;
        try {
            mLowBoundaryAmount = TrustLineAmount(
                    lowBoundaryAmountStr);
        } catch (...) {
            throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'lowBoundaryAmount' token.");
        }
    }

    tokenSeparatorPos = nextTokenSeparatorPos;
    nextTokenSeparatorPos = commandBuffer.find(
        kTokensSeparator,
        tokenSeparatorPos + 1);
    string highBoundaryAmountStr = commandBuffer.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (highBoundaryAmountStr == kNullParameter) {
        mIsHighBoundaryAmountPresent = false;
    } else {
        mIsHighBoundaryAmountPresent = true;
        try {
            mHighBoundaryAmount = TrustLineAmount(
                    highBoundaryAmountStr);
        } catch (...) {
            throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'mHighBoundaryAmount' token.");
        }
    }

    tokenSeparatorPos = nextTokenSeparatorPos;
    nextTokenSeparatorPos = commandBuffer.size() - 1;
    string paymentRecordCommandUUIDStr = commandBuffer.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (paymentRecordCommandUUIDStr == kNullParameter) {
        mIsPaymentRecordCommandUUIDPresent = false;
    } else {
        mIsPaymentRecordCommandUUIDPresent = true;
        try {
            mPaymentRecordCommandUUID = boost::lexical_cast<uuids::uuid>(paymentRecordCommandUUIDStr);
        } catch (...) {
            throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'mPaymentRecordCommandUUID' token.");
        }
    }
}

const string &HistoryPaymentsCommand::identifier()
{
    static const string identifier = "GET:history/payments";
    return identifier;
}

const size_t HistoryPaymentsCommand::historyFrom() const
{
    return mHistoryFrom;
}

const size_t HistoryPaymentsCommand::historyCount() const
{
    return mHistoryCount;
}

const DateTime HistoryPaymentsCommand::timeFrom() const
{
    return mTimeFrom;
}

const DateTime HistoryPaymentsCommand::timeTo() const
{
    return mTimeTo;
}

const bool HistoryPaymentsCommand::isTimeFromPresent() const
{
    return mIsTimeFromPresent;
}

const bool HistoryPaymentsCommand::isTimeToPresent() const
{
    return mIsTimeToPresent;
}

const TrustLineAmount& HistoryPaymentsCommand::lowBoundaryAmount() const
{
    return mLowBoundaryAmount;
}

const TrustLineAmount& HistoryPaymentsCommand::highBoundaryAmount() const
{
    return mHighBoundaryAmount;
}

const bool HistoryPaymentsCommand::isLowBoundaryAmountPresent() const
{
    return mIsLowBoundartAmountPresent;
}

const bool HistoryPaymentsCommand::isHighBoundaryAmountPresent() const
{
    return mIsHighBoundaryAmountPresent;
}

const CommandUUID& HistoryPaymentsCommand::paymentRecordCommandUUID() const
{
    return mPaymentRecordCommandUUID;
}

const bool HistoryPaymentsCommand::isPaymentRecordCommandUUIDPresent() const
{
    return mIsPaymentRecordCommandUUIDPresent;
}

CommandResult::SharedConst HistoryPaymentsCommand::resultOk(string &historyPaymentsStr) const
{
    return CommandResult::SharedConst(
        new CommandResult(
            identifier(),
            UUID(),
            200,
            historyPaymentsStr));
}
