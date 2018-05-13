/*
 * main.c
 * Copyright (C) Daniele Cruciani 2006 <daniele@smartango.com>
 * 
 * ppplogger-gtk is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * ppplogger-gtk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 *      The Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor
 *      Boston, MA  02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <libgnomeui/libgnomeui.h>
#include <glade/glade.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <gconf/gconf-client.h>
#include "config.h"

GConfClient *gconfcli;

MYSQL *DB_con;
GladeXML *gxml;
GtkListStore *call_store;
GtkListStore *conn_store;
long int from_time;
long int to_time;
char *tempo_str;
char *cost_time_str;
char *traffico_str;
char *cost_traf_str;
long int traffico;
long int tempo;
double price_traf;
double price_time;
double cost_traf;
double cost_time;
int traffic_per;
int time_per;
int div_per_cent;

char *wheres;
int connesso;


GtkStatusbar *statusbar;
guint stat_con_ctx;
guint stat_qry_ctx;
char *message_qry;

struct{
  char* connessione;
  long int da_time;
  long int a_time;
} for_wheres;

struct {
  char *host;
  char *user;
  char *passwd;
  char *dbname;
  unsigned int port;
  char *socket;
} dbcon_params;

const char *COLS_NAME[17] = {
  "DEVICE",
  "IFNAME",
  "IPLOCAL",
  "IPREMOTE",
  "PEERNAME",
  "SPEED",
  "ORIG_UID",
  "PPPLOGNAME",
  "PPPD_PID",
  "CONNECT_TIME",
  "BYTES_SENT",
  "BYTES_RCVD",
  "CALL_FILE",
  "DNS1",
  "DNS2",
  "PRICE_TIME",
  "PRICE_BYTE"
};

enum{
  DEVICE_COLUMN,
    IFNAME_COLUMN,
    IPLOCAL_COLUMN,
    IPREMOTE_COLUMN,
    PEERNAME_COLUMN,
    SPEED_COLUMN,
    ORIG_UID_COLUMN,
    PPPLOGNAME_COLUMN,
    PPPD_PID_COLUMN,
    CONNECT_TIME_COLUMN,
    BYTES_SENT_COLUMN,
    BYTES_RCVD_COLUMN,
    CALL_FILE_COLUMN,
    DNS1_COLUMN,
    DNS2_COLUMN,
    PRICE_TIME_COLUMN,
    PRICE_BYTE_COLUMN,
    N_COLUMNS
};

void connect_db()
{
  if(connesso) {
    mysql_close(DB_con);
    gtk_statusbar_pop(statusbar,stat_con_ctx);
  }
  DB_con = mysql_init(NULL);
  connesso=0;
  stat_con_ctx = gtk_statusbar_get_context_id(statusbar,"connessione");
  //  if (mysql_real_connect(DB_con,NULL, "pppd","pppd","pppdconnection", 0, NULL, 0) == NULL) {
  if (mysql_real_connect(DB_con,
			 dbcon_params.host,
			 dbcon_params.user,
			 dbcon_params.passwd,
			 dbcon_params.dbname,
			 dbcon_params.port,
			 dbcon_params.socket,
			 0) == NULL) {
    printf("DB connection error\n");
    gtk_statusbar_push(statusbar,stat_con_ctx,"Errore di connessione");
    return;
  }
  connesso=1;
  gtk_statusbar_push(statusbar,stat_con_ctx,"Connesso");
}

void fill_comboconn()
{
  char *strqry;
  MYSQL_RES *result;
  MYSQL_ROW row;
  GtkComboBox *conCombo;
  GtkTreeIter iter;
  if (!connesso)return;
  gtk_list_store_clear (conn_store);

  conCombo = (GtkComboBox *) glade_xml_get_widget(gxml,"combo_connessioni");
  // seleziona tutte le connessioni
  gtk_list_store_append(conn_store,&iter);
  gtk_list_store_set(conn_store,&iter,0,"",-1);
  gtk_combo_box_set_active_iter(conCombo,&iter);

  asprintf(&strqry,"SELECT DISTINCT DEVICE FROM start_connect");
  result=NULL;
  mysql_query(DB_con,strqry);
  result = mysql_store_result(DB_con);

  while((row= mysql_fetch_row(result))) {
    gtk_list_store_append(conn_store,&iter);
    if (row[0])
      gtk_list_store_set(conn_store,&iter,0,row[0],-1);
  }
  mysql_free_result(result);
  free(strqry);
}

void fill_field()
{
  char *strqry;
  MYSQL_RES *result;
  MYSQL_ROW row;
  int num_res;
  if (!connesso)return;
  gtk_statusbar_pop(statusbar,stat_qry_ctx);
  stat_qry_ctx = gtk_statusbar_get_context_id(statusbar,"query");

  asprintf(&strqry,"SELECT start_connect.DEVICE, start_connect.IFNAME,start_connect.IPLOCAL, start_connect.IPREMOTE, start_connect.PEERNAME,start_connect.SPEED,start_connect.ORIG_UID, start_connect.PPPLOGNAME,start_connect.PPPD_PID,end_connect.CONNECT_TIME, end_connect.BYTES_SENT, end_connect.BYTES_RCVD,end_connect.CALL_FILE, end_connect.DNS1,end_connect.DNS2,end_connect.CONNECT_TIME /%d * %g /%d /%d, (end_connect.BYTES_SENT + end_connect.BYTES_RCVD)/%d * %g /%d FROM start_connect,end_connect WHERE start_connect.id_inizio = end_connect.id_inizio %s",time_per, price_time,time_per,div_per_cent, traffic_per,price_traf,div_per_cent,wheres);
  //asprintf(&strqry,"SELECT * FROM start_connect");
  //printf ("Query: '%s'\n", strqry);
  result = NULL;
  if (mysql_query(DB_con,strqry)!=0){
    printf("Ci deve essere un errore\n");
    printf("QUERY: '%s'\n",strqry);
  }
  result = mysql_store_result(DB_con);
  free(strqry);
  if(result == NULL) {printf("era NULL\n");return;}
  //printf("SEMBRA OK ,...\n");

    gtk_list_store_clear (call_store);
  while((row= mysql_fetch_row(result))) {

    //printf("%s\n", row[0] ? row[0] : "NULL");return;
    GtkTreeIter   iter;
    
    gtk_list_store_append(call_store,&iter);
    gtk_list_store_set(call_store,&iter,
		       DEVICE_COLUMN, row[0],
		       IFNAME_COLUMN,  row[1],
		       IPLOCAL_COLUMN, row[2],
		       IPREMOTE_COLUMN, row[3],
		       PEERNAME_COLUMN, row[4],
		       SPEED_COLUMN, row[5],
		       ORIG_UID_COLUMN, row[6],
		       PPPLOGNAME_COLUMN, row[7],
		       PPPD_PID_COLUMN, row[8],
		       CONNECT_TIME_COLUMN, row[9],
		       BYTES_SENT_COLUMN, row[10],
		       BYTES_RCVD_COLUMN, row[11],
		       CALL_FILE_COLUMN, row[12],
		       DNS1_COLUMN, row[13],
		       DNS2_COLUMN, row[14],
		       PRICE_TIME_COLUMN, row[15],
		       PRICE_BYTE_COLUMN, row[16],
		       -1);
    
  }
  num_res = mysql_num_rows(result);
  mysql_free_result(result);
  free(message_qry);
  message_qry=NULL;
  asprintf(&message_qry,"%d connessioni caricate",num_res);
  gtk_statusbar_push(statusbar,stat_con_ctx,message_qry);

}

void update_cost_time()
{
  char *strqry;
  MYSQL_RES *result;
  MYSQL_ROW row;
  GtkEntry *cost_time_e;
  GtkEntry *tempo_e;
  if (!connesso)return;
  cost_time_e = (GtkEntry*) glade_xml_get_widget(gxml,"entry_costotempo");
  tempo_e = (GtkEntry*) glade_xml_get_widget(gxml,"entry_tempo");
  asprintf(&strqry,"SELECT SUM(end_connect.CONNECT_TIME) from start_connect,end_connect WHERE start_connect.id_inizio = end_connect.id_inizio %s", wheres);
  //printf("%s\n",strqry);
  //return;
  mysql_query(DB_con,strqry);
  result = NULL;
  result = mysql_store_result(DB_con);
  free(strqry);
  row = mysql_fetch_row(result);
  if (row[0]==NULL) tempo = 0;
  else
    tempo = atol(row[0]);
  mysql_free_result(result);
  cost_time = tempo * price_time / time_per / div_per_cent;
  asprintf(&tempo_str,"%d",tempo);
  asprintf(&cost_time_str,"%g",cost_time);
  gtk_entry_set_text(tempo_e,tempo_str);
  gtk_entry_set_text(cost_time_e,cost_time_str);
}

void update_cost_traf()
{
  char *strqry;
  MYSQL_RES *result;
  MYSQL_ROW row;
  GtkEntry *cost_traf_e;
  GtkEntry *traff_e;
  if (!connesso)return;
  cost_traf_e = (GtkEntry*) glade_xml_get_widget(gxml,"entry_costotraffico");
  traff_e = (GtkEntry*) glade_xml_get_widget(gxml,"entry_traffico");
  asprintf(&strqry,"SELECT SUM(end_connect.BYTES_SENT + end_connect.BYTES_RCVD) from start_connect,end_connect WHERE start_connect.id_inizio = end_connect.id_inizio %s", wheres);
  //printf("%s\n",strqry);
  //return;
  mysql_query(DB_con,strqry);
  result = NULL;
  result = mysql_store_result(DB_con);
  free(strqry);
  row = mysql_fetch_row(result);
  if (row[0]==NULL) traffico = 0;
  else
    traffico = atol(row[0]);
  mysql_free_result(result);
  cost_traf = traffico /traffic_per * price_traf / div_per_cent;
  asprintf(&traffico_str,"%d",traffico);
  asprintf(&cost_traf_str,"%g",cost_traf);
  gtk_entry_set_text(traff_e,traffico_str);
  gtk_entry_set_text(cost_traf_e,cost_traf_str);
}

void update_value()
{
  fill_field();
  update_cost_time();
  update_cost_traf();
}
void on_combo_trafficoper_changed(GtkWidget *widget, gpointer user_data)
{
  GtkComboBox *traf_com = (GtkComboBox*)widget;
  char *traf_per_str;
  traf_per_str = gtk_combo_box_get_active_text(traf_com);
  traffic_per = atoi(traf_per_str);
  //printf("%d\n",traffic_per);
  update_value();
}
void on_combo_tempoper_changed(GtkWidget *widget, gpointer user_data)
{
  GtkComboBox *tempo_com = (GtkComboBox*)widget;
  if( gtk_combo_box_get_active(tempo_com) == 0) {
    time_per = 60;
  } else {
    time_per = 1;
  }
  update_value();
}
void on_btn_cancel_conn_clicked(GtkWidget *widget, gpointer user_data)
{
  GtkWidget *connd = glade_xml_get_widget(gxml,"connectDialog");
  gtk_widget_hide(connd);
}

void on_btn_connect_clicked(GtkWidget *widget, gpointer user_data)
{
  GtkEntry *host = (GtkEntry*) glade_xml_get_widget(gxml,"entry_host");
  GtkEntry *port = (GtkEntry*) glade_xml_get_widget(gxml,"entry_port");
  GtkEntry *socket = (GtkEntry*) glade_xml_get_widget(gxml,"entry_socket");
  GtkEntry *user = (GtkEntry*) glade_xml_get_widget(gxml,"entry_user");
  GtkEntry *passwd = (GtkEntry*) glade_xml_get_widget(gxml,"entry_passwd");
  GtkEntry *dbname = (GtkEntry*) glade_xml_get_widget(gxml,"entry_dbname");

  if (strlen(gtk_entry_get_text(host)) ==0) {
    free(dbcon_params.host);
    dbcon_params.host = NULL;
  } else {
    asprintf(&(dbcon_params.host),"%s",gtk_entry_get_text(host));
  }
  if (strlen(gtk_entry_get_text(port)) ==0) {
    dbcon_params.port = 0;
  } else {
    dbcon_params.port = atoi(gtk_entry_get_text(port));
  }
  if (strlen(gtk_entry_get_text(socket)) ==0) {
    free(dbcon_params.socket);
    dbcon_params.socket = NULL;
  } else {
    asprintf(&(dbcon_params.socket),"%s",gtk_entry_get_text(socket));
  }
  if (strlen(gtk_entry_get_text(user)) ==0) {
    free(dbcon_params.user);
    dbcon_params.user = NULL;
  } else {
    asprintf(&(dbcon_params.user),"%s",gtk_entry_get_text(user));
  }
  if (strlen(gtk_entry_get_text(passwd)) ==0) {
    free(dbcon_params.passwd);
    dbcon_params.passwd = NULL;
  } else {
    asprintf(&(dbcon_params.passwd),"%s",gtk_entry_get_text(passwd));
  }
  if (strlen(gtk_entry_get_text(dbname)) ==0) {
    free(dbcon_params.dbname);
    dbcon_params.dbname = NULL;
  } else {
    asprintf(&(dbcon_params.dbname),"%s",gtk_entry_get_text(dbname));
  }
  gconf_client_set_string(gconfcli,"/apps/ppplogger-gtk/dbcon/hostname",dbcon_params.host?dbcon_params.host:"",NULL);
  gconf_client_set_int(gconfcli,"/apps/ppplogger-gtk/dbcon/port",dbcon_params.port,NULL);
  gconf_client_set_string(gconfcli,"/apps/ppplogger-gtk/dbcon/socket",dbcon_params.socket?dbcon_params.socket:"",NULL);
  gconf_client_set_string(gconfcli,"/apps/ppplogger-gtk/dbcon/user",dbcon_params.user?dbcon_params.user:"",NULL);
  gconf_client_set_string(gconfcli,"/apps/ppplogger-gtk/dbcon/passwd",dbcon_params.passwd?dbcon_params.passwd:"",NULL);
  gconf_client_set_string(gconfcli,"/apps/ppplogger-gtk/dbcon/dbname",dbcon_params.dbname?dbcon_params.dbname:"",NULL);
  connect_db();
  fill_comboconn();
  update_value();
  GtkWidget *connd = glade_xml_get_widget(gxml,"connectDialog");
  gtk_widget_hide(connd);  
}

void load_db_params()
{
  GtkEntry *host = (GtkEntry*) glade_xml_get_widget(gxml,"entry_host");
  GtkEntry *port = (GtkEntry*) glade_xml_get_widget(gxml,"entry_port");
  GtkEntry *socket = (GtkEntry*) glade_xml_get_widget(gxml,"entry_socket");
  GtkEntry *user = (GtkEntry*) glade_xml_get_widget(gxml,"entry_user");
  GtkEntry *passwd = (GtkEntry*) glade_xml_get_widget(gxml,"entry_passwd");
  GtkEntry *dbname = (GtkEntry*) glade_xml_get_widget(gxml,"entry_dbname");
  char *str=NULL; int portn;
  str = gconf_client_get_string(gconfcli,"/apps/ppplogger-gtk/dbcon/hostname",NULL);
  if (str && strlen(str) == 0) {
    free(str);
    str=NULL;
  }
  dbcon_params.host =str;
  portn = gconf_client_get_int(gconfcli,"/apps/ppplogger-gtk/dbcon/port",NULL);
  if (portn >0) {
    dbcon_params.port =portn;
  } else {
    dbcon_params.port =0;
  }
  str=NULL;
  str = gconf_client_get_string(gconfcli,"/apps/ppplogger-gtk/dbcon/socket",NULL);
  if (str && strlen(str) == 0) {
    free(str);
    str=NULL;
  }
  dbcon_params.socket =str;
  str=NULL;
  str = gconf_client_get_string(gconfcli,"/apps/ppplogger-gtk/dbcon/user",NULL);
  if (str && strlen(str) == 0) {
    free(str);
    str=NULL;
  }
  dbcon_params.user =str;
  str=NULL;
  str = gconf_client_get_string(gconfcli,"/apps/ppplogger-gtk/dbcon/passwd",NULL);
  if (str && strlen(str) == 0) {
    free(str);
    str=NULL;
  }
  dbcon_params.passwd =str;
  str=NULL;
  str = gconf_client_get_string(gconfcli,"/apps/ppplogger-gtk/dbcon/dbname",NULL);
  if (str && strlen(str) == 0) {
    free(str);
    str=NULL;
  }
  dbcon_params.dbname =str;
  str=NULL;
  if (dbcon_params.port>0) {
    asprintf(&str,"%d",dbcon_params.port);
    gtk_entry_set_text(port,str);
    free(str);
  }
  gtk_entry_set_text(host,dbcon_params.host?dbcon_params.host:"");
  gtk_entry_set_text(socket,dbcon_params.socket?dbcon_params.socket:"");
  gtk_entry_set_text(user,dbcon_params.user?dbcon_params.user:"");
  gtk_entry_set_text(passwd,dbcon_params.passwd?dbcon_params.passwd:"");
  gtk_entry_set_text(dbname,dbcon_params.dbname?dbcon_params.dbname:"");
}

gboolean on_window_close(GtkWidget *widget, gpointer user_data)
{
  gtk_widget_hide(widget);
  gtk_main_quit();
  return FALSE;
}
void on_entry_costoptempo_changed(GtkEntry *widget,gpointer user_data)
{
  char *pricetime = (char *)gtk_entry_get_text(widget);
  price_time = strtod(pricetime,NULL);
  free(pricetime);
  update_value();
  //printf("%g\n",price_time);
}

void on_entry_costoptraffico_changed(GtkWidget *widget,gpointer user_data)
{
  char *pricetime = (char*)gtk_entry_get_text((GtkEntry*)widget);
  price_traf = strtod(pricetime,NULL);
  free(pricetime);
  update_value();
  //printf("%g\n",price_time);
}

void update_wheres()
{
  char *wherecon;
  char *whereda;
  char *wherea;
  wherecon = NULL; whereda=NULL;wherea=NULL;
  if (for_wheres.connessione != NULL) {
    asprintf(&wherecon," AND DEVICE='%s'",for_wheres.connessione);
  }
  if (for_wheres.da_time >0) {
    asprintf(&whereda," AND UNIX_TIMESTAMP(fine)>%d",for_wheres.da_time);
  }
  if (for_wheres.a_time >0) {
    asprintf(&wherea," AND UNIX_TIMESTAMP(inizio)<%d",for_wheres.a_time);
  }
  asprintf(&wheres,"%s%s%s",wherecon?wherecon:"",whereda?whereda:"",wherea?wherea:"");
  free(wherecon);
  free(whereda);
  free(wherea);
}

void on_combo_connessioni_changed(GtkWidget *widget,gpointer user_data)
{
  GtkComboBox *con_combo =(GtkComboBox*)widget;
  GtkTreeIter aiter;
  char *connes_name;

  if (gtk_combo_box_get_active_iter(con_combo,&aiter)) {
    gtk_tree_model_get (GTK_TREE_MODEL(conn_store), &aiter,
			0, &connes_name,-1);
    for_wheres.connessione = connes_name;
    if(strlen(connes_name)==0) {
      free(for_wheres.connessione);
      for_wheres.connessione = NULL;
    }
  } else {
    free(for_wheres.connessione);
    for_wheres.connessione = NULL;
  }
  update_wheres();
  update_value();
}

void on_dateedit_da_changed(GtkWidget *widget,gpointer user_data)
{
  GnomeDateEdit *da = (GnomeDateEdit *)widget;
  for_wheres.da_time = gnome_date_edit_get_time(da);
  update_wheres();
  update_value();
}

void on_dateedit_a_changed(GtkWidget *widget,gpointer user_data)
{
  GnomeDateEdit *at = (GnomeDateEdit *)widget;
  for_wheres.a_time = gnome_date_edit_get_time(at);
  update_wheres();
  update_value();
}

void on_toolbutton4_clicked(GtkWidget *widget, gpointer user_data)
{
  GtkWindow *main_win = (GtkWindow*)glade_xml_get_widget(gxml,"window");
  gchar *authors[] = { "Daniele Cruciani <daniele@smartango.com>", NULL };
  gtk_show_about_dialog(main_win,
			"authors",authors,
			"license","GPL",
			"copyright","Copyright 2005,2006 Daniele Cruciani",
			"website","http://www.smartango.com/",
			"website-label","www.smartango.com",
			NULL);
}

void on_toolbutton1_clicked(GtkWidget *widget, gpointer user_data)
{
  GtkWidget *aboutd = glade_xml_get_widget(gxml,"connectDialog");
  gtk_widget_show(aboutd);
  //gtk_window_activate_default(aboutd);
}

void on_check_cent_toggled(GtkWidget *widget, gpointer user_data)
{
  GtkToggleButton *check_cent = (GtkToggleButton *)widget;
  if(gtk_toggle_button_get_active(check_cent))
    div_per_cent = 100;
  else
    div_per_cent = 1;
  update_value();
}

void on_button1_clicked(GtkWidget *widget, gpointer user_data) {
//some_signal_handler_func

  /* do something useful here */
  printf("solo una cosa stupidissima per vedere se funziona\n");
}

