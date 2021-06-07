#define RAPIDJSON_HAS_STDSTRING 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "WwwFormEncodedDict.h"
#include "HttpClient.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

void connectToGunrock();
void outputError();
void printBalance(double balance);
void runInteractiveMode();
void runBatchModeOn(std::string fileName);
void errorCheckThenExecute(std::string command);
vector<string> splitString(string str);

bool execAuthPOST(string username, string password);
void execAuthPUT(string email);
void execBalance();
void execStripeDeposit(double amountDollars, string cardNum, string expYear, string expMonth, string cvc);
void execGunrockDeposit(int amountCents, string cardTokenId);
void execSend(string toUser, double amountDollars);
void execLogout();
void execDeleteAuthToken();

int API_SERVER_PORT = 8080;
string API_SERVER_HOST = "localhost";
string PUBLISHABLE_KEY = "";
HttpClient *gunrockClient;

string auth_token;
string user_id;

int main(int argc, char *argv[])
{
    stringstream config;
    int fd = open("config.json", O_RDONLY);
    if (fd < 0)
    {
        cout << "could not open config.json" << endl;
        exit(1);
    }
    int ret;
    char buffer[4096];
    while ((ret = read(fd, buffer, sizeof(buffer))) > 0)
    {
        config << string(buffer, ret);
    }
    Document d;
    d.Parse(config.str());
    API_SERVER_PORT = d["api_server_port"].GetInt();
    API_SERVER_HOST = d["api_server_host"].GetString();
    PUBLISHABLE_KEY = d["stripe_publishable_key"].GetString();

    if (argc > 2)
        outputError();
    else if (argc == 1)
        runInteractiveMode();
    else
        runBatchModeOn(argv[1]);

    return 0;
}

void connectToGunrock(bool includeAuthTokenHeader)
{
    delete gunrockClient;
    try
    {
        gunrockClient = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);
    }
    catch (const std::exception &e)
    {
        outputError();
    }

    if (includeAuthTokenHeader)
        gunrockClient->set_header("x-auth-token", auth_token);
}

