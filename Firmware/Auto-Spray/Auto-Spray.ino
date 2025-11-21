#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <Bounce2.h>

// === ПИНЫ (для Pro Mini: только D2 и D3 поддерживают внешние прерывания) ===
#define PIN_BUTTON            2   // D2 — кнопка (GND при нажатии)
#define PIN_LIGHT_DIGITAL     3   // D3 — цифровой выход датчика света (HIGH = светло)
#define PIN_MOTOR             4
#define PIN_LED_BLOCK         5   // red
#define PIN_AUTO_MODE_SWITCH  6   // LOW = автоматический режим включён
#define PIN_LED_WORK          7   // green
#define PIN_LED_READY         8   // blue
#define PIN_BUZZER            9

// === РЕЖИМ ===
#define SPRAY_ON_TIMER_ONLY   // раскомментируйте для пшика сразу по таймеру

// === КОНСТАНТЫ (все временные значения удвоены для 8 МГц) ===
#define BLOCK_DURATION_SEC        (30UL * 60UL)        // 30 минут (без изменений)
#define AUTO_READY_DELAY_SEC      (3UL * 60UL)         // 3 минуты (без изменений)
#define SPRAY_PULSE_COUNT         2
#define SPRAY_PULSE_DURATION_MS   2                    // 2 * 2 = 4 мс реального времени
#define SPRAY_INTER_PULSE_DELAY_MS 5                    // 5 * 2 = 10 мс реального времени
#define BUZZER_ON_DURATION_MS     150                  // 150 * 2 = 300 мс реального времени
#define LED_BLINK_INTERVAL_MS     500                  // 500 * 2 = 1000 мс реального времени

// === ФЛАГИ ПРОБУЖДЕНИЯ ===
volatile bool wokeByButton = false;
volatile bool wokeByLight = false;
volatile bool wdtTriggered = false;

// === СОСТАВНЫЕ ПЕРЕМЕННЫЕ ===
static Bounce buttonDebouncer = Bounce(); // debounce для кнопки
static bool isBlocked = false;
static bool isAutoReady = false;
static uint32_t blockStartTime = 0;      // секунды
static uint32_t lightOnStartTime = 0;    // секунды
static uint32_t lastLedToggle = 0;       // миллисекунды
static bool isBuzzerOn = false;
static uint32_t buzzerOffTime = 0;       // миллисекунды
static bool systemAwake = true;          // true = активен, false = в соне (логически)

// === ПРЕРЫВАНИЯ ===

ISR(INT0_vect) { // D2 — кнопка
    wokeByButton = true;
}

ISR(INT1_vect) { // D3 — свет
    wokeByLight = true;
}

ISR(WDT_vect) {
    wdtTriggered = true;
}

// === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===

void setupWatchdogForBlocking() {
    cli();
    wdt_reset();
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    WDTCSR = _BV(WDIE) | _BV(WDP2) | _BV(WDP1); // 1 сек
    sei();
}

void disableWatchdog() {
    cli();
    wdt_reset();
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    WDTCSR = 0;
    sei();
}

void sleepForever() {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
    cli();
}

// Выполняет распыление с учётом частоты
void performSpray() {
    digitalWrite(PIN_LED_WORK, HIGH);

    for (uint8_t i = 0; i < SPRAY_PULSE_COUNT; i++) {
        digitalWrite(PIN_MOTOR, HIGH);
        delay(SPRAY_PULSE_DURATION_MS);  // на 8 МГц — 2 мс * 2 = 4 мс реального времени
        digitalWrite(PIN_MOTOR, LOW);

        if (i < SPRAY_PULSE_COUNT - 1) {
            delay(SPRAY_INTER_PULSE_DELAY_MS);  // 5 мс * 2 = 10 мс
        }
    }

    digitalWrite(PIN_LED_WORK, LOW);
}

void enterBlockedState() {
    isBlocked = true;
    isAutoReady = false;
    blockStartTime = millis() / 1000UL;
    digitalWrite(PIN_BUZZER, LOW);
    isBuzzerOn = false;
    setupWatchdogForBlocking(); // включаем WDT для отсчёта блокировки
}

void updateLeds() {
    uint32_t now = millis();
    bool isLightOn = (digitalRead(PIN_LIGHT_DIGITAL) == HIGH);

    // Синий LED
    if (isAutoReady && isLightOn) {
        digitalWrite(PIN_LED_READY, HIGH);
    } else if (!isBlocked && isLightOn) {
        if (now - lastLedToggle >= LED_BLINK_INTERVAL_MS) {
            lastLedToggle = now;
            digitalWrite(PIN_LED_READY, !digitalRead(PIN_LED_READY));
        }
    } else {
        digitalWrite(PIN_LED_READY, LOW);
    }

    // Красный LED
    if (isBlocked && isLightOn) {
        if (now - lastLedToggle >= LED_BLINK_INTERVAL_MS) {
            lastLedToggle = now;
            digitalWrite(PIN_LED_BLOCK, !digitalRead(PIN_LED_BLOCK));
        }
    } else {
        digitalWrite(PIN_LED_BLOCK, LOW);
    }
}

