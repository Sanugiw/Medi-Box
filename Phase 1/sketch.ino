#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C  // Fixed typo in SCREEN_ADDRESS

#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

#define NTP_SERVER "pool.ntp.org"

#define MAX_ALARMS 2  

// Global variables
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

int utc_offset = 0; // Store UTC offset in seconds, initialized to 0 (UTC)
int utc_offset_dst = 0; // No DST adjustment by default

unsigned long timeNow = 0;
unsigned long timeLast = 0;
unsigned long alarmSnoozeTime = 0;

bool alarm_enabled = true;
int n_alarms = MAX_ALARMS;
int alarm_hours[MAX_ALARMS] = {0};
int alarm_minutes[MAX_ALARMS] = {0};
bool alarm_active[MAX_ALARMS] = {false};
bool alarm_triggered[MAX_ALARMS] = {false};

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

int current_mode = 0;
int max_modes = 6;
String modes[] = {
  "1 - Set Alarm 1", 
  "2 - Set Alarm 2", 
  "3 - View Alarms", 
  "4 - Delete Alarm",
  "5 - Disable Alarms",
  "6 - Set Time Zone"
};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

void setup() {
  // put your setup code here, to run once:
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);
  // Initialize serial monitor and OLED display
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 initialization failed"));
    for (;;);
  }

  // Turn on OLED display
  display.display();
  delay(500);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0, 0, 2);
    display.display();  // Add missing display update
  }

  display.clearDisplay();
  print_line("Connected to WIFI", 0, 0, 2);
  display.display();  // Add missing display update
  delay(1000);  // Add delay to show the connected message

  // Initially set time to UTC, time zone can be configured later
  configTime(utc_offset, utc_offset_dst, NTP_SERVER);

  // Clear OLED display
  display.clearDisplay();

  print_line("Welcome to Medi Box", 10, 10, 2);
  display.display();  // Add missing display update
  delay(2000);  // Allow time to read the welcome message
  display.clearDisplay();
  
  // Prompt user to set time zone on first boot
  display.clearDisplay();
  print_line("Please set your", 0, 0, 1);
  print_line("time zone", 0, 20, 1);
  display.display();  // Add missing display update
  delay(2000);
  set_time_zone();
}

void loop() {
  update_time_with_check_alarm();
  if (digitalRead(PB_OK) == LOW) {
    delay(200);  // Debounce
    go_to_menu();
  }
  check_temp();
}

void print_line(String text, int column, int row, int text_size) {
  // Display a custom message
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();
  // Note: display.display() is not called here to allow multiple lines before updating
}

void print_time_now() {
  display.clearDisplay();
  
  // Format time with leading zeros for better readability
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
  String minutesStr = minutes < 10 ? "0" + String(minutes) : String(minutes);
  String secondsStr = seconds < 10 ? "0" + String(seconds) : String(seconds);
  String daysStr = days < 10 ? "0" + String(days) : String(days);
  
  print_line(daysStr, 0, 0, 2);
  print_line(":", 20, 0, 2);
  print_line(hoursStr, 30, 0, 2);
  print_line(":", 50, 0, 2);
  print_line(minutesStr, 60, 0, 2);
  print_line(":", 80, 0, 2);
  print_line(secondsStr, 90, 0, 2);
  
  display.display();  // Update display once all elements are drawn
}

void update_time() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;  // Add error handling for time sync failure
  }

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay, 3, "%d", &timeinfo);
  days = atoi(timeDay);
}

