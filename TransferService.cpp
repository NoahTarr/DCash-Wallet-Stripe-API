#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

TransferService::TransferService() : HttpService("/transfers") {}

void TransferService::post(HTTPRequest *request, HTTPResponse *response)
{
    User *fromUser = getAuthenticatedUser(request);
    Transfer *newTransfer;
    string to;
    User *toUser;
    int amount;

    try
    {
        to = request->formEncodedBody().get("to");
        amount = stoi(request->formEncodedBody().get("amount"));
    }
    catch (const std::exception &e)
    {
        //Either to or amount not included
        throw ClientError::badRequest();
    }

    try
    {
        toUser = m_db->users.at(to);
    }
    catch (const std::exception &e)
    {
        //to user doesn't exist
        throw ClientError::notFound();
    }

    if (amount > fromUser->balance || amount < 0)
        throw ClientError::badRequest();

    fromUser->balance -= amount;
    toUser->balance += amount;

    newTransfer = new Transfer();
    newTransfer->from = fromUser;
    newTransfer->to = toUser;
    newTransfer->amount = amount;

    m_db->transfers.push_back(newTransfer);

    //print json object
    Document document;
    Document::AllocatorType &a = document.GetAllocator();

    Value topObj;
    topObj.SetObject();
    topObj.AddMember("balance", fromUser->balance, a);

    Value transfersArray;
    transfersArray.SetArray();

    for (const auto transfer : m_db->transfers)
    {
        if (transfer->from->username.compare(toUser->username) &&
            transfer->from->username.compare(fromUser->username) &&
            transfer->to->username.compare(toUser->username) &&
            transfer->to->username.compare(fromUser->username))
            continue;
        Value transferObj;
        transferObj.SetObject();
        transferObj.AddMember("from", transfer->from->username, a);
        transferObj.AddMember("to", transfer->to->username, a);
        transferObj.AddMember("amount", transfer->amount, a);
        transfersArray.PushBack(transferObj, a);
    }
    topObj.AddMember("transfers", transfersArray, a);

    responseJsonFinalizer(response, &document, &topObj);
}
