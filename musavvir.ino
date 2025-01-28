#include <WiFi.h>
#include<time.h>
#include <FS.h>
#include <WebServer.h>
#include <Wire.h>                    
#include <LiquidCrystal_I2C.h> 
#include <Preferences.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <SPIFFS.h>
#include <Keypad.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <set>
#include <map>
#include <SD.h>
#include <RTClib.h>
#include <Wire.h>
#include <hd44780.h>                    
#include <hd44780ioClass/hd44780_I2Cexp.h>
hd44780_I2Cexp lcd;
RTC_DS3231 rtc;
#define MAX_STUDENTS 100  // Adjust this based on the maximum number of students
bool isFingerprintSensorOperational = true; // Flag to track sensor status
bool isFingerprintEnabled = false;


bool sendToSupabase(const char* table, const String& jsonData, const String& method = "POST", int teacherId = -1);
bool attendedStudentsarray[MAX_STUDENTS] = {false};  // Array to track attendance
bool isAttendanceMode = false;

//56

std::vector<String> parseCSVLine(const String& line, const std::vector<int>& columns) {
    std::vector<String> result;
    int columnIndex = 0;
    int startIndex = 0;
    int foundIndex = line.indexOf(',');

    while (foundIndex != -1) {
        if (std::find(columns.begin(), columns.end(), columnIndex) != columns.end()) {
            result.push_back(line.substring(startIndex, foundIndex));
        }
        columnIndex++;
        startIndex = foundIndex + 1;
        foundIndex = line.indexOf(',', startIndex);
    }

    // Check last column after the last comma
    if (std::find(columns.begin(), columns.end(), columnIndex) != columns.end()) {
        result.push_back(line.substring(startIndex));
    }

    return result;
}

Preferences preferences;
const byte ROWS = 4;
const byte COLS = 4; 
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[COLS] = {26, 25, 33, 32}; // Adjust as per your wiring
byte colPins[ROWS] = {13, 15, 27, 4};   // Adjust as per your wiring
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
#define RX_PIN 16
#define TX_PIN 17
#define MAX_RETRIES 3
#define RETRY_DELAY 5000
SoftwareSerial mySerial(RX_PIN, TX_PIN);
Adafruit_Fingerprint finger(&mySerial);
WebServer server(80);
String ssid = "";
String password = "";
String coursecode = "";
const char* adminUsername = "admin";
const char* adminPassword = "admin123";
const char* teacherDataFile = "/teachers.csv";
const char* studentDataFile = "/students.csv";
const char* attendanceDataFile = "/attendance.csv";
const char* SUPABASE_URL = "https://vwhxnfnvtsvycwgmcmwh.supabase.co";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InZ3aHhuZm52dHN2eWN3Z21jbXdoIiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzY4NzYyNTksImV4cCI6MjA1MjQ1MjI1OX0.o29ooc_KF-weSvlHqxUOjeRxuHmt7_hA6PLnilMWbS4";
const char* offlineAttendanceFile = "/offline_attendance.csv"; 
void handleEditTeacher();
void handleEditStudent();
void handleUpdateTeacher();
void handleUpdateStudent();
void handleDeleteTeacher();
void handleDeleteStudent();
void handleAttendanceDetails();
void handleAttendanceDetails1();
void handleDateWiseDownload();
void handleDownloadAttendanceByDate();
void handleDownloadAttendance();
void handleDownloadDateAttendance();
void sendOfflineDataToSupabase();
void handleDownloadOfflineData();
void saveOfflineAttendance();
void handleKeypadInput();
void checkWiFiConnection();

//1 for edit teacher no need to supabase or sd
void handleEditTeacher() {
    if (!authenticateUser()) {
        return;
    }

    String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <style>
            table {
                width: 100%;
                border-collapse: collapse;
                margin-bottom: 20px;
            }
            th, td {
                border: 1px solid #ddd;
                padding: 8px;
                text-align: left;
            }
            th {
                background-color: #4CAF50;
                color: white;
            }
            .edit-btn {
                background-color: #4CAF50;
                color: white;
                padding: 5px 10px;
                border: none;
                cursor: pointer;
            }
            .delete-btn {
                background-color: #f44336;
                color: white;
                padding: 5px 10px;
                border: none;
                cursor: pointer;
            }
            .container {
                width: 80%;
                margin: 0 auto;
            }
            h2 {
                text-align: center;
            }
        </style>
    </head>
    <body>
        <div class="container">
            <h2>Edit Teachers</h2>
            <table>
                <tr>
                    <th>ID</th>
                    <th>Name</th>
                    <th>Course Name</th>
                    <th>Course Code</th>
                    <th>Actions</th>
                </tr>
    )rawliteral";

    File file = SPIFFS.open(teacherDataFile, "r");
    if (!file) {
        server.send(500, "text/html", "Error opening teacher data file.");
        return;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            int firstComma = line.indexOf(',');
            int secondComma = line.indexOf(',', firstComma + 1);
            int thirdComma = line.indexOf(',', secondComma + 1);
            String id = line.substring(0, firstComma);
            String name = line.substring(firstComma + 1, secondComma);
            String courseName = line.substring(secondComma + 1, thirdComma);
            String courseCode = line.substring(thirdComma + 1);

            htmlPage += "<tr><td>" + id + "</td><td>" + name + "</td><td>" + courseName + "</td><td>" + courseCode + 
                        "</td><td><button class='edit-btn' onclick='editTeacher(\"" + id + "\", \"" + name + "\", \"" + courseName + "\", \"" + courseCode + 
                        "\")'>Edit</button> <button class='delete-btn' onclick='deleteTeacher(\"" + id + 
                        "\")'>Delete</button></td></tr>";
        }
    }
    file.close();

    htmlPage += R"rawliteral(
            </table>
            <script>
                function editTeacher(id, name, courseName, courseCode) {
                    let newName = prompt("Enter new name:", name);
                    let newCourseName = prompt("Enter new course name:", courseName);
                    let newCourseCode = prompt("Enter new course code:", courseCode);
                    if (newName && newCourseName && newCourseCode) {
                        // Use encodeURIComponent to ensure special characters are handled correctly
                        window.location.href = `/updateTeacher?id=${id}&name=${encodeURIComponent(newName)}&courseName=${encodeURIComponent(newCourseName)}&courseCode=${encodeURIComponent(newCourseCode)}`;
                    }
                }
                function deleteTeacher(id) {
                    if (confirm("Are you sure you want to delete this teacher?")) {
                        window.location.href = `/deleteTeacher?id=${id}`;
                    }
                }
            </script>
        </div>
    </body>
    </html>
    )rawliteral";

    server.send(200, "text/html", htmlPage);
}

//2 
void handleUpdateTeacher() {
    if (!authenticateUser ()) {
        return;
    }

    // Get parameters from the request
    String id = server.arg("id");
    String newName = server.arg("name");
    String newCourseName = server.arg("courseName");
    String newCourseCode = server.arg("courseCode");

    // Ensure all parameters are provided
    if (id.isEmpty() || newName.isEmpty() || newCourseName.isEmpty() || newCourseCode.isEmpty()) {
        server.send(400, "text/plain", "Invalid input parameters. All fields are required.");
        return;
    }

    // Open files for reading and writing
    File oldFile = SPIFFS.open(teacherDataFile, "r");
    File newFile = SPIFFS.open("/temp.csv", "w");

    bool recordUpdated = false; // Track if the record was updated

    while (oldFile.available()) {
        String line = oldFile.readStringUntil('\n');
        line.trim();

        // Check if the line starts with the ID we want to update
        if (line.startsWith(id + ",")) {
            // Write updated data to the new file
            newFile.println(id + "," + newName + "," + newCourseName + "," + newCourseCode);
            recordUpdated = true;

            
        } else {
            // Write unchanged lines to the new file
            newFile.println(line);
        }
    }

    oldFile.close();
    newFile.close();

    // Replace the old file with the new one
    SPIFFS.remove(teacherDataFile);
    SPIFFS.rename("/temp.csv", teacherDataFile);

    // Handle response to the user
    if (recordUpdated) {
        server.sendHeader("Location", "/editTeacher"); // Redirect to edit teacher page
        server.send(303);
    } else {
        server.send(404, "text/plain", "Teacher record not found.");
    }
}
//3 delete
void handleDeleteTeacher() {
    if (!authenticateUser()) {
        return;
    }

    String id = server.arg("id");

    // Open the teacher data file and check for successful opening
    File oldFile = SPIFFS.open(teacherDataFile, "r");
    if (!oldFile) {
        server.send(500, "text/html", "Error reading teacher data.");
        return;
    }

    // Create a new temporary file for storing data after deletion
    File newFile = SPIFFS.open("/temp.csv", "w");
    if (!newFile) {
        oldFile.close();
        server.send(500, "text/html", "Error creating temporary file.");
        return;
    }

    bool found = false;  // Flag to track if the teacher ID was found and deleted

    // Read each line and write back to the new file, skipping the teacher to be deleted
    while (oldFile.available()) {
        String line = oldFile.readStringUntil('\n');
        line.trim();  // Clean up any unnecessary whitespace
        
        // Check if the line starts with the ID to delete
        if (line.startsWith(id + ",")) {
            found = true;  // Mark as found, will not write this line to the new file
        } else {
            newFile.println(line);  // Keep the record if it doesn't match
        }
    }

    oldFile.close();
    newFile.close();

    // If the teacher ID wasn't found, notify the user
    if (!found) {
        SPIFFS.remove("/temp.csv");
        server.send(404, "text/html", "Teacher ID not found.");
        return;
    }

    // Replace the old file with the new file
    SPIFFS.remove(teacherDataFile);
    SPIFFS.rename("/temp.csv", teacherDataFile);

    // Redirect to the teacher edit page with a success message
    server.sendHeader("Location", "/editTeacher?status=deleted");
    server.send(303);
}

// 4
void handleEditStudent() {
    if (!authenticateUser()) {
        return;
    }

    String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Edit Students</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                background-color: #f4f4f4;
                margin: 0;
                padding: 20px;
            }
            table {
                width: 100%;
                border-collapse: collapse;
                margin-bottom: 20px;
            }
            th, td {
                border: 1px solid #ddd;
                padding: 8px;
                text-align: left;
            }
            th {
                background-color: #4CAF50;
                color: white;
            }
            .edit-btn, .delete-btn {
                padding: 5px 10px;
                border: none;
                cursor: pointer;
                font-size: 14px;
            }
            .edit-btn {
                background-color: #4CAF50;
                color: white;
            }
            .delete-btn {
                background-color: #f44336;
                color: white;
            }
            .edit-btn:hover, .delete-btn:hover {
                opacity: 0.8;
            }
            h2 {
                color: #333;
            }
        </style>
    </head>
    <body>
        <h2>Edit Students</h2>
        <table>
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Name</th>
                    <th>Roll Number</th>
                    <th>Actions</th>
                </tr>
            </thead>
            <tbody>
    )rawliteral";

    // Open student data file
    File file = SPIFFS.open(studentDataFile, "r");
    if (!file) {
        server.send(500, "text/html", "Error reading student data.");
        return;
    }

    // Loop through the file lines and add student data to the table
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            int firstComma = line.indexOf(',');
            int lastComma = line.lastIndexOf(',');
            String id = line.substring(0, firstComma);
            String name = line.substring(firstComma + 1, lastComma);
            String rollNumber = line.substring(lastComma + 1);

            htmlPage += "<tr><td>" + id + "</td><td>" + name + "</td><td>" + rollNumber + 
                        "</td><td><button class='edit-btn' onclick='editStudent(\"" + id + "\", \"" + name + "\", \"" + rollNumber + 
                        "\")'>Edit</button> <button class='delete-btn' onclick='deleteStudent(\"" + id + 
                        "\")'>Delete</button></td></tr>";
        }
    }

    file.close();

    htmlPage += R"rawliteral(
            </tbody>
        </table>
        <script>
            function editStudent(id, name, rollNumber) {
                let newName = prompt("Enter new name:", name);
                let newRollNumber = prompt("Enter new roll number:", rollNumber);
                if (newName && newRollNumber) {
                    window.location.href = `/updateStudent?id=${id}&name=${encodeURIComponent(newName)}&rollNumber=${encodeURIComponent(newRollNumber)}`;
                }
            }

            function deleteStudent(id) {
                if (confirm("Are you sure you want to delete this student?")) {
                    window.location.href = `/deleteStudent?id=${id}`;
                }
            }
        </script>
    </body>
    </html>
    )rawliteral";

    // Send the HTML response
    server.send(200, "text/html", htmlPage);
}

