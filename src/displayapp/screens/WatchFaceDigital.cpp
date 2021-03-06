#include "WatchFaceDigital.h"

#include <date/date.h>
#include <lvgl/lvgl.h>
#include <cstdio>
#include "BatteryIcon.h"
#include "BleIcon.h"
#include "NotificationIcon.h"
#include "Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "../DisplayApp.h"

using namespace Pinetime::Applications::Screens;

WatchFaceDigital::WatchFaceDigital(DisplayApp* app,
                                   Controllers::DateTime& dateTimeController,
                                   Controllers::Battery& batteryController,
                                   Controllers::Ble& bleController,
                                   Controllers::NotificationManager& notificatioManager,
                                   Controllers::Settings& settingsController,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController)
  : Screen(app),
    currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificatioManager {notificatioManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {
  settingsController.SetClockFace(0);

  displayedChar[0] = 0;
  displayedChar[1] = 0;
  displayedChar[2] = 0;
  displayedChar[3] = 0;
  displayedChar[4] = 0;

  batteryIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(batteryIcon, Symbols::batteryFull);
  lv_obj_align(batteryIcon, lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -5, 2);

  batteryPlug = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(batteryPlug, Symbols::plug);
  lv_obj_align(batteryPlug, batteryIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  batteryPercent = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(batteryPercent, true);
  lv_obj_align(batteryPercent, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -20);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(bleIcon, Symbols::bluetooth);
  lv_obj_align(bleIcon, batteryPlug, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  notificationIcon = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 10, 0);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_date, true);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -40);

  label_prompt_1 = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_prompt_1, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -80);
  lv_label_set_text(label_prompt_1, "user@watch:~ $ now");
  
  label_prompt_2 = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_prompt_2, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 40);
  lv_label_set_text(label_prompt_2, "user@watch:~ $");
  
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_time, true);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -60);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(heartbeatValue, true);
  lv_label_set_text(heartbeatValue, "[L_HR]#ee3311 0 bpm#");
  lv_obj_align(heartbeatValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 20);
  
  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(stepValue, true);
  lv_label_set_text(stepValue, "[STEP]#ee3377 0 steps#");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 0);
}

WatchFaceDigital::~WatchFaceDigital() {
  lv_obj_clean(lv_scr_act());
}

