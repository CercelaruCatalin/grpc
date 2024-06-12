#include <grpcpp/grpcpp.h>
#include "proto/hello.pb.h"
#include "proto/hello.grpc.pb.h"
#include <httplib.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include <iostream>
#include <fstream>
#include <string>
#include "db.h"
#include <sqlite3.h>
#include "json.h"
#include "lxml.h"

using namespace std;
using namespace grpc;
using namespace httplib;

class RegisterService : public RegisterUser::Service {
public:
    ::grpc::Status registerUser(::grpc::ServerContext* context, const ::User* request, ::ResultRegister* response) override {
        if (db_user_exists(request->username().c_str())) {
            return ::grpc::Status(::grpc::StatusCode::ALREADY_EXISTS, "Username already taken");
        }

        db_add_user(request->username().c_str(), request->password().c_str());
        response->set_resultr("User registered successfully");
        return ::grpc::Status(::grpc::StatusCode::OK, "User registered successfully");
    }
};

class LoginService : public LoginUser::Service {
public:
    ::grpc::Status loginUser(::grpc::ServerContext* context, const ::User* request, ::ResultLogin* response) override {
        if (!db_check_user(request->username().c_str(), request->password().c_str())) {
            return ::grpc::Status(::grpc::StatusCode::UNAUTHENTICATED, "Invalid username or password");
        }

        response->set_resultl("User logged in successfully");
        return ::grpc::Status(::grpc::StatusCode::OK, "User logged in successfully");
    }
};

class ConversionServiceImpl final : public Conversion::Service {
public:
    Status performConversion(ServerContext* context, const File* request, File* response) override {
        XMLDocument document;
        if (!strstr(request->xmlpath().c_str(), ".xml")) {
            return Status(StatusCode::INVALID_ARGUMENT, "Invalid XML file extension");
        }

        // Open the XML file stream
        ofstream xmlFile("temp.xml", ios::binary);
        if (!xmlFile.is_open()) {
            return Status(StatusCode::INTERNAL, "Failed to open XML file");
        }
        xmlFile << request->contents();
        xmlFile.close();

        ifstream inputFile("temp.xml", ios::binary);
        if (!inputFile.is_open()) {
            return Status(StatusCode::INTERNAL, "Failed to open XML file");
        }

        if (!XMLDocument_load(&document, inputFile)) {
            return Status(StatusCode::INTERNAL, "Failed to load XML document");
        }

        cJSON* json = XMLDocumentToJSON(&document);
        response->set_contents(cJSON_Print(json)); // Set the JSON content in the response
        cJSON_Delete(json);

        // Set the XML path and JSON name in the response
        response->set_xmlpath(request->xmlpath());
        response->set_jsonname(request->jsonname());

        XMLDocument_free(&document);
        return Status::OK;
    }
};


void serveHtml(httplib::Server& httpServer, const std::string& url, const std::string& filepath) {
    httpServer.Get(url.c_str(), [filepath](const Request& req, Response& res) {
        ifstream file(filepath);
        if (file) {
            stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "text/html");
        } else {
            res.status = 404;
            res.set_content("File not found", "text/plain");
        }
    });
}

int main() {
    db_init(); // Initialize the database

    // Start gRPC Server
    RegisterService registerService;
    LoginService loginService;
    ConversionServiceImpl conversionService;

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:9999", grpc::InsecureServerCredentials());
    builder.RegisterService(&loginService);
    builder.RegisterService(&registerService);
    builder.RegisterService(&conversionService);
    unique_ptr<grpc::Server> grpcServer(builder.BuildAndStart());
    cout << "gRPC Server listening on port 9999" << endl;

    // Start HTTP Server
    httplib::Server httpServer;

    // Serve HTML files
    serveHtml(httpServer, "/register", "../web/register.html");
    serveHtml(httpServer, "/login", "../web/login.html");
    serveHtml(httpServer, "/convert", "../web/convert.html");

    // Create gRPC client
    auto channel = grpc::CreateChannel("localhost:9999", grpc::InsecureChannelCredentials());
    std::unique_ptr<RegisterUser::Stub> registerStub(RegisterUser::NewStub(channel));
    std::unique_ptr<LoginUser::Stub> loginStub(LoginUser::NewStub(channel));
    std::unique_ptr<Conversion::Stub> conversionStub(Conversion::NewStub(channel));

    // Handle registration
    httpServer.Post("/api/register", [&](const Request& req, Response& res) {
        cJSON *json = cJSON_Parse(req.body.c_str());
        if (!json) {
            res.set_content("Invalid JSON", "text/plain");
            return;
        }

        const char *username = cJSON_GetObjectItem(json, "username")->valuestring;
        const char *password = cJSON_GetObjectItem(json, "password")->valuestring;

        // Using gRPC client to call gRPC service
        User user;
        user.set_username(username);
        user.set_password(password);

        ResultRegister result;
        grpc::ClientContext context;
        Status status = registerStub->registerUser(&context, user, &result);

        if (status.ok()) {
            res.set_content("Registration successful", "text/plain");
        } else {
            res.set_content("Registration failed: " + status.error_message(), "text/plain");
        }

        cJSON_Delete(json);
    });

    // Handle login
    httpServer.Post("/api/login", [&](const Request& req, Response& res) {
        cJSON *json = cJSON_Parse(req.body.c_str());
        if (!json) {
            res.set_content("Invalid JSON", "text/plain");
            return;
        }

        const char *username = cJSON_GetObjectItem(json, "username")->valuestring;
        const char *password = cJSON_GetObjectItem(json, "password")->valuestring;

        // Using gRPC client to call gRPC service
        User user;
        user.set_username(username);
        user.set_password(password);

        ResultLogin result;
        grpc::ClientContext context;
        Status status = loginStub->loginUser(&context, user, &result);

        if (status.ok()) {
            res.set_content("Login successful", "text/plain");
        } else {
            res.set_content("Login failed: " + status.error_message(), "text/plain");
        }

        cJSON_Delete(json);
    });

    // Handle convert
    httpServer.Post("/api/convert", [&](const Request& req, Response& res) {
        // Check if JSON name is present in the form data
        if (!req.has_file("xmlFile")) {
            res.set_content("Missing XML file or JSON name", "text/plain");
            return;
        }

        // Get the JSON name from the form data
        auto jsonName = req.get_param_value("jsonName");

        // Get the XML file data from the request
        const auto& xmlFile = req.get_file_value("xmlFile");

        // Prepare gRPC request
        File request;
        request.set_contents(xmlFile.content);
        request.set_xmlpath(xmlFile.filename); // Use the filename as the XML path
        request.set_jsonname(jsonName);

        // Call gRPC performConversion function
        ClientContext context;
        File response;
        Status status = conversionStub->performConversion(&context, request, &response);

        if (status.ok()) {
            res.set_content(response.contents(), "application/json");
        } else {
            res.set_content("Conversion failed: " + status.error_message(), "text/plain");
        }
    });

    thread grpcThread([&grpcServer]() {
        grpcServer->Wait();
    });

    cout << "HTTP Server listening on port 8080" << endl;
    httpServer.listen("0.0.0.0", 8080);

    grpcThread.join();
    sqlite3_close(db); // Close the database connection
    return 0;
}