//5
void handleUpdateStudent() {
    if (!authenticateUser()) {
        return;
    }

    String id = server.arg("id");
    String newName = server.arg("name");
    String newRollNumber = server.arg("rollNumber");

    // Open the old file for reading
    File oldFile = SPIFFS.open(studentDataFile, "r");
    if (!oldFile) {
        Serial.println("Failed to open the student data file for reading.");
        server.send(500, "text/html", "Failed to read student data.");
        return;
    }

    // Create a new file to store the updated data
    File newFile = SPIFFS.open("/temp.csv", "w");
    if (!newFile) {
        Serial.println("Failed to open the temp file for writing.");
        oldFile.close();
        server.send(500, "text/html", "Failed to update student data.");
        return;
    }

    bool updated = false;
    while (oldFile.available()) {
        String line = oldFile.readStringUntil('\n');

        // If the line starts with the student ID, update it
        if (line.startsWith(id + ",")) {
            newFile.println(id + "," + newName + "," + newRollNumber);
            updated = true;  // Mark that we updated the student
        } else {
            newFile.println(line);
        }
    }

    oldFile.close();
    newFile.close();

    // Check if the student was updated
    if (!updated) {
        Serial.println("Student ID not found.");
        server.send(404, "text/html", "Student not found.");
        return;
    }

    // Replace the old file with the updated file
    if (SPIFFS.remove(studentDataFile)) {
        if (SPIFFS.rename("/temp.csv", studentDataFile)) {
            Serial.println("Student updated successfully.");
        } else {
            Serial.println("Failed to rename the temp file.");
            server.send(500, "text/html", "Failed to update student data.");
            return;
        }
    } else {
        Serial.println("Failed to remove the old student data file.");
        server.send(500, "text/html", "Failed to update student data.");
        return;
    }

    // Redirect to the edit student page after updating
    server.sendHeader("Location", "/editStudent");
    server.send(303);
}

//6

void handleDeleteStudent() {
    if (!authenticateUser()) {
        return;
    }

    String id = server.arg("id");
    
    // Open the old file for reading
    File oldFile = SPIFFS.open(studentDataFile, "r");
    if (!oldFile) {
        Serial.println("Failed to open the student data file for reading.");
        server.send(500, "text/html", "Failed to read student data.");
        return;
    }

    // Create a new file to store the updated data
    File newFile = SPIFFS.open("/temp.csv", "w");
    if (!newFile) {
        Serial.println("Failed to open the temp file for writing.");
        oldFile.close();
        server.send(500, "text/html", "Failed to update student data.");
        return;
    }

    bool deleted = false;
    while (oldFile.available()) {
        String line = oldFile.readStringUntil('\n');
        
        // If the line starts with the student ID, skip it (deleting the student)
        if (line.startsWith(id + ",")) {
            deleted = true; // Mark that we found and deleted the student
        } else {
            newFile.println(line);
        }
    }

    oldFile.close();
    newFile.close();

    // Check if the student was deleted
    if (!deleted) {
        Serial.println("Student ID not found.");
        server.send(404, "text/html", "Student not found.");
        return;
    }

    // Replace the old file with the updated file
    if (SPIFFS.remove(studentDataFile)) {
        if (SPIFFS.rename("/temp.csv", studentDataFile)) {
            Serial.println("Student deleted successfully.");
        } else {
            Serial.println("Failed to rename the temp file.");
            server.send(500, "text/html", "Failed to update student data.");
            return;
        }
    } else {
        Serial.println("Failed to remove the old student data file.");
        server.send(500, "text/html", "Failed to update student data.");
        return;
    }

    // Redirect to the edit student page after deletion
    server.sendHeader("Location", "/editStudent");
    server.send(303);
}

//7

bool initializeFingerprint() {
    mySerial.begin(57600);
    finger.begin(57600);

    // Short delay to ensure proper initialization
    delay(100);

    int retry = 0;
    const int maxRetries = 3;
    
    while (!finger.verifyPassword()) {
        // If we have tried too many times, display an error message
        if (retry >= maxRetries) {
            Serial.println("Fingerprint sensor not detected or communication error!");
            isFingerprintSensorOperational = false;  // Set flag to false
            
            // Display error message on LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Sensor Error!");
            lcd.setCursor(0, 1);
            lcd.print("Check Connection");
            
            return false;  // Return false to indicate failure
        }

        // Retry initialization with an increasing delay between retries
        Serial.println("Retrying sensor connection...");
        delay(1000 * (retry + 1));  // Exponential backoff for retries
        retry++;
    }

    // Successfully initialized fingerprint sensor
    Serial.println("Fingerprint sensor initialized successfully!");
    isFingerprintSensorOperational = true;  // Set flag to true

    return true;  // Return true to indicate success
}

//8

bool sendToSupabase(const char* table, const String& jsonData, const String& method, int teacherId) {
    HTTPClient http;
    String url = String(SUPABASE_URL) + "/rest/v1/" + table;

    // Add WHERE clause for PATCH method with teacher_id
    if (method == "PATCH" && teacherId != -1) {
        url += "?teacher_id=eq." + String(teacherId); // Append WHERE clause for teacher_id
    }

    // Initialize HTTP request
    http.begin(url);
    http.addHeader("apikey", SUPABASE_KEY);  // API Key for authentication
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));  // Bearer token for authentication
    http.addHeader("Content-Type", "application/json");  // Content-Type header for JSON payload
    http.addHeader("Prefer", "return=minimal");  // Minimize response data (for PATCH/DELETE)

    // Send request based on the method (POST, PATCH, DELETE)
    int httpCode = 0;
    if (method == "DELETE") {
        httpCode = http.sendRequest("DELETE", jsonData);  // Send DELETE request
    } else if (method == "PATCH") {
        httpCode = http.sendRequest("PATCH", jsonData);   // Send PATCH request
    } else {
        httpCode = http.POST(jsonData);                   // Default to POST request
    }

    // Log HTTP response code and body for debugging
    String response = http.getString();
    Serial.println("HTTP Code: " + String(httpCode));
    Serial.println("Response: " + response);

    // Check if the request was successful (200 or 201)
    bool success = (httpCode == 200 || httpCode == 201);
    if (!success) {
        // Log detailed error message if the request failed
        Serial.println("Error sending to Supabase: " + response);
    }

    // Clean up and close the HTTP connection
    http.end();
    return success;
}

//9

bool checkSerialExists(const String& serial) {
    // Prepare URL for the GET request to check if the serial exists
    String url = String(SUPABASE_URL) + "/rest/v1/ips?serial=eq." + serial;

    // Initialize HTTP client
    HTTPClient http;
    http.begin(url);
    http.addHeader("apikey", SUPABASE_KEY); // API Key for authorization
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY)); // Bearer token
    http.addHeader("Content-Type", "application/json"); // Content-Type header for JSON

    // Send GET request to check if serial exists
    int httpCode = http.GET();
    bool exists = false;

    // Check if the HTTP request was successful (200 OK)
    if (httpCode == 200) {
        String response = http.getString();

        // Log the response for debugging purposes
        Serial.println("Response: " + response);

        // A valid response will typically contain data if the serial exists
        if (response.length() > 2) { // Assuming a valid response will have more than just brackets
            exists = true;
        } else {
            Serial.println("No data found for serial: " + serial);
        }
    } else {
        // Log the error with the HTTP response code
        String errorMsg = "Error checking serial: " + String(httpCode);
        if (httpCode == 0) {
            errorMsg += " (Possible connection error or timeout)";
        }
        Serial.println(errorMsg);
    }

    // Clean up and close the HTTP connection
    http.end();
    return exists;
}

//10
bool updateIPAddress(const String& serial, const String& ip) {
    // Prepare JSON document for the update
    StaticJsonDocument<200> doc;
    doc["ip"] = ip; // New IP address to update
    String jsonString;
    serializeJson(doc, jsonString);

    // Prepare URL for the request
    String url = String(SUPABASE_URL) + "/rest/v1/ips?serial=eq." + serial;

    // Initialize HTTP client and set headers
    HTTPClient http;
    http.begin(url);
    http.addHeader("apikey", SUPABASE_KEY); // Supabase API Key
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY)); // Bearer token for authentication
    http.addHeader("Content-Type", "application/json"); // Content type as JSON
    http.addHeader("Prefer", "return=representation"); // Request updated data on success

    // Send PATCH request to update the IP address
    int httpCode = http.PATCH(jsonString);

    // Check the HTTP response code
    if (httpCode == 200 || httpCode == 204) {
        // If response code is 200 (OK) or 204 (No Content), the update is successful
        Serial.println("IP address updated successfully.");
        http.end();
        return true;
    } else {
        // Handle error if response code is not 200 or 204
        String errorMessage = "Failed to update IP address. HTTP Code: " + String(httpCode);
        if (httpCode > 0) {
            String responseBody = http.getString(); // Get response body for debugging
            Serial.println("Response: " + responseBody);
            errorMessage += "\nResponse Body: " + responseBody;
        } else {
            errorMessage += "\nError: " + String(http.errorToString(httpCode));
        }

        Serial.println(errorMessage);
        http.end();
        return false;
    }
}

//11
bool connectToWiFi(const char* ssid, const char* password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting...");
    Serial.print("Connecting to Wi-Fi");

    int retryCount = 0;
    const int maxRetries = 30;  // Retry up to 30 times (15 seconds)

    while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries) {
        delay(500);
        Serial.print(".");
        retryCount++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to Wi-Fi");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wi-Fi Failed");
        delay(2000);
        return false;
    }

    Serial.println("\nConnected to Wi-Fi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());

    String serial = "1";  // Unique serial number (set according to your system)
    String ip = WiFi.localIP().toString();

    // Check if serial number exists and update the IP or add a new entry
    if (checkSerialExists(serial)) {
        if (updateIPAddress(serial, ip)) {
            Serial.println("IP address updated successfully.");
        } else {
            Serial.println("Failed to update IP address.");
            return false;
        }
    } else {
        StaticJsonDocument<200> doc;
        doc["serial"] = serial;  // Serial number
        doc["ip"] = ip;          // IP address
        String jsonString;
        serializeJson(doc, jsonString);

        if (!sendToSupabase("ips", jsonString)) {
            Serial.println("Failed to send IP address to Supabase");
            return false;
        }
    }

    return true;
}

//12

