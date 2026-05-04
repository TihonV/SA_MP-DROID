/* ============================================================================
 * MRussia Plugin - Автоматическое подключение к серверу MRussia
 * Сервер: 188.127.241.74:4968
 * ============================================================================
 * 
 * Этот плагин:
 * 1. Автоматически подключается к серверу MRussia при запуске
 * 2. Перехватывает сюжетный режим и заменяет его мультиплеером
 * 3. Добавляет защиту от вылетов и автоматическое переподключение
 * 
 * ============================================================================
 */

#define SAMP_SDK_IMPLEMENTATION
#define SAMP_SDK_WANT_AMX_EVENTS
#define SAMP_SDK_WANT_PROCESS_TICK

#include "../samp-sdk/samp-sdk.hpp"

// ============================================================================
// КОНФИГУРАЦИЯ СЕРВЕРА MRUSSIA
// ============================================================================
#define MRUSSIA_HOST "188.127.241.74"
#define MRUSSIA_PORT 4968
#define MRUSSIA_NAME "MRussia RolePlay"

// Глобальные переменные
static bool g_bAutoConnect = true;      // Автоподключение
static bool g_bConnected = false;       // Статус подключения
static int g_iReconnectAttempts = 0;    // Попытки переподключения
static const int MAX_RECONNECT_ATTEMPTS = 5;

// ============================================================================
// ПЕРЕХВАТ ОСНОВНЫХ ФУНКЦИЙ
// ============================================================================

// 1. Перехват инициализации одиночной игры (сюжетки)
// Вместо запуска сюжетки запускаем мультиплеер
Plugin_Native_Hook(StartSinglePlayer, AMX* amx, cell* params) {
    samp::LogMessage(LogLevel::INFO, "[MRussia] Блокируем сюжетный режим, запускаем мультиплеер...");
    
    // Вместо запуска одиночной игры подключаемся к мультиплееру
    Pawn_Native(ConnectToServer, MRUSSIA_HOST, MRUSSIA_PORT);
    
    return 1;  // Успешно
}

// 2. Перехват главного меню
// Изменяем кнопки меню для прямого подключения
Plugin_Native_Hook(ShowMainMenu, AMX* amx, cell* params) {
    // Вызываем оригинальное меню
    Pawn_Native(ShowMainMenu, params[1]);
    
    // Через 100мс автоматически нажимаем "Подключиться к MRussia"
    // Используем таймер для отложенного действия
    Pawn_Native(SetTimer, "AutoConnectToMRussia", 100, false, 0);
    
    return 1;
}

// 3. Автоматическое подключение к MRussia
Plugin_Public(OnPlayerConnect, int playerid) {
    char message[256];
    snprintf(message, sizeof(message), "Добро пожаловать на %s!", MRUSSIA_NAME);
    
    // Отправляем приветствие подключившемуся игроку
    Pawn_Native(SendClientMessage, playerid, 0x00FF00FF, message);
    Pawn_Native(SendClientMessage, playerid, 0xFFFF00AA, "Сервер работает в тестовом режиме");
    
    g_bConnected = true;
    g_iReconnectAttempts = 0;
    
    return 1;
}

// ============================================================================
// ДОПОЛНИТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ MRUSSIA
// ============================================================================

// 4. Создание своей нативной функции (доступна из Pawn)
Plugin_Native(GetMRussiaInfo, AMX* amx, cell* params) {
    /*
     * Использование из Pawn:
     * new ip[64], port;
     * GetMRussiaInfo(ip, port);
     */
    
    int playerid = params[1];  // ID игрока, запросившего информацию
    
    // Получаем указатель на буфер, куда запишем IP
    char* ip_buffer = samp::GetString(amx, params[2]);
    strcpy(ip_buffer, MRUSSIA_HOST);
    
    // Возвращаем порт
    return MRUSSIA_PORT;
}

// 5. Защита от потери соединения - автопереподключение
Plugin_Public(OnPlayerDisconnect, int playerid, int reason) {
    g_bConnected = false;
    
    if (g_iReconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        g_iReconnectAttempts++;
        samp::LogMessage(LogLevel::WARNING, 
            "[MRussia] Потеряно соединение. Попытка переподключения %d/%d",
            g_iReconnectAttempts, MAX_RECONNECT_ATTEMPTS);
        
        // Пытаемся переподключиться через 5 секунд
        Pawn_Native(SetTimer, "AutoReconnectToMRussia", 5000, false, 0);
    }
    
    return 1;
}

// 6. Обработка команд (пример)
Plugin_Public(OnPlayerCommandText, int playerid, const char* cmdtext) {
    // Если игрок ввел /mrusia - показать информацию
    if (strcmp(cmdtext, "/mrusia") == 0) {
        char message[256];
        snprintf(message, sizeof(message), 
            "=== MRussia Info ===\nIP: %s:%d\nСтатус: %s\nВерсия плагина: v1.0",
            MRUSSIA_HOST, MRUSSIA_PORT,
            g_bConnected ? "Подключен" : "Не подключен");
        
        Pawn_Native(SendClientMessage, playerid, 0x44AAFFFF, message);
        return 1;  // Команда обработана
    }
    
    // Если игрок ввел /reconnect - принудительное переподключение
    if (strcmp(cmdtext, "/reconnect") == 0) {
        Pawn_Native(ForceClassSelection, playerid);
        Pawn_Native(TogglePlayerSpectating, playerid, true);
        Pawn_Native(SendClientMessage, playerid, 0xFFFF00AA, "Переподключение...");
        Pawn_Native(ConnectToServer, MRUSSIA_HOST, MRUSSIA_PORT);
        return 1;
    }
    
    return 0;  // Команда не распознана
}

// ============================================================================
// ТИК-ФУНКЦИЯ (вызывается каждый фрейм сервера)
// ============================================================================
void OnProcessTick() {
    static int last_check = 0;
    static int tick_counter = 0;
    
    tick_counter++;
    
    // Каждые 1000 тиков (~10 секунд) проверяем статус
    if (tick_counter - last_check >= 1000) {
        last_check = tick_counter;
        
        if (!g_bConnected && g_bAutoConnect && g_iReconnectAttempts == 0) {
            // Автоматически подключаем первого игрока
            samp::LogMessage(LogLevel::INFO, "[MRussia] Автоподключение к серверу...");
            Pawn_Native(ConnectToServer, MRUSSIA_HOST, MRUSSIA_PORT);
        }
    }
}

// ============================================================================
// ЖИЗНЕННЫЙ ЦИКЛ ПЛАГИНА
// ============================================================================

bool OnLoad() {
    samp::LogMessage(LogLevel::INFO, "========================================");
    samp::LogMessage(LogLevel::INFO, "  MRussia Plugin v1.0 загружен!");
    samp::LogMessage(LogLevel::INFO, "  Сервер: %s:%d", MRUSSIA_HOST, MRUSSIA_PORT);
    samp::LogMessage(LogLevel::INFO, "  Режим: Автоподключение");
    samp::LogMessage(LogLevel::INFO, "========================================");
    return true;
}

void OnUnload() {
    samp::LogMessage(LogLevel::INFO, "[MRussia] Плагин выгружен");
}

void OnAmxLoad(AMX* amx) {
    samp::LogMessage(LogLevel::DEBUG, "[MRussia] Новый скрипт загружен в AMX");
}

void OnAmxUnload(AMX* amx) {
    samp::LogMessage(LogLevel::DEBUG, "[MRussia] Скрипт выгружен из AMX");
}

// Получение флагов поддержки
unsigned int GetSupportFlags() {
    return SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}
