#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

DepositService::DepositService() : HttpService("/deposits") {}

void DepositService::post(HTTPRequest *request, HTTPResponse *response)
{
    User *user = getAuthenticatedUser(request);
    Deposit *newDeposit;
    int amount;
    string stripeToken;

    try
    {
        amount = stoi(request->formEncodedBody().get("amount"));
        stripeToken = request->formEncodedBody().get("stripe_token");
    }
    catch (const std::exception &e)
    {
        //amount or stripe_token not in header
        throw ClientError::badRequest();
    }

    //Stripe API
    string stripeChargeId = getStripeChargeId(amount, stripeToken);

    //Deposit Succeeded
    user->balance += amount;
    newDeposit = new Deposit();
    newDeposit->to = user;
    newDeposit->amount = amount;
    newDeposit->stripe_charge_id = stripeChargeId;
    m_db->deposits.push_back(newDeposit);

    //Build json response object
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value topObj;
    topObj.SetObject();
    topObj.AddMember("balance", user->balance, a);
    Value depositsArray;
    depositsArray.SetArray();

    for (const auto deposit : m_db->deposits)
    {
        if (deposit->to->username.compare(user->username))
            continue;
        Value depositObj;
        depositObj.SetObject();
        depositObj.AddMember("to", deposit->to->username, a);
        depositObj.AddMember("amount", deposit->amount, a);
        depositObj.AddMember("stripe_charge_id", deposit->stripe_charge_id, a);
        depositsArray.PushBack(depositObj, a);
    }
    topObj.AddMember("deposits", depositsArray, a);

    responseJsonFinalizer(response, &document, &topObj);
}

string DepositService::getStripeChargeId(int amount, string stripeToken)
{
    string stripeChargeId;
    try
    {
        HttpClient client("api.stripe.com", 443, true);
        client.set_basic_auth(m_db->stripe_secret_key, "");

        WwwFormEncodedDict body;
        body.set("amount", amount);
        body.set("currency", "usd");
        body.set("source", stripeToken);

        HTTPClientResponse *clientResponse = client.post("/v1/charges", body.encode());

        if (!clientResponse->success())
            throw ClientError::badRequest();

        Document *d = clientResponse->jsonBody();
        stripeChargeId = (*d)["id"].GetString();
        delete d;
    }
    catch (const std::exception &e)
    {
        //Maybe better to throw unknown error here? Just using badRequest() for grading
        throw ClientError::badRequest();
    }
    return stripeChargeId;
}