// Start Wi-Fi in AP mode
void startAccessPoint(const String& apSSID = "ESP32_Config", const String& apPassword = "123456789") {
    // Set Wi-Fi mode to Access Point (AP)
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    
    // Verify if the AP is started successfully
    if (WiFi.softAPgetStationNum() == 0) {
        Serial.println("Error: Failed to start Access Point.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("AP Start Failed");
        lcd.setCursor(0, 1);
        lcd.print("Please try again");
        return; // Exit if AP mode failed to start
    }

    Serial.println("Access Point Started");

    // Get the IP address of the access point
    IPAddress apIP = WiFi.softAPIP();
    
    Serial.print("AP IP Address: ");
    Serial.println(apIP);
    
    // Display on LCD
    lcd.clear(); // Clear the LCD
    lcd.setCursor(0, 0); // Set cursor to the first row
    lcd.print("AP Started");
    lcd.setCursor(0, 1); // Set cursor to the second row
    lcd.print("IP: ");
    lcd.print(apIP); // Display the actual IP address

    // Optionally, print the number of connected clients to serial monitor
    int numClients = WiFi.softAPgetStationNum();
    Serial.print("Number of clients connected: ");
    Serial.println(numClients);
}

//13

// Authenticate User
bool authenticateUser() {
  // Check if the user provided credentials are valid
  if (!server.authenticate(adminUsername, adminPassword)) {
    // Log unauthorized attempt for monitoring purposes
    Serial.println("Authentication failed: Unauthorized access attempt.");
    
    // Send authentication request to the client (browser)
    server.requestAuthentication();

    // Optionally, you can send an HTTP error message instead of just requesting authentication
    server.send(401, "text/html", "<h2>Authentication Required</h2><p>Please enter valid credentials.</p>");

    // Return false to indicate failed authentication
    return false;
  }

  // Log successful authentication for debugging purposes
  Serial.println("Authentication successful.");
  
  // Return true to indicate successful authentication
  return true;
}

//14
void handleRoot() {
    if (!authenticateUser()) {
        return;
    }

    String wifiStatus = "Not connected";
    if (WiFi.status() == WL_CONNECTED) {
        wifiStatus = WiFi.SSID();
    }

    String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Admin Dashboard</title>
        <style>
            body {
                font-family: 'Arial', sans-serif;
                margin: 0;
                padding: 0;
                background: linear-gradient(to right, #74ebd5, #acb6e5);
                display: flex;
                justify-content: center;
                align-items: center;
                height: 100vh;
            }

            .dashboard-container {
                width: 90%;
                height: 90%;
                display: flex;
                flex-direction: column;
                justify-content: flex-start;
                align-items: center;
                background: white;
                border-radius: 16px;
                box-shadow: 0 8px 40px rgba(0, 0, 0, 0.2);
                padding: 30px;
                position: relative;
            }

            .university-logo {
                width: 250px;
                height: auto;
                margin-bottom: 20px;
                transition: transform 0.3s;
            }

            .university-logo:hover {
                transform: scale(1.15);
            }

            .header-text {
                font-size: 36px;
                margin-bottom: 10px;
                color: #2c3e50;
                font-weight: bold;
                text-align: center;
                letter-spacing: 1px;
                text-shadow: 2px 2px 10px rgba(0, 0, 0, 0.1);
            }

            .project-info {
                font-size: 20px;
                color: #555;
                margin-bottom: 30px;
                text-align: center;
                letter-spacing: 1px;
            }

            .wifi-status {
                margin-bottom: 20px;
                padding: 15px;
                background: #f9f9f9;
                border: 1px solid #ddd;
                border-radius: 6px;
                font-size: 18px;
                width: 60%;
                text-align: center;
                box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
            }

            .menu-container {
                display: flex;
                flex-wrap: wrap;
                justify-content: center;
                gap: 15px;
                width: 100%;
                max-width: 900px;
                margin-bottom: 20px;
            }

            .menu-button, .dropbtn, .delete-button {
                font-size: 18px;
                padding: 15px 30px;
                border: none;
                border-radius: 8px;
                color: white;
                text-decoration: none;
                cursor: pointer;
                transition: transform 0.3s, box-shadow 0.3s;
                text-align: center;
                letter-spacing: 1px;
            }

            .menu-button {
                background-color: #4caf50;
                box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
                width: 200px;
            }

            .menu-button:hover {
                background-color: #45a049;
                transform: scale(1.05);
            }

            .dropbtn {
                background-color: #2196f3;
                width: 200px;
                box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
            }

            .dropbtn:hover {
                background-color: #1e88e5;
                transform: scale(1.05);
            }

            .dropdown-content {
                display: none;
                position: absolute;
                background-color: white;
                box-shadow: 0 8px 16px rgba(0, 0, 0, 0.2);
                border-radius: 6px;
                z-index: 1;
                min-width: 160px;
            }

            .dropdown-content a {
                padding: 12px 16px;
                display: block;
                text-decoration: none;
                color: #333;
                font-size: 16px;
                border-bottom: 1px solid #f1f1f1;
            }

            .dropdown-content a:hover {
                background-color: #f1f1f1;
            }

            .dropdown:hover .dropdown-content {
                display: block;
            }

            .delete-button {
                background-color: #ff4444;
                width: 240px;
                font-size: 20px;
                padding: 18px 35px;
                margin-top: 30px;
                box-shadow: 0 4px 10px rgba(255, 0, 0, 0.2);
                text-align: center;
            }

            .delete-button:hover {
                background-color: #ff6b6b;
                transform: scale(1.1);
            }

            @media (max-width: 480px) {
                .menu-container {
                    flex-direction: column;
                }

                .menu-button, .dropbtn, .delete-button {
                    width: 90%;
                    margin-bottom: 10px;
                }
            }
        </style>
    </head>
    <body>
        <div class="dashboard-container">
            <img src="https://just.edu.bd/logo/download.png" alt="JUST Logo" class="university-logo">
            <h1 class="header-text">Jashore University of Science and Technology</h1>
            <p class="project-info">Department of Electrical and Electronic Engineering<br>Project by Musavvir,Rifat,Tanim,Pitom</p>
            <p class="wifi-status">Current Wi-Fi Status: )rawliteral" + wifiStatus + R"rawliteral(</p>
            <div class="menu-container">
                <div class="dropdown">
                    <button class="dropbtn">Teacher</button>
                    <div class="dropdown-content">
                        <a href="/enrollTeacher">Enroll Teacher</a>
                        <a href="/downloadTeachers">Download Teacher Data</a>
                        <a href="/editTeacher">Edit Teachers</a>
                    </div>
                </div>
                <div class="dropdown">
                    <button class="dropbtn">Student</button>
                    <div class="dropdown-content">
                        <a href="/enrollStudent">Enroll Student</a>
                        <a href="/downloadStudents">Download Student Data</a>
                        <a href="/editStudent">Edit Students</a>
                    </div>
                </div>
                <a href="/downloadAttendance" class="menu-button">View Attendance</a>
                <a href="/wifi" class="menu-button" onmouseover="this.style.transform='scale(1.05)'" onmouseout="this.style.transform='scale(1)'">Change Wi-Fi Settings</a>
            </div>
            <a href="/deleteAllFiles" 
               class="delete-button"
               onclick="return confirm('This action will permanently delete all CSV files. Proceed?')">
               Delete All CSV Files
            </a>
        </div>
    </body>
    </html>
    )rawliteral";

    server.send(200, "text/html", htmlPage);
}
//15

void handleWifiSettings() {
    if (!authenticateUser()) {
        return;
    }

    // Perform Wi-Fi scan
    int networkCount = WiFi.scanNetworks();
    String options = "";

    if (networkCount == 0) {
        options = "<option value='' disabled>No networks found</option>";
    } else {
        for (int i = 0; i < networkCount; i++) {
            String ssid = WiFi.SSID(i);
            options += "<option value='" + ssid + "'>" + ssid + "</option>";
        }
    }

    // HTML page for Wi-Fi settings
    String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Change Wi-Fi Settings</title>
        <style>
            body {
                font-family: 'Arial', sans-serif;
                margin: 0;
                padding: 0;
                background: linear-gradient(to right, #74ebd5, #acb6e5);
                display: flex;
                justify-content: center;
                align-items: center;
                height: 100vh;
            }
            .wifi-settings-container {
                background: white;
                padding: 30px;
                border-radius: 16px;
                box-shadow: 0 8px 40px rgba(0, 0, 0, 0.2);
                width: 90%;
                max-width: 500px;
                text-align: center;
            }
            .wifi-settings-container h2 {
                font-size: 28px;
                color: #2c3e50;
                margin-bottom: 20px;
            }
            .wifi-settings-container label {
                display: block;
                font-size: 16px;
                color: #555;
                text-align: left;
                margin-bottom: 8px;
            }
            .wifi-settings-container select, .wifi-settings-container input[type="text"], .wifi-settings-container input[type="password"] {
                width: 100%;
                padding: 10px;
                margin-bottom: 20px;
                border: 1px solid #ddd;
                border-radius: 8px;
                box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.1);
                font-size: 16px;
            }
            .wifi-settings-container select:focus, .wifi-settings-container input[type="text"]:focus, .wifi-settings-container input[type="password"]:focus {
                outline: none;
                border-color: #4caf50;
                box-shadow: 0 0 8px rgba(76, 175, 80, 0.3);
            }
            .wifi-settings-container input[type="submit"] {
                background-color: #4caf50;
                color: white;
                padding: 12px 20px;
                font-size: 18px;
                border: none;
                border-radius: 8px;
                cursor: pointer;
                transition: all 0.3s ease;
                width: 100%;
            }
            .wifi-settings-container input[type="submit"]:hover {
                background-color: #45a049;
                box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
                transform: scale(1.05);
            }
            .status-message {
                font-size: 14px;
                color: #888;
                margin-top: 10px;
            }
        </style>
    </head>
    <body>
        <div class="wifi-settings-container">
            <h2>Change Wi-Fi Settings</h2>
            <form action="/saveWifi" method="POST">
                <label for="ssid">Select SSID:</label>
                <select id="ssid" name="ssid">
                    <option value="">Select a network</option>
                    )rawliteral" + options + R"rawliteral(
                </select>
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" placeholder="Enter Wi-Fi Password">
                <input type="submit" value="Save Wi-Fi Settings">
            </form>
            <p class="status-message">Please select a Wi-Fi network and enter the password.</p>
        </div>
    </body>
    </html>
    )rawliteral";

    // Send the HTML page to the client
    server.send(200, "text/html", htmlPage);
}

//16

void handleSaveWifi() {
    if (!server.hasArg("ssid") || !server.hasArg("password")) {
        // If SSID or password is missing, return a detailed error message
        server.send(400, "text/html", "<h2>Error: Missing SSID or password.</h2>");
        return;
    }

    // Get SSID and password from the HTTP request
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    // Validate SSID and password length
    if (ssid.length() == 0 || password.length() == 0) {
        server.send(400, "text/html", "<h2>Error: SSID or password cannot be empty.</h2>");
        return;
    }

    // Save SSID and password to preferences
    preferences.begin("WiFi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    // Log the saving of credentials for debugging purposes
    Serial.println("WiFi credentials saved:");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);

    // Send a successful response and redirect the user
    server.sendHeader("Location", "https://rifaaateee.github.io/Attendance-System/");
    server.send(303);  // 303 See Other

    // Allow the user to see the redirection for 2 seconds
    delay(2000);  // Wait for 2 seconds to let the redirect be processed

    // Restart the ESP32 to apply the new settings
    Serial.println("Restarting ESP32 to apply WiFi settings...");
    ESP.restart();
}

//17
// Teacher enrollment page
void handleEnrollTeacher() {
  if (!authenticateUser()) {
    return;
  }

  String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Teacher Enrollment</title>
      <style>
        body {
          font-family: Arial, sans-serif;
          background-color: #f4f4f9;
          margin: 0;
          padding: 20px;
        }
        h2 {
          text-align: center;
          color: #333;
        }
        form {
          margin: 0 auto;
          max-width: 400px;
          padding: 20px;
          background-color: white;
          border: 1px solid #ddd;
          border-radius: 8px;
        }
        label {
          font-weight: bold;
        }
        input[type="text"] {
          width: 100%;
          padding: 8px;
          margin: 10px 0 20px 0;
          border: 1px solid #ccc;
          border-radius: 4px;
        }
        input[type="submit"] {
          background-color: #4CAF50;
          color: white;
          border: none;
          padding: 10px 20px;
          cursor: pointer;
          border-radius: 4px;
          font-size: 16px;
        }
        input[type="submit"]:hover {
          background-color: #45a049;
        }
      </style>
    </head>
    <body>
      <h2>Teacher Enrollment</h2>
      <form action="/enrollTeacherSubmit" method="POST">
        <label for="teacherId">Teacher ID (SL):</label><br>
        <input type="text" id="teacherId" name="teacherId" required><br><br>
        
        <label for="teacherName">Teacher Name:</label><br>
        <input type="text" id="teacherName" name="teacherName" required><br><br>
        
        <label for="courseName">Course Name:</label><br>
        <input type="text" id="courseName" name="courseName" required><br><br>
        
        <label for="coursecode">Course Code:</label><br>
        <input type="text" id="coursecode" name="coursecode" required><br><br>
        
        <input type="submit" value="Enroll Teacher">
      </form>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", htmlPage);
}

//18

// Enroll a teacher and take attendance
void enrollTeacher(int teacherId, const String& teacherName, const String& courseName, const String& coursecode) {
    // Initialize the fingerprint sensor
    if (!initializeFingerprint()) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sensor Error!");
        lcd.setCursor(0, 1);
        lcd.print("Try Again Later");
        delay(3000);
        return;
    }

    // Display initial message
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enrolling Teacher");
    lcd.setCursor(0, 1);
    lcd.print("ID: " + String(teacherId));
    delay(2000);

    // Start fingerprint enrollment process
    Serial.print("Starting enrollment for Teacher ID: ");
    Serial.println(teacherId);

    // Directly initialize the fingerprint sensor
    mySerial.begin(57600);
    delay(1000);
    finger.begin(57600);

    int status;

    // Wait for the teacher's finger to be placed on the sensor
    while ((status = finger.getImage()) != FINGERPRINT_OK) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Place your finger");
        lcd.setCursor(0, 1);
        lcd.print("on the sensor.");
        delay(2000);
        Serial.println("Place your finger on the sensor.");
    }

    // Capture the first fingerprint
    Serial.println("Fingerprint image taken.");
    if (finger.image2Tz(1) != FINGERPRINT_OK) {
        Serial.println("Failed to create template for the first image.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Error creating");
        lcd.setCursor(0, 1);
        lcd.print("first template.");
        delay(2000);
        return;
    }

    // Prompt user for the same finger again
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Place same finger");
    lcd.setCursor(0, 1);
    lcd.print("on the sensor.");
    delay(2000);
    Serial.println("Place the same finger again.");

    // Wait for the second fingerprint
    while ((status = finger.getImage()) != FINGERPRINT_OK) {
        Serial.println("Waiting for second fingerprint image.");
        delay(1000);
    }

    // Capture the second fingerprint
    if (finger.image2Tz(2) != FINGERPRINT_OK) {
        Serial.println("Failed to create template for the second image.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Error creating");
        lcd.setCursor(0, 1);
        lcd.print("second template.");
        delay(2000);
        return;
    }

    // Create the fingerprint model
    if (finger.createModel() != FINGERPRINT_OK) {
        Serial.println("Failed to create fingerprint model.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Model creation");
        lcd.setCursor(0, 1);
        lcd.print("failed.");
        delay(2000);
        return;
    }

    // Store the fingerprint model
    if (finger.storeModel(teacherId) != FINGERPRINT_OK) {
        Serial.println("Failed to store fingerprint model.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Failed to store");
        lcd.setCursor(0, 1);
        lcd.print("fingerprint.");
        delay(2000);
        return;
    }

    // Successful enrollment message
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print("Enrollment Success");
    delay(2000);
    Serial.println("Fingerprint enrollment successful.");

    // Save teacher data to local storage and SD card
    saveTeacherData(teacherId, teacherName, courseName, coursecode);
}

//19
void saveTeacherData(int teacherId, const String& teacherName, const String& courseName, const String& coursecode) {
    String dataLine = String(teacherId) + "," + teacherName + "," + courseName + "," + coursecode + "\n";

    // Save to local storage (SPIFFS)
    File file = SPIFFS.open(teacherDataFile, FILE_APPEND);
    if (!file) {
        Serial.println("Error: Failed to open teacher data file in SPIFFS for appending.");
        return;
    }
    file.print(dataLine);
    file.close();
    Serial.println("Teacher data saved to SPIFFS.");

    // Save to SD card
    File sdFile = SD.open(teacherDataFile, FILE_APPEND);
    if (sdFile) {
        sdFile.print(dataLine);
        sdFile.close();
        Serial.println("Teacher data saved to SD card.");
    } else {
        Serial.println("Error: Failed to open teacher data file on SD card!");
    }

    // Prepare JSON for Supabase
    StaticJsonDocument<300> doc;
    doc["teacher_id"] = teacherId;
    doc["name"] = teacherName;
    doc["course_name"] = courseName;
    doc["course_code"] = coursecode;

    String jsonString;
    serializeJson(doc, jsonString);

    // Send to Supabase if connected to WiFi
    if (WiFi.status() == WL_CONNECTED) {
        if (sendToSupabase("teachers", jsonString)) {
            Serial.println("Teacher data sent to Supabase successfully.");
        } else {
            Serial.println("Error: Failed to send teacher data to Supabase.");
        }
    } else {
        Serial.println("Error: Not connected to WiFi. Teacher data not sent to Supabase.");
    }
}