void ring_alarm() {
  display.clearDisplay();
  print_line("MEDICINE TIME!", 0, 0, 2);
  display.display();  // Update display

  digitalWrite(LED_1, HIGH);

  bool break_happened = false;
  bool snooze_pressed = false;
  unsigned long alarmStartTime = millis();
  const unsigned long ALARM_TIMEOUT = 60000;  // Automatically stop after 1 minute

  // Ring the buzzer with snooze option
  while (!break_happened) {
    for (int i = 0; i < n_notes && !break_happened; i++) {
      // Check for cancel button (stop alarm)
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);  // Debounce
        break_happened = true;
        break;
      }
      
      // Check for OK button (snooze)
      if (digitalRead(PB_OK) == LOW) {
        delay(200);  // Debounce
        snooze_pressed = true;
        break_happened = true;
        break;
      }
      
      // Check for alarm timeout
      if (millis() - alarmStartTime > ALARM_TIMEOUT) {
        break_happened = true;
        break;
      }
      
      // Play tone
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  // Handle snooze
  if (snooze_pressed) {
    alarmSnoozeTime = millis() + (5 * 60 * 1000); // 5 minutes snooze
    display.clearDisplay();
    print_line("Snoozed for", 0, 0, 2);
    print_line("5 minutes", 0, 20, 2);
    display.display();
    delay(1500);
  }

  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}

void update_time_with_check_alarm() {
  update_time();
  print_time_now();
  
  if (alarm_enabled) {
    for (int i = 0; i < n_alarms; i++) {
      // Check if alarm is active and not yet triggered
      if (alarm_active[i] && !alarm_triggered[i] && 
          alarm_hours[i] == hours && 
          alarm_minutes[i] == minutes && 
          seconds < 10) {  // Only trigger in the first 10 seconds of the minute
        
        // Check if we're past a potential snooze time
        if (alarmSnoozeTime == 0 || millis() > alarmSnoozeTime) {
          ring_alarm();
          alarm_triggered[i] = true;
          alarmSnoozeTime = 0;
        }
      }
      
      // Reset triggered status at next minute
      if (alarm_triggered[i] && (minutes != alarm_minutes[i] || hours != alarm_hours[i])) {
        alarm_triggered[i] = false;
      }
    }
  }
}

int wait_for_button_press() {
  unsigned long startTime = millis();
  
  while (true) {
    // Check for button presses
    if (digitalRead(PB_UP) == LOW) {
      delay(200);  // Debounce
      return PB_UP;
    }
    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);  // Debounce
      return PB_DOWN;
    }
    else if (digitalRead(PB_OK) == LOW) {
      delay(200);  // Debounce
      return PB_OK;
    }
    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);  // Debounce
      return PB_CANCEL;
    }
    
    // Add a small delay to reduce CPU usage
    delay(50);
    
    // Add a timeout to prevent indefinite waiting
    if (millis() - startTime > 3000) {  
      return PB_CANCEL;
    }
  }
}

void go_to_menu() {
  bool exit_menu = false;
  
  while (!exit_menu) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);
    display.display();  // Update display

    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      current_mode = (current_mode + 1) % max_modes;
    }
    else if (pressed == PB_DOWN) {
      current_mode = (current_mode - 1 + max_modes) % max_modes;
    }
    else if (pressed == PB_OK) {
      run_mode(current_mode);
    }
    else if (pressed == PB_CANCEL) {
      exit_menu = true;
    }
  }
}

void view_alarms() {
  display.clearDisplay();
  int active_count = 0;
  
  for (int i = 0; i < n_alarms; i++) {
    if (alarm_active[i]) {
      String hours_str = alarm_hours[i] < 10 ? "0" + String(alarm_hours[i]) : String(alarm_hours[i]);
      String mins_str = alarm_minutes[i] < 10 ? "0" + String(alarm_minutes[i]) : String(alarm_minutes[i]);
      
      String alarm_text = "Alarm " + String(i+1) + ": " + hours_str + ":" + mins_str;
      print_line(alarm_text, 0, active_count * 16, 1);  // Increased spacing between lines
      active_count++;
    }
  }
  
  if (active_count == 0) {
    print_line("No active alarms", 0, 0, 1);
  }
  
  String status_text = alarm_enabled ? "Alarms: ON" : "Alarms: OFF";
  print_line(status_text, 0, 48, 1);  // Add alarm enabled status
  
  display.display();  // Update display
  
  // Wait for a button press to exit
  wait_for_button_press();
  display.clearDisplay();
}

