#include "eudaq/RunControl.hh"
#include <QAbstractListModel>
#include <vector>
#include <memory>

class RunControlConnection {
public:
  RunControlConnection(eudaq::ConnectionSPC id);
  QString operator[](int) const;
  static int NumColumns();
  static const char *ColumnName(int i);
  eudaq::Status GetStatus() const { return m_status; }
  int GetLevel() const { return m_status.GetLevel(); }
  bool IsConnected() const { return !m_id.unique(); }
  eudaq::ConnectionSPC GetId() const { return m_id; }
  void SetStatus(const eudaq::Status &status) { m_status = status; }

private:
  eudaq::ConnectionSPC m_id;
  eudaq::Status m_status;
};

class ConnectionSorter {
public:
  ConnectionSorter(std::vector<RunControlConnection> *messages)
    : m_msgs(messages), m_col(0), m_asc(true){}
  void SetSort(int col, bool ascending);
  bool operator()(size_t lhs, size_t rhs);

private:
  std::vector<RunControlConnection> *m_msgs;
  int m_col;
  bool m_asc;
};

class RunControlModel : public QAbstractListModel {
  Q_OBJECT

public:
  RunControlModel(QObject *parent = 0);

  void newconnection(eudaq::ConnectionSPC id);
  void disconnected(eudaq::ConnectionSPC id);
  int GetLevel(const QModelIndex &index) const;
  void UpdateDisplayed();
  bool CheckConfigured();
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const;
  void SetStatus(eudaq::ConnectionSPC id, eudaq::Status status);
  void sort(int column, Qt::SortOrder order);

private:
  std::vector<RunControlConnection> m_data;
  std::vector<size_t> m_disp;
  ConnectionSorter m_sorter;
};