//20

// Serve the teacher data file for download
void handleDownloadTeachers() {
  if (!authenticateUser()) {
    server.send(403, "text/html", "<h2>Authentication Failed. Access Denied!</h2>");
    return;
  }

  File file = SPIFFS.open(teacherDataFile, FILE_READ);
  if (!file) {
    String errorMessage = "<h2>Failed to open teacher data file. Please try again later.</h2>";
    server.send(500, "text/html", errorMessage);
    return;
  }

  // Check if file is empty
  if (file.size() == 0) {
    String noDataMessage = "<h2>No teacher data available. The file is empty.</h2>";
    server.send(200, "text/html", noDataMessage);
    file.close();
    return;
  }

  // Set content type and stream the file
  server.streamFile(file, "text/csv");
  file.close();
}

//21

// Student enrollment page
void handleEnrollStudent() {
  if (!authenticateUser()) {
    server.send(403, "text/html", "<h2>Authentication Failed. Access Denied!</h2>");
    return;
  }

  String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Student Enrollment</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                background-color: #f4f4f9;
                margin: 0;
                padding: 20px;
            }
            h2 {
                color: #333;
            }
            label {
                font-weight: bold;
            }
            input[type="text"], input[type="submit"] {
                width: 100%;
                padding: 10px;
                margin: 8px 0;
                border: 1px solid #ccc;
                border-radius: 5px;
                box-sizing: border-box;
            }
            input[type="submit"] {
                background-color: #4CAF50;
                color: white;
                border: none;
            }
            input[type="submit"]:hover {
                background-color: #45a049;
            }
        </style>
    </head>
    <body>
        <h2>Student Enrollment</h2>
        <p>Please enter the details of the student to enroll:</p>
        <form action="/enrollStudentSubmit" method="POST">
          <label for="studentId">Student ID (SL):</label><br>
          <input type="text" id="studentId" name="studentId" required><br><br>
          
          <label for="studentName">Student Name:</label><br>
          <input type="text" id="studentName" name="studentName" required><br><br>
          
          <label for="stdroll">Student Roll Number:</label><br>
          <input type="text" id="stdroll" name="stdroll" required><br><br>
          
          <input type="submit" value="Enroll Student">
        </form>

        <br>
        <a href="/">Go back to Home</a>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", htmlPage);
}

//22


void enrollStudent(int studentId, const String& studentName, const String& stdroll) {
    if (!initializeFingerprint()) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sensor Error!");
        lcd.setCursor(0, 1);
        lcd.print("Try Again Later");
        delay(3000);
        return;
    }

    Serial.print("Starting enrollment for Student ID: ");
    Serial.println(studentId);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("StR ENR st id");
    lcd.setCursor(0, 1);
    lcd.print(stdroll);
    delay(2000);

    // Initialize fingerprint sensor
    mySerial.begin(57600);
    delay(1000);
    finger.begin(57600);

    // Capture first fingerprint image
    int status;
    while ((status = finger.getImage()) != FINGERPRINT_OK) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("place :");
        lcd.setCursor(0, 1);
        lcd.print("finger");
        Serial.println("Place your finger on the sensor.");
        delay(1000);
    }

    Serial.println("Fingerprint image taken.");
    if (finger.image2Tz(1) != FINGERPRINT_OK) {
        Serial.println("Failed to create template.");
        return;
    }

    // Capture second fingerprint image for validation
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("place same:");
    lcd.setCursor(0, 1);
    lcd.print("finger again");
    delay(2000);
    Serial.println("Place the same finger again.");
    while ((status = finger.getImage()) != FINGERPRINT_OK) {
        Serial.println("Waiting for second fingerprint image.");
        delay(1000);
    }

    if (finger.image2Tz(2) != FINGERPRINT_OK) {
        Serial.println("Failed to create second template.");
        return;
    }

    // Create and store fingerprint model
    if (finger.createModel() != FINGERPRINT_OK) {
        Serial.println("Failed to create fingerprint model.");
        return;
    }

    if (finger.storeModel(studentId) != FINGERPRINT_OK) {
        Serial.println("Failed to store fingerprint model.");
        return;
    }

    // Final success message
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint Enroll");
    lcd.setCursor(0, 1);
    lcd.print("success");
    delay(2000);
    Serial.println("Fingerprint enrollment successful.");

    // Save student data locally and to SD card
    File file = SPIFFS.open("/students.csv", FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open student data file for appending.");
        return;
    }
    String dataLine = String(studentId) + "," + studentName + "," + stdroll + "\n";
    file.print(dataLine);
    file.close();

    File sdFile = SD.open("/students.csv", FILE_APPEND);
    if (sdFile) {
        sdFile.print(dataLine);
        sdFile.close();
    } else {
        Serial.println("Failed to open student data file on SD card!");
    }

    // Prepare JSON for Supabase
    StaticJsonDocument<200> doc;
    doc["student_id"] = studentId;
    doc["name"] = studentName;
    doc["roll_number"] = stdroll;

    String jsonString;
    serializeJson(doc, jsonString);

    // Send to Supabase
    if (WiFi.status() == WL_CONNECTED) {
        sendToSupabase("students", jsonString);
    }
}

//23

void saveStudentData(int studentId, const String& studentName, const String& stdroll) {
    String dataLine = String(studentId) + "," + studentName + "," + stdroll + "\n";

    // Save to SPIFFS
    File file = SPIFFS.open(studentDataFile, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open student data file for appending in SPIFFS.");
    } else {
        file.print(dataLine);
        file.close();
    }

    // Save to SD Card
    File sdFile = SD.open(studentDataFile, FILE_APPEND);
    if (!sdFile) {
        Serial.println("Failed to open student data file for appending on SD card.");
    } else {
        sdFile.print(dataLine);
        sdFile.close();
    }

    // Prepare JSON for Supabase
    StaticJsonDocument<200> doc;
    doc["student_id"] = studentId;
    doc["name"] = studentName;
    doc["roll_number"] = stdroll;

    String jsonString;
    serializeJson(doc, jsonString);

    // Send to Supabase if connected to WiFi
    if (WiFi.status() == WL_CONNECTED) {
        sendToSupabase("students", jsonString);
    }
}


//24
void handleDownloadStudents() {
    if (!authenticateUser()) {
        // If authentication fails, send a 401 Unauthorized response
        server.send(401, "text/html", "<h2>Unauthorized Access. Please log in.</h2>");
        return;
    }

    // Try to open the student data file
    File file = SPIFFS.open(studentDataFile, FILE_READ);
    if (!file) {
        // Send a detailed error message if the file fails to open
        server.send(500, "text/html", "<h2>Failed to open student data file. Please try again later.</h2>");
        return;
    }

    // Send the file stream with the correct MIME type (CSV file)
    server.streamFile(file, "text/csv");

    // Close the file after streaming
    file.close();
}

//25

void handleAttendanceMode() {
    if (!authenticateUser()) {
        // Return if authentication fails
        return;
    }

    // Construct the HTML page for starting the attendance mode
    String htmlPage = R"rawliteral(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <meta http-equiv="refresh" content="3;url=/attendanceMode"> <!-- Redirect after 3 seconds -->
            <title>Starting Attendance Mode</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    text-align: center;
                    margin-top: 50px;
                }
                h2 {
                    color: #4CAF50;
                }
                .message {
                    font-size: 18px;
                }
            </style>
        </head>
        <body>
            <h2>Attendance Mode Starting...</h2>
            <p class="message">Redirecting to attendance mode...</p>
            <script>
                // Optional: Provide immediate redirection in case the meta refresh doesn't work
                setTimeout(function() {
                    window.location.href = '/attendanceMode';
                }, 3000); // 3 seconds delay
            </script>
        </body>
        </html>
    )rawliteral";

    // Send the HTML response to the client
    server.send(200, "text/html", htmlPage);
}

//26


bool isTeacherFingerprintValid(int teacherId, const String& coursecode) {
    // Check if the teacher data file exists
    if (!SPIFFS.exists(teacherDataFile)) {
        Serial.println("Teacher data file does not exist!");
        return false;  // Return false if the file does not exist
    }

    // Open the teacher CSV file for reading
    File file = SPIFFS.open(teacherDataFile, "r");
    if (!file) {
        Serial.println("Failed to open teacher data file!");
        return false;  // Return false if the file cannot be opened
    }

    // Read each line from the teacher data file
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();  // Trim any extra whitespace
        if (line.length() == 0) continue;  // Skip empty lines

        // Parse the CSV line into components
        int comma1 = line.indexOf(',');
        int comma2 = line.lastIndexOf(',');

        // Ensure there are two commas (for ID and course code), and they are not the same
        if (comma1 == -1 || comma2 == -1 || comma1 == comma2) {
            continue;  // Skip malformed lines
        }

        String idStr = line.substring(0, comma1);
        String courseCodeStr = line.substring(comma2 + 1);

        // Validate teacher ID and course code
        if (idStr.toInt() == teacherId && courseCodeStr.equalsIgnoreCase(coursecode)) {
            file.close();  // Close file before returning
            Serial.println("Teacher fingerprint verified for course: " + coursecode);
            return true;  // Valid teacher ID and course code found
        }
    }

    file.close();  // Ensure file is closed after reading
    Serial.println("Teacher ID and course code do not match.");
    return false;  // No match found for teacher ID and course code
}



//27




int getFingerprintID() {
    // Capture the fingerprint image
    uint8_t p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
        // Inform the user that no finger was detected and wait for input
        Serial.println("No finger detected. Please place your finger.");
        return -1;  // Return early if no finger is detected
    }

    if (p != FINGERPRINT_OK) {
        // Handle other capture errors and provide feedback
        Serial.println("Error capturing fingerprint image. Error code: " + String(p));
        return -1;  // Return early for other capture errors
    }

    // Convert the captured image to a template
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        // Inform the user about the conversion failure
        Serial.println("Error converting fingerprint image. Error code: " + String(p));
        return -1;
    }

    // Perform a fast search for the fingerprint in the database
    p = finger.fingerFastSearch();
    if (p != FINGERPRINT_OK) {
        // If no match is found, inform the user and return an error
        Serial.println("No matching fingerprint found. Error code: " + String(p));
        return -1;
    }

    // Return the ID of the matched fingerprint
    Serial.println("Fingerprint matched successfully! ID: " + String(finger.fingerID));
    return finger.fingerID;
}




//28
void clearSensorBuffer() {
    unsigned long startTime = millis();  // Track the start time to avoid infinite waiting
    unsigned long timeout = 5000; // Timeout after 5 seconds

    while (finger.getImage() != FINGERPRINT_NOFINGER) {
        delay(100); // Wait for sensor to report no finger

        // Check if the timeout has been exceeded
        if (millis() - startTime > timeout) {
            Serial.println("Error: Fingerprint sensor did not clear in time.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Sensor Timeout");
            delay(2000); // Show the error message on LCD for 2 seconds
            return; // Exit the function if timeout occurs
        }
    }

    // If the sensor cleared successfully, notify the user
    Serial.println("Sensor buffer cleared successfully.");
}


//29

bool verifyTeacherFingerprint(const String &coursecode) {
    int teacherId;
    clearSensorBuffer();  // Ensure the sensor buffer is clear before starting

    while (true) {
        teacherId = getFingerprintID();  // Fetch fingerprint ID

        // No finger detected, prompt the user to try again
        if (teacherId == -1) {
            Serial.println("No fingerprint detected. Please place your finger.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("No finger found");
            lcd.setCursor(0, 1);
            lcd.print("Try again...");
            delay(2000);
            continue;  // Keep waiting until a fingerprint is detected
        }

        // Check if the fingerprint matches the teacher ID and course code
        if (isTeacherFingerprintValid(teacherId, coursecode)) {
            Serial.println("Teacher fingerprint verified successfully.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Verification");
            lcd.setCursor(0, 1);
            lcd.print("Success!");
            delay(2000);
            return true;  // Verification successful
        } else {
            Serial.println("Fingerprint detected but verification failed.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Verification");
            lcd.setCursor(0, 1);
            lcd.print("Failed!");
            delay(2000);
            return false;  // Verification failed
        }
    }
}


//30

int processStudentAttendance(const String &coursecode) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan Student Finger");
    Serial.println("Awaiting student fingerprint...");

    while (true) {
        int result = finger.getImage();
        if (result == FINGERPRINT_NOFINGER) {
            delay(100);
            continue;
        }

        if (result != FINGERPRINT_OK) {
            Serial.println("Error capturing fingerprint image.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Error Reading");
            lcd.setCursor(0, 1);
            lcd.print("Try Again");
            delay(2000);
            return -2;
        }

        result = finger.image2Tz();
        if (result != FINGERPRINT_OK) {
            Serial.println("Error converting fingerprint image.");
            return -2;
        }

        result = finger.fingerFastSearch();
        if (result != FINGERPRINT_OK) {
            Serial.println("Fingerprint not recognized!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Invalid Finger");
            delay(2000);
            return -1;
        }

        int studentId = finger.fingerID;

        // Check if student exists in the database
        String studentName = fetchStudentName(studentId);
        if (studentName.isEmpty() || studentName == "Unknown Student") {
            Serial.println("Fingerprint matched but student not in database!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Unknown Student");
            lcd.setCursor(0, 1);
            lcd.print("Not Recorded");
            delay(2000);
            return -4; // New error code for unknown student
        }

        if (studentId < 0 || studentId >= MAX_STUDENTS) {
            Serial.println("Invalid student ID.");
            return -1;
        }

        if (attendedStudentsarray[studentId]) {
            Serial.println("Duplicate attendance detected.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Already Marked");
            lcd.setCursor(0, 1);
            lcd.print("Attendance");
            delay(2000);
            return -3; // Error code for duplicate attendance
        }

        attendedStudentsarray[studentId] = true;
        Serial.println("Fingerprint recognized. ID: " + String(studentId));
        
        // Once everything is processed, return studentId
        return studentId;
    }
}