void delete_alarm() {
  int selected_alarm = 0;
  
  while (true) {
    display.clearDisplay();
    
    // Show current alarm state
    String status_text = alarm_active[selected_alarm] ? 
                         "Alarm " + String(selected_alarm + 1) + " is SET" :
                         "Alarm " + String(selected_alarm + 1) + " is EMPTY";
    print_line(status_text, 0, 0, 1);
    
    if (alarm_active[selected_alarm]) {
      String time_str = String(alarm_hours[selected_alarm]) + ":" + 
                      (alarm_minutes[selected_alarm] < 10 ? "0" : "") + 
                      String(alarm_minutes[selected_alarm]);
      print_line(time_str, 0, 16, 2);
    }
    
    print_line("Delete?", 0, 40, 1);
    display.display();  // Update display
    
    int pressed = wait_for_button_press();
    
    if (pressed == PB_UP || pressed == PB_DOWN) {
      selected_alarm = (selected_alarm + 1) % n_alarms;  // Toggle between alarms
    }
    else if (pressed == PB_OK) {
      if (alarm_active[selected_alarm]) {
        // Reset the specific alarm
        alarm_active[selected_alarm] = false;
        alarm_hours[selected_alarm] = 0;
        alarm_minutes[selected_alarm] = 0;
        alarm_triggered[selected_alarm] = false;
        
        display.clearDisplay();
        print_line("Alarm Deleted", 0, 0, 2);
        display.display();  // Update display
        delay(1000);
      } else {
        display.clearDisplay();
        print_line("No alarm to delete", 0, 0, 1);
        display.display();
        delay(1000);
      }
      break;
    }
    else if (pressed == PB_CANCEL) {
      break;
    }
  }
}

void set_alarm(int alarm) {
  int temp_hour = alarm_hours[alarm];
  int temp_minute = alarm_minutes[alarm];

  // Set hour
  while (true) {
    display.clearDisplay();
    String hour_str = temp_hour < 10 ? "0" + String(temp_hour) : String(temp_hour);
    print_line("Enter hour: " + hour_str, 0, 0, 2);
    display.display();  // Update display

    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      temp_hour = (temp_hour + 1) % 24;
    }
    else if (pressed == PB_DOWN) {
      temp_hour = (temp_hour - 1 + 24) % 24;
    }
    else if (pressed == PB_OK) {
      alarm_hours[alarm] = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL) {
      return;
    }
  }

  // Set minute
  while (true) {
    display.clearDisplay();
    String min_str = temp_minute < 10 ? "0" + String(temp_minute) : String(temp_minute);
    print_line("Enter minute: " + min_str, 0, 0, 2);
    display.display();  // Update display

    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      temp_minute = (temp_minute + 1) % 60;
    }
    else if (pressed == PB_DOWN) {
      temp_minute = (temp_minute - 1 + 60) % 60;
    }
    else if (pressed == PB_OK) {
      alarm_minutes[alarm] = temp_minute;
      alarm_active[alarm] = true;  // Activate the alarm
      alarm_triggered[alarm] = false;
      break;
    }
    else if (pressed == PB_CANCEL) {
      return;
    }
  }

  display.clearDisplay();
  String time_str = (alarm_hours[alarm] < 10 ? "0" : "") + String(alarm_hours[alarm]) + ":" + 
                    (alarm_minutes[alarm] < 10 ? "0" : "") + String(alarm_minutes[alarm]);
  print_line("Alarm " + String(alarm + 1) + " set to", 0, 0, 1);
  print_line(time_str, 0, 16, 2);
  display.display();  // Update display
  delay(2000);
}

