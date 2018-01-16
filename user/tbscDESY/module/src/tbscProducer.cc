#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>
#ifndef _WIN32
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

//---- SQL related lib:
#include <sql.h>
#include <sqlext.h>

#define MAX_COL_NAME_LEN  256


void printNestedMap ( std::map<std::string, std::map<std::string, std::string>> map_data){
  
  std::map<std::string, std::string>::iterator itr1;
  std::map<std::string, std::map<std::string, std::string> >::iterator itr2;
  for(itr2 = map_data.begin(); itr2 != map_data.end(); itr2++)
    {
      std::map<std::string, std::string> data = itr2->second;
      for(itr1 = data.begin(); itr1 != data.end(); itr1++)
	{
	  std::cout << "*^*Map: " << itr2->first << " \\ " << itr1->first << " \\ " << itr1->second <<std::endl;
	}
    }
}

void readCSVToVector (std::string csv_str,
		      std::vector<std::string> &csv_vec){
  std::stringstream csv_ss(csv_str);
  std::string itoken;
  csv_vec.clear();
  
  while( std::getline(csv_ss, itoken, ',') ){
    csv_vec.push_back(itoken);
  }
  
  //printf("[debug] string = '%s'\n\t csv_vec.size() == %d\n", csv_str.c_str(), csv_vec.size());
  std::cout<<"==> You are requesting tags from SC: \t[";
  for (int j=0; j<csv_vec.size(); j++) std::cout<<" "<< csv_vec.at(j)<<",";
  std::cout<<"]\n";
}

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class tbscProducer : public eudaq::Producer {
  public:
  tbscProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void Mainloop();

  static const uint32_t m_id_factory = eudaq::cstr2hash("tbscProducer");

  
  std::vector<std::string> ConvertStringToVec(std::string str, char delim)
  {// added by wmq TBD: to integrate to a separate tool header!
    std::vector<std::string> vec;
    std::stringstream strstream(str);
    std::string cell;
    
    while (getline(strstream, cell, delim)){
      vec.push_back(cell);
    }
    return vec;
  }
  void fillChannelMap(std::string inttablename="");
  void odbcFetchData(SQLHSTMT stmt, /*statement handle allocated to a database*/
		     SQLCHAR* dbcommand, /*mysql command to execute w/ stmt*/
		     SQLSMALLINT columns, 
		     std::map<std::string, std::map<std::string, std::string>> &data_map /*output*/
		     ); /* columns in result-set*/
  void odbcListDSN();
  void odbcDoAlotPrint();
  void odbcExtractError(std::string fn, /*input string to print out*/
			SQLHANDLE handle, /*an odbc handle*/
			SQLSMALLINT type);  /*type of handle*/
private:
  bool m_debug;
  SQLHENV m_env;
  SQLHDBC m_dbc;
  SQLHSTMT m_stmt;
  SQLRETURN m_ret; /* ODBC API return status */
  SQLSMALLINT m_columns; /* number of columns in result-set */

  std::string m_tbsc_dsn;
  std::string m_tbsc_db;
  std::string m_tbsc_tab_uni;
  std::string m_tbsc_tab_data;

  std::string m_sqlcli;

  unsigned int m_s_intvl;
  //std::string m_tbsc_mask;
  std::vector<std::string> m_tbsc_mask;
  
  std::thread m_thd_run;

  // mapa<(string)ch_id, std::mapb>, mapb<aida_channels.colName, val>
  std::map <std::string, std::map<std::string, std::string>> m_sc_para_map;
  
  ///------ stale below
  //bool m_flag_ts;
  //bool m_flag_tg;
  //uint32_t m_plane_id;
  //FILE* m_file_lock;
  //std::chrono::milliseconds m_ms_busy;

  bool m_exit_of_run;

  //  std::string m_stream_path;
  //std::ifstream m_file_stream;
  struct stat m_sb;
  std::map<std::string, std::string> m_tag_map;
  std::vector<std::string> m_tag_vc, m_value_vc;
 
};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------
//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<tbscProducer, const std::string&, const std::string&>(tbscProducer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
tbscProducer::tbscProducer(const std::string & name, const std::string & runcontrol):
  eudaq::Producer(name, runcontrol),
  m_exit_of_run(false),
  m_debug(false),
  m_s_intvl(90)
{
 
  /* Allocate an environment handle */
  SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_env);
  /* We want ODBC 3 support */
  SQLSetEnvAttr(m_env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
  /* Allocate a connection handle */
  SQLAllocHandle(SQL_HANDLE_DBC, m_env, &m_dbc);

}
//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void tbscProducer::DoInitialise(){
  auto ini = GetInitConfiguration();
  m_tbsc_dsn = ini->Get("TBSC_DSN", "myodbc5a");
  m_tbsc_db  = ini->Get("TBSC_DATABASE", "aidaTest");
  m_tbsc_tab_uni = ini->Get("TBSC_TAB_UNIT", "aida_channels");
  m_tbsc_tab_data = ini->Get("TBSC_TAB_DATA", "aidaSC");

  std::string ini_debug = ini->Get("TBSC_DEBUG", "");
  m_debug = (ini_debug=="true"||ini_debug=="True")? true : false;
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void tbscProducer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_tbsc_dsn=conf->Get("TBSC_DSN", m_tbsc_dsn);
  m_tbsc_db = conf->Get("TBSC_DATABASE", m_tbsc_db);
  m_tbsc_tab_uni = conf->Get("TBSC_TAB_UNIT", m_tbsc_tab_uni);
  m_tbsc_tab_data = conf->Get("TBSC_TAB_DATA", m_tbsc_tab_data);

  std::string conf_debug = conf->Get("TBSC_DEBUG", "");
  if (conf_debug!="")
    m_debug = (conf_debug=="true" || conf_debug=="True")? true: false;
 
  m_s_intvl = conf->Get("TBSC_INTERVAL_SEC", m_s_intvl);
  printf("check invl: %d\n", m_s_intvl);

  /* read the parameter mask from config, decide which parameters saved to events*/
  std::string tbsc_mask = conf->Get("TBSC_PARA_MASK", "ch0,ch10,ch20,ch30");
  readCSVToVector(tbsc_mask, m_tbsc_mask);
  
  /* Connect to the DSN m_tbsc_dsn */
  SQLDisconnect(m_dbc); /* in case multiple press on configure */
  m_ret = SQLDriverConnect(m_dbc, NULL,
			   (SQLCHAR*)("DSN="+m_tbsc_dsn+";DATABASE="+m_tbsc_db+";").c_str(), SQL_NTS,
			   NULL, 0,
			   NULL, SQL_DRIVER_COMPLETE);
  if (m_debug){
    EUDAQ_INFO("Listing installed Data Sources from ODBC: ");
    odbcListDSN();
  } else;
  if (!SQL_SUCCEEDED(m_ret)){
    odbcExtractError("SQLAllocHandle for dbc", m_dbc, SQL_HANDLE_DBC);
    EUDAQ_THROW("Unable to connect to the DSN = '"+m_tbsc_dsn+"', please check!");
  }else {
    EUDAQ_INFO("DSN '"+m_tbsc_dsn+"' Connected!");
    if (m_debug) odbcDoAlotPrint();
  }

      
  /* Allocate a statement handle */
  SQLAllocHandle(SQL_HANDLE_STMT, m_dbc, &m_stmt);

  /*Fill in the readout data map*/
  fillChannelMap(m_tbsc_tab_uni);
  if (m_debug) printNestedMap(m_sc_para_map);
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void tbscProducer::DoStartRun(){
  m_exit_of_run = false;
  
  m_thd_run = std::thread(&tbscProducer::Mainloop, this);
}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void tbscProducer::DoStopRun(){
  m_exit_of_run = true;
  if(m_thd_run.joinable()){
    m_thd_run.join();
  }
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void tbscProducer::DoReset(){

  SQLDisconnect(m_dbc); // disconnect from driver 
  //  if (m_fb.is_open()) m_fb.close();
  
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
  // fclose(m_file_lock);

  //  m_file_stream.close();
  
  m_thd_run = std::thread();
  m_exit_of_run = false;
  
}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void tbscProducer::DoTerminate(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
  //  fclose(m_file_lock);
  //  m_file_stream.close();

  /* free up allocated handles */
  SQLFreeHandle(SQL_HANDLE_DBC, m_dbc);
  SQLFreeHandle(SQL_HANDLE_ENV, m_env);
  SQLFreeHandle(SQL_HANDLE_STMT, m_stmt);

}

//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void tbscProducer::Mainloop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  
  /* Allocate a statement handle */
  // SQLAllocHandle(SQL_HANDLE_STMT, m_dbc, &m_stmt);
  /* Retrieve a list of tables */
  // SQLTables(m_stmt, NULL, 0, NULL, 0,
  // 	    NULL, 0,
  // 	    //(SQLCHAR*)"aidaSC", SQL_NTS, /* TableName */
  // 	    (SQLCHAR*)"TABLE", SQL_NTS);

  m_exit_of_run=false;
  
  int acounter=0;
  std::string latest_update="NULL";
  //SQLCHAR* checkUpdate= (SQLCHAR*)"select UPDATE_TIME from information_schema.tables where TABLE_SCHEMA='aidaTest' and TABLE_NAME='aidaSC';";
  m_sqlcli="select UPDATE_TIME from information_schema.tables where TABLE_SCHEMA='"+m_tbsc_db+"' and TABLE_NAME='"+m_tbsc_tab_data+"';";
  SQLCHAR* checkUpdate= (SQLCHAR*)m_sqlcli.c_str();

  std::map<std::string, std::map<std::string, std::string>> sc_data;
  do{
    /*sleep for m_s_invl seconds, but awake every second to check status*/
    if(acounter!=0){
      for( int i_intvl=m_s_intvl; i_intvl>=0; i_intvl--){
	auto tp_next = std::chrono::steady_clock::now() +  std::chrono::seconds(1);
	std::this_thread::sleep_until(tp_next);
	if(m_exit_of_run) break;
      }
    }
    acounter++;
    
    printf(" #%d check update_time <-|\n", acounter);
    SQLFreeStmt(m_stmt,SQL_CLOSE);
    SQLExecDirect(m_stmt, checkUpdate, SQL_NTS);
    
    if (SQL_SUCCEEDED(SQLFetch(m_stmt))){
      SQLLEN indicator;
      char buf_ut[512];
      if ( SQL_SUCCEEDED(SQLGetData(m_stmt, 1, SQL_C_CHAR, &buf_ut, sizeof(buf_ut), &indicator))){
	//---> in case empty, give null to the buffer
	if (indicator == SQL_NULL_DATA) strcpy (buf_ut,"NULL"); 
	printf("\tUpdateTime: %s\n", buf_ut);
	
	if (std::string(buf_ut)!=latest_update){
	  latest_update=std::string(buf_ut);

	  sc_data.clear();
	  m_sqlcli.clear();
	  m_sqlcli="select * from "+m_tbsc_tab_data+" order by timer desc limit 2;";
	  odbcFetchData(m_stmt,
			//(SQLCHAR*)"select * from aidaSC order by timer desc limit 2;",
			(SQLCHAR*)m_sqlcli.c_str(),
			m_columns,
			sc_data);
	  
	}else {
	  printf("\tSame as before.\n");
	  continue;
	}
	if (m_debug) printNestedMap(sc_data);
      }
    }

    auto rawevt = eudaq::Event::MakeUnique("SCRawEvt");
    
    
    /* loop over sc data to set tag to events if shown up in the mask vector*/
    auto tagdata = sc_data["1"]; 
    for(auto it = tagdata.cbegin(); it != tagdata.cend(); ++it){
      if (std::find(m_tbsc_mask.begin(), m_tbsc_mask.end(), it->first) != m_tbsc_mask.end()){
	/* if channel is required to tag to event*/
	std::cout << it->first << " " << it->second << "\n";
	rawevt->SetTag(it->first, it->second);
      }
    }
    
    //--> If you want a timestampe
    bool flag_ts = true; // to be moved to config
    if (flag_ts){
      auto tp_current_evt = std::chrono::steady_clock::now();
      auto tp_end_of_busy = tp_current_evt + std::chrono::seconds(m_s_intvl);
      std::chrono::nanoseconds du_ts_beg_ns(tp_current_evt - tp_start_run);
      std::chrono::nanoseconds du_ts_end_ns(tp_end_of_busy - tp_start_run);
      rawevt->SetTimestamp(du_ts_beg_ns.count(), du_ts_end_ns.count());
      std::cout<< "CHECK: start =="<<du_ts_beg_ns.count() <<"; end =="<< du_ts_end_ns.count()<<std::endl;
    }

    SendEvent(std::move(rawevt)); 
   
   
  }while(!m_exit_of_run);

  
  printf("\nComplete.\n");

}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------

