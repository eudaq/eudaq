#include "eudaq/TransportBase.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/counted_ptr.hh"
#include <QAbstractListModel>
#include <vector>

class RunControlConnection {
public:
  RunControlConnection(const eudaq::ConnectionInfo & id);
  QString operator [] (int) const;
  static int NumColumns();
  static const char * ColumnName(int i);
  eudaq::Status GetStatus() const { return m_status; }
  int GetLevel() const { return m_status.GetLevel(); }
  bool IsConnected() const { return m_id->IsEnabled(); }
  void SetConnected(bool con) { m_id->SetState(2*con - 1); }
  const eudaq::ConnectionInfo & GetId() const { return *m_id; }
  void SetStatus(const eudaq::Status & status) { m_status = status; }
private:
  counted_ptr<eudaq::ConnectionInfo> m_id;
  eudaq::Status m_status;
};

class ConnectionSorter {
public:
  ConnectionSorter(std::vector<RunControlConnection> * messages) : m_msgs(messages), m_col(0), m_asc(true) {}
  void SetSort(int col, bool ascending) { m_col = col; m_asc = ascending; }
  bool operator() (size_t lhs, size_t rhs) {
    QString l = (*m_msgs)[lhs][m_col];
    QString r = (*m_msgs)[rhs][m_col];
    return m_asc ^ (QString::compare(l, r, Qt::CaseInsensitive) < 0);
  }
private:
  std::vector<RunControlConnection> * m_msgs;
  int m_col;
  bool m_asc;
};

class RunControlModel : public QAbstractListModel {
  Q_OBJECT

public:
  RunControlModel(QObject *parent = 0);

  void newconnection(const eudaq::ConnectionInfo & id);
  void disconnected(const eudaq::ConnectionInfo & id);

  int GetLevel(const QModelIndex &index) const;
  void UpdateDisplayed();

  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const;

  void SetStatus(const eudaq::ConnectionInfo & id, eudaq::Status status);
  void sort(int column, Qt::SortOrder order) {
    //std::cout << "sorting " << column << ", " << order << std::endl;
    m_sorter.SetSort(column, order == Qt::AscendingOrder);
    UpdateDisplayed();
  }
private:
  std::vector<RunControlConnection> m_data;
  std::vector<size_t> m_disp;
  ConnectionSorter m_sorter;
 };
