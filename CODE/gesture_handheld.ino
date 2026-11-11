#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const uint8_t PIN_SDA = 21;
const uint8_t PIN_SCL = 22;
const uint8_t PIN_ARM = 25;
const uint8_t PIN_MODE = 26;
const uint8_t PIN_LED_R = 16;
const uint8_t PIN_LED_G = 17;
const uint8_t PIN_LED_B = 18;
const uint8_t PIN_HAPTIC = 19;
const uint8_t PIN_BATTERY = 34;

uint8_t CAR_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const uint8_t WIFI_CHANNEL = 1;
const uint8_t TEAM_ID = 7;
const uint8_t PACKET_MAGIC = 0xA7;

const uint8_t MPU_ADDR = 0x68;
const float ACCEL_LSB_PER_G = 8192.0f;
const float GYRO_LSB_PER_DPS = 65.5f;
const int GYRO_CAL_SAMPLES = 200;

const bool DEBUG_SERIAL = true;
const bool ARM_HOLD_MODE = false;
const bool SWAP_PITCH_ROLL = false;
const float PITCH_SIGN = 1.0f;
const float ROLL_SIGN = 1.0f;

const float FILTER_ALPHA = 0.96f;
const float DEADZONE_DEG = 6.0f;
const float PITCH_MAX_DEG = 30.0f;
const float ROLL_MAX_DEG = 30.0f;
const float OUTPUT_SMOOTHING = 0.35f;
const float FLIP_DISARM_DEG = 75.0f;

const float SHAKE_G = 2.4f;
const uint32_t SHAKE_WINDOW_MS = 700;
const uint32_t SHAKE_MIN_GAP_MS = 80;
const uint32_t SPIN_MS = 1500;

const uint32_t IMU_PERIOD_MS = 5;
const uint32_t SEND_PERIOD_MS = 20;
const uint32_t DEBOUNCE_MS = 30;
const uint32_t DEBUG_PERIOD_MS = 200;
const uint8_t IMU_FAIL_LIMIT = 20;
const uint32_t IMU_RETRY_MS = 1000;

const uint8_t OLED_ADDR = 0x3C;
const uint8_t OLED_WIDTH = 128;
const uint8_t OLED_HEIGHT = 64;
const uint16_t OLED_BYTES = (OLED_WIDTH * OLED_HEIGHT) / 8;
const uint8_t OLED_CHUNK = 32;
const uint32_t DISPLAY_PERIOD_MS = 150;
const uint8_t DISPLAY_FAIL_LIMIT = 20;

const uint32_t TELEMETRY_TIMEOUT_MS = 1000;
const uint32_t LONG_PRESS_MS = 800;

const bool BATTERY_WARNINGS = true;
const float HAND_BATTERY_DIVIDER = 2.0f;
const float HAND_BATTERY_SMOOTHING = 0.02f;
const uint32_t HAND_BATTERY_PERIOD_MS = 50;
const uint16_t HAND_LOW_MV = 3500;
const uint32_t LOW_BUZZ_PERIOD_MS = 60000;

const uint16_t HAPTIC_NEAR_CM = 100;
const uint16_t HAPTIC_VERY_NEAR_CM = 15;
const uint16_t HAPTIC_SLOW_MS = 600;
const uint16_t HAPTIC_FAST_MS = 120;
const uint16_t HAPTIC_PULSE_MS = 60;
const int8_t HAPTIC_MIN_THROTTLE = 5;

const uint16_t DISTANCE_UNKNOWN = 0xFFFF;

const uint16_t CURVE_MV[9] = {3300, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200};
const uint8_t CURVE_PCT[9] = {0, 10, 20, 35, 50, 65, 80, 90, 100};

const uint16_t PAT_ARM[] = {80};
const uint16_t PAT_DISARM[] = {60, 70, 60};
const uint16_t PAT_MODE_1[] = {30};
const uint16_t PAT_MODE_2[] = {30, 60, 30};
const uint16_t PAT_MODE_3[] = {30, 60, 30, 60, 30};
const uint16_t PAT_FOLLOW_ON[] = {50, 70, 50, 70, 50};
const uint16_t PAT_FOLLOW_OFF[] = {250};
const uint16_t PAT_REJECT[] = {120};
const uint16_t PAT_SPIN[] = {40};
const uint16_t PAT_LINK_LOST[] = {400, 150, 400};
const uint16_t PAT_BATTERY_LOW[] = {150, 120, 150};
#define PATTERN_LEN(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))

