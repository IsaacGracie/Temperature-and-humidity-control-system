// ===== 按键处理（菜单 + 长按重复） =====
void handleButtons() {
    unsigned long now = millis();
    Button* buttons[] = {&btnPower, &btnMode, &btnUp, &btnDown};

    for (auto btn : buttons) {
        bool reading = digitalRead(btn->pin);

        if (reading != btn->lastState) {
            btn->pressTime = now;
            btn->longPressedHandled = false;
            btn->lastRepeatTime = now;
        }

        if ((now - btn->pressTime) > DEBOUNCE_TIME) {
            if (reading == HIGH && !btn->pressed) {
                btn->pressed = true;
                btn->pressTime = now;
                btn->lastRepeatTime = now;
            }
            else if (reading == LOW && btn->pressed) {
                if ((now - btn->pressTime) < LONG_PRESS_TIME) {
                    // 短按
                    if (btn == &btnPower) {
                        if (inMenu) exitMenu();
                        else totalPowerOn = !totalPowerOn;
                    }
                    else if (btn == &btnMode) {
                        if (inMenu) currentMenuItem = (currentMenuItem + 1) % MENU_ITEMS;
                        else enterMenu();
                    }
                    else if (btn == &btnUp) {
                        if (inMenu) adjustMenuItem(1);
                        else { autoMode = true; targetTemp += 1.0f;
                               targetTemp = constrain(targetTemp, 16.0f, 35.0f); 
                             }
                    }
                    else if (btn == &btnDown) {
                        if (inMenu) adjustMenuItem(-1);
                        else { autoMode = false; targetHum -= 5.0f;
                               targetHum = constrain(targetHum, 30.0f, 90.0f);
                             }
                    }
                }
                btn->pressed = false;
            }

            if (reading == HIGH && (now - btn->pressTime) > LONG_PRESS_TIME) {
                if (!btn->longPressedHandled) {
                    btn->longPressedHandled = true;
                    if (btn == &btnPower) {
                        totalPowerOn = acPowerOn = humPowerOn = false;
                        exitMenu();
                    }
                    else if (btn == &btnMode) exitMenu();
                    else if (btn == &btnUp || btn == &btnDown) {
                        adjustMenuItem(btn == &btnUp ? 1 : -1);
                        btn->lastRepeatTime = now;
                    }
                }
                else if (btn == &btnUp || btn == &btnDown) {
                    if (now - btn->lastRepeatTime > REPEAT_TIME) {
                        adjustMenuItem(btn == &btnUp ? 1 : -1);
                        btn->lastRepeatTime = now;
                    }
                }
            }
        }
        btn->lastState = reading;
    }
}