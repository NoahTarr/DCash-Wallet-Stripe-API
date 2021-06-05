#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AuthService::AuthService() : HttpService("/auth-tokens")
{
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response)
{
    string username;
    string password;
    User *user;
    string authToken = StringUtils::createAuthToken();

    if (any_of(username.begin(), username.end(), &::isupper))
        throw ClientError::methodNotAllowed();

    try
    {
        username = request->formEncodedBody().get("username");
        password = request->formEncodedBody().get("password");
    }
    catch (const std::exception &e)
    {
        throw ClientError::badRequest();
    }

    try //User Exists
    {
        user = m_db->users.at(username);

        if (user->password != password)
            throw ClientError::forbidden();

        response->setStatus(200);
    }
    catch (const out_of_range &e) //New User
    {
        user = new User();
        user->email = "";
        user->username = username;
        user->password = password;
        user->user_id = StringUtils::createUserId();
        user->balance = 0;

        m_db->users.insert(pair<string, User *>(username, user));
        response->setStatus(201);
    }
    m_db->auth_tokens.insert(pair<string, User *>(authToken, user));

    //print json object
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value obj;
    obj.SetObject();
    obj.AddMember("auth_token", authToken, a);
    obj.AddMember("user_id", user->user_id, a);
    responseJsonFinalizer(response, &document, &obj);
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response)
{
    User *user = getAuthenticatedUser(request);
    string headerAuthToken = request->getAuthToken();
    string urlAuthToken = request->getPathComponents().back();

    //Url auth_token different from header x-auth-token
    //Must ensure both auth-tokens point to the same user
    if (urlAuthToken.compare(headerAuthToken) &&
        user->user_id.compare(getAuthenticatedUser(urlAuthToken)->user_id))
    {
        //Both auth_tokens exist but point to different users
        throw ClientError::forbidden();
    }

    m_db->auth_tokens.erase(urlAuthToken);
}