void outputError()
{
    char error_message[7] = "Error\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

void printBalance(double balance)
{
    cout.precision(2);
    cout << fixed;
    cout << "Balance: $" << balance / 100 << endl;
}

void runInteractiveMode()
{
    std::string userInput = "";
    while (1)
    {
        std::cout << "d$> ";
        getline(std::cin, userInput);
        errorCheckThenExecute(userInput);
    }
}

void runBatchModeOn(std::string fileName)
{
    std::ifstream file;
    std::string fileInput;

    file.open(fileName, std::fstream::in);
    if (file.is_open())
        while (getline(file, fileInput))
            errorCheckThenExecute(fileInput);
    else
        outputError();
}

void errorCheckThenExecute(std::string command)
{
    if (command.empty())
    {
        outputError();
        return;
    }

    vector<string> splitCommand = splitString(command);
    if (!splitCommand.front().compare("auth") && splitCommand.size() == 4)
    {
        if (execAuthPOST(splitCommand[1], splitCommand[2]))
            execAuthPUT(splitCommand[3]);
    }
    else if (!splitCommand.front().compare("balance") && splitCommand.size() == 1)
        execBalance();
    else if (!splitCommand.front().compare("deposit") && splitCommand.size() == 6)
        execStripeDeposit(atof(splitCommand[1].c_str()), splitCommand[2], splitCommand[3], splitCommand[4], splitCommand[5]);
    else if (!splitCommand.front().compare("send") && splitCommand.size() == 3)
        execSend(splitCommand[1], atof(splitCommand[2].c_str()));
    else if (!splitCommand.front().compare("logout") && splitCommand.size() == 1)
        execLogout();
    else
        outputError();
}

vector<string> splitString(string str)
{
    vector<string> strings;
    stringstream ss(str);
    string word;
    while (ss >> word)
        strings.push_back(word);
    return strings;
}

bool execAuthPOST(string username, string password)
{
    //POST /auth-tokens - create or login account
    connectToGunrock(false);
    WwwFormEncodedDict body;
    body.set("username", username);
    body.set("password", password);
    HTTPClientResponse *response = gunrockClient->post("/auth-tokens", body.encode());

    if (!response->success())
    {
        outputError();
        return false;
    }
    else
    {
        //Remove prior off token if it's set
        if (!auth_token.empty())
            execDeleteAuthToken();

        //Load new auth token
        Document *d = response->jsonBody();
        auth_token = (*d)["auth_token"].GetString();
        user_id = (*d)["user_id"].GetString();
        delete d;
    }
    return true;
}

void execAuthPUT(string email)
{
    //PUT /users/{user_id} - update email and get balance
    connectToGunrock(true);
    WwwFormEncodedDict body;
    body.set("email", email);
    HTTPClientResponse *response = gunrockClient->put(string("/users/") + user_id, body.encode());

    if (!response->success())
        outputError();
    else
    {
        Document *d = response->jsonBody();
        printBalance((*d)["balance"].GetInt());
        delete d;
    }
}

void execBalance()
{
    //GET /users/{user_id} - get balance
    connectToGunrock(true);
    HTTPClientResponse *response = gunrockClient->get(string("/users/") + user_id);

    if (!response->success())
        outputError();
    else
    {
        Document *d = response->jsonBody();
        printBalance((*d)["balance"].GetInt());
        delete d;
    }
}

void execStripeDeposit(double amountDollars, string cardNum, string expYear, string expMonth, string cvc)
{
    //POST /v1/tokens - request card token from stripe
    if (cardNum.length() != 16)
    {
        outputError();
        return;
    }
    string cardTokenId;
    bool tokenRecieved = false;
    try // Post to stripe for card token
    {
        HttpClient stripeClient("api.stripe.com", 443, true);
        stripeClient.set_header("Authorization", string("Bearer ") + PUBLISHABLE_KEY);

        WwwFormEncodedDict body;
        body.set("card[number]", cardNum);
        body.set("card[exp_year]", expYear);
        body.set("card[exp_month]", expMonth);
        body.set("card[cvc]", cvc);

        HTTPClientResponse *stripeResponse = stripeClient.post("/v1/tokens", body.encode());
        if (!stripeResponse->success())
            throw "error";

        //Convert HTTP body to rapidjson document
        Document *d = stripeResponse->jsonBody();
        cardTokenId = (*d)["id"].GetString();
        tokenRecieved = true;
        delete d;
    }
    catch (const char *msg)
    {
        outputError();
    }

    if (tokenRecieved)
        execGunrockDeposit(int(amountDollars * 100), cardTokenId);
}

void execGunrockDeposit(int amountCents, string cardTokenId)
{
    //POST /deposit - deposit money from stripeCardToken to wallet
    connectToGunrock(true);
    WwwFormEncodedDict body;
    body.set("amount", amountCents);
    body.set("stripe_token", cardTokenId);
    HTTPClientResponse *response = gunrockClient->post("/deposits", body.encode());

    if (!response->success())
        outputError();
    else
    {
        Document *d = response->jsonBody();
        printBalance((*d)["balance"].GetInt());
        delete d;
    }
}

void execSend(string toUser, double amountDollars)
{
    //POST /transfers - create or login account
    connectToGunrock(true);
    WwwFormEncodedDict body;
    body.set("to", toUser);
    body.set("amount", int(amountDollars * 100));
    HTTPClientResponse *response = gunrockClient->post("/transfers", body.encode());

    if (!response->success())
        outputError();
    else
    {
        Document *d = response->jsonBody();
        printBalance((*d)["balance"].GetInt());
        delete d;
    }
}

void execLogout()
{
    execDeleteAuthToken();
    exit(0);
}

void execDeleteAuthToken()
{
    //DEL /auth-tokens/auth_token - remove auth_token from database
    connectToGunrock(true);
    HTTPClientResponse *response = gunrockClient->del(string("/auth-tokens/") + auth_token);

    if (!response->success())
        outputError();
    else
        auth_token.clear();
}