//31
String fetchStudentName(int studentId) {
    // Open the student file in read mode
    File studentFile = SPIFFS.open("/students.csv", FILE_READ);
    if (!studentFile) {
        Serial.println("Failed to open students.csv file!");
        return "";
    }

    // Loop through the file line by line
    while (studentFile.available()) {
        String line = studentFile.readStringUntil('\n');
        
        // Find the first comma (student ID) and last comma (name end)
        int idIndex = line.indexOf(',');
        int nameIndex = line.lastIndexOf(',');

        if (idIndex == -1 || nameIndex == -1 || idIndex == nameIndex) {
            continue;  // Skip malformed lines
        }

        String id = line.substring(0, idIndex); // Extract student ID
        String name = line.substring(idIndex + 1, nameIndex); // Extract student name

        // Compare the extracted ID with the studentId
        if (id.toInt() == studentId) {
            studentFile.close();
            return name;  // Return the name if match is found
        }
    }

    // If student ID is not found
    studentFile.close();
    return "Unknown Student";  // Return a default name if not found
}



// void attendanceMode() {
//     if (!initializeFingerprint()) {
//         lcd.clear();
//         lcd.setCursor(0, 0);
//         lcd.print("Sensor Error!");
//         lcd.setCursor(0, 1);
//         lcd.print("Try Again Later");
//         delay(3000);
//         return;
//     }
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Attendance Mode");
//     lcd.setCursor(0, 1);
//     lcd.print("Starting...");
//     Serial.println("Attendance mode started.");

//     String coursecode = "";

//     // Reset the attendance tracking array
//     memset(attendedStudentsarray, false, sizeof(attendedStudentsarray));

//     // Prompt for course code
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Enter Course Code:");
//     Serial.println("Awaiting course code...");

//     while (true) {
//         char key = keypad.getKey();
//         if (key) {
//             if (key == '#') {  // End of input
//                 Serial.println("Course code entered: " + coursecode);
//                 break;
//             } else {
//                 coursecode += key;
//                 lcd.print(key);
//                 Serial.print(key);
//             }
//         }
//     }

//     // Verify teacher fingerprint
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Verify Teacher...");
//     Serial.println("Place teacher's finger on the sensor.");

//     if (!verifyTeacherFingerprint(coursecode)) {
//         lcd.clear();
//         lcd.setCursor(0, 0);
//         lcd.print("Verification Failed");
//         Serial.println("Teacher fingerprint verification failed.");
//         delay(3000);
//         return;
//     }

//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Attendance Start");
//     Serial.println("Attendance started for course: " + coursecode);

//     while (true) {
//         lcd.clear();
//         lcd.setCursor(0, 0);
//         lcd.print("Awaiting Student");

//         int studentId = processStudentAttendance(coursecode);
           
//         if (studentId > 0) {
//             String studentName = fetchStudentName(studentId);
//             // Double check to ensure student exists in database
//             if (!studentName.isEmpty() && studentName != "Unknown Student") {
//                 saveAttendance(coursecode, studentName, studentId);
//                 lcd.clear();
//                 lcd.setCursor(0, 0);
//                 lcd.print("Marked for:");
//                 lcd.setCursor(0, 1);
//                 lcd.print(studentName);
//                 Serial.println("Attendance recorded for: " + studentName);
//                 delay(2000);
//             }
//         } else if (studentId == -1) {
//             Serial.println("Invalid fingerprint. Attendance not recorded.");
//         } else if (studentId == -3) {
//             Serial.println("Duplicate attendance. Skipping.");
//         } else if (studentId == -4) {
//             Serial.println("Unknown student. Attendance not recorded.");
//         }

//         // Allow teacher to end attendance session
//         if (verifyTeacherFingerprint(coursecode)) {
//             lcd.clear();
//             lcd.setCursor(0, 0);
//             lcd.print("Attendance Ended");
//             Serial.println("Attendance session ended.");
//             delay(3000);
//             break;
//         }
//     }
// }

//33
void saveAttendance(String coursecode, String studentName, int studentId) {
    DateTime now = rtc.now(); // Get current time from RTC
    char timestamp[32];
    sprintf(timestamp, "%04d-%02d-%02d", now.year(), now.month(), now.day()); // Format the date

    // Save to SD Card
    File sdAttendanceFile = SD.open("/attendance.csv", FILE_APPEND);
    if (!sdAttendanceFile) {
        Serial.println("Failed to open attendance file on SD card!");
    } else {
        String record = coursecode + "," + studentName + "," + String(studentId) + "," + String(timestamp) + "\n";
        sdAttendanceFile.print(record);
        sdAttendanceFile.close();
        Serial.println("Attendance saved to SD card: " + record);
    }

    // Save locally
    File attendanceFile = SPIFFS.open("/attendance.csv", FILE_APPEND);
    if (!attendanceFile) {
        Serial.println("Failed to open attendance file!");
        return;
    }

    String record = coursecode + "," + studentName + "," + String(studentId) + "," + String(timestamp) + "\n";
    attendanceFile.print(record);
    attendanceFile.close();
    Serial.println("Attendance saved: " + record);

    // Prepare JSON for Supabase
    StaticJsonDocument<300> doc;
    doc["course_code"] = coursecode;
    doc["student_name"] = studentName;
    doc["student_id"] = studentId;
    doc["timestamp"] = timestamp;

    String jsonString;
    serializeJson(doc, jsonString);

    // Send to Supabase
    if (WiFi.status() == WL_CONNECTED) {
        sendToSupabase("attendance", jsonString);
    }
}


//34

void handleDownloadAttendance() {
    if (!authenticateUser()) {
        return;
    }

    // Start HTML page
    String htmlPage = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Download Attendance</title>
            <style>
                .container {
                    max-width: 800px;
                    margin: 0 auto;
                    padding: 20px;
                    font-family: Arial, sans-serif;
                }
                h2 {
                    color: #333;
                }
                .option-button {
                    display: inline-block;
                    padding: 10px 20px;
                    margin: 10px;
                    background-color: #4CAF50;
                    color: white;
                    text-decoration: none;
                    border-radius: 5px;
                    cursor: pointer;
                }
                select {
                    padding: 8px;
                    margin: 10px 0;
                    width: 250px;
                    font-size: 14px;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <h2>Select Course Code to Download Attendance</h2>
                <select id="courseCode">
    )rawliteral";

    // Read course codes and names from teachers.csv
    File teacherFile = SPIFFS.open("/teachers.csv", "r");
    if (teacherFile) {
        std::map<String, String> courseMap;  // courseCode -> courseName
        
        while (teacherFile.available()) {
            String line = teacherFile.readStringUntil('\n');
            int firstComma = line.indexOf(',');
            int secondComma = line.indexOf(',', firstComma + 1);
            int thirdComma = line.indexOf(',', secondComma + 1);
            
            if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
                String courseName = line.substring(secondComma + 1, thirdComma);
                String courseCode = line.substring(thirdComma + 1);
                courseCode.trim();
                if (courseCode.length() > 0) {
                    courseMap[courseCode] = courseName;
                }
            }
        }
        teacherFile.close();

        // Add options for each unique course code
        for (const auto& course : courseMap) {
            htmlPage += "    <option value=\"" + course.first + "\">" + 
                       course.second + " (" + course.first + ")</option>\n";
        }
    }

    htmlPage += R"rawliteral(
                </select>
                <br><br>
                <a href="#" onclick="viewTotalAttendance()" class="option-button">View Total Attendance</a>
                <a href="#" onclick="viewDatewiseAttendance()" class="option-button">View Date-wise Attendance</a>

                <div id="attendanceData"></div>

                <script>
                    function viewTotalAttendance() {
                        var courseCode = document.getElementById('courseCode').value;
                        window.location.href = '/attendanceDetails?courseCode=' + courseCode + '&viewType=total';
                    }

                    function viewDatewiseAttendance() {
                        var courseCode = document.getElementById('courseCode').value;
                        window.location.href = '/attendanceDetails?courseCode=' + courseCode + '&viewType=datewise';
                    }
                </script>
            </div>
        </body>
        </html>
    )rawliteral";

    // Send the generated HTML page
    server.send(200, "text/html", htmlPage);
}

//36

void handleViewAttendance() {
    if (!authenticateUser()) {
        return;
    }

    // Start HTML page
    String htmlPage = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Attendance View</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    margin: 20px;
                    line-height: 1.6;
                }
                h2 {
                    color: #333;
                }
                form {
                    margin-top: 20px;
                }
                select, input[type="submit"] {
                    margin-top: 10px;
                    padding: 8px;
                    font-size: 14px;
                }
                a {
                    display: block;
                    margin-top: 10px;
                    color: #007BFF;
                    text-decoration: none;
                }
                a:hover {
                    text-decoration: underline;
                }
            </style>
        </head>
        <body>
            <h2>Attendance View</h2>
            <form action="/attendanceDetails" method="GET">
                <label for="courseCode">Select Course Code:</label>
                <select name="courseCode" id="courseCode">
    )rawliteral";

    // Fetch unique course codes from teachers.csv
    std::set<String> courseCodes;
    File teacherFile = SPIFFS.open("/teachers.csv", "r");
    if (teacherFile) {
        while (teacherFile.available()) {
            String line = teacherFile.readStringUntil('\n');
            int lastComma = line.lastIndexOf(',');
            if (lastComma != -1) {
                String courseCode = line.substring(lastComma + 1);
                courseCodes.insert(courseCode);
            }
        }
        teacherFile.close();
    }

    // Add course code options to the dropdown
    for (const String& courseCode : courseCodes) {
        htmlPage += "<option value=\"" + courseCode + "\">" + courseCode + "</option>";
    }

    // Add links for total and date-wise attendance options
    htmlPage += R"rawliteral(
                </select>
                <input type="submit" value="View Attendance">
            </form>
            <h3>Quick Links:</h3>
    )rawliteral";

    for (const String& courseCode : courseCodes) {
        htmlPage += "<a href=\"/attendanceDetails?courseCode=" + courseCode + "&viewType=total\">Total Attendance for " + courseCode + "</a>";
        htmlPage += "<a href=\"/attendanceDetails?courseCode=" + courseCode + "&viewType=datewise\">Date-wise Attendance for " + courseCode + "</a>";
    }

    htmlPage += R"rawliteral(
            <a href="/datewiseDownload">Download Date-wise Attendance</a>
        </body>
        </html>
    )rawliteral";

    // Send the generated HTML page
    server.send(200, "text/html", htmlPage);
}


