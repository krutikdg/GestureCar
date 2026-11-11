#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <VL53L1X.h>
#include <Adafruit_NeoPixel.h>

const uint8_t PIN_L_IN1 = 25;
const uint8_t PIN_L_IN2 = 26;
const uint8_t PIN_L_PWM = 27;
const uint8_t PIN_R_IN1 = 32;
const uint8_t PIN_R_IN2 = 33;
const uint8_t PIN_R_PWM = 13;
const uint8_t PIN_STBY = 23;
const uint8_t PIN_BATTERY = 34;
const uint8_t PIN_SDA = 21;
const uint8_t PIN_SCL = 22;
const uint8_t PIN_US_TRIG = 18;
const uint8_t PIN_US_ECHO = 19;
const uint8_t PIN_LED_STRIP = 4;

const uint8_t WIFI_CHANNEL = 1;
const uint8_t TEAM_ID = 7;
const uint8_t PACKET_MAGIC = 0xA7;

const bool DEBUG_SERIAL = true;
const bool LEFT_REVERSED = false;
const bool RIGHT_REVERSED = false;
const bool BRAKE_AT_ZERO = true;
const bool BATTERY_GUARD = true;

const uint32_t PWM_FREQ_HZ = 20000;
const uint8_t PWM_BITS = 8;
const float MOTOR_MIN_DUTY = 0.30f;
const float DEADBAND = 0.04f;

const float SPEED_SCALE[3] = {0.45f, 0.75f, 1.0f};
const float STEER_GAIN = 0.7f;
const float SPIN_POWER = 0.8f;
const float ACCEL_PER_SEC = 3.0f;
const float DECEL_PER_SEC = 6.0f;

const uint32_t FAILSAFE_MS = 250;
const uint32_t CONTROL_PERIOD_MS = 10;
const uint32_t BATTERY_PERIOD_MS = 20;
const uint32_t TELEMETRY_PERIOD_MS = 100;
const uint32_t DEBUG_PERIOD_MS = 250;

const float BATTERY_DIVIDER = 3.0f;
const float BATTERY_SMOOTHING = 0.05f;
const uint16_t BATTERY_LOW_MV = 6800;
const uint16_t BATTERY_CUT_MV = 6400;
const uint16_t BATTERY_RECOVER_MV = 6900;
const uint32_t BATTERY_CUT_HOLD_MS = 1500;
const uint16_t DISTANCE_UNKNOWN = 0xFFFF;

const bool AUTOBRAKE_ENABLED = true;
const uint16_t BRAKE_STOP_CM = 25;
const uint16_t BRAKE_SLOW_CM = 70;
const float SENSOR_LOST_LIMIT = 0.30f;
const uint32_t RANGE_STALE_MS = 400;
const uint16_t RANGE_MAX_CM = 400;
const uint16_t TOF_BUDGET_US = 50000;
const uint32_t TOF_PERIOD_MS = 50;
const uint32_t TOF_RETRY_MS = 2000;
const uint32_t ULTRASONIC_PERIOD_MS = 60;
const uint32_t ULTRASONIC_WAIT_MS = 35;
const uint32_t ULTRASONIC_FAR_US = 23200;

const bool FOLLOW_ENABLED = true;
const uint16_t FOLLOW_TARGET_CM = 150;
const uint16_t FOLLOW_DEADBAND_CM = 20;
const uint16_t FOLLOW_MAX_CM = 800;
const float FOLLOW_KP = 0.8f;
const float FOLLOW_MAX_THROTTLE = 0.6f;
const float FOLLOW_BEARING_FULL_DEG = 35.0f;
const float FOLLOW_TURN_ONLY_DEG = 50.0f;
const float FOLLOW_STEER_DEADBAND_DEG = 5.0f;
const float FOLLOW_FILTER = 0.4f;
const uint32_t FOLLOW_FIX_STALE_MS = 600;

const bool UNDERGLOW_ENABLED = true;
const uint16_t UNDERGLOW_LEDS = 12;
const uint8_t UNDERGLOW_BRIGHTNESS = 60;
const uint32_t UNDERGLOW_PERIOD_MS = 33;

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

struct Motor {
  uint8_t in1;
  uint8_t in2;
  uint8_t pwm;
  uint8_t channel;
  bool reversed;
};

Motor leftMotor = {PIN_L_IN1, PIN_L_IN2, PIN_L_PWM, 0, LEFT_REVERSED};
Motor rightMotor = {PIN_R_IN1, PIN_R_IN2, PIN_R_PWM, 1, RIGHT_REVERSED};

