#include "TheH-File.h"

// UDP instance for telemetry transmission.
EthernetUDP TelemetryUdp;
unsigned int localTelemetryPort = 8888;  // Local port for UDP transmissions

// Forward declarations for external module objects.
// VESC objects defined in VescController.cpp.
extern VescUart vesc1;  
extern VescUart vesc2;
// ODrive object defined in OdrvController.cpp.
extern ODriveUART odrive;

// Initialize Teensy Ethernet for telemetry.
void setupTelemetryEthernet() {
    Serial.println("Initializing Teensy Ethernet for telemetry...");
    Ethernet.begin(mac, ip);
    delay(2000);  // Allow time for initialization
    TelemetryUdp.begin(localTelemetryPort);
    Serial.print("Telemetry Sender Local IP: ");
    Serial.println(Ethernet.localIP());
}

// Sends a UDP packet containing telemetry in CSV format.
// Data fields (in order):
// ODrive vbus voltage, ODrive bus current, ODrive position, ODrive velocity,
// VESC1 average input current, VESC1 input voltage, VESC1 rpm,
// VESC2 average input current, VESC2 input voltage, VESC2 rpm.
void sendTelemetry() {
    // Get ODrive telemetry.
    ODriveFeedback fb = odrive.getFeedback();
    float od_pos = fb.pos;    // ODrive position
    float od_vel = fb.vel;    // ODrive velocity
    float od_vol = odrive.getParameterAsFloat("vbus_voltage");
    float od_cur = odrive.getParameterAsFloat("ibus");

    // Retrieve telemetry from both VESC modules.
    bool vesc1_ok = vesc1.getVescValues();
    bool vesc2_ok = vesc2.getVescValues();

    float vesc1_current = vesc1_ok ? vesc1.data.avgInputCurrent : 0.0f;
    float vesc1_voltage = vesc1_ok ? vesc1.data.inpVoltage : 0.0f;
    int   vesc1_rpm     = vesc1_ok ? vesc1.data.rpm : 0;

    float vesc2_current = vesc2_ok ? vesc2.data.avgInputCurrent : 0.0f;
    float vesc2_voltage = vesc2_ok ? vesc2.data.inpVoltage : 0.0f;
    int   vesc2_rpm     = vesc2_ok ? vesc2.data.rpm : 0;

    // Format the telemetry packet as a comma-separated string.
    // Order: ODrive vbus voltage, ODrive bus current, ODrive position, ODrive velocity,
    //        VESC1 current, VESC1 voltage, VESC1 rpm,
    //        VESC2 current, VESC2 voltage, VESC2 rpm.
    char packetBuffer[256];
    snprintf(packetBuffer, sizeof(packetBuffer),
             "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%.2f,%.2f,%d",
             od_vol, od_cur, od_pos, od_vel,
             vesc1_current, vesc1_voltage, vesc1_rpm,
             vesc2_current, vesc2_voltage, vesc2_rpm);

    // Transmit the telemetry packet via UDP.
    TelemetryUdp.beginPacket(UDP_REMOTE_IP, UDP_REMOTE_PORT);
    TelemetryUdp.write(packetBuffer);
    TelemetryUdp.endPacket();

    // Debug output.
    Serial.print("Telemetry packet sent: ");
    Serial.println(packetBuffer);
}
