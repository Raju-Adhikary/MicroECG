#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <WebSocketsServer.h>

const char* ssid = "Your_WiFi";
const char* password = "Your_Password";

ESP8266WebServer httpRedirect(80);    // HTTP server (for redirect)
BearSSL::ESP8266WebServerSecure server(443);  // HTTPS server
WebSocketsServer webSocket(81);      // Secure WebSocket

// Simple Web Page
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP8266 WSS Test</title>
</head>
<body>
    <h1>WebSocket Secure (WSS) Test</h1>
    <script>
        var socket = new WebSocket("wss://" + location.host + ":81/");
        socket.onopen = function() {
            console.log("WebSocket Connected!");
            socket.send("Hello ESP8266");
        };
        socket.onmessage = function(event) {
            console.log("Received:", event.data);
        };
        socket.onerror = function(error) {
            console.error("WebSocket Error:", error);
        };
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");

    // ** Redirect HTTP to HTTPS **
    httpRedirect.onNotFound([]() {
        httpRedirect.sendHeader("Location", String("https://") + WiFi.localIP().toString(), true);
        httpRedirect.send(301, "text/plain", "Redirecting to HTTPS...");
    });
    httpRedirect.begin();
    Serial.println("HTTP Redirect Active");

    // ** Start Secure WebSocket **
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    // ** Serve HTTPS Page **
    server.on("/", []() {
        server.send_P(200, "text/html", webpage);
    });
    server.begin();
    Serial.println("HTTPS & WebSocket Server Ready");
}

void loop() {
    httpRedirect.handleClient();  // Redirect HTTP users
    webSocket.loop();
    server.handleClient();
}

// WebSocket Event Handling
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        Serial.println("Received: " + String((char*)payload));
        webSocket.broadcastTXT("ESP8266 Received: " + String((char*)payload));
    }
}