portMUX_TYPE rxMux = portMUX_INITIALIZER_UNLOCKED;
ControlPacket rxPacket = {};
uint32_t rxMs = 0;
bool rxValid = false;
volatile uint32_t rxTotal = 0;
volatile uint32_t rxLost = 0;
uint8_t rxLastSeq = 0;
uint8_t peerMac[6] = {0, 0, 0, 0, 0, 0};
bool peerPending = false;

uint8_t telemetryMac[6] = {0, 0, 0, 0, 0, 0};
bool telemetryReady = false;
uint8_t telemetrySeq = 0;

float curLeft = 0;
float curRight = 0;
bool linkOk = false;
bool armedNow = false;
bool spinActive = false;
uint8_t speedMode = 1;

float batteryMv = 0;
bool batterySeeded = false;
bool batteryLow = false;
bool batteryCut = false;
bool cutTiming = false;
uint32_t cutSince = 0;

uint32_t lastControlMs = 0;
uint32_t lastBatteryMs = 0;
uint32_t lastTelemetryMs = 0;
uint32_t lastDebugMs = 0;

struct Range {
  uint16_t cm;
  uint32_t stamp;
  bool everValid;
};

Range frontRange = {DISTANCE_UNKNOWN, 0, false};
Range rearRange = {DISTANCE_UNKNOWN, 0, false};
VL53L1X tof;
bool tofOk = false;
uint32_t lastTofTryMs = 0;

portMUX_TYPE echoMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t echoRiseUs = 0;
volatile uint32_t echoFallUs = 0;
volatile bool echoRose = false;
volatile bool echoFell = false;
bool usWaiting = false;
uint32_t usSentMs = 0;
uint32_t lastUsMs = 0;

bool obstacleBraking = false;
bool followRequested = false;
bool followActive = false;

struct UwbFix {
  float distCm;
  float bearingDeg;
  uint32_t stamp;
  bool valid;
};

UwbFix uwbFix = {0, 0, 0, false};

Adafruit_NeoPixel strip(UNDERGLOW_LEDS, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);
uint32_t lastGlowMs = 0;

float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

uint8_t checksum8(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) sum ^= data[i];
  return sum;
}

uint32_t ageMs(uint32_t now, uint32_t then) {
  int32_t d = (int32_t)(now - then);
  return d < 0 ? 0 : (uint32_t)d;
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void pwmSetup(uint8_t pin, uint8_t channel) {
  ledcAttach(pin, PWM_FREQ_HZ, PWM_BITS);
}
void pwmWrite(uint8_t pin, uint8_t channel, uint32_t duty) {
  ledcWrite(pin, duty);
}
#else
void pwmSetup(uint8_t pin, uint8_t channel) {
  ledcSetup(channel, PWM_FREQ_HZ, PWM_BITS);
  ledcAttachPin(pin, channel);
}
void pwmWrite(uint8_t pin, uint8_t channel, uint32_t duty) {
  ledcWrite(channel, duty);
}
#endif

void initMotor(const Motor &m) {
  pinMode(m.in1, OUTPUT);
  pinMode(m.in2, OUTPUT);
  digitalWrite(m.in1, LOW);
  digitalWrite(m.in2, LOW);
  pwmSetup(m.pwm, m.channel);
  pwmWrite(m.pwm, m.channel, 0);
}

void releaseMotor(const Motor &m) {
  digitalWrite(m.in1, LOW);
  digitalWrite(m.in2, LOW);
  pwmWrite(m.pwm, m.channel, 0);
}

void driveMotor(const Motor &m, float cmd) {
  if (m.reversed) cmd = -cmd;
  float mag = fabsf(cmd);
  if (mag > 1.0f) mag = 1.0f;
  if (mag < DEADBAND) {
    uint8_t level = BRAKE_AT_ZERO ? HIGH : LOW;
    digitalWrite(m.in1, level);
    digitalWrite(m.in2, level);
    pwmWrite(m.pwm, m.channel, 0);
    return;
  }
  digitalWrite(m.in1, cmd > 0 ? HIGH : LOW);
  digitalWrite(m.in2, cmd > 0 ? LOW : HIGH);
  float duty = MOTOR_MIN_DUTY + (1.0f - MOTOR_MIN_DUTY) * mag;
  pwmWrite(m.pwm, m.channel, (uint32_t)lroundf(duty * ((1u << PWM_BITS) - 1)));
}

void stopMotors() {
  curLeft = 0;
  curRight = 0;
  releaseMotor(leftMotor);
  releaseMotor(rightMotor);
  digitalWrite(PIN_STBY, LOW);
}

void mixDrive(float throttle, float steer, bool spin, uint8_t mode, float &left, float &right) {
  if (spin) {
    left = SPIN_POWER;
    right = -SPIN_POWER;
    return;
  }
  float l = throttle + steer * STEER_GAIN;
  float r = throttle - steer * STEER_GAIN;
  float peak = fmaxf(fabsf(l), fabsf(r));
  if (peak > 1.0f) {
    l /= peak;
    r /= peak;
  }
  left = l * SPEED_SCALE[mode];
  right = r * SPEED_SCALE[mode];
}

float slew(float current, float target, float dt) {
  float rate = fabsf(target) > fabsf(current) ? ACCEL_PER_SEC : DECEL_PER_SEC;
  float step = rate * dt;
  if (target > current + step) return current + step;
  if (target < current - step) return current - step;
  return target;
}

void handleReceive(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != (int)sizeof(ControlPacket)) return;
  ControlPacket p;
  memcpy(&p, data, sizeof(p));
  if (p.magic != PACKET_MAGIC || p.teamId != TEAM_ID) return;
  if (p.checksum != checksum8(data, sizeof(p) - 1)) return;
  portENTER_CRITICAL(&rxMux);
  if (rxTotal > 0) {
    uint8_t gap = (uint8_t)(p.seq - rxLastSeq - 1);
    if (gap < 100) rxLost += gap;
  }
  rxLastSeq = p.seq;
  rxTotal++;
  rxPacket = p;
  rxMs = millis();
  rxValid = true;
  if (memcmp(mac, peerMac, 6) != 0) {
    memcpy(peerMac, mac, 6);
    peerPending = true;
  }
  portEXIT_CRITICAL(&rxMux);
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  handleReceive(info->src_addr, data, len);
}
#else
void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
  handleReceive(mac, data, len);
}
#endif

