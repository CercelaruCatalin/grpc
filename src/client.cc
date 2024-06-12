#include <grpcpp/grpcpp.h>
#include "proto/hello.grpc.pb.h"
#include "proto/hello.pb.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace std;
using namespace grpc;

void printInitialMenu() {
    cout << "\nInitial Menu Options:" << endl;
    cout << "1. Register" << endl;
    cout << "2. Login" << endl;
    cout << "3. Exit" << endl;
    cout << "Enter your choice: ";
}

void printUserMenu() {
    cout << "\nUser Menu Options:" << endl;
    cout << "1. Conversion" << endl;
    cout << "2. Logout" << endl;
    cout << "Enter your choice: ";
}

int main(int argc, char** argv) {
    // Create a Stub object
    auto channel = grpc::CreateChannel("localhost:9999", grpc::InsecureChannelCredentials());
    unique_ptr<RegisterUser::Stub> registerStub(RegisterUser::NewStub(channel));
    unique_ptr<LoginUser::Stub> loginStub(LoginUser::NewStub(channel));
    unique_ptr<Conversion::Stub> conversionStub(Conversion::NewStub(channel));

    User registerRequest;
    ResultRegister responseRegister;

    User loginRequest;
    ResultLogin loginResponse;

    File fileRequest;
    File fileResponse;

    string username;
    string password;
    grpc::Status status;

    int choice;
    bool running = true;
    bool loggedIn = false;

    while (running) {
        if (!loggedIn) {
            printInitialMenu();
            cin >> choice;

            switch (choice) {
                case 1:
                    // Register a new user
                    cout << "Enter a username: ";
                    cin >> username;
                    registerRequest.set_username(username);
                    cout << "Enter a password: ";
                    cin >> password;
                    registerRequest.set_password(password);

                    {
                        grpc::ClientContext context;  // Create a new context for each request
                        status = registerStub->registerUser(&context, registerRequest, &responseRegister);
                    }

                    if (status.ok()) {
                        cout << "Registration successful" << endl;
                    } else {
                        cerr << "Registration failed: " << status.error_message() << endl;
                    }
                    break;

                case 2:
                    // Login to an existing user
                    cout << "Enter a username: ";
                    cin >> username;
                    loginRequest.set_username(username);
                    cout << "Enter a password: ";
                    cin >> password;
                    loginRequest.set_password(password);

                    {
                        grpc::ClientContext context;  // Create a new context for each request
                        status = loginStub->loginUser(&context, loginRequest, &loginResponse);
                    }

                    if (status.ok()) {
                        cout << "Login successful" << endl;
                        loggedIn = true;  // Set the loggedIn flag to true after successful login
                    } else {
                        cerr << "Login failed: " << status.error_message() << endl;
                    }
                    break;

                case 3:
                    // Exit the program
                    running = false;
                    cout << "Exiting the program." << endl;
                    break;

                default:
                    cout << "Invalid choice. Please try again." << endl;
            }
        } else {
            printUserMenu();
            cin >> choice;

            string xmlFileName;
            ifstream xmlFile;
            string xmlContent;
            string jsonName;

            switch (choice) {
                case 1:
                    // Placeholder for conversion functionality
                    cout << "Conversion option selected." << endl;

                    // Read XML file
                    cout << "Enter XML file path: ";
                    cin >> xmlFileName;
                    fileRequest.set_xmlpath(xmlFileName);

                    cout << "Enter JSON file name: ";
                    cin >> jsonName;
                    fileRequest.set_jsonname(jsonName);
                    
                    xmlFile.open(xmlFileName);
                    if (!xmlFile.is_open()) {
                        cerr << "Failed to open XML file" << endl;
                        break;
                    }
                    xmlContent = string((istreambuf_iterator<char>(xmlFile)), istreambuf_iterator<char>());
                    xmlFile.close();

                    // Set file content in request
                    fileRequest.set_contents(xmlContent);

                    // Perform conversion
                    {
                        grpc::ClientContext context;  // Create a new context for each request
                        status = conversionStub->performConversion(&context, fileRequest, &fileResponse);
                    }

                    if (status.ok()) {
                        cout << "Conversion successful" << endl;
                        // Handle converted file response
                        // For now, just print the converted content
                        cout << "Converted content: " << fileResponse.contents() << endl;
                    } else {
                        cerr << "Conversion failed: " << status.error_message() << endl;
                    }

                    break;
            
                case 2:
                    // Logout the user
                    loggedIn = false;
                    cout << "Logged out successfully." << endl;
                    break;

                default:
                    cout << "Invalid choice. Please try again." << endl;
            }
        }
    }

    return 0;
}