//37
void handleAttendanceDetails1() {
    if (!authenticateUser()) {
        return;
    }

    String courseCode = server.arg("courseCode");
    String viewType = server.arg("viewType");

    String htmlPage = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Attendance Details - )rawliteral" + courseCode + R"rawliteral(</title>
            <style>
                table {
                    border-collapse: collapse;
                    width: 100%;
                }
                th, td {
                    border: 1px solid black;
                    padding: 8px;
                    text-align: left;
                }
            </style>
        </head>
        <body>
            <h2>Attendance Details for )rawliteral" + courseCode + R"rawliteral(</h2>
    )rawliteral";

    if (viewType == "total") {
        // Fetch attendance data and calculate total classes
        std::map<String, int> attendanceData;
        int totalClasses = 0;
        if (File attendanceFile = SPIFFS.open("/attendance.csv", "r")) {
            while (attendanceFile.available()) {
                String line = attendanceFile.readStringUntil('\n');
                int firstComma = line.indexOf(',');
                int secondComma = line.indexOf(',', firstComma + 1);
                int thirdComma = line.indexOf(',', secondComma + 1);
                if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
                    String fileCourseCode = line.substring(0, firstComma);
                    String studentRoll = line.substring(secondComma + 1, thirdComma);
                    if (fileCourseCode == courseCode) {
                        attendanceData[studentRoll]++;
                        totalClasses++;
                    }
                }
            }
            attendanceFile.close();
        }

        // Generate table for total attendance
        htmlPage += R"rawliteral(
            <table>
                <tr>
                    <th>Roll</th>
                    <th>Name</th>
                    <th>Attendance</th>
                    <th>Marks</th>
                </tr>
        )rawliteral";

        if (File studentFile = SPIFFS.open("/students.csv", "r")) {
            while (studentFile.available()) {
                String line = studentFile.readStringUntil('\n');
                int firstComma = line.indexOf(',');
                int secondComma = line.indexOf(',', firstComma + 1);
                if (firstComma != -1 && secondComma != -1) {
                    String studentRoll = line.substring(0, firstComma);
                    String studentName = line.substring(firstComma + 1, secondComma);
                    int attendance = attendanceData.count(studentRoll) ? attendanceData[studentRoll] : 0;
                    double attendancePercentage = (attendance * 100.0) / totalClasses;
                    int marks = (attendancePercentage >= 95) ? 8 :
                                (attendancePercentage >= 90) ? 7 :
                                (attendancePercentage >= 85) ? 6 :
                                (attendancePercentage >= 80) ? 5 :
                                (attendancePercentage >= 75) ? 4 :
                                (attendancePercentage >= 70) ? 3 :
                                (attendancePercentage >= 65) ? 2 :
                                (attendancePercentage >= 60) ? 1 : 0;

                    htmlPage += "<tr><td>" + studentRoll + "</td><td>" + studentName + "</td><td>" +
                                String(attendance) + "/" + String(totalClasses) + " (" + String(attendancePercentage, 2) + "%)</td><td>" +
                                String(marks) + "</td></tr>";
                }
            }
            studentFile.close();
        }

        htmlPage += R"rawliteral(
            </table>
        )rawliteral";

    } else if (viewType == "datewise") {
        // Fetch unique dates for datewise view
        std::set<String> dates;
        if (File attendanceFile = SPIFFS.open("/attendance.csv", "r")) {
            while (attendanceFile.available()) {
                String line = attendanceFile.readStringUntil('\n');
                int firstComma = line.indexOf(',');
                int lastComma = line.lastIndexOf(',');
                if (firstComma != -1 && lastComma != -1) {
                    String fileCourseCode = line.substring(0, firstComma);
                    String fileDate = line.substring(lastComma + 1, lastComma + 11);
                    if (fileCourseCode == courseCode) {
                        dates.insert(fileDate);
                    }
                }
            }
            attendanceFile.close();
        }

        // Generate HTML form for selecting a date
        htmlPage += R"rawliteral(
            <form action="/datewiseAttendance" method="GET">
                <input type="hidden" name="courseCode" value=")rawliteral" + courseCode + R"rawliteral(">
                <label for="date">Select Date:</label>
                <select name="date" id="date">
        )rawliteral";

        for (const String& date : dates) {
            htmlPage += "<option value=\"" + date + "\">" + date + "</option>";
        }

        htmlPage += R"rawliteral(
                </select>
                <br><br>
                <input type="submit" value="View Attendance">
            </form>
        )rawliteral";
    }

    htmlPage += R"rawliteral(
        </body>
        </html>
    )rawliteral";

    server.send(200, "text/html", htmlPage);
}


//38
void handleDatewiseAttendance() {
    if (!authenticateUser()) {
        return;
    }

    String courseCode = server.arg("courseCode");
    String date = server.arg("date");

    String htmlPage = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Date-wise Attendance - )rawliteral" + courseCode + R"rawliteral(</title>
            <style>
                table {
                    border-collapse: collapse;
                    width: 100%;
                }
                th, td {
                    border: 1px solid black;
                    padding: 8px;
                    text-align: left;
                }
            </style>
        </head>
        <body>
            <h2>Date-wise Attendance for )rawliteral" + courseCode + R"rawliteral( on )rawliteral" + date + R"rawliteral(</h2>
            <table>
                <tr>
                    <th>Roll</th>
                    <th>Name</th>
                    <th>Attendance</th>
                </tr>
    )rawliteral";

    // Fetch attendance data from attendance.csv for the given date and course code
    std::set<String> attendedStudents;
    File attendanceFile = SPIFFS.open("/attendance.csv", "r");
    if (attendanceFile) {
        while (attendanceFile.available()) {
            String line = attendanceFile.readStringUntil('\n');
            int firstComma = line.indexOf(',');
            int secondComma = line.indexOf(',', firstComma + 1);
            int thirdComma = line.indexOf(',', secondComma + 1);
            int lastComma = line.lastIndexOf(',');
            if (firstComma != -1 && secondComma != -1 && thirdComma != -1 && lastComma != -1) {
                String fileCourseCode = line.substring(0, firstComma);
                String studentRoll = line.substring(secondComma + 1, thirdComma);
                String dateTime = line.substring(lastComma + 1);
                String fileDate = dateTime.substring(0, 10);
                if (fileCourseCode == courseCode && fileDate == date) {
                    attendedStudents.insert(studentRoll);
                }
            }
        }
        attendanceFile.close();
    }

    // Generate HTML table for date-wise attendance
    File studentFile = SPIFFS.open("/students.csv", "r");
    if (studentFile) {
        while (studentFile.available()) {
            String line = studentFile.readStringUntil('\n');
            int firstComma = line.indexOf(',');
            int secondComma = line.indexOf(',', firstComma + 1);
            if (firstComma != -1 && secondComma != -1) {
                String studentRoll = line.substring(0, firstComma);
                String studentName = line.substring(firstComma + 1, secondComma);
                String attendance = (attendedStudents.count(studentRoll) > 0) ? "Present" : "Absent";
                htmlPage += "<tr><td>" + studentRoll + "</td><td>" + studentName + "</td><td>" + attendance + "</td></tr>";
            }
        }
        studentFile.close();
    }

    htmlPage += R"rawliteral(
            </table>
        </body>
        </html>
    )rawliteral";

    server.send(200, "text/html", htmlPage);
}



// 39
void deleteAllCSVFiles() {
    struct FileDetail {
        const char* filePath;
        const char* description;
    };

    // List of files to delete
    FileDetail files[] = {
        {teacherDataFile, "teachers.csv"},
        {studentDataFile, "students.csv"},
        {"/attendance.csv", "attendance.csv"}
    };

    for (const auto& file : files) {
        if (SPIFFS.exists(file.filePath)) {
            if (SPIFFS.remove(file.filePath)) {
                Serial.printf("%s deleted successfully.\n", file.description);
            } else {
                Serial.printf("Failed to delete %s.\n", file.description);
            }
        } else {
            Serial.printf("%s does not exist.\n", file.description);
        }
    }
}


//40
void handleDeleteAllCSVFiles() {
  if (!authenticateUser()) {
    return;
  }
  
  deleteAllCSVFiles();
  server.send(200, "text/html", "<h2>All CSV files deleted successfully!</h2>");
}


//41
struct AttendanceRecord {
    int studentId = -1; // Default invalid ID
    String studentName = ""; // Default empty name
    bool isValid = false; // Default not valid
};


//42 Array to store temporary attendance records
const int MAX_TEMP_RECORDS = 100;
AttendanceRecord tempAttendance[MAX_TEMP_RECORDS];
int tempAttendanceCount = 0;
//43
void startAttendanceProcess() {
    if (!initializeFingerprint()) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sensor Error!");
        lcd.setCursor(0, 1);
        lcd.print("Try Again Later");
        delay(3000);
        isAttendanceMode = false;
        return;
    }

    // Reset temporary attendance storage
    tempAttendanceCount = 0;
    memset(attendedStudentsarray, false, sizeof(attendedStudentsarray));

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Attendance Mode");
    lcd.setCursor(0, 1);
    lcd.print("Starting...");
    Serial.println("Attendance mode started.");

    String coursecode = "";

    // Prompt for course code
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter Course Code:");
    Serial.println("Awaiting course code...");

    while (isAttendanceMode) {
        char key = keypad.getKey();
        if (key) {
            if (key == 'B') { // Cancel attendance mode
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Session Cancelled");
                lcd.setCursor(0, 1);
                lcd.print("No Records Saved");
                Serial.println("Attendance mode cancelled.");
                delay(2000);
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Ready");
                isAttendanceMode = false;
                tempAttendanceCount = 0; // Reset temporary storage
                return;
            } else if (key == '*') { // Backspace functionality
                if (coursecode.length() > 0) {
                    coursecode = coursecode.substring(0, coursecode.length() - 1);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Enter Course Code:");
                    lcd.setCursor(0, 1);
                    lcd.print(coursecode);
                }
            } else if (key == '#') { // End of input
                if (coursecode.length() > 0) {
                    Serial.println("Course code entered: " + coursecode);
                    break;
                }
            } else { // Normal number input
                coursecode += key;
                lcd.setCursor(0, 1);
                lcd.print(coursecode);
                Serial.print(key);
            }
        }
    }

    if (!isAttendanceMode) return;

    // Verify initial teacher fingerprint
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Verify Teacher...");
    Serial.println("Place teacher's finger on the sensor.");

    while (true) {
        if (verifyTeacherFingerprint(coursecode)) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Verification Success");
            Serial.println("Teacher fingerprint verified successfully.");
            delay(2000);
            break;
        } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Verification Failed");
            lcd.setCursor(0, 1);
            lcd.print("Try Again");
            Serial.println("Teacher fingerprint verification failed. Waiting for correct fingerprint...");
            delay(2000);
        }
    }

    // Main attendance collection loop
    while (isAttendanceMode) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Place Finger");
        lcd.setCursor(0, 1);
        lcd.print("Or End Session");

        int studentId = getFingerprintID();

        // Check if it's the teacher's fingerprint
        if (studentId > 0 && isTeacherFingerprintValid(studentId, coursecode)) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Saving Records...");

            for (int i = 0; i < tempAttendanceCount; i++) {
                if (tempAttendance[i].isValid) {
                    saveAttendance(coursecode, tempAttendance[i].studentName, tempAttendance[i].studentId);
                }
            }

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Session Ended");
            lcd.setCursor(0, 1);
            lcd.print("Records: ");
            lcd.print(tempAttendanceCount);
            delay(3000);
            isAttendanceMode = false;
            break;
        }

        // Process student fingerprint
        if (studentId > 0) {
            String studentName = fetchStudentName(studentId);
            if (!studentName.isEmpty() && studentName != "Unknown Student" && !attendedStudentsarray[studentId]) {
                attendedStudentsarray[studentId] = true;

                if (tempAttendanceCount < MAX_TEMP_RECORDS) {
                    tempAttendance[tempAttendanceCount].studentId = studentId;
                    tempAttendance[tempAttendanceCount].studentName = studentName;
                    tempAttendance[tempAttendanceCount].isValid = true;
                    tempAttendanceCount++;

                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Recorded:");
                    lcd.setCursor(0, 1);
                    lcd.print(studentName);
                    delay(1500);
                }
            } else {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Unknown Student");
                lcd.setCursor(0, 1);
                lcd.print("Not Recorded");
                delay(2000);
            }
        }

        // Check for cancel key
        char key = keypad.getKey();
        if (key == 'B') {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Session Cancelled");
            lcd.setCursor(0, 1);
            lcd.print("No Records Saved");
            Serial.println("Attendance mode cancelled.");
            delay(2000);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Ready");
            isAttendanceMode = false;
            tempAttendanceCount = 0; // Reset temporary storage
            return;
        }

        delay(100); // Small delay to prevent too rapid scanning
    }
}


//44
void cancelAttendanceProcess() {
    // Clear the LCD and display cancellation message
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Session Cancelled");
    lcd.setCursor(0, 1);
    lcd.print("No Records Saved");

    // Log to Serial Monitor
    Serial.println("Attendance mode cancelled.");

    // Pause for a brief moment before resetting
    delay(2000);

    // Reset LCD and status
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ready");

    // Reset relevant variables to their initial state
    isAttendanceMode = false;
    tempAttendanceCount = 0;  // Reset temporary storage for attendance
}

//45

