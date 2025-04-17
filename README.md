# 💊 MediBox – Smart Medicine Reminder System

MediBox is an Arduino-based smart medicine reminder system that helps users manage their medication schedules with ease. It features alarm notifications, environmental tracking, and an intuitive OLED display interface. This project is built and tested using the **Wokwi Simulator** and supports integration with real-world hardware.

---

## 🚀 Features

- ⏰ **Set Current Time**  
  Set and update the current time using push buttons.

- ⏳ **Set Two Alarms**  
  Schedule up to two alarms to remind the user when it's time to take their medication.

- ❌ **Remove Alarms**  
  Easily clear any previously set alarm.

- 🌡️ **Temperature and Humidity Monitoring**  
  Uses a DHT22 sensor to monitor environmental conditions. Triggers an alert if values are out of a safe range.

- 🔔 **Alarm Notification**  
  Buzzer and LED alerts ensure the user doesn’t miss their medicine time.

---

## 📟 Core Functions

- `void print_line(String message)`  
  Prints a message line on the OLED display.

- `void update_time()`  
  Updates the system time using the `millis()` function.

- `void print_time_now()`  
  Displays current time in Days, Hours, Minutes, and Seconds on the OLED.

- `void update_time_with_check_alarm()`  
  Updates and displays time. Checks for any set alarms and triggers them if it's medicine time.

- `void ring_alarm()`  
  Activates the buzzer and LED, displaying an alert message on OLED.

- `int wait_for_button_press()`  
  Detects and returns the identifier of the pressed push button.

- `void go_to_menu()`  
  Navigates through different modes based on button press.

- `void run_mode()`  
  Executes the selected function/mode.

- `void check_temp()`  
  Monitors temperature and humidity using DHT22. Displays warnings and lights LED if needed.

---

## 🛠️ Tools & Technologies

| Tool | Purpose |
|------|---------|
| 🧪 Wokwi Simulator | Arduino + ESP32 simulation and testing |
| 💡 Fritzing | Circuit design and documentation |
| 🔧 EasyEDA | Circuit simulation and PCB design |
| 🧠 Altium CircuitMaker | Professional PCB and schematic design |
| 💻 Arduino IDE | Arduino programming |
| 🌐 Node-RED | IoT integration and flow management |
| 🖥️ VS Code | Coding in C++ with advanced IDE features |

