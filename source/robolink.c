#include "robolink.h"

//------------------------------------------------------------------------------
/// Отправка команды на управляющий порт робота.
static gssize
robolink_send_control_cmd (robolink * t, char *cmd_name, char *msg, size_t size)
{
  gssize result = 0;
  t->client_time = clock ();

  t->command_moment_us = COMMAND_WAS_NOT_SEND;
  char portnum[10];
  uint16_t prev_answ_seq = t->rx_seq_num;
  if ((t->cmd_count > (prev_answ_seq + 1)) && (prev_answ_seq!=0))
    {
      g_print
	("Error! Answer for previous command (SEQ_NUM = <%u>) was not recieved.\n",
	 prev_answ_seq + 1);
      exit (1);
    }

  snprintf (portnum, sizeof (portnum), "%i", t->rbt_ctrl_port);


  if (t->ctrl_socket)
    {
      t->error = NULL;
      msg_apply_CRC16modbus (msg, size);
      result = g_socket_send_to (t->ctrl_socket, t->rbt_ctrl_address, msg,
				 size, NULL, &t->error);
      robolink_handle_error (t);
    }
         g_inet_socket_address_get_address ((GInetSocketAddress*) t->rbt_ctrl_address);
  if (result > -1)
    {
     t->command_moment_us = g_get_real_time();
     g_print
    ("Cmd <%s> SEQ_NUM = <%u> was sent to <%s:%s>, %lu bytes.\n",
     cmd_name,t->cmd_count, t->rbt_ctrl_address_txt->str,  portnum, (long unsigned int) result);
     wait_answer(t);
    }
  else
    {
      g_print
//	("Cmd <%s> was NOT sent to robot's control port <%s>, SEQ_NUM = <%u>,  %lu bytes.\n",
    ("Cmd <%s> SEQ_NUM =<%u> was NOT sent to <%s:%s>, %lu bytes.\n",
    // cmd_name, portnum, t->cmd_count, (long unsigned int) result);
     cmd_name,t->cmd_count, t->rbt_ctrl_address_txt->str,  portnum, (long unsigned int) result);
    }
  g_print ("...\n");

  return result;
}

//------------------------------------------------------------------------------
uint32_t
loop (robolink * t)
{
  VS_INCOME_CMD cmd;

  cmd.test_connection = rbt_test_connection_def;
  cmd.test_connection.msg.head.SEQ_NUM = robolink_get_seq_num (t);
  robolink_send_control_cmd (t, cmd.test_connection.TXT_ID,
			    (char *) &(cmd.test_connection.msg),
			    sizeof (rbt_test_connection_def.msg)
			    );
  return 0;
}

//------------------------------------------------------------------------------
/// Обработчик события главного цикла, отправляет команды роботу.
gboolean
cmd_handler (gpointer user_data)
{
  robolink *t;
  t = (robolink *) user_data;


  if (t->en_conncheck_in_loop)
    loop (t);
  return G_SOURCE_CONTINUE;
}

//------------------------------------------------------------------------------
/// Функция приема сообщений от робота.
static gssize
robolink_read_cmd (robolink * t, VS_OUTCOME_CMD * cmd, gboolean dont_use_delay)
{
  (void) dont_use_delay;
  size_t buflen = sizeof (VS_OUTCOME_CMD);
  char buf[sizeof (VS_OUTCOME_CMD)];

  g_clear_object(&t->income_addr);
  gssize len = g_socket_receive_from (t->current_socket,
				      (GSocketAddress **) & t->income_addr,
				      buf, buflen,
				      NULL, &t->error);
  robolink_handle_error (t);

  //if ((len > 0) && (len < sizeof(VS_INCOME_CMD)))
  if ((len > 0) && ((unsigned long) len <= sizeof (VS_OUTCOME_CMD)))
    {
      memcpy (cmd, buf, (size_t) len);
      t->rx_seq_num = cmd->header.SEQ_NUM;
    }
  return len;
}

