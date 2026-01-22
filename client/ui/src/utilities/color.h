#pragma once

#include <QColor>

// ============================================================================
// Primary Colors
// ============================================================================
inline const QColor COLOR_PRIMARY = QColor(21, 119, 232);              // Основной синий
inline const QColor COLOR_PRIMARY_HOVER = QColor(18, 113, 222);        // Синий при наведении
inline const QColor COLOR_PRIMARY_DARK = QColor(16, 103, 202);         // Темный синий (pressed)
inline const QColor COLOR_PRIMARY_DARKER = QColor(16, 107, 212);       // Еще темнее
inline const QColor COLOR_PRIMARY_SELECTION = QColor(79, 161, 255);    // Синий для выделения
inline const QColor COLOR_PRIMARY_LIGHT = QColor(33, 150, 243);        // Светлый синий

// ============================================================================
// Status Colors
// ============================================================================
inline const QColor COLOR_SUCCESS = QColor(100, 200, 100);              // Зеленый (успех)
inline const QColor COLOR_SUCCESS_LIGHT = QColor(82, 196, 65);          // Светло-зеленый
inline const QColor COLOR_SUCCESS_TEXT = QColor(25, 186, 0);           // Текст успеха
inline const QColor COLOR_SUCCESS_BG = QColor(235, 249, 237);          // Фон успеха

inline const QColor COLOR_ERROR = QColor(220, 80, 80);                  // Красный (ошибка)
inline const QColor COLOR_ERROR_DARK = QColor(220, 0, 0);               // Темно-красный
inline const QColor COLOR_ERROR_BG = QColor(255, 212, 212);             // Фон ошибки
inline const QColor COLOR_ERROR_BANNER = QColor(232, 53, 53);           // Красный для баннера
inline const QColor COLOR_ERROR_BANNER_HOVER = QColor(212, 43, 43);    // Красный баннер hover
inline const QColor COLOR_ERROR_BANNER_LIGHT = QColor(252, 121, 121);  // Светло-красный

inline const QColor COLOR_WARNING = QColor(255, 165, 0);                // Оранжевый (предупреждение)
inline const QColor COLOR_CALLING = QColor(255, 165, 0);                // Оранжевый для звонка

// ============================================================================
// Text Colors
// ============================================================================
inline const QColor COLOR_TEXT_PRIMARY = QColor(1, 11, 19);             // Основной текст (#010B13)
inline const QColor COLOR_TEXT_SECONDARY = QColor(51, 51, 51);          // Вторичный текст (#333333)
inline const QColor COLOR_TEXT_TERTIARY = QColor(60, 60, 60);           // Третичный текст
inline const QColor COLOR_TEXT_DISABLED = QColor(100, 100, 100);        // Отключенный текст
inline const QColor COLOR_TEXT_LIGHT = QColor(102, 102, 102);           // Светлый текст
inline const QColor COLOR_TEXT_LIGHTER = QColor(115, 115, 115);         // Еще светлее
inline const QColor COLOR_TEXT_PLACEHOLDER = QColor(120, 120, 120);     // Плейсхолдер
inline const QColor COLOR_TEXT_DARK = QColor(240, 240, 240);            // Темный текст (для темного фона)

// ============================================================================
// Background Colors
// ============================================================================
inline const QColor COLOR_BG_PRIMARY = QColor(230, 230, 230);           // Основной фон
inline const QColor COLOR_BG_SECONDARY = QColor(240, 240, 240);         // Вторичный фон
inline const QColor COLOR_BG_TERTIARY = QColor(245, 245, 245);          // Третичный фон
inline const QColor COLOR_BG_WHITE = QColor(255, 255, 255);            // Белый
inline const QColor COLOR_BG_DARK = QColor(25, 25, 25);                 // Темный фон
inline const QColor COLOR_BG_LIGHT_GRAY = QColor(248, 250, 252);        // Светло-серый фон
inline const QColor COLOR_BG_UPDATE = QColor(226, 243, 231);            // Фон обновления
inline const QColor COLOR_BG_ERROR_LIGHT = QColor(255, 240, 240);       // Светлый фон ошибки