bool WatchFaceDigital::Refresh() {
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated()) {
    auto batteryPercent = batteryPercentRemaining.Get();
    lv_label_set_text(batteryIcon, BatteryIcon::GetBatteryIcon(batteryPercent));
    auto isCharging = batteryController.IsCharging() || batteryController.IsPowerPresent();
    lv_label_set_text(batteryPlug, BatteryIcon::GetPlugIcon(isCharging));
  }

  bleState = bleController.IsConnected();
  if (bleState.IsUpdated()) {
    if(bleState.Get() == true) {
      lv_label_set_text(bleIcon, BleIcon::GetIcon(true));
    } else {
      lv_label_set_text(bleIcon, BleIcon::GetIcon(false));
    }
  }
  lv_obj_align(batteryIcon, lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -5, 5);
  lv_obj_align(batteryPlug, batteryIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_align(bleIcon, batteryPlug, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  notificationState = notificatioManager.AreNewNotificationsAvailable();
  if(notificationState.IsUpdated()) {
    if(notificationState.Get() == true)
      lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(true));
    else
      lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(false));
  }

  currentDateTime = dateTimeController.CurrentDateTime();

  if(currentDateTime.IsUpdated()) {
    auto newDateTime = currentDateTime.Get();

    auto dp = date::floor<date::days>(newDateTime);
    auto time = date::make_time(newDateTime-dp);
    auto yearMonthDay = date::year_month_day(dp);

    auto year = (int)yearMonthDay.year();
    auto month = static_cast<Pinetime::Controllers::DateTime::Months>((unsigned)yearMonthDay.month());
    auto day = (unsigned)yearMonthDay.day();
    auto dayOfWeek = static_cast<Pinetime::Controllers::DateTime::Days>(date::weekday(yearMonthDay).iso_encoding());

    auto hour = time.hours().count();
    auto minute = time.minutes().count();
	  auto second = time.seconds().count();

    char minutesChar[3];
    sprintf(minutesChar, "%02d", static_cast<int>(minute));

    char hoursChar[3];
    char ampmChar[3];
	    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H24) {
        sprintf(hoursChar, "%02lld", hour);
      } else {
        if (hour == 0 && hour != 12) {
          hour = 12;
          sprintf(ampmChar, "AM");
        } else if (hour == 12 && hour != 0) {
          hour = 12;
          sprintf(ampmChar, "PM");
        } else if (hour < 12 && hour != 0) {
          sprintf(ampmChar, "AM");
        } else if (hour > 12 && hour != 0) {
          hour = hour - 12;
          sprintf(ampmChar, "PM");
        }
        sprintf(hoursChar, "%02lld", hour);
    }

	char secondsChar[3];
    sprintf(secondsChar, "%02d", static_cast<int>(second));

	auto batteryValue = static_cast<uint8_t>(batteryController.PercentRemaining());

	char batteryGauge[12];
    if (batteryValue > 75){
      sprintf(batteryGauge, "[++++]");  //100-75
    }
    else if (batteryValue > 50){
      sprintf(batteryGauge, "[+++.]");  //75-50
    }
    else if (batteryValue > 25){
      sprintf(batteryGauge, "[++..]");  //50-25
    }
    else if (batteryValue > 10){
      sprintf(batteryGauge, "[+....]"); //25-10
    }
    else if (batteryValue > 0){
      sprintf(batteryGauge, "[!!!!!]"); //10-0
    }

    char battStr[28];
    sprintf(battStr, "[BATT]#387b54 %s %d%%#", batteryGauge, batteryValue);
    lv_label_set_text(batteryPercent, battStr);
	
	
    char timeStr[38];
    sprintf(timeStr, "[TIME]#11cc55 %c%c:%c%c:%c%c %s#", hoursChar[0],hoursChar[1],minutesChar[0], minutesChar[1], secondsChar[0], secondsChar[1], ampmChar);
	
    if(hoursChar[0] != displayedChar[0] || hoursChar[1] != displayedChar[1] || minutesChar[0] != displayedChar[2] || minutesChar[1] != displayedChar[3]) {
      displayedChar[0] = hoursChar[0];
      displayedChar[1] = hoursChar[1];
      displayedChar[2] = minutesChar[0];
      displayedChar[3] = minutesChar[1];

      lv_label_set_text(label_time, timeStr);
    }

    if ((year != currentYear) || (month != currentMonth) || (dayOfWeek != currentDayOfWeek) || (day != currentDay)) {

	    char dateStr[38];
      sprintf(dateStr, "[DATE]#007fff %s %d %d#", dateTimeController.DayOfWeekShortToString(), char(month), char(day));
      lv_label_set_text(label_date, dateStr);


      currentYear = year;
      currentMonth = month;
      currentDayOfWeek = dayOfWeek;
      currentDay = day;
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if(heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    char heartbeatBuffer[28];
    if(heartbeatRunning.Get())
      sprintf(heartbeatBuffer, "[L_HR]#ee3311 %d bpm#", heartbeat.Get());
    else
      sprintf(heartbeatBuffer, "[L_HR]#ee3311 ---#");

    lv_label_set_text(heartbeatValue, heartbeatBuffer);
  }

  stepCount = motionController.NbSteps();
  motionSensorOk = motionController.IsSensorOk();
  char stepString[34];
  if (stepCount.IsUpdated() || motionSensorOk.IsUpdated()) {
    sprintf(stepString, "[STEP]#ee3377 %lu steps#", stepCount.Get());
    lv_label_set_text(stepValue, stepString);
  }
  return running;
}