const char *const SPEED_NAMES[3] = {"CRAWL", "NORMAL", "TURBO"};

struct __attribute__((packed)) ControlPacket {
  uint8_t magic;
  uint8_t teamId;
  uint8_t seq;
  int8_t throttle;
  int8_t steer;
  uint8_t flags;
  uint8_t checksum;
};

struct __attribute__((packed)) TelemetryPacket {
  uint8_t magic;
  uint8_t teamId;
  uint8_t seq;
  uint16_t batteryMv;
  uint16_t frontCm;
  uint16_t rearCm;
  uint8_t flags;
  uint8_t checksum;
};

const uint8_t FLAG_ARMED = 0x01;
const uint8_t FLAG_SPIN = 0x02;
const uint8_t FLAG_FOLLOW = 0x04;
const uint8_t FLAG_SPEED_SHIFT = 4;

const uint8_t TFLAG_ARMED_ACK = 0x01;
const uint8_t TFLAG_FOLLOW_ACTIVE = 0x02;
const uint8_t TFLAG_OBSTACLE = 0x04;
const uint8_t TFLAG_FAULT = 0x08;

const float RAD_TO_DEG_F = 57.29578f;

float fusedRoll = 0;
float fusedPitch = 0;
float mappedRoll = 0;
float mappedPitch = 0;
float neutralRoll = 0;
float neutralPitch = 0;
float gyroBiasX = 0;
float gyroBiasY = 0;
float smoothThrottle = 0;
float smoothSteer = 0;
int8_t outThrottle = 0;
int8_t outSteer = 0;
bool fusionReady = false;
bool imuOk = false;
bool gyroCalibrated = false;
bool armed = false;
bool followMode = false;
bool armRaw = false;
bool armStable = false;
uint8_t imuFails = 0;
uint8_t speedMode = 1;
uint8_t shakeCount = 0;
uint8_t controlSeq = 0;
uint8_t whoAmI = 0;
uint32_t lastImuUs = 0;
uint32_t lastImuMs = 0;
uint32_t lastImuRetryMs = 0;
uint32_t lastSendMs = 0;
uint32_t lastDebugMs = 0;
uint32_t armChangeMs = 0;
uint32_t lastShakeMs = 0;
uint32_t spinUntilMs = 0;

void hapticStart(const uint16_t *steps, uint8_t count);

uint32_t ageMs(uint32_t now, uint32_t then) {
  int32_t d = (int32_t)(now - then);
  return d < 0 ? 0 : (uint32_t)d;
}

uint8_t checksum8(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) sum ^= data[i];
  return sum;
}

bool mpuWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool mpuInit() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x75);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)1) != 1) return false;
  whoAmI = Wire.read();
  if (!mpuWrite(0x6B, 0x00)) return false;
  delay(50);
  if (!mpuWrite(0x1A, 0x03)) return false;
  if (!mpuWrite(0x1B, 0x08)) return false;
  if (!mpuWrite(0x1C, 0x08)) return false;
  return true;
}

bool mpuRead(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)14) != 14) return false;
  int16_t raw[7];
  for (int i = 0; i < 7; i++) {
    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    raw[i] = (int16_t)((hi << 8) | lo);
  }
  if (raw[0] == 0 && raw[1] == 0 && raw[2] == 0) return false;
  ax = raw[0] / ACCEL_LSB_PER_G;
  ay = raw[1] / ACCEL_LSB_PER_G;
  az = raw[2] / ACCEL_LSB_PER_G;
  gx = raw[4] / GYRO_LSB_PER_DPS;
  gy = raw[5] / GYRO_LSB_PER_DPS;
  gz = raw[6] / GYRO_LSB_PER_DPS;
  return true;
}