void on_button2_clicked( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
  printf("Qualcosa di inutile dal bottone2\n");
}

void on_window1_delete_event(GtkWidget *widget,
			     gpointer user_data)
{
  gtk_main_quit();
}

void init_tree_view()
{
  GtkTreeView *table_con;
  GtkCellRenderer *renderer;
  GtkComboBox *conCombo;
  int i;
  // conCombo
  conCombo = (GtkComboBox *) glade_xml_get_widget(gxml,"combo_connessioni");

  renderer = gtk_cell_renderer_text_new ();
  conn_store = gtk_list_store_new(1,G_TYPE_STRING);
  do {
    
    gtk_combo_box_set_model(conCombo,GTK_TREE_MODEL(conn_store));
    gtk_cell_layout_clear(GTK_CELL_LAYOUT(conCombo));
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(conCombo),renderer,TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(conCombo),renderer,"text",0,NULL);
  }while(0);

  // TreeView
  call_store = gtk_list_store_new(N_COLUMNS,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING
				  );

  table_con = (GtkTreeView *)glade_xml_get_widget(gxml,"treeview_conn");
  gtk_tree_view_set_model(table_con,GTK_TREE_MODEL(call_store));


  for(i=0;i<N_COLUMNS;i++) {
    GtkTreeViewColumn *column;
    column = gtk_tree_view_column_new_with_attributes (COLS_NAME[i],
						     renderer,
						     "text",
						     i,
						     NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (table_con), column);
  }
}

