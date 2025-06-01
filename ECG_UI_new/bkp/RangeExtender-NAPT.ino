#include <ESP8266WiFi.h>
#include <lwip/napt.h>
#include <lwip/dns.h>

#define STASSID "ESP-J67GHYY"
#define STAPSK "<masked>"

void setup() {
  Serial.begin(115200);
  Serial.println("\nNAPT WiFi Extender Starting...");

  // Connect to the upstream WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  // Configure DHCP to use the real DNS server
  auto& server = WiFi.softAPDhcpServer();
  server.setDns(WiFi.dnsIP(0));

  // Set up AP mode with a new SSID
  WiFi.softAPConfig(IPAddress(172, 217, 28, 254), IPAddress(172, 217, 28, 254), IPAddress(255, 255, 255, 0));
  WiFi.softAP(STASSID "Ext", STAPSK);
  Serial.println("AP Mode Enabled!");

  // Enable NAT (NAPT)
  if (ip_napt_init(1000, 10) == ERR_OK) {
    if (ip_napt_enable_no(SOFTAP_IF, 1) == ERR_OK) {
      Serial.println("NAPT Enabled. Your extended WiFi is now functional.");
    }
  }
}

void loop() {}