void calibrateGyro() {
  float sumX = 0;
  float sumY = 0;
  int good = 0;
  for (int i = 0; i < GYRO_CAL_SAMPLES; i++) {
    float ax, ay, az, gx, gy, gz;
    if (mpuRead(ax, ay, az, gx, gy, gz)) {
      sumX += gx;
      sumY += gy;
      good++;
    }
    delay(3);
  }
  if (good > 0) {
    gyroBiasX = sumX / good;
    gyroBiasY = sumY / good;
    gyroCalibrated = true;
  }
}

void disarm() {
  bool wasArmed = armed;
  armed = false;
  followMode = false;
  smoothThrottle = 0;
  smoothSteer = 0;
  outThrottle = 0;
  outSteer = 0;
  spinUntilMs = 0;
  shakeCount = 0;
  if (wasArmed) hapticStart(PAT_DISARM, PATTERN_LEN(PAT_DISARM));
}

void tryArm() {
  if (!imuOk || !fusionReady) return;
  neutralPitch = mappedPitch;
  neutralRoll = mappedRoll;
  smoothThrottle = 0;
  smoothSteer = 0;
  armed = true;
  hapticStart(PAT_ARM, PATTERN_LEN(PAT_ARM));
}

void detectShake(float ax, float ay, float az, uint32_t now) {
  if (!armed) return;
  float magnitude = sqrtf(ax * ax + ay * ay + az * az);
  if (magnitude < SHAKE_G) return;
  if (now - lastShakeMs < SHAKE_MIN_GAP_MS) return;
  if (now - lastShakeMs > SHAKE_WINDOW_MS) shakeCount = 0;
  shakeCount++;
  lastShakeMs = now;
  if (shakeCount >= 2) {
    spinUntilMs = now + SPIN_MS;
    shakeCount = 0;
  }
}

void updateFusion(uint32_t now) {
  if (!imuOk && now - lastImuRetryMs >= IMU_RETRY_MS) {
    lastImuRetryMs = now;
    if (mpuInit()) {
      if (!gyroCalibrated) calibrateGyro();
      fusionReady = false;
      imuFails = 0;
      imuOk = true;
      lastImuUs = micros();
    }
  }
  if (!imuOk) return;
  float ax, ay, az, gx, gy, gz;
  if (!mpuRead(ax, ay, az, gx, gy, gz)) {
    if (imuFails < 255) imuFails++;
    if (imuFails >= IMU_FAIL_LIMIT) {
      imuOk = false;
      disarm();
    }
    return;
  }
  imuFails = 0;
  gx -= gyroBiasX;
  gy -= gyroBiasY;
  uint32_t nowUs = micros();
  float dt = (nowUs - lastImuUs) / 1000000.0f;
  lastImuUs = nowUs;
  if (dt <= 0 || dt > 0.1f) dt = IMU_PERIOD_MS / 1000.0f;
  float accRoll = atan2f(ay, az) * RAD_TO_DEG_F;
  float accPitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG_F;
  if (!fusionReady) {
    fusedRoll = accRoll;
    fusedPitch = accPitch;
    fusionReady = true;
  } else {
    fusedRoll = FILTER_ALPHA * (fusedRoll + gx * dt) + (1.0f - FILTER_ALPHA) * accRoll;
    fusedPitch = FILTER_ALPHA * (fusedPitch + gy * dt) + (1.0f - FILTER_ALPHA) * accPitch;
  }
  float p = SWAP_PITCH_ROLL ? fusedRoll : fusedPitch;
  float r = SWAP_PITCH_ROLL ? fusedPitch : fusedRoll;
  mappedPitch = p * PITCH_SIGN;
  mappedRoll = r * ROLL_SIGN;
  detectShake(ax, ay, az, now);
}

float applyDeadzone(float degrees, float deadzone, float maxDegrees) {
  float magnitude = fabsf(degrees);
  if (magnitude <= deadzone) return 0;
  float scaled = (magnitude - deadzone) / (maxDegrees - deadzone);
  if (scaled > 1.0f) scaled = 1.0f;
  return degrees < 0 ? -scaled : scaled;
}

