#ifndef INCLUDED_LogCollectorModel_hh
#define INCLUDED_LogCollectorModel_hh

#include "eudaq/LogMessage.hh"
#include <QAbstractListModel>
#include <QRegExp>
#include <vector>
#include <stdexcept>
#include <iostream>

class LogMessage : public eudaq::LogMessage {
public:
  LogMessage(const eudaq::LogMessage & msg);
  LogMessage(const std::string & fileline);
  QString operator [] (int) const;
  std::string Text(int) const;
  static int NumColumns();
  static const char * ColumnName(int i);
  static int ColumnWidth(int);
  bool IsOK() const { return m_ok; }
private:
  bool m_ok;
};

class LogSearcher {
public:
  LogSearcher() : m_set(false) {
    m_regexp.setPatternSyntax(QRegExp::Wildcard); // or QRegExp::RegExp2
    m_regexp.setCaseSensitivity(Qt::CaseInsensitive);
  }
  void SetSearch(const std::string & regexp);
  bool Match(const LogMessage & msg);
private:
  bool m_set;
  QRegExp m_regexp;
};

class LogSorter {
public:
  LogSorter(std::vector<LogMessage> * messages) : m_msgs(messages), m_col(0), m_asc(true) {}
  void SetSort(int col, bool ascending) { m_col = col; m_asc = ascending; }
  bool operator() (size_t lhs, size_t rhs) {
    QString l = (*m_msgs)[lhs].Text(m_col).c_str();
    QString r = (*m_msgs)[rhs].Text(m_col).c_str();
    return m_asc ^ (QString::compare(l, r, Qt::CaseInsensitive) < 0);
  }
private:
  std::vector<LogMessage> * m_msgs;
  int m_col;
  bool m_asc;
};

class LogCollectorModel : public QAbstractListModel {
  Q_OBJECT

public:
  LogCollectorModel(QObject *parent = 0);

  std::vector<std::string> LoadFile(const std::string & filename); // returns list of sources
  QModelIndex AddMessage(const LogMessage & msg);

  int GetLevel(const QModelIndex &index) const;

  bool IsDisplayed(size_t index);
  void SetDisplayLevel(int level) { m_displaylevel = level; UpdateDisplayed(); }
  void SetDisplayNames(const std::string & type, const std::string & name) { m_displaytype = type; m_displayname = name; UpdateDisplayed(); }
  void SetSearch(const std::string & regexp) { m_search.SetSearch(regexp); UpdateDisplayed(); }
  void UpdateDisplayed();

  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role) const;
  const LogMessage & GetMessage(int row) const;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const;
  void sort(int column, Qt::SortOrder order) {
    m_sorter.SetSort(column, order == Qt::AscendingOrder);
    UpdateDisplayed();
  }
private:
  std::vector<LogMessage> m_all;
  std::vector<size_t> m_disp;
  int m_displaylevel;
  std::string m_displaytype, m_displayname;
  LogSearcher m_search;
  LogSorter m_sorter;
 };

#endif // INCLUDED_LogCollectorModel_hh
