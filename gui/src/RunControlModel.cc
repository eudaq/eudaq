#include "RunControlModel.hh"

#include <vector>
#include <string>
#include "qmetatype.h"

namespace {
  static const char *const g_columns[] =
    {"type", "name", "state", "connection", "info"};
}

RunControlConnection::RunControlConnection(eudaq::ConnectionSPC id)
  : m_id(id) {}

QString RunControlConnection::operator[](int i) const {
  switch (i) {
  case 0:
    return m_id->GetType().c_str();
  case 1:
    return m_id->GetName().c_str();
  case 2:
    return m_id.unique()?"DEAD":to_string(m_status).c_str();
  case 3:
    return m_id->GetRemote().c_str();
  default:
    return "";
  }
}

int RunControlConnection::NumColumns() {
  return sizeof g_columns / sizeof *g_columns;
}

const char *RunControlConnection::ColumnName(int i) {
  if (i < 0 || i >= NumColumns()) {
    return "";
  }
  return g_columns[i];
}

void ConnectionSorter::SetSort(int col, bool ascending) {
  m_col = col;
  m_asc = ascending;
}

bool ConnectionSorter::operator()(size_t lhs, size_t rhs) {
  QString l = (*m_msgs)[lhs][m_col];
  QString r = (*m_msgs)[rhs][m_col];
  return m_asc ^ (QString::compare(l, r, Qt::CaseInsensitive) < 0);
}

RunControlModel::RunControlModel(QObject *parent)
  : QAbstractListModel(parent), m_sorter(&m_data){
  qRegisterMetaType<QVector<int> >("QVector<int>");

}

void RunControlModel::newconnection(eudaq::ConnectionSPC id) {
  for (size_t i = 0; i < m_data.size(); ++i) {
    if (!m_data[i].IsConnected() && id == m_data[i].GetId()) {
      m_data[i] = RunControlConnection(id);
      UpdateDisplayed();
      return;
    }
  }

  m_data.push_back(RunControlConnection(id));
  std::vector<size_t>::iterator it =
    std::lower_bound(m_disp.begin(), m_disp.end(), m_data.size() - 1, m_sorter);
  size_t pos = it - m_disp.begin();
  beginInsertRows(QModelIndex(), pos, pos);
  m_disp.insert(it, m_data.size() - 1);
  endInsertRows();
}

void RunControlModel::disconnected(eudaq::ConnectionSPC id) {
  for (size_t i = 0; i < m_data.size(); ++i) {
    if (id == m_data[i].GetId()){
      // m_data[i].SetConnected(false);
      //TODO: yiliu urgent
      break;
    }
  }
  UpdateDisplayed();
}

void RunControlModel::SetStatus(eudaq::ConnectionSPC id,
                                eudaq::Status status) {
  for (size_t i = 0; i < m_data.size(); ++i) {
    if (id == m_data[i].GetId()) {
      m_data[i].SetStatus(status);
      break;
    }
  }
  UpdateDisplayed();
}

void RunControlModel::UpdateDisplayed() {
  if (m_disp.size() > 0) {
    std::sort(m_disp.begin(), m_disp.end(), m_sorter);
    emit dataChanged(createIndex(0, 0), createIndex(m_data.size() - 1, 0));
  }
}

bool RunControlModel::CheckConfigured() {
  std::string connectionState;
  for(RunControlConnection i: m_data){
    connectionState = i[2].toStdString();
    if(connectionState.size()<5)
      return false;
  }
  return true;
}

int RunControlModel::GetLevel(const QModelIndex &index) const {
  const RunControlConnection &conn = m_data[m_disp[index.row()]];
  if (conn.IsConnected())
    return conn.GetLevel();
  return eudaq::Status::LVL_DEBUG;
}




int RunControlModel::rowCount(const QModelIndex & /*parent*/) const {
  return m_data.size();
}

int RunControlModel::columnCount(const QModelIndex & /*parent*/) const {
  return RunControlConnection::NumColumns();
}

QVariant RunControlModel::data(const QModelIndex &index, int role) const {
  if (role != Qt::DisplayRole || !index.isValid())
    return QVariant();

  if (index.column() < columnCount() && index.row() < rowCount())
    return m_data[m_disp[index.row()]][index.column()];

  return QVariant();
}

QVariant RunControlModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal && section < columnCount())
    return RunControlConnection::ColumnName(section);

  return QVariant();
}

void RunControlModel::sort(int column, Qt::SortOrder order) {
  m_sorter.SetSort(column, order == Qt::AscendingOrder);
  UpdateDisplayed();
}