//**********************************************************
//**********************************************************
//----------DOC-MARK-----BEG*toolFunc-----DOC-MARK----------


void tbscProducer::odbcExtractError(std::string fn,
					   SQLHANDLE handle,
					   SQLSMALLINT type){
  SQLINTEGER i = 0;
  SQLINTEGER native;
  SQLCHAR state[ 7 ];
  SQLCHAR text[256];
  SQLSMALLINT len;
  SQLRETURN ret;
  fprintf(stderr,"\n");
  EUDAQ_INFO("The driver reported the following diagnostics whilst running "+fn);
  do
    {
      ret = SQLGetDiagRec(type, handle, ++i, state, &native, text,
			  sizeof (text), &len );
      if (SQL_SUCCEEDED(ret))
	printf("%s:%d:%d:%s\n", state, i, native, text);
    }
  while ( ret == SQL_SUCCESS );
  printf("\n");
}

void tbscProducer::odbcDoAlotPrint(){
  EUDAQ_INFO("[DEBUG] Print a lot info of ODBC driver/driver manager/data source:");
    
  SQLCHAR dbms_name[256], dbms_ver[256];
  SQLUINTEGER getdata_support;
  SQLUSMALLINT max_concur_act;
  SQLSMALLINT string_len;
  //
  // Find something out about the driver.
  //
  SQLGetInfo(m_dbc, SQL_DBMS_NAME, (SQLPOINTER)dbms_name,
	     sizeof (dbms_name), NULL);
  SQLGetInfo(m_dbc, SQL_DBMS_VER, (SQLPOINTER)dbms_ver,
	     sizeof (dbms_ver), NULL);
  SQLGetInfo(m_dbc, SQL_GETDATA_EXTENSIONS, (SQLPOINTER)&getdata_support,
	     0, 0);
  SQLGetInfo(m_dbc, SQL_MAX_CONCURRENT_ACTIVITIES, &max_concur_act, 0, 0);
  printf("\tDBMS Name: %s\n", dbms_name);
  printf("\tDBMS Version: %s\n", dbms_ver);
  if (max_concur_act == 0) {
    printf("\tSQL_MAX_CONCURRENT_ACTIVITIES - no limit or undefined\n");
  } else {
    printf("\tSQL_MAX_CONCURRENT_ACTIVITIES = %u\n", max_concur_act);
  }
  if (getdata_support & SQL_GD_ANY_ORDER)
    printf("\tSQLGetData - columns can be retrieved in any order\n");
  else
    printf("\tSQLGetData - columns must be retrieved in order\n");
  if (getdata_support & SQL_GD_ANY_COLUMN)
    printf("\tSQLGetData - can retrieve columns before last bound one\n");
  else
    printf("\tSQLGetData - columns must be retrieved after last bound one\n");

  printf("\n");
}