bool fetchPacket(ControlPacket &out, uint32_t &stamp) {
  portENTER_CRITICAL(&rxMux);
  out = rxPacket;
  stamp = rxMs;
  bool valid = rxValid;
  portEXIT_CRITICAL(&rxMux);
  return valid;
}

void ensurePeer() {
  if (!peerPending) return;
  uint8_t mac[6];
  portENTER_CRITICAL(&rxMux);
  memcpy(mac, peerMac, 6);
  peerPending = false;
  portEXIT_CRITICAL(&rxMux);
  memcpy(telemetryMac, mac, 6);
  if (esp_now_is_peer_exist(mac)) {
    telemetryReady = true;
    return;
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = WIFI_CHANNEL;
  peer.encrypt = false;
  telemetryReady = esp_now_add_peer(&peer) == ESP_OK;
}

void updateBattery(uint32_t now) {
  float pack = analogReadMilliVolts(PIN_BATTERY) * BATTERY_DIVIDER;
  if (!batterySeeded) {
    batteryMv = pack;
    batterySeeded = true;
  } else {
    batteryMv += BATTERY_SMOOTHING * (pack - batteryMv);
  }
  batteryLow = batteryMv < BATTERY_LOW_MV;
  if (!BATTERY_GUARD) return;
  if (batteryMv < BATTERY_CUT_MV) {
    if (!cutTiming) {
      cutTiming = true;
      cutSince = now;
    } else if (now - cutSince >= BATTERY_CUT_HOLD_MS) {
      batteryCut = true;
    }
  } else {
    cutTiming = false;
    if (batteryMv > BATTERY_RECOVER_MV) batteryCut = false;
  }
}

void setRange(Range &r, uint16_t cm, uint32_t now) {
  r.cm = cm;
  r.stamp = now;
  r.everValid = true;
}

bool rangeFresh(const Range &r, uint32_t now) {
  return r.everValid && ageMs(now, r.stamp) <= RANGE_STALE_MS;
}

uint16_t rangeCm(const Range &r, uint32_t now) {
  return rangeFresh(r, now) ? r.cm : DISTANCE_UNKNOWN;
}

float rangeLimit(const Range &r, uint32_t now) {
  if (!r.everValid) return 1.0f;
  if (!rangeFresh(r, now)) return SENSOR_LOST_LIMIT;
  if (r.cm <= BRAKE_STOP_CM) return 0.0f;
  if (r.cm >= BRAKE_SLOW_CM) return 1.0f;
  return (float)(r.cm - BRAKE_STOP_CM) / (float)(BRAKE_SLOW_CM - BRAKE_STOP_CM);
}

bool initTof() {
  tof.setTimeout(100);
  if (!tof.init()) return false;
  tof.setDistanceMode(VL53L1X::Long);
  tof.setMeasurementTimingBudget(TOF_BUDGET_US);
  tof.startContinuous(TOF_PERIOD_MS);
  return true;
}

void updateTof(uint32_t now) {
  if (!tofOk) {
    if (armedNow || ageMs(now, lastTofTryMs) < TOF_RETRY_MS) return;
    lastTofTryMs = now;
    tofOk = initTof();
    return;
  }
  if (!tof.dataReady()) return;
  tof.read(false);
  uint16_t cm = tof.ranging_data.range_mm / 10;
  VL53L1X::RangeStatus st = tof.ranging_data.range_status;
  if (st == VL53L1X::RangeValid) setRange(frontRange, cm > RANGE_MAX_CM ? RANGE_MAX_CM : cm, now);
  else if (st == VL53L1X::RangeValidMinRangeClipped) setRange(frontRange, cm, now);
  else if (st == VL53L1X::SignalFail) setRange(frontRange, RANGE_MAX_CM, now);
}

void IRAM_ATTR echoIsr() {
  uint32_t t = micros();
  bool high = digitalRead(PIN_US_ECHO) != 0;
  portENTER_CRITICAL_ISR(&echoMux);
  if (high) {
    echoRiseUs = t;
    echoRose = true;
    echoFell = false;
  } else if (echoRose) {
    echoFallUs = t;
    echoFell = true;
  }
  portEXIT_CRITICAL_ISR(&echoMux);
}

void updateUltrasonic(uint32_t now) {
  if (!usWaiting) {
    if (ageMs(now, lastUsMs) < ULTRASONIC_PERIOD_MS) return;
    lastUsMs = now;
    portENTER_CRITICAL(&echoMux);
    echoRose = false;
    echoFell = false;
    portEXIT_CRITICAL(&echoMux);
    digitalWrite(PIN_US_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_US_TRIG, LOW);
    usWaiting = true;
    usSentMs = now;
    return;
  }
  if (ageMs(now, usSentMs) < ULTRASONIC_WAIT_MS) return;
  usWaiting = false;
  uint32_t rise;
  uint32_t fall;
  bool rose;
  bool fell;
  portENTER_CRITICAL(&echoMux);
  rise = echoRiseUs;
  fall = echoFallUs;
  rose = echoRose;
  fell = echoFell;
  portEXIT_CRITICAL(&echoMux);
  if (!rose) return;
  if (!fell) {
    setRange(rearRange, RANGE_MAX_CM, now);
    return;
  }
  uint32_t pulse = fall - rise;
  if (pulse >= ULTRASONIC_FAR_US) {
    setRange(rearRange, RANGE_MAX_CM, now);
    return;
  }
  uint32_t cm = pulse / 58;
  if (cm < 1) cm = 1;
  if (cm > RANGE_MAX_CM) cm = RANGE_MAX_CM;
  setRange(rearRange, (uint16_t)cm, now);
}

void uwbSubmit(float distCm, float bearingDeg, uint32_t now) {
  if (!(distCm > 0.0f) || distCm > FOLLOW_MAX_CM) return;
  bearingDeg = clampf(bearingDeg, -90.0f, 90.0f);
  if (!uwbFix.valid || ageMs(now, uwbFix.stamp) > FOLLOW_FIX_STALE_MS) {
    uwbFix.distCm = distCm;
    uwbFix.bearingDeg = bearingDeg;
  } else {
    uwbFix.distCm += FOLLOW_FILTER * (distCm - uwbFix.distCm);
    uwbFix.bearingDeg += FOLLOW_FILTER * (bearingDeg - uwbFix.bearingDeg);
  }
  uwbFix.stamp = now;
  uwbFix.valid = true;
}

bool followCompute(uint32_t now, float &throttle, float &steer) {
  if (!uwbFix.valid || ageMs(now, uwbFix.stamp) > FOLLOW_FIX_STALE_MS) return false;
  float error = uwbFix.distCm - (FOLLOW_TARGET_CM + FOLLOW_DEADBAND_CM);
  float forward = error > 0 ? FOLLOW_KP * error / 100.0f : 0.0f;
  forward = clampf(forward, 0.0f, FOLLOW_MAX_THROTTLE);
  float bearing = uwbFix.bearingDeg;
  float turnFactor = 1.0f - fabsf(bearing) / FOLLOW_TURN_ONLY_DEG;
  if (turnFactor < 0.0f) turnFactor = 0.0f;
  throttle = forward * turnFactor;
  steer = fabsf(bearing) < FOLLOW_STEER_DEADBAND_DEG ? 0.0f : clampf(bearing / FOLLOW_BEARING_FULL_DEG, -1.0f, 1.0f);
  return true;
}

float breathe(uint32_t now, uint32_t periodMs) {
  float phase = (now % periodMs) / (float)periodMs;
  return 0.15f + 0.85f * (0.5f - 0.5f * cosf(phase * 6.2831853f));
}

void glowFill(uint8_t r, uint8_t g, uint8_t b, float scale) {
  uint32_t c = strip.Color((uint8_t)(r * scale), (uint8_t)(g * scale), (uint8_t)(b * scale));
  for (uint16_t i = 0; i < UNDERGLOW_LEDS; i++) strip.setPixelColor(i, c);
}

void glowChase(uint32_t now) {
  for (uint16_t i = 0; i < UNDERGLOW_LEDS; i++) {
    uint16_t hue = (uint16_t)((now * 40UL + i * (65536UL / UNDERGLOW_LEDS)) & 0xFFFF);
    strip.setPixelColor(i, strip.ColorHSV(hue, 255, 255));
  }
}

void updateUnderglow(uint32_t now) {
  if (!UNDERGLOW_ENABLED || ageMs(now, lastGlowMs) < UNDERGLOW_PERIOD_MS) return;
  lastGlowMs = now;
  bool fastBlink = ((now / 250) % 2) == 0;
  bool slowBlink = ((now / 500) % 2) == 0;
  if (batteryCut) glowFill(255, 0, 0, fastBlink ? 1.0f : 0.0f);
  else if (batteryLow && slowBlink) glowFill(255, 90, 0, 1.0f);
  else if (!linkOk) glowFill(0, 40, 255, breathe(now, 3000));
  else if (!armedNow) glowFill(0, 255, 60, breathe(now, 3000));
  else if (spinActive) glowChase(now);
  else if (followActive) glowFill(160, 0, 255, breathe(now, 800));
  else if (followRequested) glowFill(160, 0, 255, slowBlink ? 1.0f : 0.1f);
  else if (obstacleBraking) glowFill(255, 0, 0, 1.0f);
  else if (curLeft + curRight < -0.1f) glowFill(255, 255, 255, 1.0f);
  else if (speedMode == 0) glowFill(0, 200, 255, 1.0f);
  else if (speedMode == 1) glowFill(0, 255, 60, 1.0f);
  else glowFill(255, 0, 200, 1.0f);
  strip.show();
}

void controlStep(uint32_t now, float dt) {
  ControlPacket p = {};
  uint32_t stamp = 0;
  bool have = fetchPacket(p, stamp);
  linkOk = have && ageMs(now, stamp) <= FAILSAFE_MS;
  bool wantArmed = linkOk && (p.flags & FLAG_ARMED);
  armedNow = wantArmed && !batteryCut;
  if (!armedNow) {
    stopMotors();
    spinActive = false;
    obstacleBraking = false;
    followRequested = false;
    followActive = false;
    return;
  }
  uint8_t mode = (p.flags >> FLAG_SPEED_SHIFT) & 0x03;
  if (mode > 2) mode = 1;
  
  speedMode = mode;
  spinActive = (p.flags & FLAG_SPIN) != 0;
  float throttle = clampf(p.throttle / 100.0f, -1.0f, 1.0f);
  float steer = clampf(p.steer / 100.0f, -1.0f, 1.0f);
  followRequested = FOLLOW_ENABLED && (p.flags & FLAG_FOLLOW) != 0;
  followActive = false;
  if (followRequested) {
    spinActive = false;
    float followThrottle = 0;
    float followSteer = 0;
    followActive = followCompute(now, followThrottle, followSteer);
    throttle = followThrottle;
    steer = followSteer;
  }
  bool braking = false;
  if (AUTOBRAKE_ENABLED && !spinActive) {
    if (throttle > 0) {
      float limit = rangeLimit(frontRange, now);
      if (throttle > limit) {
        throttle = limit;
        braking = true;
      }
    } else if (throttle < 0) {
      float limit = rangeLimit(rearRange, now);
      if (-throttle > limit) {
        throttle = -limit;
        braking = true;
      }
    }
  }
  obstacleBraking = braking;
  float left = 0;
  float right = 0;
  mixDrive(throttle, steer, spinActive, mode, left, right);
  digitalWrite(PIN_STBY, HIGH);
  curLeft = slew(curLeft, left, dt);
  curRight = slew(curRight, right, dt);
  if (braking) {
    if (fabsf(left) < fabsf(curLeft)) curLeft = left;
    if (fabsf(right) < fabsf(curRight)) curRight = right;
  }
  driveMotor(leftMotor, curLeft);
  driveMotor(rightMotor, curRight);
}

void sendTelemetry(uint32_t now) {
  if (!telemetryReady) return;
  TelemetryPacket t;
  t.magic = PACKET_MAGIC;
  t.teamId = TEAM_ID;
  t.seq = telemetrySeq++;
  t.batteryMv = (uint16_t)lroundf(clampf(batteryMv, 0.0f, 65000.0f));
  t.frontCm = rangeCm(frontRange, now);
  t.rearCm = rangeCm(rearRange, now);
  t.flags = 0;
  if (armedNow) t.flags |= TFLAG_ARMED_ACK;
  if (followActive) t.flags |= TFLAG_FOLLOW_ACTIVE;
  if (obstacleBraking) t.flags |= TFLAG_OBSTACLE;
  if (batteryLow || batteryCut) t.flags |= TFLAG_FAULT;
  t.checksum = checksum8((const uint8_t *)&t, sizeof(t) - 1);
  esp_now_send(telemetryMac, (const uint8_t *)&t, sizeof(t));
}

bool initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return false;
  return esp_now_register_recv_cb(onReceive) == ESP_OK;
}

