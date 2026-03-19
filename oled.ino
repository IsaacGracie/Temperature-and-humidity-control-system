// ===== OLED 显示 =====
void updateOLEDDisplay() {
    if (!oledOK) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    display.setCursor(0, 0);
    display.print("T:"); display.print(currentTemp, 1); display.print(" H:"); display.print(currentHum, 1); display.print("%");

    display.setCursor(0, 12);
    if (inMenu) {
        display.print("> "); display.print(menuLabels[currentMenuItem]); display.print(": ");
        switch (currentMenuItem) {
            case 0: display.print(autoMode ? "AUTO" : "MAN"); break;
            case 1: display.print(targetTemp, 1); break;
            case 2: display.print(targetHum, 0); break;
            case 3: display.print(acPowerOn ? "ON" : "OFF"); break;
            case 4: display.print(acCoolMode ? "COOL" : "HEAT"); break;
            case 5: display.print(humPowerOn ? "ON" : "OFF"); break;
            case 6: display.print(humiAddMode ? "ADD" : "DRY"); break;
        }
    } else {
        display.print("SET T:"); display.print(targetTemp, 1); display.print(" H:"); display.print(targetHum, 0);
    }

    display.setCursor(0, 24);
    display.print("MODE:"); display.print(autoMode ? "AUTO" : "MANUAL");

    display.setCursor(0, 36);
    display.print("AC :"); display.print(acPowerOn ? "ON " : "OFF");
    if (acPowerOn) display.print(acCoolMode ? "COOL" : "HEAT");

    display.setCursor(0, 48);
    display.print("HUM:"); display.print(humPowerOn ? "ON " : "OFF");
    if (humPowerOn) display.print(humiAddMode ? "ADD" : "DRY");

    display.display();
}