//------------------------------------------------------------------------------
///  Обработчик события главного цикла, выполняет прием ответов робота с порта
///  логирования.
static gboolean
robolink_view_mode_handler (robolink * t)
{
  gboolean dont_use_delay = TRUE;
  //if (events & BELLE_SIP_EVENT_READ )
  {
    VS_OUTCOME_CMD answ;
    t->rx_res = robolink_read_cmd (t, &answ, dont_use_delay);
    if (answ.header.T_ANS_T_MSG != SERVER_ANSWER_FLAGS)
      {
	g_print ("T_ANS_T_MSG has wrong value.\n");
      }

    if (answ.header.SEQ_NUM == 0)
      {
	g_print ("SEQ_NUM = <0>.\n");
      }

    if (answ.header.SEQ_NUM != t->cmd_count)
      {
	g_print ("Error! RX SEQ_NUM = <%u> instead of <%u>\n",
		 answ.header.SEQ_NUM, t->cmd_count);
	exit (3);
      }

    if ((t->rx_res > 0) && (answ.header.T_ANS_T_MSG == SERVER_ANSWER_FLAGS))
      {
	guint16 income_port = g_inet_socket_address_get_port (t->income_addr);
	gchar *income_addr =
	  g_inet_address_to_string (g_inet_socket_address_get_address
				    (t->income_addr));
    g_print (INPUT_INFO_UNIFIED_FMT, income_addr, income_port, t->rx_res);
	g_free (income_addr);

	t->difftime = difftime (t->time, time (0));
	t->latest_answer = answ;
	uint16_t cmd_code = answ.header.CMD;
	if (t->current_socket == t->client_socket)
	  cmd_code = cmd_code - CLIENT_CMD_OFFSET;

	// USED uint16_t seq_num = answ.header.SEQ_NUM;
	if (answ.header.ERR_CODE != 0)
	  robot_error_fatal--;

	switch (cmd_code)
	  {
	  case VS_TEST_CONNECTION_CMD:
	    //  g_print("%s", rbt_test_connection_answ_def.TXT_ID, seq_num);
	    print_error_status (answ);
	    break;
	  }
      }
  }
  return TRUE;

//------------------------------------------------------------------------------
}/// Обработчик события главного цикла, выполняет прием ответов клиентского порта робота
/// c сообщениями лога.
gboolean
robolink_view_answer_handler (GSocket * sock, GIOCondition cond,
			     gpointer user_data)
{
  (void) cond;
  robolink *t;
  t = (robolink *) user_data;
  t->current_socket = sock;
  return robolink_view_mode_handler (t);
}

//------------------------------------------------------------------------------
static void
view_mode (robolink * tester)
{
  // Выполняем замену хандлера, так как в этом режиме нам требуется только печатать лог.
  g_source_set_callback (tester->ml_income_source,
			 (GSourceFunc) robolink_view_answer_handler,
			 (gpointer) tester, NULL);

  // Далее программа просто крутится в главном цикле, печатая на экран строки
  // лога прилетевшие от робота.
  g_print ("View mode is runing.\n");
  while (1)
    {
      g_main_context_iteration (tester->context, TRUE);
    }
}

//------------------------------------------------------------------------------
gboolean
control (gpointer data)
{
  robolink *t = (robolink *) data;

  robolink_set_seq_num(&tester, initial_seq_num);

  // Если задан IP удаленного робота то идем подписываться и смотреть его лог.
  if (t->vrobot_IP_view)
    {
      view_mode (t);
    }

 
   
  g_main_loop_quit (t->ml);
  return FALSE;			// Выполняем функцию один раз.
}

//------------------------------------------------------------------------------
gint
main (int argc, char *argv[])
{
    GOptionContext *option_context= NULL;
    option_context = g_option_context_new ("-  robot client");
    g_option_context_add_main_entries (option_context, cli_options, NULL);
    GError *error = NULL;
    g_set_prgname("./roboclient");

    }

    if (log_file)
    {
        g_print ("log file: <%s>\n", log_file);
        if (log_file_filter)
        {
            g_print ("log file filter: <%s>\n", log_file_filter);
        }
    }

    if (log_file)
    {
        log_file_fd = fopen (log_file, "w");
    }


    tester.rbt_ctrl_port = vrobot_control_port;
    tester.rbt_client_port = vrobot_client_port;
    tester.rbt_script_port = vrobot_script_port;
    tester.rbt_stim_port = 0;
    tester.rbt_config_filename = vrobot_cfg_filename;
    tester.draw_pdf = draw_pdf;
    tester.waiting_timeout_ms = answer_waiting_timeout;
    if (defines)
    {
        (tester.defines) = defines;
    }

    robolink_init (&tester, vrobot_address);

  if (run_robot_arg)
  {
      g_string_assign (tester.run_robot_arg, run_robot_arg);
      g_free (run_robot_arg);
  }


  // Периодическая отправка роботу команд проверки связи.
  tester.ml_source = g_timeout_source_new (3000);
  g_source_set_callback (tester.ml_source, (GSourceFunc) cmd_handler,
			 (gpointer) & tester, NULL);
  g_source_attach (tester.ml_source, tester.context);

  // Прием ответов от робота.
  tester.ml_income_source =
    g_socket_create_source (tester.ctrl_socket, G_IO_IN, NULL);
  g_source_set_callback (tester.ml_income_source,
			 (GSourceFunc) robolink_control_answer_handler,
			 (gpointer) & tester, NULL);
  g_source_attach (tester.ml_income_source, tester.context);

  // Заряжаем таймер на однократный запуск функции control.
  GSource *src3 = g_timeout_source_new (100);
  g_source_set_callback (src3, control, (gpointer) & tester, NULL);
  g_source_attach (src3, tester.context);

  // И наконец запускаем главный цикл.
  g_main_loop_run (tester.ml);

  // Завершение работы после выхода из главного цикла.
  g_source_destroy (src3);
  g_main_context_unref (tester.context);

  if (log_file)
  {
      fflush (log_file_fd);
      fclose (log_file_fd);
  }

   exit (EXIT_SUCCESS);		/* should never reach here */
}


