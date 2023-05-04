#ifndef ROBOLINK_H
#define ROBOLINK_H

#include <string.h>
#ifndef _WIN32_WCE
#include </usr/include/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#endif /*_WIN32_WCE*/
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>


#ifdef WIN32
#include <ws2tcpip.h>
#include <ctype.h>
#ifndef _WIN32_WCE
#include <conio.h>
#endif /*_WIN32_WCE*/
#else
#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/stat.h>
#endif

#include <time.h>
#include <stdlib.h>
#include <stddef.h>
#include <glib-2.0/gio/gio.h>
#include <glib-2.0/glib.h>

//------------------------------------------------------------------------------
/// Структура для управления тестированием готосового робота.
typedef struct
{
  /// Счетчик команд отправляемых роботу.
  /// Используется для установки уникального поля команды SEQ_NUM.
  /// Доступ этоому полю только через функцию get_sec_num().
  uint16_t cmd_count;

  /// Счетчик времени для отправки периодических команд клиентам.
  clock_t client_time;

  /// Значение SEQ_NUM последней принятой команды.
  uint16_t rx_seq_num;

  /// Вспомогательная переменная
  uint16_t last_seq_num;

  /// Разрешение циклической проверки соединения.
  bool_t en_conncheck_in_loop;

  /// IP Адрес робота по которому тестер получает полное управление роботом.
  GSocketAddress *rbt_ctrl_address;
  GInetAddress *rbt_ctrl_ia;
  guint16 rbt_ctrl_port;		/// Управляющий порт робота.
  GString *rbt_ctrl_address_txt;	/// Адрес роботва в текстовой форме.
  /// Сокет, через который тестер взаимодействует с роботом.
  GSocket *ctrl_socket;

  /// IP Адрес робота с которым работает тестер притворяясь клиентом, с этого порта
  /// доступен только короткий перечень команд.
  GSocketAddress *rbt_client_address;
  GInetAddress *rbt_client_ia;
  guint16 rbt_client_port;	/// Управляющий порт робота.
  guint16 rbt_script_port;	/// Порт робота для взаимодействия со скриптами фильтров.
  GString *rbt_client_address_txt;	/// Адрес роботва в текстовой форме.
  /// Сокет, через который тестер имитирует отправку клиентами команд роботу на клиентский порт
  GSocket *client_socket;

  /// IP Адрес робота на который данный сокет будет отправлять произвольные сообщения.
  GSocketAddress *rbt_stim_address;
  GInetAddress *rbt_stim_ia;
  guint16 rbt_stim_port;		/// Произвольный порт робота.
  GString *rbt_stim_address_txt;	/// Адрес роботва в текстовой форме.
  /// Сокет, через который тестер может послать пакет по произвольному адресу.
  GSocket *stim_socket;
  GByteArray *stim_answer_buf;
  GByteArray *stim_answer_ref_buf;
  gboolean stim_answer_correct;

  GSocket *current_socket;

  /// Указатель в который будет помещен адрес источника ответа.
  GInetSocketAddress *income_addr;

  /// Указатель на контекст главного цикла glib.
  GMainContext *context;

  /// Указатель на главный цикл glib.
  GMainLoop *ml;

  /// Источники событий для главного цикла glib.
  GSource *ml_source;
  GSource *ml_income_source;
  GSource *ml_client_income_source;
  GSource *ml_stim_income_source;
  GSource *ml_cunit_source;
  GSource *ml_stop_source;

  GString *test_suite;
  GString *test_path;

  /// Последний ответ полученный от робота.
  VS_OUTCOME_CMD latest_answer;

  /// Связанный список строк.
  GList *str_list;

  /// Поле общего назначения, например для передачи аргумента функции.
  int arg1;

  /// Поле общего назначения, например для передачи строкового аргумента
  /// функции.
  char *str_arg;

  /// Поле общего назначения, например для передачи результата функции.
  gssize tx_res;

  /// Поле общего назначения, например для передачи результата функции.
  gssize rx_res;

  /// Поле общего назначения для передачи времени.
  time_t time;

  /// Поле общего назначения для передачи разницы времен.
  double difftime;

  /// Счетчик сообщений об ошибках полученных на робота.
  int server_err_cnt;

  /// Буфер для прием длинных ответов из нескольких сообщений.
  GList *long_rx_buf;

  /// Структура для получения сообщения об ошибке от библиотеки glib. Перед использованием
  /// должна быть равна нулю. После получения и использования нужно освободить память от сообщения.
  GError *error;

  /// Буфер для лога.
  GString *short_log_message;

  /// Имя файла конфигурации, который будет построчно передан роботу.
  gchar *rbt_config_filename;

  /// Строка которая хранит IP адрес, порт управления и PID удаленного робота, к которому
  /// требуется подписаться на получение сообщений лога.
  gchar *rbterver_view;

  /// Адрес робота лог которого передается клиенту для печати.
  GSocketAddress *rbterver_IP_view;

  /// Таймоут на ожидание ответа на команду, ms. Если таймаут не назначен, то равен -1.
  gint waiting_timeout_ms;

  /// Момент, в который была отправлена последняя команда, мкс.Если командв не отправлялась, равен -1.
  gint64 command_moment_us;

  /// Фактическое время ожидания ответа на команду, мкс. Если ответ не был получен, равно -1.
  gint64 answer_delay_us;


} _robolink;

typedef _robolink robolink;

#endif