void updateControl() {
  float deltaPitch = mappedPitch - neutralPitch;
  float deltaRoll = mappedRoll - neutralRoll;
  if (armed && (fabsf(deltaPitch) > FLIP_DISARM_DEG || fabsf(deltaRoll) > FLIP_DISARM_DEG)) {
    disarm();
  }
  if (!armed || !imuOk) {
    smoothThrottle = 0;
    smoothSteer = 0;
  } else {
    float targetThrottle = applyDeadzone(deltaPitch, DEADZONE_DEG, PITCH_MAX_DEG);
    float targetSteer = applyDeadzone(deltaRoll, DEADZONE_DEG, ROLL_MAX_DEG);
    smoothThrottle += OUTPUT_SMOOTHING * (targetThrottle - smoothThrottle);
    smoothSteer += OUTPUT_SMOOTHING * (targetSteer - smoothSteer);
  }
  outThrottle = (int8_t)lroundf(smoothThrottle * 100.0f);
  outSteer = (int8_t)lroundf(smoothSteer * 100.0f);
}

void readArmButton(uint32_t now) {
  bool raw = digitalRead(PIN_ARM) == LOW;
  if (raw != armRaw) {
    armRaw = raw;
    armChangeMs = now;
  }
  if (now - armChangeMs >= DEBOUNCE_MS && raw != armStable) {
    armStable = raw;
    if (ARM_HOLD_MODE) {
      if (armStable) tryArm();
      else disarm();
    } else if (armStable) {
      if (armed) disarm();
      else tryArm();
    }
  }
}

uint8_t buildFlags(uint32_t now) {
  uint8_t flags = 0;
  if (armed) flags |= FLAG_ARMED;
  if (armed && now < spinUntilMs) flags |= FLAG_SPIN;
  if (armed && followMode) flags |= FLAG_FOLLOW;
  flags |= (uint8_t)((speedMode & 0x03) << FLAG_SPEED_SHIFT);
  return flags;
}

void sendControl(uint32_t now) {
  ControlPacket packet;
  packet.magic = PACKET_MAGIC;
  packet.teamId = TEAM_ID;
  packet.seq = controlSeq++;
  packet.throttle = armed ? outThrottle : 0;
  packet.steer = armed ? outSteer : 0;
  packet.flags = buildFlags(now);
  packet.checksum = checksum8((const uint8_t *)&packet, sizeof(packet) - 1);
  esp_now_send(CAR_MAC, (const uint8_t *)&packet, sizeof(packet));
}

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1, 400000UL, 400000UL);
bool displayOk = false;
uint8_t displayFails = 0;
uint16_t flushPos = OLED_BYTES;
uint32_t lastFrameMs = 0;

portMUX_TYPE telMux = portMUX_INITIALIZER_UNLOCKED;
TelemetryPacket telPacket = {};
uint32_t telMs = 0;
bool telValid = false;

bool carLink = false;
bool carAck = false;
bool carFault = false;
bool carFollow = false;
uint16_t carMv = 0;
uint16_t carFrontCm = DISTANCE_UNKNOWN;
uint16_t carRearCm = DISTANCE_UNKNOWN;

float handMv = 0;
bool handSeeded = false;
bool handLow = false;
uint8_t handPct = 0;
uint32_t lastHandMs = 0;
uint32_t lastLowBuzzMs = 0;

bool modeRaw = false;
bool modeStable = false;
bool modeLongDone = false;
uint32_t modeChangeMs = 0;
uint32_t modePressMs = 0;

const uint16_t *hapticSteps = nullptr;
uint8_t hapticCount = 0;
uint8_t hapticIdx = 0;
uint32_t hapticStepEndMs = 0;
bool hapticActive = false;

bool spinNow = false;
bool spinPrev = false;

void handleTelemetry(const uint8_t *data, int len) {
  if (len != (int)sizeof(TelemetryPacket)) return;
  TelemetryPacket t;
  memcpy(&t, data, sizeof(t));
  if (t.magic != PACKET_MAGIC || t.teamId != TEAM_ID) return;
  if (t.checksum != checksum8(data, sizeof(t) - 1)) return;
  portENTER_CRITICAL(&telMux);
  telPacket = t;
  telMs = millis();
  telValid = true;
  portEXIT_CRITICAL(&telMux);
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  handleTelemetry(data, len);
}
#else
void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
  handleTelemetry(data, len);
}
#endif