void setup_combos()
{
  GtkComboBox *traf_comb = (GtkComboBox *)glade_xml_get_widget(gxml,"combo_trafficoper");
  GtkComboBox *temp_comb = (GtkComboBox *)glade_xml_get_widget(gxml,"combo_tempoper");
  GtkToggleButton *check_cent = (GtkToggleButton *)glade_xml_get_widget(gxml,"check_cent");
  gtk_combo_box_set_active(traf_comb,0);
  gtk_combo_box_set_active(temp_comb,1);
  traffic_per=1024;
  time_per =1;
  
  if(gtk_toggle_button_get_active(check_cent))
    div_per_cent = 100;
  else
    div_per_cent = 1;
}

void reset_struct()
{
  //for_wheres.
  dbcon_params.host=NULL;
  dbcon_params.socket=NULL;
  dbcon_params.port=0;
  dbcon_params.user=NULL;
  dbcon_params.passwd=NULL;
  dbcon_params.dbname=NULL;
  for_wheres.connessione=NULL;
  for_wheres.da_time =-1;
  for_wheres.a_time =-1;
}

#define GLADE_FILE PACKAGE_DATA_DIR"/ppplogger-gtk/glade/ppplog-gtk.glade"

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);
  gconf_init(argc,argv,NULL); // please explain which other func if
			      // this is deprecated
  /* load the interface */
  gxml = glade_xml_new(GLADE_FILE, NULL, NULL);
  init_tree_view();
  reset_struct();

  gconfcli = gconf_client_get_default();

  load_db_params();

  statusbar = (GtkStatusbar*)glade_xml_get_widget(gxml,"statusbar1");

  asprintf(&wheres,"");
  setup_combos();
  connesso=0;
  message_qry=NULL;

  connect_db();
  fill_comboconn();

  /* connect the signals in the interface */
  glade_xml_signal_autoconnect(gxml);

  fill_field();

  /* start the event loop */
  gtk_main();

  mysql_close(DB_con);
  return 0;

}