void handleAttendanceDetails() {
    if (!authenticateUser()) {
        return;
    }

    String courseCode = server.arg("courseCode");
    String type = server.arg("type");

    String htmlPage = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Attendance Details</title>
            <style>
                .container {
                    max-width: 900px;
                    margin: 0 auto;
                    padding: 20px;
                }
                table {
                    width: 100%;
                    border-collapse: collapse;
                    margin: 20px 0;
                }
                th, td {
                    border: 1px solid #ddd;
                    padding: 8px;
                    text-align: left;
                }
                th {
                    background-color: #4CAF50;
                    color: white;
                }
                .download-link {
                    display: inline-block;
                    padding: 5px 10px;
                    background-color: #4CAF50;
                    color: white;
                    text-decoration: none;
                    border-radius: 3px;
                    margin: 5px;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <h2>Attendance Details for Course: )rawliteral" + courseCode + R"rawliteral(</h2>
    )rawliteral";
    
    // Get course name for the header
    String courseName = "";
    File teacherFile = SPIFFS.open("/teachers.csv", "r");
    if (teacherFile) {
        while (teacherFile.available()) {
            String line = teacherFile.readStringUntil('\n');
            int firstComma = line.indexOf(',');
            int secondComma = line.indexOf(',', firstComma + 1);
            int thirdComma = line.indexOf(',', secondComma + 1);
            String fileCourseName = line.substring(secondComma + 1, thirdComma);
            String fileCourseCode = line.substring(thirdComma + 1);
            if (fileCourseCode.equals(courseCode)) {
                courseName = fileCourseName;
                break;
            }
        }
        teacherFile.close();
    }

    htmlPage += "<h2>Attendance Details for " + courseName + " (" + courseCode + ")</h2>";
    
    if (type == "total") {
        // Calculate total attendance
        std::map<String, int> studentAttendance;
        int totalClasses = 0;
        std::set<String> uniqueDates;

        // Read attendance.csv to get total classes and attendance counts
        File attendanceFile = SPIFFS.open("/attendance.csv", "r");
        if (attendanceFile) {
            while (attendanceFile.available()) {
                String line = attendanceFile.readStringUntil('\n');
                if (line.startsWith(courseCode + ",")) {
                    int firstComma = line.indexOf(',');
                    int secondComma = line.indexOf(',', firstComma + 1);
                    int thirdComma = line.indexOf(',', secondComma + 1);
                    
                    String studentId = line.substring(secondComma + 1, thirdComma);
                    String date = line.substring(thirdComma + 1, thirdComma + 11); // Extract date part
                    
                    uniqueDates.insert(date);
                    studentAttendance[studentId]++;
                }
            }
            attendanceFile.close();
        }

        totalClasses = uniqueDates.size(); // Number of unique dates is total classes

        // Create attendance table
        htmlPage += R"rawliteral(
            <table>
                <tr>
                    <th>Roll Number</th>
                    <th>Name</th>
                    <th>Attendance</th>
                    <th>Percentage</th>
                    <th>Marks</th>
                </tr>
        )rawliteral";

        // Read students.csv and display attendance data
        File studentFile = SPIFFS.open("/students.csv", "r");
        if (studentFile) {
            while (studentFile.available()) {
                String line = studentFile.readStringUntil('\n');
                int firstComma = line.indexOf(',');
                int secondComma = line.indexOf(',', firstComma + 1);
                
                if (firstComma != -1 && secondComma != -1) {
                    String studentId = line.substring(0, firstComma);
                    String studentName = line.substring(firstComma + 1, secondComma);
                    String rollNumber = line.substring(secondComma + 1);
                    
                    int attended = studentAttendance[studentId];
                    float percentage = totalClasses > 0 ? (attended * 100.0 / totalClasses) : 0;
                    int marks = calculateMarks(percentage);

                    htmlPage += "<tr><td>" + rollNumber + "</td><td>" + studentName + 
                               "</td><td>" + String(attended) + "/" + String(totalClasses) + 
                               "</td><td>" + String(percentage, 1) + "%" +
                               "</td><td>" + String(marks) + "</td></tr>";
                }
            }
            studentFile.close();
        }
        
        htmlPage += "</table>";
        
    } else if (type == "datewise") {
        // Show date-wise attendance with download links
        std::set<String> dates;
        
        // Get unique dates for this course
        File attendanceFile = SPIFFS.open("/attendance.csv", "r");
        if (attendanceFile) {
            while (attendanceFile.available()) {
                String line = attendanceFile.readStringUntil('\n');
                if (line.startsWith(courseCode + ",")) {
                    int lastComma = line.lastIndexOf(',');
                    String date = line.substring(lastComma + 1, lastComma + 11); // Extract date part
                    dates.insert(date);
                }
            }
            attendanceFile.close();
        }

        htmlPage += R"rawliteral(
            <table>
                <tr>
                    <th>Date</th>
                    <th>Action</th>
                </tr>
        )rawliteral";

        for (const String& date : dates) {
            htmlPage += "<tr><td>" + date + "</td><td>" +
                       "<a href='/downloadDateAttendance?courseCode=" + courseCode + "&date=" + date + 
                       "' class='download-link'>Download</a></td></tr>";
        }
        
        htmlPage += "</table>";
    }

    htmlPage += R"rawliteral(
            </div>
        </body>
        </html>
    )rawliteral";

    server.send(200, "text/html", htmlPage);
}

//46
String generateTotalAttendance(const String& courseCode) {
    std::map<String, int> attendanceData;
    int totalClasses = 0;

    // Parse attendance.csv
    File attendanceFile = SPIFFS.open("/attendance.csv", "r");
    if (!attendanceFile) {
        Serial.println("Failed to open attendance file.");
        return "<p>Error loading attendance data.</p>";
    }

    // Collect attendance data for the course
    while (attendanceFile.available()) {
        String line = attendanceFile.readStringUntil('\n');
        auto parsed = parseCSVLine(line, {0, 1}); // Parse course code and roll number
        if (parsed.size() > 1 && parsed[0] == courseCode) {
            attendanceData[parsed[1]]++;
            totalClasses++;
        }
    }
    attendanceFile.close();

    // If no classes are found, return a message
    if (totalClasses == 0) {
        return "<p>No attendance records found for this course.</p>";
    }

    // Generate HTML table
    String table = R"rawliteral(
        <table border="1">
            <tr>
                <th>Roll</th>
                <th>Name</th>
                <th>Attendance</th>
                <th>Marks</th>
            </tr>
    )rawliteral";

    File studentFile = SPIFFS.open("/students.csv", "r");
    if (!studentFile) {
        Serial.println("Failed to open student file.");
        return "<p>Error loading student data.</p>";
    }

    // Create table rows for each student
    while (studentFile.available()) {
        String line = studentFile.readStringUntil('\n');
        auto parsed = parseCSVLine(line, {0, 1}); // Parse roll and name
        if (parsed.size() > 1) {
            String roll = parsed[0];
            String name = parsed[1];
            int attendance = attendanceData[roll];
            double percentage = (attendance * 100.0) / totalClasses;
            int marks = calculateMarks(percentage);

            table += "<tr><td>" + roll + "</td><td>" + name + "</td><td>" +
                     String(attendance) + "/" + String(totalClasses) + " (" +
                     String(percentage, 2) + "%)</td><td>" + String(marks) + "</td></tr>";
        }
    }
    studentFile.close();

    table += "</table>";
    return table;
}

//47
String generateDatewiseAttendance(const String& courseCode) {
    std::set<String> dates;

    // Parse attendance.csv to fetch dates
    File attendanceFile = SPIFFS.open("/attendance.csv", "r");
    if (!attendanceFile) {
        Serial.println("Failed to open attendance file.");
        return "";
    }

    while (attendanceFile.available()) {
        String line = attendanceFile.readStringUntil('\n');
        auto parsed = parseCSVLine(line, {0, 3}); // Parse course code and date
        if (parsed.size() > 1 && parsed[0] == courseCode) {
            dates.insert(parsed[1].substring(0, 10)); // Extract date in yyyy-mm-dd format
        }
    }
    attendanceFile.close();

    // If no dates found for the given course, return a message
    if (dates.empty()) {
        return "<p>No attendance records found for the selected course.</p>";
    }

    // Generate HTML table
    String table = R"rawliteral(
        <form action="/datewiseAttendance" method="GET">
            <label for="date">Select Date:</label>
            <select name="date" id="date">
    )rawliteral";

    for (const auto& date : dates) {
        table += "<option value=\"" + date + "\">" + date + "</option>";
    }

    table += R"rawliteral(
            </select>
            <br><br>
            <input type="hidden" name="courseCode" value=")rawliteral" + courseCode + R"rawliteral(">
            <input type="submit" value="View Attendance">
        </form>
    )rawliteral";

    return table;
}

//48
int calculateMarks(double percentage) {
    if (percentage >= 95) return 8; // Excellent
    if (percentage >= 90) return 7; // Very Good
    if (percentage >= 85) return 6; // Good
    if (percentage >= 80) return 5; // Satisfactory
    if (percentage >= 75) return 4; // Fair
    if (percentage >= 70) return 3; // Average
    if (percentage >= 65) return 2; // Below Average
    if (percentage >= 60) return 1; // Needs Improvement
    return 0; // Fail
}



// Usage Example: auto parsed = parseCSVLine(line, {0, 1});




//49
void handleDateWiseDownload() {
    if (!authenticateUser()) {
        server.send(401, "text/plain", "Unauthorized access");
        return;
    }

    String htmlPage = R"rawliteral(
        <h2>Date-wise Attendance Download</h2>
        <table border="1">
            <tr>
                <th>Course Name (Code)</th>
                <th>Date</th>
                <th>Download Link</th>
            </tr>
    )rawliteral";

    // Get course names and codes
    std::map<String, String> courseNames;
    File teacherFile = SPIFFS.open("/teachers.csv", "r");
    if (!teacherFile) {
        server.send(500, "text/plain", "Failed to open teachers file.");
        return;
    }

    while (teacherFile.available()) {
        String line = teacherFile.readStringUntil('\n');
        int firstComma = line.indexOf(',');
        int secondComma = line.indexOf(',', firstComma + 1);
        int thirdComma = line.indexOf(',', secondComma + 1);

        if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
            String courseName = line.substring(secondComma + 1, thirdComma);
            String courseCode = line.substring(thirdComma + 1);
            courseCode.trim();
            courseNames[courseCode] = courseName;
        }
    }
    teacherFile.close();

    // Get attendance records by date for each course
    std::map<String, std::set<String>> datesByCourse;
    File attendanceFile = SPIFFS.open("/attendance.csv", "r");
    if (!attendanceFile) {
        server.send(500, "text/plain", "Failed to open attendance file.");
        return;
    }

    while (attendanceFile.available()) {
        String line = attendanceFile.readStringUntil('\n');
        int firstComma = line.indexOf(',');
        int lastComma = line.lastIndexOf(',');

        if (firstComma != -1 && lastComma != -1) {
            String courseCode = line.substring(0, firstComma);
            String date = line.substring(lastComma + 1, lastComma + 11); // Extract date in yyyy-mm-dd format
            datesByCourse[courseCode].insert(date);
        }
    }
    attendanceFile.close();

    // Generate HTML table rows for each course and date
    for (const auto& course : datesByCourse) {
        String courseCode = course.first;
        String courseName = courseNames[courseCode];

        for (const String& date : course.second) {
            htmlPage += "<tr><td>" + courseName + " (" + courseCode + ")</td><td>" + 
                       date + "</td><td><a href=\"/downloadAttendance?courseCode=" + 
                       courseCode + "&date=" + date + "\">Download</a></td></tr>";
        }
    }

    htmlPage += "</table>";
    server.send(200, "text/html", htmlPage);
}


//50
void handleDownloadAttendanceByDate() {
   if (!authenticateUser()) {
       server.send(401, "text/plain", "Unauthorized access");
       return;
   }

   // Check if required parameters are present in the URL
   String dateRequested = server.arg("date");
   String courseCode = server.arg("courseCode");

   if (dateRequested.isEmpty() || courseCode.isEmpty()) {
       server.send(400, "text/plain", "Missing 'courseCode' or 'date' parameters");
       return;
   }

   // Build the file name based on the course and date, if necessary
   String fileName = "/attendance_" + courseCode + "_" + dateRequested + ".csv";

   File fileToDownload = SPIFFS.open(fileName, "r");
   if (!fileToDownload) {
       server.send(404, "text/plain", "Attendance file not found for the requested date and course.");
       return;
   }

   // Add Content-Disposition for proper download behavior
   server.sendHeader("Content-Disposition", "attachment; filename=" + fileName);
   
   // Stream the file content to the client
   server.streamFile(fileToDownload, "text/csv");

   fileToDownload.close();
}



//51
// void handleDownloadAttendance25() {
//     if (!authenticateUser()) {
//         server.send(401, "text/plain", "Unauthorized access");
//         return;
//     }

//     File file = SPIFFS.open("/attendance.csv", "r");
//     if (!file) {
//         server.send(500, "text/plain", "Failed to open attendance file.");
//         return;
//     }

//     // Adding Content-Disposition for proper download behavior
//     server.sendHeader("Content-Disposition", "attachment; filename=attendance.csv");

//     // Stream the file content to the client
//     server.streamFile(file, "text/csv");

//     file.close();
// }






//52

void handleDownloadDateAttendance() {
    if (!authenticateUser()) {
        server.send(401, "text/plain", "Unauthorized access");
        return;
    }

    String courseCode = server.arg("courseCode");
    String date = server.arg("date");

    if (courseCode.isEmpty() || date.isEmpty()) {
        server.send(400, "text/plain", "Invalid request. Missing courseCode or date parameter.");
        return;
    }

    String courseName = "";
    // Get course name from teachers.csv
    File teacherFile = SPIFFS.open("/teachers.csv", "r");
    if (!teacherFile) {
        server.send(500, "text/plain", "Failed to open teachers file.");
        return;
    }

    while (teacherFile.available()) {
        String line = teacherFile.readStringUntil('\n');
        if (line.indexOf(courseCode) != -1) {
            int firstComma = line.indexOf(',');
            int secondComma = line.indexOf(',', firstComma + 1);
            int thirdComma = line.indexOf(',', secondComma + 1);
            courseName = line.substring(secondComma + 1, thirdComma);
            break;
        }
    }
    teacherFile.close();

    if (courseName.isEmpty()) {
        server.send(404, "text/plain", "Course not found.");
        return;
    }

    // Create temporary file for filtered attendance
    File tempFile = SPIFFS.open("/temp_download.csv", "w");
    if (!tempFile) {
        server.send(500, "text/plain", "Error creating temporary file.");
        return;
    }

    tempFile.println("Course Name,Course Code,Student Name,Roll Number,Time");

    // Read and filter attendance records
    File attendanceFile = SPIFFS.open("/attendance.csv", "r");
    if (!attendanceFile) {
        tempFile.close();
        SPIFFS.remove("/temp_download.csv");
        server.send(500, "text/plain", "Failed to open attendance file.");
        return;
    }

    while (attendanceFile.available()) {
        String line = attendanceFile.readStringUntil('\n');
        if (line.startsWith(courseCode + ",") && line.indexOf(date) != -1) {
            tempFile.print(courseName + ",");
            tempFile.println(line);
        }
    }
    attendanceFile.close();
    tempFile.close();

    // Serve the filtered file
    File downloadFile = SPIFFS.open("/temp_download.csv", "r");
    if (!downloadFile) {
        server.send(500, "text/plain", "Error reading filtered attendance file.");
        SPIFFS.remove("/temp_download.csv");
        return;
    }

    server.sendHeader("Content-Disposition", "attachment; filename=attendance_" + courseCode + "_" + date + ".csv");
    server.streamFile(downloadFile, "text/csv");
    downloadFile.close();

    // Clean up
    SPIFFS.remove("/temp_download.csv");
}