void debugPrint(uint32_t now) {
  if (!DEBUG_SERIAL || now - lastDebugMs < DEBUG_PERIOD_MS) return;
  lastDebugMs = now;
  Serial.printf("link %d armed %d mode %u spin %d L %.2f R %.2f batt %.0f mV low %d cut %d rx %lu lost %lu front %u rear %u brake %d tof %d follow %d tag %.0f cm %.0f deg\n",
                linkOk ? 1 : 0, armedNow ? 1 : 0, speedMode, spinActive ? 1 : 0, curLeft, curRight,
                batteryMv, batteryLow ? 1 : 0, batteryCut ? 1 : 0,
                (unsigned long)rxTotal, (unsigned long)rxLost,
                (unsigned)rangeCm(frontRange, now), (unsigned)rangeCm(rearRange, now),
                obstacleBraking ? 1 : 0, tofOk ? 1 : 0, followActive ? 1 : 0,
                uwbFix.valid ? uwbFix.distCm : -1.0f, uwbFix.bearingDeg);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_STBY, OUTPUT);
  digitalWrite(PIN_STBY, LOW);
  initMotor(leftMotor);
  initMotor(rightMotor);
  pinMode(PIN_BATTERY, INPUT);
  analogSetPinAttenuation(PIN_BATTERY, ADC_11db);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  tofOk = initTof();
  Serial.printf("tof %s\n", tofOk ? "ok" : "FAILED");
  pinMode(PIN_US_TRIG, OUTPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  pinMode(PIN_US_ECHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_US_ECHO), echoIsr, CHANGE);
  strip.begin();
  strip.setBrightness(UNDERGLOW_BRIGHTNESS);
  strip.clear();
  strip.show();
  bool radioOk = initEspNow();
  Serial.printf("espnow %s\n", radioOk ? "ok" : "FAILED");
  Serial.println(WiFi.macAddress());
  uint32_t now = millis();
  lastControlMs = now;
  lastBatteryMs = now;
  lastTelemetryMs = now;
  lastDebugMs = now;
  lastTofTryMs = now;
  lastUsMs = now;
  lastGlowMs = now;
}

void loop() {
  uint32_t now = millis();
  ensurePeer();
  updateTof(now);
  updateUltrasonic(now);
  if (now - lastBatteryMs >= BATTERY_PERIOD_MS) {
    lastBatteryMs = now;
    updateBattery(now);
  }
  if (now - lastControlMs >= CONTROL_PERIOD_MS) {
    float dt = clampf((now - lastControlMs) / 1000.0f, 0.001f, 0.05f);
    lastControlMs = now;
    controlStep(now, dt);
  }
  if (now - lastTelemetryMs >= TELEMETRY_PERIOD_MS) {
    lastTelemetryMs = now;
    sendTelemetry(now);
  }
  updateUnderglow(now);
  debugPrint(now);
}