// ============================================================================
// Gray Scale Colors (унифицированные оттенки серого)
// ============================================================================
inline const QColor COLOR_GRAY_50 = QColor(250, 250, 250);              // Очень светло-серый
inline const QColor COLOR_GRAY_100 = QColor(245, 245, 245);             // Светло-серый
inline const QColor COLOR_GRAY_150 = QColor(240, 240, 240);              // Светло-серый
inline const QColor COLOR_GRAY_200 = QColor(230, 230, 230);             // Серый
inline const QColor COLOR_GRAY_210 = QColor(210, 210, 210);             // Серый для границ
inline const QColor COLOR_GRAY_235 = QColor(235, 235, 235);             // Серый для кнопок
inline const QColor COLOR_GRAY_208 = QColor(208, 208, 208);             // Серый для скроллбара
inline const QColor COLOR_GRAY_180 = QColor(180, 180, 180);             // Средне-серый
inline const QColor COLOR_GRAY_176 = QColor(176, 176, 176);             // Серый hover скроллбара
inline const QColor COLOR_GRAY_160 = QColor(160, 160, 160);             // Серый disabled
inline const QColor COLOR_GRAY_150_DARK = QColor(150, 150, 150);        // Темно-серый
inline const QColor COLOR_GRAY_144 = QColor(144, 144, 144);             // Серый pressed скроллбара
inline const QColor COLOR_GRAY_120 = QColor(120, 120, 120);             // Темно-серый disabled
inline const QColor COLOR_GRAY_102 = QColor(102, 102, 102);             // Темно-серый текст
inline const QColor COLOR_GRAY_100_DARK = QColor(100, 100, 100);        // Темно-серый
inline const QColor COLOR_GRAY_90 = QColor(90, 90, 90);                 // Очень темно-серый
inline const QColor COLOR_GRAY_80 = QColor(80, 80, 80);                 // Очень темно-серый
inline const QColor COLOR_GRAY_77 = QColor(77, 77, 77);                 // Темно-серый для слайдеров
inline const QColor COLOR_GRAY_51 = QColor(51, 51, 51);                  // Очень темно-серый (#333333)

// ============================================================================
// Border Colors
// ============================================================================
inline const QColor COLOR_BORDER_LIGHT = QColor(200, 200, 200, 100);    // Светлая граница
inline const QColor COLOR_BORDER_WHITE = QColor(255, 255, 255, 40);     // Белая граница

