#include <Arduino.h>
#include <ESP32Servo.h>

// ============================================================
// CẤU HÌNH CHÂN GPIO
// ============================================================
const int PIN_LM393      = 32;  // Cảm biến hồng ngoại LM393 (DO)
const int PIN_L298N_ENA  = 25;  // Bật/tắt động cơ JGA25-370 (HIGH = chạy)
const int PIN_L298N_IN1  = 26;  // Chiều quay 1
const int PIN_L298N_IN2  = 27;  // Chiều quay 2
const int PIN_SERVO_1    = 18;  // Servo gạt cam Loại 1
const int PIN_SERVO_2    = 19;  // Servo gạt cam Loại 2

// ============================================================
// TRẠNG THÁI CẢM BIẾN LM393
// ============================================================
const int SENSOR_DETECTED = LOW; // LM393 xuất LOW khi có vật cản (cam đi qua)

// ============================================================
// GÓC SERVO MG90S
// ============================================================
const int SERVO_REST_ANGLE = 0;   // Góc nghỉ - ép sát thành băng chuyền
const int SERVO_GATE_ANGLE = 60;  // Góc chắn - xoay ra chéo băng chuyền để gạt cam

// ============================================================
// THÔNG SỐ THỜI GIAN (ms) - Điều chỉnh theo băng chuyền thực tế
// ============================================================
const int DEBOUNCE_DELAY_MS  = 50;    // Chống nhiễu cảm biến LM393
const int INSPECT_STOP_MS    = 1500;  // Thời gian dừng băng chuyền để chụp ảnh/phân loại
const int SERVO_PREPARE_MS   = 300;   // Chờ Servo xoay ra hoàn toàn trước khi chạy lại băng chuyền
const int PASS_SERVO1_MS     = 2000;  // Thời gian cam đi từ vị trí dừng qua Servo 1 vào máng 1
const int PASS_SERVO2_MS     = 3500;  // Thời gian cam đi từ vị trí dừng qua Servo 2 vào máng 2
const int PASS_THROUGH_MS    = 1000;  // Thời gian cam Loại 3 đi qua khỏi cảm biến LM393

// ============================================================
// ĐỐI TƯỢNG SERVO
// ============================================================
Servo servo1;
Servo servo2;

// ============================================================
// HÀM ĐIỀU KHIỂN BĂNG CHUYỀN
// ============================================================

// Bật động cơ JGA25-370 kéo băng chuyền chạy
void startConveyor() {
  digitalWrite(PIN_L298N_IN1, HIGH);
  digitalWrite(PIN_L298N_IN2, LOW);
  digitalWrite(PIN_L298N_ENA, HIGH);
  Serial.println(">> Băng chuyền: CHẠY");
}

// Ngắt động cơ JGA25-370 dừng băng chuyền ngay lập tức
void stopConveyor() {
  digitalWrite(PIN_L298N_IN1, LOW);
  digitalWrite(PIN_L298N_IN2, LOW);
  digitalWrite(PIN_L298N_ENA, LOW);
  Serial.println(">> Băng chuyền: DỪNG");
}

// ============================================================
// HÀM PHÂN LOẠI CHẤT LƯỢNG CAM
// ============================================================

// TODO: Tích hợp nhận diện kết quả từ Camera tại đây
// Hiện tại dùng random để mô phỏng phân loại:
//   1 = Loại 1 (gạt bằng Servo 1)
//   2 = Loại 2 (gạt bằng Servo 2)
//   3 = Loại 3 (không gạt, chạy thẳng hết băng chuyền)
int classifyOrange() {
  return random(1, 4); // Sinh ngẫu nhiên 1, 2 hoặc 3
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  // Khởi tạo Serial Monitor
  Serial.begin(115200);
  Serial.println("=== HỆ THỐNG PHÂN LOẠI CAM - KHỞI ĐỘNG ===");

  // Cấu hình chân cảm biến LM393
  pinMode(PIN_LM393, INPUT_PULLUP);

  // Cấu hình chân điều khiển L298N (bật/tắt đơn giản, không cần PWM)
  pinMode(PIN_L298N_ENA, OUTPUT);
  pinMode(PIN_L298N_IN1, OUTPUT);
  pinMode(PIN_L298N_IN2, OUTPUT);

  // Khởi tạo 2 Servo và đưa về góc nghỉ (0°)
  servo1.attach(PIN_SERVO_1);
  servo2.attach(PIN_SERVO_2);
  servo1.write(SERVO_REST_ANGLE);
  servo2.write(SERVO_REST_ANGLE);

  // Khởi tạo seed cho hàm random
  randomSeed(analogRead(35));

  // Bật băng chuyền chạy chờ cam
  startConveyor();

  Serial.println("=== SẴN SÀNG - CHỜ CAM ĐI QUA CẢM BIẾN ===");
}

// ============================================================
// LOOP - Quy trình chính
// ============================================================
void loop() {
  // Kiểm tra cảm biến LM393 phát hiện cam đi qua
  if (digitalRead(PIN_LM393) == SENSOR_DETECTED) {

    // Chống nhiễu: delay ngắn rồi kiểm tra lại
    delay(DEBOUNCE_DELAY_MS);
    if (digitalRead(PIN_LM393) != SENSOR_DETECTED) {
      return; // Nhiễu, bỏ qua
    }

    // ---- Bước 1: Dừng băng chuyền & Delay chụp ảnh ----
    stopConveyor();
    Serial.println("Phát hiện cam! Dừng băng chuyền để kiểm tra...");
    delay(INSPECT_STOP_MS);

    // ---- Bước 2: Phân loại chất lượng ----
    int quality = classifyOrange();
    Serial.print("Kết quả phân loại: Loại ");
    Serial.println(quality);

    // ---- Bước 3: Xoay Servo sẵn sàng & Chạy lại băng chuyền ----
    switch (quality) {
      case 1:
        // Xoay Servo 1 ra chắn sẵn sàng đón cam
        servo1.write(SERVO_GATE_ANGLE);
        Serial.println("Servo 1: Xoay ra góc chắn đón cam");
        delay(SERVO_PREPARE_MS);

        // Bật lại băng chuyền cho cam đi qua Servo 1 vào máng 1
        startConveyor();
        delay(PASS_SERVO1_MS);

        // Thu Servo 1 về vị trí nghỉ
        servo1.write(SERVO_REST_ANGLE);
        Serial.println("Servo 1: Thu về góc nghỉ");
        break;

      case 2:
        // Xoay Servo 2 ra chắn sẵn sàng đón cam
        servo2.write(SERVO_GATE_ANGLE);
        Serial.println("Servo 2: Xoay ra góc chắn đón cam");
        delay(SERVO_PREPARE_MS);

        // Bật lại băng chuyền cho cam đi qua Servo 2 vào máng 2
        startConveyor();
        delay(PASS_SERVO2_MS);

        // Thu Servo 2 về vị trí nghỉ
        servo2.write(SERVO_REST_ANGLE);
        Serial.println("Servo 2: Thu về góc nghỉ");
        break;

      case 3:
        // Loại 3: Không gạt, cho cam chạy thẳng hết băng chuyền
        Serial.println("Loại 3: Không gạt, cam chạy thẳng hết băng chuyền");
        startConveyor();
        delay(PASS_THROUGH_MS);
        break;
    }

    // ---- Bước 4: Đảm bảo cam đã rời khỏi cảm biến ----
    while (digitalRead(PIN_LM393) == SENSOR_DETECTED) {
      delay(20);
    }

    Serial.println("--- Sẵn sàng đón quả cam tiếp theo ---\n");
  }
}