// Updated function to set time zone dynamically
void set_time_zone() {
  int hours_offset = utc_offset / 3600;
  int minutes_offset = (utc_offset % 3600) / 60;
  bool is_positive = hours_offset >= 0;
  
  if (!is_positive) {
    hours_offset = -hours_offset;
    minutes_offset = -minutes_offset;
    if (minutes_offset < 0) minutes_offset = -minutes_offset;  // Fix for negative minutes
  }
  
  // Set hours offset
  while (true) {
    display.clearDisplay();
    String sign = is_positive ? "+" : "-";
    print_line("UTC " + sign + String(hours_offset) + " hours", 0, 0, 1);
    print_line("Set hour offset", 0, 20, 1);
    print_line("(UP/DOWN to change)", 0, 32, 1);
    print_line("(OK to confirm)", 0, 44, 1);
    display.display();  // Update display
    
    int pressed = wait_for_button_press();
    
    if (pressed == PB_UP) {
      if (hours_offset < 12) {
        hours_offset++;
      }
    }
    else if (pressed == PB_DOWN) {
      if (hours_offset > 0) {
        hours_offset--;
      } else if (hours_offset == 0) {
        // Toggle between positive and negative
        is_positive = !is_positive;
      }
    }
    else if (pressed == PB_OK) {
      break;
    }
    else if (pressed == PB_CANCEL) {
      return;
    }
  }
  
  // Set minutes offset (typically 0, 15, 30, or 45)
  while (true) {
    display.clearDisplay();
    String sign = is_positive ? "+" : "-";
    print_line("UTC " + sign + String(hours_offset) + ":" + 
              (minutes_offset < 10 ? "0" : "") + String(minutes_offset), 0, 0, 2);
    print_line("Set minute offset", 0, 20, 1);
    print_line("(steps of 15 min)", 0, 32, 1);
    display.display();  // Update display
    
    int pressed = wait_for_button_press();
    
    if (pressed == PB_UP) {
      minutes_offset = (minutes_offset + 15) % 60;
    }
    else if (pressed == PB_DOWN) {
      minutes_offset = (minutes_offset - 15 + 60) % 60;
    }
    else if (pressed == PB_OK) {
      // Calculate and save the UTC offset
      int new_offset = (is_positive ? 1 : -1) * (hours_offset * 3600 + minutes_offset * 60);
      utc_offset = new_offset;
      
      // Update the time configuration
      configTime(utc_offset, utc_offset_dst, NTP_SERVER);
      
      display.clearDisplay();
      print_line("Time zone set to", 0, 0, 1);
      print_line("UTC " + sign + String(hours_offset) + ":" + 
                (minutes_offset < 10 ? "0" : "") + String(minutes_offset), 0, 16, 2);
      display.display();  // Update display
      delay(2000);
      break;
    }
    else if (pressed == PB_CANCEL) {
      return;
    }
  }
}

void run_mode(int mode) {
  switch(mode) {
    case 0:
    case 1:
      set_alarm(mode);
      break;
    case 2:
      view_alarms();
      break;
    case 3:
      delete_alarm();
      break;
    case 4:
      alarm_enabled = !alarm_enabled;
      display.clearDisplay();
      print_line(alarm_enabled ? "Alarms enabled" : "Alarms disabled", 0, 0, 2);
      display.display();  // Update display
      delay(1500);
      break;
    case 5:
      set_time_zone();
      break;
  }
}

void check_temp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  
  // Check for sensor reading errors
  if (isnan(data.temperature) || isnan(data.humidity)) {
    print_line("Sensor Error", 0, 40, 1);
    display.display();  // Update display
    return;
  }
  
  if (data.temperature >= 32) {
    print_line("TEMP HIGH", 0, 40, 1);
  }
  else if (data.temperature <= 24) {
    print_line("TEMP LOW", 0, 40, 1);
  }
  if (data.humidity >= 80) {
    print_line("HUMIDITY HIGH", 0, 50, 1);
  }
  else if (data.humidity <= 65) {
    print_line("HUMIDITY LOW", 0, 50, 1);
  }
  
  display.display();  // Update display after adding temperature/humidity info
}