// ============================================================================
// Glass/Transparent Colors
// ============================================================================
inline const QColor COLOR_GLASS_WHITE_60 = QColor(255, 255, 255, 60);   // Белое стекло 60%
inline const QColor COLOR_GLASS_WHITE_100 = QColor(255, 255, 255, 100); // Белое стекло 100%
inline const QColor COLOR_GLASS_WHITE_180 = QColor(255, 255, 255, 180); // Белое стекло 180%
inline const QColor COLOR_GLASS_WHITE_190 = QColor(255, 255, 255, 190); // Белое стекло 190%
inline const QColor COLOR_GLASS_WHITE_200 = QColor(255, 255, 255, 200); // Белое стекло 200%
inline const QColor COLOR_GLASS_WHITE_220 = QColor(255, 255, 255, 220); // Белое стекло 220%
inline const QColor COLOR_GLASS_WHITE_230 = QColor(255, 255, 255, 230); // Белое стекло 230%
inline const QColor COLOR_GLASS_WHITE_235 = QColor(255, 255, 255, 235); // Белое стекло 235%
inline const QColor COLOR_GLASS_GRAY_150 = QColor(245, 245, 245, 150);  // Серое стекло 150%
inline const QColor COLOR_GLASS_GRAY_180 = QColor(240, 240, 240, 180);  // Серое стекло 180%
inline const QColor COLOR_GLASS_GRAY_190 = QColor(245, 245, 245, 190);  // Серое стекло 190%
inline const QColor COLOR_GLASS_GRAY_200 = QColor(245, 245, 245, 200);  // Серое стекло 200%
inline const QColor COLOR_GLASS_GRAY_235 = QColor(245, 245, 245, 235);  // Серое стекло 235%
inline const QColor COLOR_GLASS_PRIMARY_8 = QColor(21, 119, 232, 20);   // Синее стекло 8% (0.08 * 255 ≈ 20)
inline const QColor COLOR_GLASS_PRIMARY_20 = QColor(21, 119, 232, 51);  // Синее стекло 20% (0.20 * 255 = 51)
inline const QColor COLOR_GLASS_PRIMARY_36 = QColor(21, 119, 232, 36);  // Синее стекло 36%
inline const QColor COLOR_GLASS_PRIMARY_80 = QColor(21, 119, 232, 80);  // Синее стекло 80%
inline const QColor COLOR_GLASS_PRIMARY_120 = QColor(21, 119, 232, 120); // Синее стекло 120%
inline const QColor COLOR_GLASS_PRIMARY_150 = QColor(21, 119, 232, 150); // Синее стекло 150%
inline const QColor COLOR_GLASS_PRIMARY_LIGHT_185 = QColor(33, 150, 243, 60); // Светлое синее стекло 185% (для светлого фона)
inline const QColor COLOR_GLASS_PRIMARY_LIGHT_200 = QColor(150, 200, 255, 100); // Светлое синее стекло (для выделения устройств - очень светлый синий)
inline const QColor COLOR_GLASS_PRIMARY_190 = QColor(21, 119, 232, 190); // Синее стекло 190% (hover)
inline const QColor COLOR_GLASS_PRIMARY_220 = QColor(21, 119, 232, 220); // Синее стекло 220% (pressed)
inline const QColor COLOR_GLASS_ERROR_100 = QColor(220, 80, 80, 100);  // Красное стекло 100%
inline const QColor COLOR_GLASS_ERROR_200 = QColor(220, 80, 80, 200);  // Красное стекло 200%
inline const QColor COLOR_GLASS_ERROR_BANNER_180 = QColor(232, 53, 53, 180); // Красное стекло баннера 180%
inline const QColor COLOR_GLASS_ERROR_BANNER_220 = QColor(232, 53, 53, 220); // Красное стекло баннера 220%
inline const QColor COLOR_GLASS_SETTINGS_110 = QColor(235, 235, 235, 110); // Серое стекло настроек 110%
inline const QColor COLOR_GLASS_SETTINGS_120 = QColor(235, 235, 235, 120); // Серое стекло настроек 120%
inline const QColor COLOR_GLASS_SETTINGS_190 = QColor(235, 235, 235, 190); // Серое стекло настроек 190%
inline const QColor COLOR_GLASS_CALLING_180 = QColor(255, 245, 235, 180); // Оранжевое стекло звонка 180%
inline const QColor COLOR_GLASS_DISABLED_150 = QColor(160, 160, 160, 150); // Серое стекло disabled 150%
inline const QColor COLOR_GLASS_GRAY_150_DARK_20 = QColor(150, 150, 150, 51); // Серое стекло 150 dark 20% (0.20 * 255 = 51)
inline const QColor COLOR_GLASS_GRAY_90_75 = QColor(90, 90, 90, 191); // Серое стекло 90 75% (0.75 * 255 ≈ 191)

// ============================================================================
// Shadow Colors
// ============================================================================
inline const QColor COLOR_SHADOW_BLACK_8 = QColor(0, 0, 0, 8);           // Черная тень 8%
inline const QColor COLOR_SHADOW_BLACK_15 = QColor(0, 0, 0, 15);        // Черная тень 15%
inline const QColor COLOR_SHADOW_BLACK_50 = QColor(0, 0, 0, 50);       // Черная тень 50%
inline const QColor COLOR_SHADOW_BLACK_60 = QColor(0, 0, 0, 60);        // Черная тень 60%
inline const QColor COLOR_SHADOW_BLACK_80 = QColor(0, 0, 0, 80);       // Черная тень 80%
inline const QColor COLOR_SHADOW_BLACK_150 = QColor(0, 0, 0, 150);     // Черная тень 150%
inline const QColor COLOR_SHADOW_BLACK_160 = QColor(25, 25, 25, 160);  // Темная тень 160%
inline const QColor COLOR_SHADOW_PRIMARY_120 = QColor(33, 150, 243, 120); // Синяя тень 120%