//53
void saveOfflineAttendance(String courseCode, String studentName, int studentId) {
    // Helper function to format the current date as "yyyy-mm-dd"
    auto getCurrentDate = []() {
        DateTime now = rtc.now(); // Get current time from RTC
        char timestamp[11];
        sprintf(timestamp, "%04d-%02d-%02d", now.year(), now.month(), now.day()); // Format as yyyy-mm-dd
        return String(timestamp);
    };

    String timestamp = getCurrentDate();

    // Record format: CourseCode,StudentName,StudentId,Date
    String record = courseCode + "," + studentName + "," + String(studentId) + "," + timestamp + "\n";

    // Helper function to save data to a file
    auto saveToFile = [](String fileName, const String& record, bool isSDCard = false) {
        File file = isSDCard ? SD.open(fileName, FILE_APPEND) : SPIFFS.open(fileName, FILE_APPEND);
        if (!file) {
            Serial.printf("Failed to open %s for writing!\n", fileName.c_str());
            return false;
        }
        file.print(record);
        file.close();
        Serial.printf("Saved to %s: %s\n", fileName.c_str(), record.c_str());
        return true;
    };

    // Save to SD card
    if (!saveToFile("/attendance.csv", record, true)) {
        Serial.println("SD card save failed.");
    }

    // Save to SPIFFS
    if (!saveToFile("/attendance.csv", record)) {
        Serial.println("SPIFFS save failed.");
    }

    // Save to offline attendance file
    if (!saveToFile("/offline_attendance.csv", record)) {
        Serial.println("Offline attendance save failed.");
    }
}

//54
void sendOfflineDataToSupabase() {
    static unsigned long lastSyncTime = 0;
    const unsigned long syncInterval = 60000; // Sync every 60 seconds
    const int maxRetries = 3; // Max retry attempts for each record

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Not connected to Wi-Fi. Skipping offline data sync.");
        return;
    }

    // Only attempt to sync at regular intervals
    if (millis() - lastSyncTime < syncInterval) {
        return;
    }
    lastSyncTime = millis();

    File offlineFile = SPIFFS.open("/offline_attendance.csv", "r");
    if (!offlineFile) {
        Serial.println("No offline data to sync.");
        return;
    }

    bool allSentSuccessfully = true; // Track if all records were sent successfully
    String unsyncedData = "";        // Buffer to store records that failed to sync

    while (offlineFile.available()) {
        String line = offlineFile.readStringUntil('\n');
        if (line.length() > 0) {
            // Parse the line
            int firstComma = line.indexOf(',');
            int secondComma = line.indexOf(',', firstComma + 1);
            int thirdComma = line.indexOf(',', secondComma + 1);

            String courseCode = line.substring(0, firstComma);
            String studentName = line.substring(firstComma + 1, secondComma);
            String studentId = line.substring(secondComma + 1, thirdComma);
            String timestamp = line.substring(thirdComma + 1);

            // Prepare JSON for Supabase
            StaticJsonDocument<300> doc;
            doc["course_code"] = courseCode;
            doc["student_name"] = studentName;
            doc["student_id"] = studentId.toInt();
            doc["timestamp"] = timestamp;

            String jsonString;
            serializeJson(doc, jsonString);

            // Send to Supabase with retry logic
            bool sentSuccessfully = false;
            for (int attempt = 1; attempt <= maxRetries; attempt++) {
                HTTPClient http;
                String url = String(SUPABASE_URL) + "/rest/v1/attendance";
                http.begin(url);
                http.addHeader("apikey", SUPABASE_KEY);
                http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
                http.addHeader("Content-Type", "application/json");

                int httpCode = http.POST(jsonString);

                if (httpCode == 200 || httpCode == 201) {
                    Serial.println("Record sent to Supabase successfully: " + jsonString);
                    sentSuccessfully = true;
                    break; // Exit retry loop if successful
                } else {
                    Serial.printf("Attempt %d: Failed to send record to Supabase (HTTP %d): %s\n",
                                  attempt, httpCode, http.errorToString(httpCode).c_str());
                }
                http.end();
                delay(100); // Short delay between retries
            }

            if (!sentSuccessfully) {
                allSentSuccessfully = false;
                unsyncedData += line + "\n"; // Retain unsynced data
            }
        }
    }
    offlineFile.close();

    // Handle results
    if (allSentSuccessfully) {
        SPIFFS.remove("/offline_attendance.csv");
        Serial.println("All offline attendance data sent successfully and file removed.");
    } else {
        // Rewrite unsynced data to the file
        File writeFile = SPIFFS.open("/offline_attendance.csv", "w");
        if (writeFile) {
            writeFile.print(unsyncedData);
            writeFile.close();
            Serial.println("Unsynced data retained for the next attempt.");
        } else {
            Serial.println("Failed to rewrite unsynced data.");
        }
    }
}


//55
void handleDownloadOfflineData() {
    // Attempt to open the offline attendance file
    File offlineFile = SPIFFS.open("/offline_attendance.csv", "r");
    if (!offlineFile) {
        Serial.println("Error: Failed to open offline attendance file.");
        server.send(500, "text/plain", "Error: Failed to open offline attendance file.");
        return;
    }

    // Check if the file is empty
    if (offlineFile.size() == 0) {
        Serial.println("Warning: Offline attendance file is empty.");
        server.send(204, "text/plain", "No offline attendance data available."); // 204: No Content
        offlineFile.close();
        return;
    }

    // Prepare HTTP headers for file download
    server.sendHeader("Content-Disposition", "attachment; filename=offline_attendance.csv");
    server.sendHeader("Cache-Control", "no-cache");

    // Stream the file to the client
    if (server.streamFile(offlineFile, "text/csv") != offlineFile.size()) {
        Serial.println("Warning: File streaming incomplete.");
    } else {
        Serial.println("Offline attendance file streamed successfully.");
    }

    offlineFile.close(); // Close the file after streaming
}


void setupWebServerRoutes() {
    server.on("/editTeacher", HTTP_GET, handleEditTeacher);
    server.on("/updateTeacher", HTTP_GET, handleUpdateTeacher);
    server.on("/deleteTeacher", HTTP_GET, handleDeleteTeacher);
    server.on("/editStudent", HTTP_GET, handleEditStudent);
    server.on("/updateStudent", HTTP_GET, handleUpdateStudent);
    server.on("/deleteStudent", HTTP_GET, handleDeleteStudent);
    server.on("/", HTTP_GET, handleRoot);
    server.on("/wifi", HTTP_GET, handleWifiSettings);
    server.on("/saveWifi", HTTP_POST, handleSaveWifi);
    server.on("/enrollTeacher", HTTP_GET, handleEnrollTeacher);
    server.on("/enrollTeacherSubmit", HTTP_POST, []() {
        String teacherId = server.arg("teacherId");
        String teacherName = server.arg("teacherName");
        String courseName = server.arg("courseName");
        String coursecode = server.arg("coursecode");

        enrollTeacher(teacherId.toInt(), teacherName, courseName, coursecode);
        server.send(200, "text/html", "<h2>Teacher enrolled successfully!</h2>");
    });
    server.on("/downloadTeachers", HTTP_GET, handleDownloadTeachers);
    server.on("/enrollStudent", HTTP_GET, handleEnrollStudent);
    server.on("/enrollStudentSubmit", HTTP_POST, []() {
        String studentId = server.arg("studentId");
        String studentName = server.arg("studentName");
        String stdroll = server.arg("stdroll");

        enrollStudent(studentId.toInt(), studentName, stdroll);
        server.send(200, "text/html", "<h2>Student enrolled successfully!</h2>");
    });
    server.on("/downloadStudents", HTTP_GET, handleDownloadStudents);
  //  server.on("/attendancemode", HTTP_GET, handleattendancemode);
    server.on("/deleteAllFiles", HTTP_GET, handleDeleteAllCSVFiles);
    server.on("/downloadAttendance", HTTP_GET, handleDownloadAttendance);
    server.on("/viewAttendance", HTTP_GET, handleViewAttendance);
    server.on("/attendanceDetails", HTTP_GET, handleAttendanceDetails);
    server.on("/datewiseAttendance", HTTP_GET, handleDatewiseAttendance);
    server.on("/datewiseDownload", handleDateWiseDownload);
    //server.on("/downloadAttendance25", handleDownloadAttendance25);
    server.on("/downloadDateAttendance", HTTP_GET, handleDownloadDateAttendance);
    server.on("/downloadOfflineData", HTTP_GET, handleDownloadOfflineData);
}



void setup() {
    Serial.begin(115200);  // Initialize serial communication
    mySerial.begin(57600); // Start serial for the sensor
    finger.begin(57600);   // Initialize the fingerprint sensor

    // I2C initialization for LCD
    Wire.begin(21, 22);    // SDA = GPIO21, SCL = GPIO22
    lcd.begin(16, 2);      // Initialize 16x2 LCD
    lcd.backlight();
    lcd.clear();
    lcd.print("Starting up...");

    // Initialize the fingerprint sensor
    bool fingerprintInitialized = initializeFingerprint();
    if (!fingerprintInitialized) {
        Serial.println("Failed to initialize fingerprint sensor!");
        lcd.clear();
        lcd.print("Fingerprint Init Failed");
        // Disable fingerprint-related features
        isFingerprintEnabled = false;
    } else {
        isFingerprintEnabled = true;
        Serial.println("Fingerprint sensor initialized successfully.");
    }

    // Preferences initialization
    preferences.begin("WiFi", false);

    // SPIFFS initialization
    if (!SPIFFS.begin(true)) { // Format SPIFFS if mounting fails
        Serial.println("Failed to mount SPIFFS!");
        lcd.clear();
        lcd.print("SPIFFS Failed");
        return;
    }
    Serial.println("SPIFFS mounted successfully.");

    // Retrieve stored Wi-Fi credentials
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    lcd.setCursor(0, 0); // Set cursor to first row, first column
    lcd.print("Connecting...");

    // Wi-Fi setup
    if (ssid == "" || password == "") {
        startAccessPoint(); // Start Access Point if no credentials are saved
    } else {
        if (!connectToWiFi(ssid.c_str(), password.c_str())) { // Attempt to connect to Wi-Fi
            startAccessPoint(); // Start Access Point if connection fails
        }
    }

    // Set up web server routes
    setupWebServerRoutes();  // Moved all routes to a separate function for better organization

    // Start the web server
    server.begin();
    Serial.println("Web server started.");

    // SD Card initialization
    if (!SD.begin(5)) { // GPIO5 is the CS pin
        Serial.println("SD Card initialization failed!");
        lcd.clear();
        lcd.print("SD Card Failed");
        // Optional: Handle SD card absence gracefully
    } else {
        Serial.println("SD Card initialized.");
    }

    // RTC initialization
    if (!rtc.begin()) {
        Serial.println("RTC initialization failed!");
        lcd.clear();
        lcd.print("RTC Failed");
        while (1); // Halt if RTC is critical
    } else {
        Serial.println("RTC initialized.");
    }

    // Optional: Display a final ready message
    lcd.clear();
    lcd.print("System Ready");
    Serial.println("System setup completed successfully.");
}




//clint Server Handle with online check and key for attendance
void checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi not connected. Attempting to reconnect...");
        WiFi.reconnect();  // Try reconnecting
        delay(5000);  // Wait 5 seconds before trying again
    }
}

// Function to handle keypad input with debouncing and non-blocking logic
void handleKeypadInput() {
    char key = keypad.getKey();
    if (key) {
        Serial.println("Key Pressed: " + String(key)); // Debug line
        switch (key) {
            case 'A':
                if (!isAttendanceMode) {
                    isAttendanceMode = true;
                    Serial.println("Starting Attendance Process..."); // Debug line
                    startAttendanceProcess();
                }
                break;
            case 'B':
                if (isAttendanceMode) {
                    isAttendanceMode = false;
                    cancelAttendanceProcess();
                }
                break;
        }
    }
}

// Main loop
void loop() {
    server.handleClient();  // Handle web server client requests
    delay(1);  // Prevent blocking

    checkWiFiConnection();  // Periodically check Wi-Fi status

    // Send offline data to Supabase if Wi-Fi is connected
    if (WiFi.status() == WL_CONNECTED) {
        sendOfflineDataToSupabase();  // Send offline data asynchronously
    }

    handleKeypadInput();  // Process keypad input without blocking

    // Optionally, add more tasks or checks (e.g., for other sensors)
}