void updateTelemetry(uint32_t now) {
  TelemetryPacket t;
  uint32_t stamp;
  bool valid;
  portENTER_CRITICAL(&telMux);
  t = telPacket;
  stamp = telMs;
  valid = telValid;
  portEXIT_CRITICAL(&telMux);
  bool link = valid && ageMs(now, stamp) <= TELEMETRY_TIMEOUT_MS;
  if (carLink && !link && armed) hapticStart(PAT_LINK_LOST, PATTERN_LEN(PAT_LINK_LOST));
  carLink = link;
  if (link) {
    carMv = t.batteryMv;
    carFrontCm = t.frontCm;
    carRearCm = t.rearCm;
    carAck = (t.flags & TFLAG_ARMED_ACK) != 0;
    carFault = (t.flags & TFLAG_FAULT) != 0;
    carFollow = (t.flags & TFLAG_FOLLOW_ACTIVE) != 0;
  } else {
    carFollow = false;
    carFrontCm = DISTANCE_UNKNOWN;
    carRearCm = DISTANCE_UNKNOWN;
    carAck = false;
    carFault = false;
  }
}

void hapticStart(const uint16_t *steps, uint8_t count) {
  hapticSteps = steps;
  hapticCount = count;
  hapticIdx = 0;
  hapticStepEndMs = millis() + steps[0];
  hapticActive = true;
  digitalWrite(PIN_HAPTIC, HIGH);
}

bool obstacleHaptic(uint32_t now) {
  if (!armed || !carLink) return false;
  uint16_t d = DISTANCE_UNKNOWN;
  if (outThrottle > HAPTIC_MIN_THROTTLE) d = carFrontCm;
  else if (outThrottle < -HAPTIC_MIN_THROTTLE) d = carRearCm;
  if (d == DISTANCE_UNKNOWN || d >= HAPTIC_NEAR_CM) return false;
  if (d <= HAPTIC_VERY_NEAR_CM) return true;
  uint32_t span = HAPTIC_NEAR_CM - HAPTIC_VERY_NEAR_CM;
  uint32_t period = HAPTIC_FAST_MS + (uint32_t)(d - HAPTIC_VERY_NEAR_CM) * (HAPTIC_SLOW_MS - HAPTIC_FAST_MS) / span;
  return (now % period) < HAPTIC_PULSE_MS;
}

void updateHaptic(uint32_t now) {
  if (hapticActive) {
    if ((int32_t)(now - hapticStepEndMs) >= 0) {
      hapticIdx++;
      if (hapticIdx >= hapticCount) {
        hapticActive = false;
        digitalWrite(PIN_HAPTIC, LOW);
      } else {
        digitalWrite(PIN_HAPTIC, (hapticIdx % 2 == 0) ? HIGH : LOW);
        hapticStepEndMs = now + hapticSteps[hapticIdx];
      }
    }
    return;
  }
  digitalWrite(PIN_HAPTIC, obstacleHaptic(now) ? HIGH : LOW);
}

void setLed(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r ? HIGH : LOW);
  digitalWrite(PIN_LED_G, g ? HIGH : LOW);
  digitalWrite(PIN_LED_B, b ? HIGH : LOW);
}

void updateLed(uint32_t now) {
  bool slow = ((now / 500) % 2) == 0;
  bool fast = ((now / 125) % 2) == 0;
  if (!imuOk) {
    setLed(fast, false, fast);
  } else if (armed) {
    setLed(carLink ? true : slow, false, false);
  } else if (handLow || carFault) {
    setLed(fast, fast, false);
  } else if (!carLink) {
    setLed(false, false, slow);
  } else {
    setLed(false, true, false);
  }
}

uint8_t percentFromMv(float mv) {
  if (mv <= CURVE_MV[0]) return 0;
  if (mv >= CURVE_MV[8]) return 100;
  for (int i = 1; i < 9; i++) {
    if (mv <= CURVE_MV[i]) {
      float f = (mv - CURVE_MV[i - 1]) / (float)(CURVE_MV[i] - CURVE_MV[i - 1]);
      return (uint8_t)lroundf(CURVE_PCT[i - 1] + f * (CURVE_PCT[i] - CURVE_PCT[i - 1]));
    }
  }
  return 100;
}

