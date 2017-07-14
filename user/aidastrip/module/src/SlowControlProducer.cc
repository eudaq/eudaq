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

//#include <unistd.h>

//---- SQL related lib:
#include <sql.h>
#include <sqlext.h>

#define MAX_COL_NAME_LEN  256

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class SlowControlProducer : public eudaq::Producer {
  public:
  SlowControlProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void Mainloop();

  static const uint32_t m_id_factory = eudaq::cstr2hash("SlowControlProducer");

  
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

  void odbcFetchData(SQLHSTMT stmt, SQLSMALLINT columns);
  void odbcListDSN();
  void odbcDoAlotPrint();
  void odbcExtractError(std::string fn,
			SQLHANDLE handle,
			SQLSMALLINT type);
private:
  bool m_debug;
  SQLHENV m_env;
  SQLHDBC m_dbc;
  SQLHSTMT m_stmt;
  SQLRETURN m_ret; /* ODBC API return status */

  std::string m_tbsc_dsn;
  std::string m_tbsc_db;
  unsigned int m_s_intvl;
  
  std::thread m_thd_run;

  std::filebuf m_fb; // output file to print event

  // mapa<(string)ch_id, std::mapb>, mapb<aida_channels.colName, val>
  std::map sc_para_map <std::string, std::map<std::string, std::string>>;
  
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
    Register<SlowControlProducer, const std::string&, const std::string&>(SlowControlProducer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
SlowControlProducer::SlowControlProducer(const std::string & name, const std::string & runcontrol):
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
void SlowControlProducer::DoInitialise(){
  auto ini = GetInitConfiguration();
  m_tbsc_dsn = ini->Get("TBSC_DSN", "myodbc5a");
  m_tbsc_db  = ini->Get("TBSC_DATABASE", "aidaTest");
  std::string ini_debug = ini->Get("TBSC_DEBUG", "");
  m_debug = (ini_debug=="true"||ini_debug=="True")? true : false;
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void SlowControlProducer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  //m_tbsc_dsn= (conf->Get("TBSC_DSN","")=="" )? m_tbsc_dsn: conf->Get("TBSC_DSN","");
  m_tbsc_dsn=conf->Get("TBSC_DSN", m_tbsc_dsn);
  //m_tbsc_db = (conf->Get("TBSC_DATABASE","")=="" )? m_tbsc_db: conf->Get("TBSC_DATABASE","");
  m_tbsc_db = conf->Get("TBSC_DATABASE", m_tbsc_db);
  std::string conf_debug = conf->Get("TBSC_DEBUG", "");
  //m_debug = (conf_debug=="true" || conf_debug=="True")? true: m_debug;
  m_debug=true;
  m_s_intvl = conf->Get("TBSC_INTERVAL_SEC", m_s_intvl);
  printf("check invl: %d", m_s_intvl);
  
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
  
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void SlowControlProducer::DoStartRun(){
  m_exit_of_run = false;
  //--> start of evt print <--// wmq dev
  //m_fb.open("out.txt", std::ios::out|std::ios::app);
  m_fb.open("out.txt", std::ios::out);
  
  m_thd_run = std::thread(&SlowControlProducer::Mainloop, this);
}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void SlowControlProducer::DoStopRun(){
  m_exit_of_run = true;
  if(m_thd_run.joinable()){
    m_thd_run.join();
  }
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void SlowControlProducer::DoReset(){

  SQLDisconnect(m_dbc); // disconnect from driver 
  
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
  // fclose(m_file_lock);

  //  m_file_stream.close();
  
  m_thd_run = std::thread();
  m_exit_of_run = false;
}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void SlowControlProducer::DoTerminate(){
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
void SlowControlProducer::Mainloop(){
  std::ostream os(&m_fb);

  
  /* Allocate a statement handle */
  SQLAllocHandle(SQL_HANDLE_STMT, m_dbc, &m_stmt);
  /* Retrieve a list of tables */
  SQLTables(m_stmt, NULL, 0, NULL, 0,
  	    NULL, 0,
  	    //(SQLCHAR*)"aidaSC", SQL_NTS, /* TableName */
  	    (SQLCHAR*)"TABLE", SQL_NTS);
  /* How many columns are there */
  SQLSMALLINT columns; /* number of columns in result-set */
  SQLNumResultCols(m_stmt, &columns);

  m_exit_of_run=false;
  
  int acounter=0;
  std::string latest_update="NULL";
  SQLCHAR* checkUpdate= (SQLCHAR*)"select UPDATE_TIME from information_schema.tables where TABLE_SCHEMA='aidaTest' and TABLE_NAME='aidaSC';";
  do{
    printf(" #%d check update_time <-|\n", acounter);
    SQLFreeStmt(m_stmt,SQL_CLOSE);
    SQLExecDirect(m_stmt, checkUpdate, SQL_NTS);
    
    if (SQL_SUCCEEDED(SQLFetch(m_stmt))){
      SQLLEN indicator;
      char buf_ut[512];
      if ( SQL_SUCCEEDED(SQLGetData(m_stmt, 1, SQL_C_CHAR, &buf_ut, sizeof(buf_ut), &indicator))){
	if (indicator == SQL_NULL_DATA) strcpy (buf_ut,"NULL");
	printf("\tUpdateTime: %s\n", buf_ut);
	
	if (std::string(buf_ut)!=latest_update){
	  latest_update=std::string(buf_ut);

	  SQLFreeStmt(m_stmt,SQL_CLOSE);
	  SQLExecDirect(m_stmt, (SQLCHAR*)"select * from aidaSC order by ts desc limit 2;", SQL_NTS);
	  odbcFetchData(m_stmt, columns);
	  
	}else {
	  printf("\tSame as before.\n");
	}
      }
    }

    for( int i_intvl=m_s_intvl; i_intvl>=0; i_intvl--){
      auto tp_next = std::chrono::steady_clock::now() +  std::chrono::seconds(1);
      std::this_thread::sleep_until(tp_next);
      if(m_exit_of_run) break;
    }

    auto ev = eudaq::Event::MakeUnique("SCRawEvt"); 

    ev->Print(os, 0);
    
	
    SendEvent(std::move(ev));
    acounter++;
  }while(!m_exit_of_run);

  
  printf("\nComplete.\n");

}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------

//**********************************************************
//**********************************************************
//----------DOC-MARK-----BEG*toolFunc-----DOC-MARK----------

// void SlowControlProducer::FillChannelMap(std::string table){
//   table = (table=="")?"aida_channels":table;
//   SQLFreeStmt(m_stmt,SQL_CLOSE);
//   SQLExecDirect(m_stmt, (SQLCHAR*)("select * from "+table+";").c_str(), SQL_NTS);
//   odbcFetchData(m_stmt, columns);
// }

void SlowControlProducer::odbcExtractError(
					   std::string fn,
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

void SlowControlProducer::odbcDoAlotPrint(){
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

void SlowControlProducer::odbcListDSN(){
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
}

void SlowControlProducer::odbcFetchData(SQLHSTMT stmt, SQLSMALLINT columns ){
  SQLRETURN ret; /* ODBC API return status */
  int row = 0;

  /* Loop through the rows in the result-set */
  while (SQL_SUCCEEDED(SQLFetch(stmt))) {
    SQLUSMALLINT icol;
    printf("Row %d\n", row++);
    /* Loop through the columns */
    for (icol = 1; icol <= columns; icol++) {
      SQLLEN indicator;
      char buf[512];
      SQLCHAR colName[MAX_COL_NAME_LEN];
      SQLSMALLINT colNameLen;
      /* retrieve column data as a string */
      ret = SQLGetData(stmt, icol, SQL_C_CHAR,
  		       buf, sizeof (buf),
  		       &indicator);
      SQLRETURN ret_des =  ret = SQLDescribeCol (
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
      }
    }
  }
  printf("%s\n",std::string(20,'*').c_str());

}

//----------DOC-MARK-----END*toolFunc-----DOC-MARK----------