// ============================================================================
// Special Colors
// ============================================================================
inline const QColor COLOR_ONLINE = QColor(100, 200, 100);              // Онлайн статус
inline const QColor COLOR_OFFLINE = QColor(150, 150, 150);              // Офлайн статус
inline const QColor COLOR_DISABLED = QColor(120, 120, 120);            // Отключенный элемент
inline const QColor COLOR_PLACEHOLDER = QColor(240, 240, 240, 180);    // Плейсхолдер
inline const QColor COLOR_SCROLLBAR_BG = QColor(208, 208, 208);           // Скроллбар
inline const QColor COLOR_SCROLLBAR_BG_HOVER = QColor(176, 176, 176);     // Скроллбар hover
inline const QColor COLOR_SCROLLBAR_BG_PRESSED = QColor(144, 144, 144);   // Скроллбар pressed
inline const QColor COLOR_SLIDER_GROOVE = QColor(77, 77, 77);          // Дорожка слайдера
inline const QColor COLOR_SLIDER_HANDLE = QColor(255, 255, 255);       // Ручка слайдера
inline const QColor COLOR_SLIDER_SUBPAGE = QColor(21, 119, 232);       // Заполненная часть слайдера
inline const QColor COLOR_SELECTION_BG = QColor(225, 245, 254);        // Фон выделения
inline const QColor COLOR_SELECTION_BG_LIGHT = QColor(250, 250, 250);  // Светлый фон выделения
inline const QColor COLOR_SETTINGS_PRIMARY = QColor(224, 168, 0);      // Основной цвет настроек
inline const QColor COLOR_SETTINGS_HOVER = QColor(219, 164, 0);        // Hover цвет настроек
inline const QColor COLOR_GRADIENT_START = QColor(230, 230, 230);      // Начало градиента
inline const QColor COLOR_GRADIENT_MIDDLE = QColor(220, 230, 240);     // Середина градиента
inline const QColor COLOR_GRADIENT_END = QColor(240, 240, 240);        // Конец градиента
inline const QColor COLOR_BUTTON_GRADIENT_START = QColor(63, 139, 252, 150); // Начало градиента кнопки
inline const QColor COLOR_BUTTON_GRADIENT_MIDDLE = QColor(127, 179, 255, 50); // Середина градиента кнопки
inline const QColor COLOR_BUTTON_GRADIENT_END = QColor(189, 215, 255, 0); // Конец градиента кнопки

// ============================================================================
// Hex Color Constants (для использования в стилях)
// ============================================================================
inline constexpr const char* COLOR_HEX_PRIMARY = "#1577E8";                // Основной синий
inline constexpr const char* COLOR_HEX_PRIMARY_DARK = "#0D6BC8";           // Темный синий
inline constexpr const char* COLOR_HEX_PRIMARY_DARKER = "#0A5FC8";         // Еще темнее
inline constexpr const char* COLOR_HEX_TEXT_PRIMARY = "#010B13";           // Основной текст
inline constexpr const char* COLOR_HEX_TEXT_SECONDARY = "#101010";         // Вторичный текст
inline constexpr const char* COLOR_HEX_TEXT_TERTIARY = "#0c2a4f";          // Третичный текст
inline constexpr const char* COLOR_HEX_TEXT_LIGHT = "#666666";             // Светлый текст (#666666)
inline constexpr const char* COLOR_HEX_ERROR = "#DC5050";                  // Ошибка
inline constexpr const char* COLOR_HEX_ERROR_BANNER = "#DC3545";           // Ошибка баннера
inline constexpr const char* COLOR_HEX_SUCCESS = "#19ba00";                // Успех
inline constexpr const char* COLOR_HEX_PURPLE = "#8C6BC7";                // Фиолетовый
inline constexpr const char* COLOR_HEX_WHITE = "#FFFFFF";                 // Белый
inline constexpr const char* COLOR_HEX_BORDER = "#d0d0d0";                 // Граница
inline constexpr const char* COLOR_HEX_BORDER_HOVER = "#c0c0c0";           // Граница hover