void updateHandBattery(uint32_t now) {
  if (now - lastHandMs < HAND_BATTERY_PERIOD_MS) return;
  lastHandMs = now;
  float mv = analogReadMilliVolts(PIN_BATTERY) * HAND_BATTERY_DIVIDER;
  if (!handSeeded) {
    handMv = mv;
    handSeeded = true;
  } else {
    handMv += HAND_BATTERY_SMOOTHING * (mv - handMv);
  }
  handPct = percentFromMv(handMv);
  if (!BATTERY_WARNINGS) return;
  if (handMv < HAND_LOW_MV) handLow = true;
  else if (handMv > HAND_LOW_MV + 60) handLow = false;
  if (handLow && !hapticActive && now - lastLowBuzzMs >= LOW_BUZZ_PERIOD_MS) {
    lastLowBuzzMs = now;
    hapticStart(PAT_BATTERY_LOW, PATTERN_LEN(PAT_BATTERY_LOW));
  }
}

void cycleSpeedMode() {
  speedMode = (speedMode + 1) % 3;
  if (speedMode == 0) hapticStart(PAT_MODE_1, PATTERN_LEN(PAT_MODE_1));
  else if (speedMode == 1) hapticStart(PAT_MODE_2, PATTERN_LEN(PAT_MODE_2));
  else hapticStart(PAT_MODE_3, PATTERN_LEN(PAT_MODE_3));
}

void toggleFollow() {
  if (!armed) {
    hapticStart(PAT_REJECT, PATTERN_LEN(PAT_REJECT));
    return;
  }
  followMode = !followMode;
  if (followMode) hapticStart(PAT_FOLLOW_ON, PATTERN_LEN(PAT_FOLLOW_ON));
  else hapticStart(PAT_FOLLOW_OFF, PATTERN_LEN(PAT_FOLLOW_OFF));
}

void readModeButton(uint32_t now) {
  bool raw = digitalRead(PIN_MODE) == LOW;
  if (raw != modeRaw) {
    modeRaw = raw;
    modeChangeMs = now;
  }
  if (now - modeChangeMs >= DEBOUNCE_MS && raw != modeStable) {
    modeStable = raw;
    if (modeStable) {
      modePressMs = now;
      modeLongDone = false;
    } else if (!modeLongDone) {
      cycleSpeedMode();
    }
  }
  if (modeStable && !modeLongDone && now - modePressMs >= LONG_PRESS_MS) {
    modeLongDone = true;
    toggleFollow();
  }
}

void updateEvents(uint32_t now) {
  spinNow = armed && now < spinUntilMs;
  if (spinNow && !spinPrev) hapticStart(PAT_SPIN, PATTERN_LEN(PAT_SPIN));
  spinPrev = spinNow;
}

void drawBar(int16_t y, int8_t value, char label) {
  display.setTextSize(1);
  display.setCursor(0, y + 1);
  char text[2] = {label, 0};
  display.print(text);
  display.drawRect(10, y, 118, 9, SSD1306_WHITE);
  int16_t length = (int16_t)abs((int)value) * 57 / 100;
  if (value >= 0) display.fillRect(69, y + 2, length, 5, SSD1306_WHITE);
  else display.fillRect(69 - length, y + 2, length, 5, SSD1306_WHITE);
  display.drawFastVLine(69, y, 9, SSD1306_WHITE);
}

void drawFrame() {
  char line[32];
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(armed ? "ARMED" : "SAFE");
  display.setTextSize(1);
  display.setCursor(80, 0);
  display.print(carLink ? "LINK OK" : "NO LINK");
  if (followMode) {
    display.setCursor(80, 9);
    display.print(carFollow ? "FOLLOW" : "FIND TAG");
  }
  if (!imuOk) snprintf(line, sizeof(line), "IMU FAIL");
  else if (armed && carLink && !carAck) snprintf(line, sizeof(line), "CAR NOT ARMED");
  else snprintf(line, sizeof(line), "Mode %s%s", SPEED_NAMES[speedMode % 3], spinNow ? "  SPIN" : "");
  display.setCursor(0, 18);
  display.print(line);
  drawBar(30, armed ? outThrottle : 0, 'T');
  drawBar(42, armed ? outSteer : 0, 'S');
  if (carLink) snprintf(line, sizeof(line), "C %.2fV  H %.2fV %u%%", carMv / 1000.0f, handMv / 1000.0f, (unsigned)handPct);
  else snprintf(line, sizeof(line), "C --.--V  H %.2fV %u%%", handMv / 1000.0f, (unsigned)handPct);
  display.setCursor(0, 55);
  display.print(line);
}