void tbscProducer::odbcListDSN(){
  SQLUSMALLINT direction;
  SQLRETURN _ret;

  char dsn[256];
  char desc[256];
  SQLSMALLINT dsn_ret;
  SQLSMALLINT desc_ret;
  
  direction = SQL_FETCH_FIRST;
  while (SQL_SUCCEEDED(_ret = SQLDataSources(m_env, direction,
					    (SQLCHAR*) dsn, sizeof (dsn), &dsn_ret,
					    (SQLCHAR*) desc, sizeof (desc), &desc_ret))) {
    direction = SQL_FETCH_NEXT;
    printf("\t%s - %s\n", dsn, desc);
    if (_ret == SQL_SUCCESS_WITH_INFO) printf("\tdata truncation\n");
  }
  printf("\n");
} // ---- End of tbscProducer::odbcListDSN() ----- 

void tbscProducer::fillChannelMap(std::string intablename){
  /* Descibe: Fill in the nested map from the complimentary table from the database,
   indexing by the column names from the table noting the SC data*/
  
  intablename = (intablename=="")?"aida_channels":intablename;
  //SQLFreeStmt(m_stmt,SQL_CLOSE);
  // SQLExecDirect(m_stmt, (SQLCHAR*)("select * from "+intablename+";").c_str(), SQL_NTS);

  SQLFreeStmt(m_stmt, SQL_CLOSE);
  SQLExecDirect(m_stmt,
		(SQLCHAR*)("select * from "+intablename+" ;").c_str(),
		SQL_NTS);
  /* How many columns are there */
  SQLNumResultCols(m_stmt, &m_columns);
  /* Clear the map to refill */
  m_sc_para_map.clear();

  SQLRETURN ret; /* ODBC API return status */
  int row = 0;

 /* Loop through the rows in the result-set */
  while (SQL_SUCCEEDED(SQLFetch(m_stmt))) {
    SQLUSMALLINT icol;
    printf("Row %d\n", row++);
    /* Loop through the columns */
    
    std::string sc_para_map_index;
    std::map<std::string, std::string> resdata;
    for (icol = 1; icol <= m_columns; icol++) {
      SQLLEN indicator;
      char buf[512];
      SQLCHAR colName[MAX_COL_NAME_LEN];
      SQLSMALLINT colNameLen;
      
      /* retrieve column data as a string */
      ret = SQLGetData(m_stmt, icol, SQL_C_CHAR,
  		       buf, sizeof (buf),
  		       &indicator);
      SQLRETURN ret_des =  SQLDescribeCol (
					   m_stmt,
					   icol,
					   colName,
					   MAX_COL_NAME_LEN,
					   &colNameLen,
					   NULL,
					   NULL,
					   NULL,
					   NULL
					   );
      if (SQL_SUCCEEDED(ret) && SQL_SUCCEEDED(ret_des)) {
  	/* Handle null columns */
  	if (indicator == SQL_NULL_DATA) strcpy(buf, "NULL");
	if (std::string(buf).find("u2103") != std::string::npos) {
	  //printf("\tHere I got centigrade\n");
	  strcpy(buf,"\u2103");
	}
  	printf(" Column %u, %s : %s\n", icol, colName, buf);
	std::stringstream myss;
	myss<<colName;
	std::string colName_str;
	myss>>colName_str;
	/* Fill in the map */
	if (colName_str == "chid") {
	  sc_para_map_index = std::string(buf);
	  resdata.emplace( "value", "null");
	}else {
	  resdata.emplace( colName_str, std::string(buf) );
	} // Fill in the nested-map
      } // if column info & value get correctly
    } // loop over all columns in a given row

    m_sc_para_map.emplace(sc_para_map_index, resdata);
    
  } // loop over all rows
  
  printf("%s\n",std::string(20,'*').c_str());

} // ---- End of tbscProducer::fillChannelMap(std::string intablename) ----- 
 


