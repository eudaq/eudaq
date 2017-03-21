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
  LogMessage(const eudaq::LogMessage &msg);
  LogMessage(const std::string &fileline);
  QString operator[](int) const;
  std::string Text(int) const;
  static int NumColumns();
  static QString ColumnName(int i);
  static int ColumnWidth(int);
  bool IsOK() const { return m_ok; }

  static std::vector<QString> g_columns;

private:
  bool m_ok;
};

class LogSearcher {
public:
  LogSearcher();
  void SetSearch(const std::string &regexp);
  bool Match(const LogMessage &msg);
private:
  bool m_set;
  QRegExp m_regexp;
};

class LogSorter {
public:
  LogSorter(std::vector<LogMessage> *messages);
  void SetSort(int col, bool ascending);
  bool operator()(size_t lhs, size_t rhs);
private:
  std::vector<LogMessage> *m_msgs;
  int m_col;
  bool m_asc;
};

class LogCollectorModel : public QAbstractListModel {
  Q_OBJECT

public:
  LogCollectorModel(QObject *parent = 0);

  std::vector<std::string> LoadFile(const std::string &filename);
  QModelIndex AddMessage(const LogMessage &msg);
  int GetLevel(const QModelIndex &index) const;
  bool IsDisplayed(size_t index);
  void SetDisplayLevel(int level);
  void SetDisplayNames(const std::string &type, const std::string &name);
  void SetSearch(const std::string &regexp);
  void UpdateDisplayed();

  const LogMessage &GetMessage(int row) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  void sort(int column, Qt::SortOrder order) override;

private:
  std::vector<LogMessage> m_all;
  std::vector<size_t> m_disp;
  int m_displaylevel;
  std::string m_displaytype;
  std::string m_displayname;
  LogSearcher m_search;
  LogSorter m_sorter;
};

#endif // INCLUDED_LogCollectorModel_hh