void flushStep() {
  if (flushPos >= OLED_BYTES) return;
  if (flushPos % OLED_WIDTH == 0) {
    uint8_t page = flushPos / OLED_WIDTH;
    Wire.beginTransmission(OLED_ADDR);
    Wire.write((uint8_t)0x00);
    Wire.write((uint8_t)0x21);
    Wire.write((uint8_t)0);
    Wire.write((uint8_t)(OLED_WIDTH - 1));
    Wire.write((uint8_t)0x22);
    Wire.write(page);
    Wire.write(page);
    if (Wire.endTransmission() != 0) {
      flushPos = OLED_BYTES;
      if (++displayFails >= DISPLAY_FAIL_LIMIT) displayOk = false;
      return;
    }
  }
  Wire.beginTransmission(OLED_ADDR);
  Wire.write((uint8_t)0x40);
  Wire.write(display.getBuffer() + flushPos, OLED_CHUNK);
  if (Wire.endTransmission() != 0) {
    flushPos = OLED_BYTES;
    if (++displayFails >= DISPLAY_FAIL_LIMIT) displayOk = false;
    return;
  }
  displayFails = 0;
  flushPos += OLED_CHUNK;
}

void updateDisplay(uint32_t now) {
  if (!displayOk) return;
  if (flushPos < OLED_BYTES) {
    flushStep();
    return;
  }
  if (now - lastFrameMs >= DISPLAY_PERIOD_MS) {
    lastFrameMs = now;
    drawFrame();
    flushPos = 0;
  }
}

void showBootScreen(const char *text) {
  if (!displayOk) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print(text);
  display.display();
}

bool initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, CAR_MAC, 6);
  peer.channel = WIFI_CHANNEL;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) return false;
  return esp_now_register_recv_cb(onReceive) == ESP_OK;
}

void debugPrint(uint32_t now) {
  if (!DEBUG_SERIAL || now - lastDebugMs < DEBUG_PERIOD_MS) return;
  lastDebugMs = now;
  Serial.printf("pitch %.1f roll %.1f armed %d throttle %d steer %d link %d car %u mV hand %.0f mV %u%%\n",
                mappedPitch, mappedRoll, armed ? 1 : 0, outThrottle, outSteer, carLink ? 1 : 0,
                (unsigned)carMv, handMv, (unsigned)handPct);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_ARM, INPUT_PULLUP);
  pinMode(PIN_MODE, INPUT_PULLUP);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_HAPTIC, OUTPUT);
  setLed(false, false, false);
  digitalWrite(PIN_HAPTIC, LOW);
  pinMode(PIN_BATTERY, INPUT);
  analogSetPinAttenuation(PIN_BATTERY, ADC_11db);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  displayOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  Wire.setClock(400000);
  Serial.printf("oled %s\n", displayOk ? "ok" : "FAILED");
  showBootScreen("Hold still...");
  imuOk = mpuInit();
  Serial.printf("imu %s who_am_i 0x%02X\n", imuOk ? "ok" : "FAILED", whoAmI);
  if (imuOk) calibrateGyro();
  bool radioOk = initEspNow();
  Serial.printf("espnow %s\n", radioOk ? "ok" : "FAILED");
  Serial.println(WiFi.macAddress());
  lastImuUs = micros();
}

void loop() {
  uint32_t now = millis();
  if (now - lastImuMs >= IMU_PERIOD_MS) {
    lastImuMs = now;
    updateFusion(now);
    updateControl();
  }
  readArmButton(now);
  readModeButton(now);
  updateTelemetry(now);
  updateHandBattery(now);
  updateEvents(now);
  if (now - lastSendMs >= SEND_PERIOD_MS) {
    lastSendMs = now;
    sendControl(now);
  }
  updateHaptic(now);
  updateLed(now);
  updateDisplay(now);
  debugPrint(now);
}
