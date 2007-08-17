#include "eudaq/Utils.hh"
#include "RunControlModel.hh"

#include <vector>
#include <string>

namespace {
  static const char * const g_columns[] = {
    "type",
    "name",
    "state",
    "connection"
  };
}

RunControlConnection::RunControlConnection(const eudaq::ConnectionInfo & id)
  : m_id(id.Clone())
{}

QString RunControlConnection::operator [] (int i) const {
  switch(i) {
  case 0: return m_id->GetType().c_str();
  case 1: return m_id->GetName().c_str();
  case 2: return m_id->IsEnabled() ? to_string(m_status).c_str() : "DEAD";
  case 3: return m_id->GetRemote().c_str();
  default: return "";
  }
}

int RunControlConnection::NumColumns() {
  return sizeof g_columns / sizeof *g_columns;
}

const char * RunControlConnection::ColumnName(int i) {
  if (i < 0 || i >= NumColumns()) {
    return "";
  }
  return g_columns[i];
}

RunControlModel::RunControlModel(QObject *parent)
  : QAbstractListModel(parent),
    m_sorter(&m_data)
{
}

void RunControlModel::newconnection(const eudaq::ConnectionInfo & id) {
  for (size_t i = 0; i < m_data.size(); ++i) {
    if (!m_data[i].IsConnected() && id.Matches(m_data[i].GetId())) {
      m_data[i] = RunControlConnection(id);
      UpdateDisplayed();
      return;
    }
  }

  m_data.push_back(RunControlConnection(id));
  std::vector<size_t>::iterator it = std::lower_bound(m_disp.begin(), m_disp.end(), m_data.size() - 1, m_sorter);
  size_t pos = it - m_disp.begin();
  beginInsertRows(QModelIndex(), pos, pos);
  m_disp.insert(it, m_data.size() - 1);
  endInsertRows();
}

void RunControlModel::disconnected(const eudaq::ConnectionInfo & id) {
  for (size_t i = 0; i < m_data.size(); ++i) {
    if (id.Matches(m_data[i].GetId())) {
      m_data[i].SetConnected(false);
      break;
    }
  }
  UpdateDisplayed();
}

void RunControlModel::SetStatus(const eudaq::ConnectionInfo & id, eudaq::Status status) {
  for (size_t i = 0; i < m_data.size(); ++i) {
    if (id.Matches(m_data[i].GetId())) {
      m_data[i].SetStatus(status);
      break;
    }
  }
  UpdateDisplayed();
}

void RunControlModel::UpdateDisplayed() {
  if (m_disp.size() > 0) {
    //beginRemoveRows(createIndex(0, 0), 0, m_data.size() - 1);
    //m_disp.clear();
    //endRemoveRows();
    std::sort(m_disp.begin(), m_disp.end(), m_sorter);
    //beginInsertRows(createIndex(0, 0), 0, m_disp.size() - 1);
    //m_disp = disp;
    //endInsertRows();
    emit dataChanged(createIndex(0, 0), createIndex(m_data.size() - 1, 0));
  }
}

int RunControlModel::rowCount(const QModelIndex &/*parent*/) const {
  return m_data.size();
}

int RunControlModel::columnCount(const QModelIndex &/*parent*/) const {
  return RunControlConnection::NumColumns();
}

int RunControlModel::GetLevel(const QModelIndex &index) const {
  const RunControlConnection & conn = m_data[m_disp[index.row()]];
  if (conn.IsConnected()) return conn.GetLevel();
  return eudaq::Status::LVL_DEBUG;
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
