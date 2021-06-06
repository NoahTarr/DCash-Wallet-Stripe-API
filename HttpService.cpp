#include <iostream>

#include <stdlib.h>
#include <stdio.h>

#include "HttpService.h"
#include "ClientError.h"

using namespace std;
using namespace rapidjson;

HttpService::HttpService(string pathPrefix)
{
    this->m_pathPrefix = pathPrefix;
}

void HttpService::responseJsonFinalizer(HTTPResponse *response, Document *document, Value *obj)
{
    // rapidjson boilerplate for converting the JSON object to a string
    document->Swap(*obj);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document->Accept(writer);

    // set the return object
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));
}

User *HttpService::getAuthenticatedUser(HTTPRequest *request)
{
    if (!request->hasAuthToken())
        throw ClientError::forbidden();

    User *user;
    try
    {
        string aToken = request->getAuthToken();
        user = m_db->auth_tokens.at(aToken);
    }
    catch (const exception &e)
    {
        throw ClientError::notFound();
    }

    return user;
}

User *HttpService::getAuthenticatedUser(string authToken)
{
    User *user;
    try
    {
        user = m_db->auth_tokens.at(authToken);
    }
    catch (const exception &e)
    {
        throw ClientError::notFound();
    }

    return user;
}

string HttpService::pathPrefix()
{
    return m_pathPrefix;
}

void HttpService::head(HTTPRequest *request, HTTPResponse *response)
{
    cout << "HEAD " << request->getPath() << endl;
    throw ClientError::methodNotAllowed();
}

void HttpService::get(HTTPRequest *request, HTTPResponse *response)
{
    cout << "GET " << request->getPath() << endl;
    throw ClientError::methodNotAllowed();
}

void HttpService::put(HTTPRequest *request, HTTPResponse *response)
{
    cout << "PUT " << request->getPath() << endl;
    throw ClientError::methodNotAllowed();
}

void HttpService::post(HTTPRequest *request, HTTPResponse *response)
{
    cout << "POST " << request->getPath() << endl;
    throw ClientError::methodNotAllowed();
}

void HttpService::del(HTTPRequest *request, HTTPResponse *response)
{
    cout << "DELETE " << request->getPath() << endl;
    throw ClientError::methodNotAllowed();
}