void updateBuzzer() {
    if (isBuzzerOn && millis() >= buzzerOffTime) {
        digitalWrite(PIN_BUZZER, LOW);
        isBuzzerOn = false;
    }
}

void handleLightChange() {
    bool isLightOn = (digitalRead(PIN_LIGHT_DIGITAL) == HIGH);
    uint32_t nowSec = millis() / 1000UL;

    if (isLightOn) {
        // Свет включён — начинаем отсчёт
        if (lightOnStartTime == 0) lightOnStartTime = nowSec;
        uint32_t elapsed = nowSec - lightOnStartTime;
        if (!isBlocked && elapsed >= AUTO_READY_DELAY_SEC && !isAutoReady) {
            isAutoReady = true;
#ifdef SPRAY_ON_TIMER_ONLY
            performSpray();
            enterBlockedState();
#else
            isBuzzerOn = true;
            buzzerOffTime = millis() + BUZZER_ON_DURATION_MS;
            digitalWrite(PIN_BUZZER, HIGH);
#endif
        }
    } else {
        // Свет выключен
        lightOnStartTime = 0;
#ifndef SPRAY_ON_TIMER_ONLY
        if (isAutoReady && !isBlocked) {
            performSpray();
            enterBlockedState();
        }
#endif
        isAutoReady = false;
    }
}

// Проверяет, можно ли заснуть
void goToSleepIfNeeded() {
    bool isLightOn = (digitalRead(PIN_LIGHT_DIGITAL) == HIGH);
    uint32_t nowSec = millis() / 1000UL;

    // Если свет включён — не спим
    if (isLightOn) return;

    // Если есть блокировка — ждём её окончания
    if (isBlocked) {
        if (nowSec - blockStartTime < BLOCK_DURATION_SEC) {
            return; // ещё не время спать
        } else {
            isBlocked = false;
            disableWatchdog(); // блокировка закончилась — выключаем WDT
        }
    }

    // Свет выключен, блокировки нет → можно спать
    digitalWrite(PIN_LED_READY, LOW);
    digitalWrite(PIN_LED_BLOCK, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    systemAwake = false;
    sleepForever(); // засыпаем НАВСЕГДА, пока не прервут
}

// === SETUP / LOOP ===

void setup() {
    // Отключаем периферию (только PRR для 328P)
    PRR = _BV(PRTIM1) | _BV(PRTIM0) | _BV(PRUSART0) | _BV(PRSPI);

    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_LIGHT_DIGITAL, INPUT);
    pinMode(PIN_MOTOR, OUTPUT);
    pinMode(PIN_LED_BLOCK, OUTPUT);
    pinMode(PIN_AUTO_MODE_SWITCH, INPUT_PULLUP);
    pinMode(PIN_LED_WORK, OUTPUT);
    pinMode(PIN_LED_READY, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);

    digitalWrite(PIN_MOTOR, LOW);
    digitalWrite(PIN_LED_BLOCK, LOW);
    digitalWrite(PIN_LED_WORK, LOW);
    digitalWrite(PIN_LED_READY, LOW);
    digitalWrite(PIN_BUZZER, LOW);

    // Настройка Bounce2 для кнопки
    buttonDebouncer.attach(PIN_BUTTON);
    buttonDebouncer.interval(12); // 12 * 2 = ~25 мс реального времени

    // Прерывания: D2 (INT0) — кнопка, D3 (INT1) — свет
    EICRA = _BV(ISC01) | _BV(ISC11); // прерывание по спаду (LOW при нажатии/темноте)
    EIMSK = _BV(INTF0) | _BV(INTF1); // разрешить INT0 и INT1

    // Сигнал включения
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100); // 100 * 2 = ~200 мс
    digitalWrite(PIN_BUZZER, LOW);
}

void loop() {
    // Обработка пробуждения по кнопке
    if (wokeByButton) {
        wokeByButton = false;
        if (!isBlocked && digitalRead(PIN_AUTO_MODE_SWITCH) == LOW) {
            performSpray();
            enterBlockedState();
        }
    }

    // Обработка света
    if (wokeByLight) {
        wokeByLight = false;
        if (digitalRead(PIN_AUTO_MODE_SWITCH) == LOW) {
            handleLightChange();
        }
    }

    if (wdtTriggered) {
        wdtTriggered = false;
        setupWatchdogForBlocking(); // перезапуск WDT
    }

    // Обновление состояния (только если автоматический режим включён)
    if (digitalRead(PIN_AUTO_MODE_SWITCH) == LOW) {
        updateLeds();
        updateBuzzer();
    }

    // Решение: спать или нет?
    goToSleepIfNeeded();

    // Небольшая задержка, чтобы не грузить CPU (работает только когда свет включён)
    delay(25); // 25 * 2 = ~50 мс
}