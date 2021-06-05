#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AccountService::AccountService() : HttpService("/users")
{
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response)
{
    User *user = getAuthenticatedUser(request);
    string urlUserId = request->getPathComponents().back();

    if (user->user_id.compare(urlUserId))
    {
        //user_id in URL different from authenticated user_id
        throw ClientError::forbidden();
    }

    //print json object
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value obj;
    obj.SetObject();
    obj.AddMember("email", user->email, a);
    obj.AddMember("balance", user->balance, a);
    responseJsonFinalizer(response, &document, &obj);
}

void AccountService::put(HTTPRequest *request, HTTPResponse *response)
{
    User *user = getAuthenticatedUser(request);
    string urlUserId = request->getPathComponents().back();

    if (user->user_id.compare(urlUserId))
    {
        //user_id in URL different from authenticated user_id
        throw ClientError::forbidden();
    }

    try
    {
        user->email = request->formEncodedBody().get("email");
    }
    catch (const std::exception &e)
    {
        throw ClientError::badRequest();
    }

    //print json object
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value obj;
    obj.SetObject();
    obj.AddMember("email", user->email, a);
    obj.AddMember("balance", user->balance, a);
    responseJsonFinalizer(response, &document, &obj);
}