void tbscProducer::odbcFetchData(SQLHSTMT stmt,
					SQLCHAR* dbcommand,
					SQLSMALLINT columns,
					std::map<std::string, std::map<std::string, std::string>> &data_map
					){
  SQLRETURN ret; /* ODBC API return status */
  int row = 0;

  SQLFreeStmt(stmt, SQL_CLOSE);
  SQLExecDirect(stmt, dbcommand, SQL_NTS);
  /* How many columns are there */
  SQLNumResultCols(stmt, &columns);

  /* Loop through the rows in the result-set */
  while (SQL_SUCCEEDED(SQLFetch(stmt))) {
    SQLUSMALLINT icol;
    printf("Row %d\n", row++);

    std::map<std::string, std::string> data;

    /* Loop through the columns */
    for (icol = 1; icol <= columns; icol++) {
      printf("start looping");
      SQLLEN indicator;
      char buf[512];
      SQLCHAR colName[MAX_COL_NAME_LEN];
      SQLSMALLINT colNameLen;
      /* retrieve column data as a string */
      ret = SQLGetData(stmt, icol, SQL_C_CHAR,
  		       buf, sizeof (buf),
  		       &indicator);
      SQLRETURN ret_des =   SQLDescribeCol (
			    stmt,
			    icol,
			    colName,
			    MAX_COL_NAME_LEN,
			    &colNameLen,
			    NULL,
			    NULL,
			    NULL,
			    NULL
			    );
      if (SQL_SUCCEEDED(ret) && SQL_SUCCEEDED(ret_des)) {
  	/* Handle null columns */
  	if (indicator == SQL_NULL_DATA) strcpy(buf, "NULL");
	if (std::string(buf).find("u2103") != std::string::npos) {
	  //printf("\tHere I got centigrade\n");
	  strcpy(buf,"\u2103");
	}
  	printf(" Column %u, %s : %s\n", icol, colName, buf);
	std::stringstream myss;
	myss<<colName;
	std::string colName_str;
	myss>>colName_str;
	data.emplace( colName_str, std::string(buf));
      }
    }
    data_map.emplace(std::to_string(row), data);
  }
  printf("%s\n",std::string(20,'*').c_str());
}

//----------DOC-MARK-----END*toolFunc-----DOC-MARK----------

